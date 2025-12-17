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
#include <algorithm> 

// やられ状態への遷移に使用
#include "PlayerStateDamage.h"

using namespace DirectX;
using namespace DirectX::SimpleMath;

// UI用テクスチャ管理（グローバル）
Texture* g_uiTex = nullptr;

// パラメータ設定ファイルのパス
const char* SETTINGS_FILE = "player_settings.ini";

// ステージとカメラの移動制限範囲
const float STAGE_LIMIT_X = 6.0f;
const float CAMERA_LIMIT_X = 4.0f;

/**
 * @brief シーンの初期化処理
 * シェーダー、モデル、UI、プレイヤーの生成と設定ロードを行う
 */
void SceneBlank::Init()
{
	// ==================================================
	// 1. シェーダーの読み込み
	// ==================================================
	Shader* shader[] = {
		CreateObj<VertexShader>("VS_SkinMeshAnimation"), // スキンメッシュアニメーション用
		CreateObj<PixelShader>("PS_TexColor"),           // テクスチャカラー描画用
		CreateObj<VertexShader>("VS_Object")             // 静的オブジェクト(スカイドーム等)用
	};
	const char* file[] = {
		"Assets/Shader/VS_SkinMeshAnimation.cso",
		"Assets/Shader/PS_TexColor.cso",
		"Assets/Shader/VS_Object.cso"
	};
	int shaderNum = _countof(shader);
	for (int i = 0; i < shaderNum; ++i)
	{
		if (FAILED(shader[i]->Load(file[i])))
		{
			MessageBox(NULL, file[i], "Shader Error", MB_OK);
		}
	}

	// ==================================================
	// 2. 背景（スカイドーム）の作成
	// ==================================================
	CreateObj<Model>("SkyModel");
	Model* skyModel = GetObj<Model>("SkyModel");
	if (!skyModel->Load("Assets/Model/SkyDome/SkyDome.fbx", 1.0f, true, true))
	{
		MessageBox(NULL, "スカイドームモデルの読み込みに失敗しました", "Error", MB_OK);
	}
	skyModel->SetTexture("Assets/Model/SkyDome/SkyDome.png");
	skyModel->SetPixelShader(GetObj<Shader>("PS_TexColor"));
	m_skyDome = new SkyDome();
	m_skyDome->Init(skyModel);

	// ==================================================
	// 3. UI (HPバー) の作成
	// ==================================================
	m_barMaxWidth = 500.0f;

	// 1P HP Bar (左上)
	m_hpBar = new Image2D();
	m_hpBarPos = { 330.0f, 80.0f };
	m_hpBar->Load("Assets/Texture/hp.png", m_hpBarPos.x, m_hpBarPos.y, m_barMaxWidth, 80.0f);

	// 2P HP Bar (右上)
	m_enemyhpBar = new Image2D();
	m_enemyHpBarPos = { 950.0f, 80.0f };
	m_enemyhpBar->Load("Assets/Texture/hp.png", m_enemyHpBarPos.x, m_enemyHpBarPos.y, m_barMaxWidth, 80.0f);


	// ==================================================
	// 4. プレイヤー1 (1P) の生成
	// ==================================================
	CreateObj<Player>("Player");
	Player* player = GetObj<Player>("Player");
	player->SetInputType(PlayerInputType::PLAYER_1);

	// ==================================================
	// 5. 設定ファイル (ini) からパラメータを読み込む
	// ※ SceneDebug::SavePlayerSettings と同じ順序で読み込む必要がある
	// ==================================================
	float moveSpeed = 2.0f;
	DirectX::XMFLOAT3 scale = { 1.0f, 1.0f, 1.0f };

	// 設定適用先の一時参照を取得
	AttackParams lParams = player->GetLightPunchParams();
	AttackParams mParams = player->GetMediumPunchParams();
	AttackParams hParams = player->GetHeavyKickParams();

	std::ifstream ifs(SETTINGS_FILE);
	if (ifs.is_open())
	{
		// 基本設定
		ifs >> moveSpeed;
		ifs >> scale.x >> scale.y >> scale.z;

		// 立ち状態の当たり判定 (Head -> Body -> Legs)
		for (int i = 0; i < (int)HurtboxType::COUNT; ++i) {
			DirectX::XMFLOAT2 ext, off;
			if (!ifs.eof()) ifs >> ext.x >> ext.y >> off.x >> off.y;
			player->SetHurtboxBase((HurtboxType)i, off, ext);
		}

		// しゃがみ状態の当たり判定
		for (int i = 0; i < (int)HurtboxType::COUNT; ++i) {
			DirectX::XMFLOAT2 ext, off;
			if (!ifs.eof()) ifs >> ext.x >> ext.y >> off.x >> off.y;
			player->SetHurtboxCrouch((HurtboxType)i, off, ext);
		}

		// 弱パンチ (LightPunch) 設定読み込み
		if (!ifs.eof()) ifs >> lParams.totalDuration;
		if (!ifs.eof()) ifs >> lParams.hitboxStart;
		if (!ifs.eof()) ifs >> lParams.hitboxEnd;
		if (!ifs.eof()) ifs >> lParams.hitboxOffset.x >> lParams.hitboxOffset.y;
		if (!ifs.eof()) ifs >> lParams.hitboxExtents.x >> lParams.hitboxExtents.y;
		if (!ifs.eof()) ifs >> lParams.damage;
		if (!ifs.eof()) ifs >> lParams.hitFrame;
		if (!ifs.eof()) ifs >> lParams.blockFrame;
		// 補正値
		if (!ifs.eof()) ifs >> lParams.headOffsetVal.x >> lParams.headOffsetVal.y;
		if (!ifs.eof()) ifs >> lParams.headSizeVal.x >> lParams.headSizeVal.y;
		if (!ifs.eof()) ifs >> lParams.bodyOffsetVal.x >> lParams.bodyOffsetVal.y;
		if (!ifs.eof()) ifs >> lParams.bodySizeVal.x >> lParams.bodySizeVal.y;
		if (!ifs.eof()) ifs >> lParams.legsOffsetVal.x >> lParams.legsOffsetVal.y;
		if (!ifs.eof()) ifs >> lParams.legsSizeVal.x >> lParams.legsSizeVal.y;
		// キャンセル設定
		if (!ifs.eof()) ifs >> lParams.cancelEnabled;
		if (!ifs.eof()) ifs >> lParams.cancelStart;
		if (!ifs.eof()) ifs >> lParams.cancelEnd;
		if (!ifs.eof()) ifs >> lParams.cancelToLight >> lParams.cancelToMedium >> lParams.cancelToHeavy;

		// 中パンチ (MediumPunch) 設定読み込み
		if (!ifs.eof()) ifs >> mParams.totalDuration;
		if (!ifs.eof()) ifs >> mParams.hitboxStart;
		if (!ifs.eof()) ifs >> mParams.hitboxEnd;
		if (!ifs.eof()) ifs >> mParams.hitboxOffset.x >> mParams.hitboxOffset.y;
		if (!ifs.eof()) ifs >> mParams.hitboxExtents.x >> mParams.hitboxExtents.y;
		if (!ifs.eof()) ifs >> mParams.damage;
		if (!ifs.eof()) ifs >> mParams.hitFrame;
		if (!ifs.eof()) ifs >> mParams.blockFrame;
		// 補正値
		if (!ifs.eof()) ifs >> mParams.headOffsetVal.x >> mParams.headOffsetVal.y;
		if (!ifs.eof()) ifs >> mParams.headSizeVal.x >> mParams.headSizeVal.y;
		if (!ifs.eof()) ifs >> mParams.bodyOffsetVal.x >> mParams.bodyOffsetVal.y;
		if (!ifs.eof()) ifs >> mParams.bodySizeVal.x >> mParams.bodySizeVal.y;
		if (!ifs.eof()) ifs >> mParams.legsOffsetVal.x >> mParams.legsOffsetVal.y;
		if (!ifs.eof()) ifs >> mParams.legsSizeVal.x >> mParams.legsSizeVal.y;
		// キャンセル設定
		if (!ifs.eof()) ifs >> mParams.cancelEnabled;
		if (!ifs.eof()) ifs >> mParams.cancelStart;
		if (!ifs.eof()) ifs >> mParams.cancelEnd;
		if (!ifs.eof()) ifs >> mParams.cancelToLight >> mParams.cancelToMedium >> mParams.cancelToHeavy;

		// 大キック (HeavyKick) 設定読み込み
		if (!ifs.eof()) ifs >> hParams.totalDuration;
		if (!ifs.eof()) ifs >> hParams.hitboxStart;
		if (!ifs.eof()) ifs >> hParams.hitboxEnd;
		if (!ifs.eof()) ifs >> hParams.hitboxOffset.x >> hParams.hitboxOffset.y;
		if (!ifs.eof()) ifs >> hParams.hitboxExtents.x >> hParams.hitboxExtents.y;
		if (!ifs.eof()) ifs >> hParams.damage;
		if (!ifs.eof()) ifs >> hParams.hitFrame;
		if (!ifs.eof()) ifs >> hParams.blockFrame;
		// 補正値
		if (!ifs.eof()) ifs >> hParams.headOffsetVal.x >> hParams.headOffsetVal.y;
		if (!ifs.eof()) ifs >> hParams.headSizeVal.x >> hParams.headSizeVal.y;
		if (!ifs.eof()) ifs >> hParams.bodyOffsetVal.x >> hParams.bodyOffsetVal.y;
		if (!ifs.eof()) ifs >> hParams.bodySizeVal.x >> hParams.bodySizeVal.y;
		if (!ifs.eof()) ifs >> hParams.legsOffsetVal.x >> hParams.legsOffsetVal.y;
		if (!ifs.eof()) ifs >> hParams.legsSizeVal.x >> hParams.legsSizeVal.y;
		// キャンセル設定
		if (!ifs.eof()) ifs >> hParams.cancelEnabled;
		if (!ifs.eof()) ifs >> hParams.cancelStart;
		if (!ifs.eof()) ifs >> hParams.cancelEnd;
		if (!ifs.eof()) ifs >> hParams.cancelToLight >> hParams.cancelToMedium >> hParams.cancelToHeavy;

		ifs.close();
	}

	// 読み込んだパラメータを1Pに適用
	player->SetMoveSpeed(moveSpeed);
	player->SetScale(scale);
	player->GetLightPunchParams() = lParams;
	player->GetMediumPunchParams() = mParams;
	player->GetHeavyKickParams() = hParams;


	// ==================================================
	// 6. モデルとアニメーションのロード (1P)
	// ==================================================
	if (!player->Load("Assets/Model/knight/Idle.fbx", 0.02f, true, false))
	{
		MessageBox(NULL, "プレイヤーモデルの読み込みに失敗しました。", "Model Load Error", MB_OK);
	}
	// ループアニメーション
	player->GetModel()->LoadAnimation("Assets/Model/knight/Walking.fbx", "Walk", true);
	player->GetModel()->LoadAnimation("Assets/Model/knight/WalkBack.fbx", "WalkBack", true);
	player->GetModel()->LoadAnimation("Assets/Model/knight/CrouchIdle.fbx", "CrouchIdle", true);
	// 単発アニメーション
	player->GetModel()->LoadAnimation("Assets/Model/knight/LightPunch.fbx", "LightPunch", true);
	player->GetModel()->LoadAnimation("Assets/Model/knight/MediumPunch.fbx", "MediumPunch", true);
	player->GetModel()->LoadAnimation("Assets/Model/knight/HeavyKick.fbx", "HeavyKick", true);
	player->GetModel()->LoadAnimation("Assets/Model/knight/Jump.fbx", "Jump", true);
	player->GetModel()->LoadAnimation("Assets/Model/knight/Damage.fbx", "Damage", true);

	// 初期位置と向き
	player->SetPosition({ -2.0f, 0.0f, 0.0f });
	player->SetRotation({ 0.0f, DirectX::XM_PI / -2.0f, 0.0f });


	// ==================================================
	// 7. プレイヤー2 (2P) の生成
	// ==================================================
	CreateObj<Player>("Player2");
	Player* player2 = GetObj<Player>("Player2");
	player2->SetInputType(PlayerInputType::PLAYER_2);
	player2->SetMoveSpeed(moveSpeed);

	// スケールXをマイナスにして左右反転 (ミラーリング表示)
	DirectX::XMFLOAT3 scaleP2 = scale;
	scaleP2.x *= -1.0f;
	player2->SetScale(scaleP2);

	// パラメータは1Pと同じものを適用
	player2->GetLightPunchParams() = lParams;
	player2->GetMediumPunchParams() = mParams;
	player2->GetHeavyKickParams() = hParams;

	// 当たり判定設定もコピー
	for (int i = 0; i < (int)HurtboxType::COUNT; ++i) {
		player2->SetHurtboxBase((HurtboxType)i,
			player->GetHurtboxBaseOffset((HurtboxType)i),
			player->GetHurtboxBaseExtents((HurtboxType)i));
		player2->SetHurtboxCrouch((HurtboxType)i,
			player->GetHurtboxCrouchOffset((HurtboxType)i),
			player->GetHurtboxCrouchExtents((HurtboxType)i));
	}

	// モデルロード (2P)
	if (!player2->Load("Assets/Model/knight/Idle.fbx", 0.02f, true, false))
	{
		MessageBox(NULL, "プレイヤー2モデルの読み込みに失敗しました。", "Model Load Error", MB_OK);
	}
	player2->GetModel()->LoadAnimation("Assets/Model/knight/Walking.fbx", "Walk", true);
	player2->GetModel()->LoadAnimation("Assets/Model/knight/WalkBack.fbx", "WalkBack", true);
	player2->GetModel()->LoadAnimation("Assets/Model/knight/LightPunch.fbx", "LightPunch", true);
	player2->GetModel()->LoadAnimation("Assets/Model/knight/MediumPunch.fbx", "MediumPunch", true);
	player2->GetModel()->LoadAnimation("Assets/Model/knight/HeavyKick.fbx", "HeavyKick", true);
	player2->GetModel()->LoadAnimation("Assets/Model/knight/Jump.fbx", "Jump", true);
	player2->GetModel()->LoadAnimation("Assets/Model/knight/Damage.fbx", "Damage", true);
	player2->GetModel()->LoadAnimation("Assets/Model/knight/CrouchIdle.fbx", "CrouchIdle", true);

	player2->SetPosition({ 2.0f, 0.0f, 0.0f });
	player2->SetRotation({ 0.0f, DirectX::XM_PI / 2.0f, 0.0f });

	// ==================================================
	// 8. カメラの初期配置
	// ==================================================
	CameraBase* pCamera = GetObj<CameraBase>("Camera");
	if (pCamera)
	{
		pCamera->SetPos({ 0.0f, 1.2f, -5.0f });
		pCamera->SetLook({ 0.0f, 1.0f, 4.0f });
	}

	g_uiTex = new Texture();
}

/**
 * @brief 終了処理
 */
void SceneBlank::Uninit()
{
	if (m_hpBar) delete m_hpBar;
	if (m_enemyhpBar) delete m_enemyhpBar;
	if (m_skyDome) { delete m_skyDome; m_skyDome = nullptr; }
	if (g_uiTex) { delete g_uiTex; g_uiTex = nullptr; }
}

/**
 * @brief 更新処理
 */
void SceneBlank::Update(float tick)
{
	Player* player = GetObj<Player>("Player");
	Player* player2 = GetObj<Player>("Player2");

	// プレイヤー更新 (FSM, 物理, アニメーション)
	if (player) player->Update(tick);
	if (player2) player2->Update(tick);

	// ==================================================
	// 向き制御: 相手の方を向くように回転とスケールを調整
	// ==================================================
	if (player && player2)
	{
		float x1 = player->GetPosition().x;
		float x2 = player2->GetPosition().x;

		float absScale1 = fabsf(player->GetScale().x);
		float absScale2 = fabsf(player2->GetScale().x);

		DirectX::XMFLOAT3 s1 = player->GetScale();
		DirectX::XMFLOAT3 s2 = player2->GetScale();

		float rotLeft = DirectX::XM_PI / 2.0f;   // 左向き
		float rotRight = DirectX::XM_PI / -2.0f; // 右向き

		// 1Pが 2Pより「左」にいる場合
		if (x1 < x2)
		{
			// 1P: 右を向く (通常スケール)
			s1.x = absScale1;
			player->SetScale(s1);
			player->SetRotation({ 0.0f, rotRight, 0.0f });

			// 2P: 左を向く (反転スケール)
			s2.x = -absScale2;
			player2->SetScale(s2);
			player2->SetRotation({ 0.0f, rotLeft, 0.0f });
		}
		// 1Pが 2Pより「右」にいる場合 (位置入れ替え時)
		else
		{
			// 1P: 左を向く (反転スケール)
			s1.x = -absScale1;
			player->SetScale(s1);
			player->SetRotation({ 0.0f, rotLeft, 0.0f });

			// 2P: 右を向く (通常スケール)
			s2.x = absScale2;
			player2->SetScale(s2);
			player2->SetRotation({ 0.0f, rotRight, 0.0f });
		}
	}

	// ==================================================
	// ステージ端の移動制限
	// ==================================================
	if (player)
	{
		XMFLOAT3 pos = player->GetPosition();
		pos.x = std::clamp(pos.x, -STAGE_LIMIT_X, STAGE_LIMIT_X);
		player->SetPosition(pos);
	}
	if (player2)
	{
		XMFLOAT3 pos = player2->GetPosition();
		pos.x = std::clamp(pos.x, -STAGE_LIMIT_X, STAGE_LIMIT_X);
		player2->SetPosition(pos);
	}


	// ==================================================
	// 当たり判定と相互作用
	// ==================================================
	if (player && player2)
	{
		// A. プレイヤー同士の押し出し処理 (めり込み防止)
		bool isColliding = player->CheckCollision(player2);
		player->SetIsColliding(isColliding);
		player2->SetIsColliding(isColliding);

		// どちらも攻撃中でなければ、重ならないように位置を補正
		if (isColliding && !player->IsAttacking() && !player2->IsAttacking())
		{
			DirectX::BoundingBox box1 = player->GetHurtbox(HurtboxType::BODY);
			DirectX::BoundingBox box2 = player2->GetHurtbox(HurtboxType::BODY);

			DirectX::XMFLOAT3 pos1 = player->GetPosition();
			DirectX::XMFLOAT3 pos2 = player2->GetPosition();

			float deltaX = box1.Center.x - box2.Center.x;
			float sumExtentsX = box1.Extents.x + box2.Extents.x;
			float overlapX = sumExtentsX - abs(deltaX);

			float deltaY = box1.Center.y - box2.Center.y;
			float sumExtentsY = box1.Extents.y + box2.Extents.y;
			float overlapY = sumExtentsY - abs(deltaY);

			DirectX::XMFLOAT3 pushVector = { 0.0f, 0.0f, 0.0f };

			// X軸またはY軸で重なりが小さいほうへ押し出す
			if (overlapX < overlapY)
			{
				float pushAmount = overlapX / 2.0f;
				float direction = (deltaX > 0.0f) ? 1.0f : -1.0f;
				pushVector.x = direction * pushAmount;
			}
			else
			{
				float pushAmount = overlapY / 2.0f;
				float direction = (deltaY > 0.0f) ? 1.0f : -1.0f;
				pushVector.y = direction * pushAmount;
			}

			pos1.x += pushVector.x;
			pos2.x -= pushVector.x;
			pos1.y += pushVector.y;
			pos2.y -= pushVector.y;

			// 押し出し後もステージ外に出ないよう制限
			pos1.x = std::clamp(pos1.x, -STAGE_LIMIT_X, STAGE_LIMIT_X);
			pos2.x = std::clamp(pos2.x, -STAGE_LIMIT_X, STAGE_LIMIT_X);

			player->SetPosition(pos1);
			player2->SetPosition(pos2);
		}

		// B. 攻撃ヒット判定 (1P -> 2P)
		bool hit2 = false;
		if (player->IsAttacking() && !player->HasHit())
		{
			// 1PのHitbox(赤) vs 2PのHurtbox(緑:頭/体/足)
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
			// ダメージ適用
			AttackParams* params = player->GetCurrentAttackParams();
			int dmg = (params != nullptr) ? params->damage : 0;
			int stun = (params != nullptr) ? params->hitFrame : 30;

			player2->ReceiveDamage(dmg);
			player->OnHit(); // 多段ヒット防止

			// 2Pをやられ状態へ遷移
			player2->SetState(new PlayerStateDamage(stun));

			// HPバーの更新
			float ratio = player2->GetHpRatio();
			float currentWidth = m_barMaxWidth * ratio;
			float reduceWidth = m_barMaxWidth - currentWidth;
			m_enemyhpBar->SetSize(currentWidth, 80.0f);
			m_enemyhpBar->SetPosition(m_enemyHpBarPos.x - (reduceWidth / 2.0f), m_enemyHpBarPos.y);
		}

		// C. 攻撃ヒット判定 (2P -> 1P)
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
			AttackParams* params = player2->GetCurrentAttackParams();
			int dmg = (params != nullptr) ? params->damage : 0;
			int stun = (params != nullptr) ? params->hitFrame : 30;

			player->ReceiveDamage(dmg);
			player2->OnHit();

			player->SetState(new PlayerStateDamage(stun));

			float ratio = player->GetHpRatio();
			float currentWidth = m_barMaxWidth * ratio;
			float reduceWidth = m_barMaxWidth - currentWidth;
			m_hpBar->SetSize(currentWidth, 80.0f);
			m_hpBar->SetPosition(m_hpBarPos.x + (reduceWidth / 2.0f), m_hpBarPos.y);
		}
	}


	// ==================================================
	// カメラ制御 (プレイヤー間の中点と距離に基づく)
	// ==================================================
	CameraBase* pCamera = GetObj<CameraBase>("Camera");
	if (pCamera && player && player2)
	{
		XMFLOAT3 p1Pos = player->GetPosition();
		XMFLOAT3 p2Pos = player2->GetPosition();

		// 中点計算
		float centerX = (p1Pos.x + p2Pos.x) * 0.5f;
		centerX = std::clamp(centerX, -CAMERA_LIMIT_X, CAMERA_LIMIT_X);

		// 高さ調整
		float maxY = (p1Pos.y > p2Pos.y) ? p1Pos.y : p2Pos.y;
		float targetLookY = 1.4f + (maxY * 0.2f);
		float targetPosY = 1.5f + (maxY * 0.1f);

		// 距離に応じたズーム制御
		float distX = fabsf(p1Pos.x - p2Pos.x);
		float zoomFactorX = 0.45f;
		float zoomFactorY = 0.8f;
		float zoomAmount = 0.0f;
		float reqZoomX = (distX - 1.5f) * zoomFactorX;
		float reqZoomY = maxY * zoomFactorY;
		if (reqZoomX > reqZoomY) zoomAmount = reqZoomX;
		else zoomAmount = reqZoomY;
		if (zoomAmount < 0.0f) zoomAmount = 0.0f;

		float baseZ = -3.5f;
		float targetZ = baseZ - zoomAmount;
		if (targetZ < -9.0f) targetZ = -9.0f;

		XMFLOAT3 targetPos = { centerX, targetPosY, targetZ };
		XMFLOAT3 targetLook = { centerX, targetLookY, 0.0f };

		// スムーズな移動補間
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

		// スカイドームの位置もカメラに追従
		if (m_skyDome)
		{
			m_skyDome->Update(pCamera->GetPos());
		}
	}
}

/**
 * @brief 描画処理
 */
void SceneBlank::Draw()
{
	// 共通リソースの取得
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

	// 1. スカイドームの描画
	if (m_skyDome)
	{
		m_skyDome->Draw(pCamera->GetView(), pCamera->GetProj(), GetObj<Shader>("VS_Object"));
	}

	// 2. プレイヤー1の描画
	Player* player = GetObj<Player>("Player");
	if (player) {
		// ワールド行列の計算
		XMFLOAT3 pos = player->GetPosition();
		XMFLOAT3 rot = player->GetRotation();
		XMFLOAT3 pScale = player->GetScale();
		Matrix playerScaleMat = Matrix::CreateScale(pScale.x, pScale.y, pScale.z);
		Matrix modelBaseScaleMat = player->GetModel()->GetScaleBaseMatrix();
		Matrix rotMat = DirectX::XMMatrixRotationRollPitchYaw(rot.x, rot.y, rot.z);
		Matrix transMat = Matrix::CreateTranslation(pos.x, pos.y, pos.z);
		Matrix world = modelBaseScaleMat * playerScaleMat * rotMat * transMat;

		// シェーダーへの定数バッファ転送
		XMStoreFloat4x4(&mat[0], XMMatrixTranspose(world));
		shader[0]->WriteBuffer(0, mat);
		shader[1]->WriteBuffer(0, light);
		shader[1]->WriteBuffer(1, camera);
		player->SetVertexShader(shader[0]);
		player->SetPixelShader(shader[1]);

		// 本体とデバッグ用ボックスの描画
		player->Draw();
		player->DrawBoundingBox();
		player->DrawHitbox();
	}

	// 3. プレイヤー2の描画
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

	// 4. 2D UIの描画 (最前面)
	SimpleUI::Clear();
	DirectX::XMFLOAT4X4 identity;
	DirectX::XMStoreFloat4x4(&identity, DirectX::XMMatrixIdentity());
	SimpleUI::SetMatrix(identity, identity, identity);

	if (m_hpBar) m_hpBar->Draw();
	if (m_enemyhpBar) m_enemyhpBar->Draw();

	SimpleUI::DrawAll();
}