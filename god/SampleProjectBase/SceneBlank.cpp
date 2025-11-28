#include "math.h"
#include "SceneBlank.h"
#include "Geometory.h"
#include "DebugLog.h"
#include "Model.h"
#include "CameraBase.h" 
#include "LightBase.h"
#include "Shader.h"
#include "Player.h" 
#include "SimpleUI.h"
#include "Texture.h"
#include "Input.h" 
#include "UISprite.h"
#include "Image2D.h"
#include <fstream> 
#include <algorithm> // std::clamp (範囲制限) を使うためにインクルード

using namespace DirectX;
using namespace DirectX::SimpleMath;

Texture* g_uiTex = nullptr;
const char* SETTINGS_FILE = "player_settings.ini";

// ステージの端とカメラの移動制限範囲
const float STAGE_LIMIT_X = 14.0f; // プレイヤーが移動できる限界 (中心から±14m)
const float CAMERA_LIMIT_X = 10.0f; // カメラの中心が移動できる限界 (プレイヤーより少し手前で止める)

void SceneBlank::Init()
{
	// --- シェーダー読み込み ---
	Shader* shader[] = {
		CreateObj<VertexShader>("VS_SkinMeshAnimation"),
		CreateObj<PixelShader>("PS_TexColor"),
	};
	const char* file[] = {
		"Assets/Shader/VS_SkinMeshAnimation.cso",
		"Assets/Shader/PS_TexColor.cso",
	};
	int shaderNum = _countof(shader);
	for (int i = 0; i < shaderNum; ++i)
	{
		if (FAILED(shader[i]->Load(file[i])))
		{
			MessageBox(NULL, file[i], "Shader Error", MB_OK);
		}
	}

	// --- 2D画像 (HPバー) の読み込み ---
	// バーの最大幅を定義 (HP満タン時の幅)
	m_barMaxWidth = 500.0f;

	// 1P (左側) のHPバー
	m_hpBar = new Image2D();
	m_hpBarPos = { 330.0f, 80.0f }; // 1Pバーの初期位置
	// 画像ファイル、X座標, Y座標, 幅, 高さ で初期化
	m_hpBar->Load("Assets/Texture/hp.png", m_hpBarPos.x, m_hpBarPos.y, m_barMaxWidth, 80.0f);

	// 2P (右側) のHPバー
	m_enemyhpBar = new Image2D();
	m_enemyHpBarPos = { 950.0f, 80.0f }; // 2Pバーの初期位置
	m_enemyhpBar->Load("Assets/Texture/hp.png", m_enemyHpBarPos.x, m_enemyHpBarPos.y, m_barMaxWidth, 80.0f);


	// --- プレイヤー1生成 ---
	CreateObj<Player>("Player");
	Player* player = GetObj<Player>("Player");
	// 1Pはキーボードの 'A' 'D' キーなどで操作
	player->SetInputType(PlayerInputType::PLAYER_1);

	// --- 設定ファイルの読み込み ---
	// デバッグシーンで調整した攻撃パラメータや当たり判定サイズを読み込む
	float moveSpeed = 2.0f;
	DirectX::XMFLOAT3 scale = { 1.0f, 1.0f, 1.0f };
	AttackParams params = player->GetLightPunchParams();

	std::ifstream ifs(SETTINGS_FILE);
	if (ifs.is_open())
	{
		ifs >> moveSpeed;
		ifs >> scale.x >> scale.y >> scale.z;

		// 3部位分のくらい判定データ (Extents, Offset) を読み込む 
		for (int i = 0; i < (int)HurtboxType::COUNT; ++i) {
			DirectX::XMFLOAT2 ext, off;
			if (!ifs.eof()) ifs >> ext.x >> ext.y >> off.x >> off.y;
			player->SetHurtboxBase((HurtboxType)i, off, ext);
		}

		// 攻撃パラメータの読み込み
		if (!ifs.eof()) ifs >> params.totalDuration;
		if (!ifs.eof()) ifs >> params.hitboxStart;
		if (!ifs.eof()) ifs >> params.hitboxEnd;
		if (!ifs.eof()) ifs >> params.hitboxOffset.x >> params.hitboxOffset.y;
		if (!ifs.eof()) ifs >> params.hitboxExtents.x >> params.hitboxExtents.y;
		if (!ifs.eof()) ifs >> params.damage;
		if (!ifs.eof()) ifs >> params.hitFrame;
		if (!ifs.eof()) ifs >> params.blockFrame;

		// 攻撃中のくらい判定補正値 (位置Offset と サイズSize)
		if (!ifs.eof()) ifs >> params.headOffsetVal.x >> params.headOffsetVal.y;
		if (!ifs.eof()) ifs >> params.headSizeVal.x >> params.headSizeVal.y;

		if (!ifs.eof()) ifs >> params.bodyOffsetVal.x >> params.bodyOffsetVal.y;
		if (!ifs.eof()) ifs >> params.bodySizeVal.x >> params.bodySizeVal.y;

		if (!ifs.eof()) ifs >> params.legsOffsetVal.x >> params.legsOffsetVal.y;
		if (!ifs.eof()) ifs >> params.legsSizeVal.x >> params.legsSizeVal.y;

		ifs.close();
	}

	// 読み込んだ値をプレイヤー1に設定
	player->SetMoveSpeed(moveSpeed);
	player->SetScale(scale);
	player->GetLightPunchParams() = params;


	// 1Pモデル読み込み
	if (!player->Load("Assets/Model/knight/Idle.fbx", 0.02f, true, false))
	{
		MessageBox(NULL, "プレイヤーモデルの読み込みに失敗しました。", "Model Load Error", MB_OK);
	}
	// アニメーションのロード
	player->GetModel()->LoadAnimation("Assets/Model/knight/Walking.fbx", "Walk", true);
	player->GetModel()->LoadAnimation("Assets/Model/knight/WalkBack.fbx", "WalkBack", true);
	player->GetModel()->LoadAnimation("Assets/Model/knight/LightPunch.fbx", "LightPunch", true);
	player->GetModel()->LoadAnimation("Assets/Model/knight/Jump.fbx", "Jump", true);

	// 初期位置と向きの設定 (左向き: -PI/2)
	player->SetPosition({ -2.0f, 0.0f, 0.0f });
	player->SetRotation({ 0.0f, DirectX::XM_PI / -2.0f, 0.0f });


	// --- プレイヤー2 (相手) の生成 ---
	CreateObj<Player>("Player2");
	Player* player2 = GetObj<Player>("Player2");
	// 2Pは矢印キーなどで操作
	player2->SetInputType(PlayerInputType::PLAYER_2);

	// 2Pにも同じ設定 (スピードや当たり判定) を適用
	player2->SetMoveSpeed(moveSpeed);
	player2->SetScale(scale);
	player2->GetLightPunchParams() = params;

	// 2Pにも読み込んだ3部位の判定をセット
	for (int i = 0; i < (int)HurtboxType::COUNT; ++i) {
		player2->SetHurtboxBase((HurtboxType)i,
			player->GetHurtboxBaseOffset((HurtboxType)i),
			player->GetHurtboxBaseExtents((HurtboxType)i));
	}

	// 2Pモデル読み込み
	if (!player2->Load("Assets/Model/knight/Idle.fbx", 0.02f, true, false))
	{
		MessageBox(NULL, "プレイヤー2モデルの読み込みに失敗しました。", "Model Load Error", MB_OK);
	}
	player2->GetModel()->LoadAnimation("Assets/Model/knight/Walking.fbx", "Walk", true);
	player2->GetModel()->LoadAnimation("Assets/Model/knight/WalkBack.fbx", "WalkBack", true);
	player2->GetModel()->LoadAnimation("Assets/Model/knight/LightPunch.fbx", "LightPunch", true);
	player2->GetModel()->LoadAnimation("Assets/Model/knight/Jump.fbx", "Jump", true);

	// 初期位置と向きの設定 (右向き: PI/2)
	player2->SetPosition({ 2.0f, 0.0f, 0.0f });
	player2->SetRotation({ 0.0f, DirectX::XM_PI / 2.0f, 0.0f });

	// カメラの初期設定
	CameraBase* pCamera = GetObj<CameraBase>("Camera");
	if (pCamera)
	{
		pCamera->SetPos({ 0.0f, 1.2f, -5.0f });
		pCamera->SetLook({ 0.0f, 1.0f, 4.0f });
	}

	g_uiTex = new Texture();
}

void SceneBlank::Uninit()
{
	// 画像メモリの解放
	if (m_hpBar) delete m_hpBar;
	if (m_enemyhpBar) delete m_enemyhpBar;

	if (g_uiTex) {
		delete g_uiTex;
		g_uiTex = nullptr;
	}
}

void SceneBlank::Update(float tick)
{
	Player* player = GetObj<Player>("Player");
	Player* player2 = GetObj<Player>("Player2");

	// --- プレイヤーの更新 (入力、アニメーション、FSM) ---
	if (player) {
		player->Update(tick);
	}
	if (player2) {
		player2->Update(tick);
	}

	// --- ステージ端の制限 (壁) ---
	// プレイヤーが STAGE_LIMIT_X より外に出ないように位置を補正する
	if (player)
	{
		XMFLOAT3 pos = player->GetPosition();
		// X座標を -14.0 ～ 14.0 の範囲に収める (std::clamp)
		pos.x = std::clamp(pos.x, -STAGE_LIMIT_X, STAGE_LIMIT_X);
		player->SetPosition(pos);
	}
	if (player2)
	{
		XMFLOAT3 pos = player2->GetPosition();
		pos.x = std::clamp(pos.x, -STAGE_LIMIT_X, STAGE_LIMIT_X);
		player2->SetPosition(pos);
	}


	// --- 当たり判定チェック ---
	if (player && player2)
	{
		// 1. 押し出し判定 (CheckCollisionで3部位すべてチェック)
		// 体が重ならないように、当たっていたら位置をずらす
		bool isColliding = player->CheckCollision(player2);

		player->SetIsColliding(isColliding);
		player2->SetIsColliding(isColliding);

		if (isColliding)
		{
			// 簡易押し出し: X軸中心でお互いに反発させる
			DirectX::XMFLOAT3 pos1 = player->GetPosition();
			DirectX::XMFLOAT3 pos2 = player2->GetPosition();
			float dist = pos1.x - pos2.x;
			float pushDist = 0.5f * tick; // 少しずつ押し出す量

			if (dist > 0) {
				pos1.x += pushDist; pos2.x -= pushDist;
			}
			else {
				pos1.x -= pushDist; pos2.x += pushDist;
			}

			// 押し出した結果、壁の外に出てしまわないように再度クランプ
			pos1.x = std::clamp(pos1.x, -STAGE_LIMIT_X, STAGE_LIMIT_X);
			pos2.x = std::clamp(pos2.x, -STAGE_LIMIT_X, STAGE_LIMIT_X);

			player->SetPosition(pos1);
			player2->SetPosition(pos2);
		}

		// --- 2. 攻撃判定 (Hitbox) のチェック & HP更新 ---

		// ★ 1P -> 2P (ヒット)
		// 1Pが攻撃中で、まだヒットしておらず、1PのHitboxが2PのHurtbox(3部位のどれか)に当たったか
		bool hit2 = false;
		if (player->IsAttacking() && !player->HasHit())
		{
			BoundingBox atk = player->GetActiveHitbox();
			if (atk.Intersects(player2->GetHurtbox(HurtboxType::HEAD)) ||
				atk.Intersects(player2->GetHurtbox(HurtboxType::BODY)) ||
				atk.Intersects(player2->GetHurtbox(HurtboxType::LEGS)))
			{
				hit2 = true;
			}
		}

		if (hit2)
		{
			// ダメージ処理
			AttackParams& params = player->GetLightPunchParams();
			player2->ReceiveDamage(params.damage);
			player->OnHit(); // 多段ヒット防止のためフラグを立てる

			// --- 2P HPバーの更新 (右側) ---
			float ratio = player2->GetHpRatio();
			float currentWidth = m_barMaxWidth * ratio;

			// 減った幅 (reduceWidth) を計算
			float reduceWidth = m_barMaxWidth - currentWidth;

			// バーのサイズを更新
			m_enemyhpBar->SetSize(currentWidth, 80.0f);

			// ★位置補正: 減った幅の半分だけ「左(-)」にずらす
			// 画像の中心基準描画において、右端を固定して縮んでいるように見せるための計算
			m_enemyhpBar->SetPosition(m_enemyHpBarPos.x - (reduceWidth / 2.0f), m_enemyHpBarPos.y);
		}

		// ★ 2P -> 1P (ヒット)
		bool hit1 = false;
		if (player2->IsAttacking() && !player2->HasHit())
		{
			BoundingBox atk = player2->GetActiveHitbox();
			if (atk.Intersects(player->GetHurtbox(HurtboxType::HEAD)) ||
				atk.Intersects(player->GetHurtbox(HurtboxType::BODY)) ||
				atk.Intersects(player->GetHurtbox(HurtboxType::LEGS)))
			{
				hit1 = true;
			}
		}

		if (hit1)
		{
			// ダメージ処理
			AttackParams& params = player2->GetLightPunchParams();
			player->ReceiveDamage(params.damage);
			player2->OnHit(); // 多段ヒット防止

			// --- 1P HPバーの更新 (左側) ---
			float ratio = player->GetHpRatio();
			float currentWidth = m_barMaxWidth * ratio;
			float reduceWidth = m_barMaxWidth - currentWidth;

			// バーのサイズを更新
			m_hpBar->SetSize(currentWidth, 80.0f);

			// ★位置補正: 減った幅の半分だけ「右(+)」にずらす
			// 画像の中心基準描画において、左端を固定して縮んでいるように見せるための計算
			m_hpBar->SetPosition(m_hpBarPos.x + (reduceWidth / 2.0f), m_hpBarPos.y);
		}
	}


	// ★★★ カメラ制御 (格ゲー風) ★★★
	CameraBase* pCamera = GetObj<CameraBase>("Camera");
	if (pCamera && player && player2)
	{
		XMFLOAT3 p1Pos = player->GetPosition();
		XMFLOAT3 p2Pos = player2->GetPosition();

		// 1. 中心の計算 (X軸)
		float centerX = (p1Pos.x + p2Pos.x) * 0.5f;

		// カメラの中心もステージ外に行かないように制限する (CAMERA_LIMIT_X)
		// これにより、画面端でカメラが止まるようになる
		centerX = std::clamp(centerX, -CAMERA_LIMIT_X, CAMERA_LIMIT_X);

		// 2. 高さの最大値 (Y軸)
		// どちらか高い方のプレイヤーに合わせて、見切れないように高さを調整
		float maxY = (p1Pos.y > p2Pos.y) ? p1Pos.y : p2Pos.y;
		float targetLookY = 1.4f + (maxY * 0.2f);
		float targetPosY = 1.5f + (maxY * 0.1f);

		// 3. ズーム計算 (Z軸)
		// 2人の距離 (X軸) に応じてカメラを引く
		float distX = fabsf(p1Pos.x - p2Pos.x);

		// ズーム係数 (値を大きくすると、より大きく引くようになる)
		float zoomFactorX = 0.45f; // 横距離に対する引き具合 (0.45 = 緩やか)
		float zoomFactorY = 0.8f;  // 高さに対する引き具合 (0.8 = ジャンプ対応)

		// 横と縦、どちらのために引くべきか大きい方を採用する
		float zoomAmount = 0.0f;
		float reqZoomX = (distX - 1.5f) * zoomFactorX; // 1.5fはベース距離(これより近ければ引かない)
		float reqZoomY = maxY * zoomFactorY;

		if (reqZoomX > reqZoomY) zoomAmount = reqZoomX;
		else zoomAmount = reqZoomY;

		if (zoomAmount < 0.0f) zoomAmount = 0.0f;

		// ベース位置 (-3.5f) から引く
		float baseZ = -3.5f;
		float targetZ = baseZ - zoomAmount;

		// ズームアウト制限 (-9.0f より奥には行かないようにする = キャラが小さくなりすぎない)
		if (targetZ < -9.0f) targetZ = -9.0f;

		// 4. 目標座標の設定
		XMFLOAT3 targetPos = { centerX, targetPosY, targetZ };
		XMFLOAT3 targetLook = { centerX, targetLookY, 0.0f };

		// 5. 滑らかに移動 (Lerp補間)
		// 4.0f * tick は「ゆっくり」追従する設定。重厚感が出る。
		float smoothSpeed = 4.0f * tick;

		XMFLOAT3 currentPos = pCamera->GetPos();
		XMFLOAT3 currentLook = pCamera->GetLook();

		XMVECTOR vCurPos = XMLoadFloat3(&currentPos);
		XMVECTOR vTarPos = XMLoadFloat3(&targetPos);
		XMVECTOR vNewPos = XMVectorLerp(vCurPos, vTarPos, smoothSpeed);

		XMVECTOR vCurLook = XMLoadFloat3(&currentLook);
		XMVECTOR vTarLook = XMLoadFloat3(&targetLook);
		XMVECTOR vNewLook = XMVectorLerp(vCurLook, vTarLook, smoothSpeed);

		XMFLOAT3 newPos, newLook;
		XMStoreFloat3(&newPos, vNewPos);
		XMStoreFloat3(&newLook, vNewLook);

		pCamera->SetPos(newPos);
		pCamera->SetLook(newLook);
	}
}

void SceneBlank::Draw()
{
	CameraBase* pCamera = GetObj<CameraBase>("Camera");
	LightBase* pLight = GetObj<LightBase>("Light");
	DirectX::XMFLOAT4X4 mat[3];
	DirectX::XMStoreFloat4x4(&mat[0], DirectX::XMMatrixIdentity());
	mat[1] = pCamera->GetView();
	mat[2] = pCamera->GetProj();
	DirectX::XMFLOAT3 lightDir = pLight->GetDirection();
	DirectX::XMFLOAT4 light[] = {
		pLight->GetDiffuse(),
		pLight->GetAmbient(),
		{lightDir.x, lightDir.y, lightDir.z, 0.0f}
	};
	DirectX::XMFLOAT3 camPos = pCamera->GetPos();
	DirectX::XMFLOAT4 camera[] = {
		{camPos.x, camPos.y, camPos.z, 0.0f}
	};
	Shader* shader[] = {
		GetObj<Shader>("VS_SkinMeshAnimation"),
		GetObj<Shader>("PS_TexColor"),
	};

	// --- プレイヤー1の描画 ---
	Player* player = GetObj<Player>("Player");
	if (player) {
		XMFLOAT3 pos = player->GetPosition();
		XMFLOAT3 rot = player->GetRotation();
		XMFLOAT3 pScale = player->GetScale();
		Matrix playerScaleMat = Matrix::CreateScale(pScale.x, pScale.y, pScale.z);
		Matrix modelBaseScaleMat = player->GetModel()->GetScaleBaseMatrix();
		Matrix rotMat = DirectX::XMMatrixRotationRollPitchYaw(rot.x, rot.y, rot.z);
		Matrix transMat = Matrix::CreateTranslation(pos.x, pos.y, pos.z);
		Matrix world = modelBaseScaleMat * playerScaleMat * rotMat * transMat;

		XMStoreFloat4x4(&mat[0], XMMatrixTranspose(world));

		shader[0]->WriteBuffer(0, mat);
		shader[1]->WriteBuffer(0, light);
		shader[1]->WriteBuffer(1, camera);

		player->SetVertexShader(shader[0]);
		player->SetPixelShader(shader[1]);
		player->Draw();
		player->DrawBoundingBox(); // くらい判定 (緑)
		player->DrawHitbox();      // 攻撃判定 (赤)
	}

	// --- プレイヤー2の描画 ---
	Player* player2 = GetObj<Player>("Player2");
	if (player2) {
		XMFLOAT3 pos = player2->GetPosition();
		XMFLOAT3 rot = player2->GetRotation();
		XMFLOAT3 pScale = player2->GetScale();
		Matrix playerScaleMat = Matrix::CreateScale(pScale.x, pScale.y, pScale.z);
		Matrix modelBaseScaleMat = player2->GetModel()->GetScaleBaseMatrix();
		Matrix rotMat = DirectX::XMMatrixRotationRollPitchYaw(rot.x, rot.y, rot.z);
		Matrix transMat = Matrix::CreateTranslation(pos.x, pos.y, pos.z);
		Matrix world = modelBaseScaleMat * playerScaleMat * rotMat * transMat;

		XMStoreFloat4x4(&mat[0], XMMatrixTranspose(world));

		shader[0]->WriteBuffer(0, mat);
		shader[1]->WriteBuffer(0, light);
		shader[1]->WriteBuffer(1, camera);

		player2->SetVertexShader(shader[0]);
		player2->SetPixelShader(shader[1]);
		player2->Draw();
		player2->DrawBoundingBox();
		player2->DrawHitbox();
	}

	// --- 2D UIの描画 ---

	// SimpleUIの準備 (クリア -> 描画登録 -> 一括描画)
	SimpleUI::Clear();
	DirectX::XMFLOAT4X4 identity;
	DirectX::XMStoreFloat4x4(&identity, DirectX::XMMatrixIdentity());
	SimpleUI::SetMatrix(identity, identity, identity);

	if (m_hpBar) m_hpBar->Draw();
	if (m_enemyhpBar) m_enemyhpBar->Draw();

	SimpleUI::DrawAll();
}