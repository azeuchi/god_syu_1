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
#include <algorithm> // std::clamp (値を範囲内に収める関数) を使用
#include "PlayerStateDamage.h"

using namespace DirectX;
using namespace DirectX::SimpleMath;

// グローバル変数: UI用テクスチャ管理
Texture* g_uiTex = nullptr;

// 設定ファイルパス
const char* SETTINGS_FILE = "player_settings.ini";

// --- ステージとカメラの移動制限設定 ---
// プレイヤーが移動できる限界 (中心(0,0)から左右に±14m)
const float STAGE_LIMIT_X = 14.0f;
// カメラの中心注視点が移動できる限界 (プレイヤーより少し手前で止めることで、壁際に追い込んだ感を出す)
const float CAMERA_LIMIT_X = 10.0f;

/**
 * @brief シーンの初期化処理
 * アセットのロード、オブジェクトの生成、初期配置を行う
 */
void SceneBlank::Init()
{
	// ==================================================
	// 1. シェーダーの読み込み
	// ==================================================
	Shader* shader[] = {
		CreateObj<VertexShader>("VS_SkinMeshAnimation"), // スキンメッシュアニメーション用VS
		CreateObj<PixelShader>("PS_TexColor"),           // テクスチャカラー描画用PS
		CreateObj<VertexShader>("VS_Object")
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

	// スカイドーム用のモデルを作成・ロード
	CreateObj<Model>("SkyModel");
	Model* skyModel = GetObj<Model>("SkyModel");

	// ここにスカイドーム用のFBXファイルのパスを指定
	if (!skyModel->Load("Assets/Model/SkyDome/SkyDome.fbx", 1.0f, true, true))
	{
		MessageBox(NULL, "スカイドームモデルの読み込みに失敗しました", "Error", MB_OK);
	}
	else
	{
		DebugLog::log(DebugLog::INFO_LOG, "スカイドーム");
	}

	skyModel->SetTexture("Assets/Model/SkyDome/sky.jpg");

	// スカイドームにピクセルシェーダー(テクスチャ描画用)をセット
	skyModel->SetPixelShader(GetObj<Shader>("PS_TexColor"));

	// スカイドームクラスのインスタンス生成と初期化
	m_skyDome = new SkyDome();
	m_skyDome->Init(skyModel);

	// ==================================================
	// 2. 2D画像 (HPバー) の生成と配置
	// ==================================================
	// HPバーの最大幅 (HP満タン時の画像の横幅ピクセル数)
	m_barMaxWidth = 500.0f;

	// --- 1P (左側) のHPバー設定 ---
	m_hpBar = new Image2D();
	m_hpBarPos = { 330.0f, 80.0f }; // 1Pバーの初期位置 
	// 画像ファイル、X座標, Y座標, 幅, 高さ で初期化
	m_hpBar->Load("Assets/Texture/hp.png", m_hpBarPos.x, m_hpBarPos.y, m_barMaxWidth, 80.0f);

	// --- 2P (右側) のHPバー設定 ---
	m_enemyhpBar = new Image2D();
	m_enemyHpBarPos = { 950.0f, 80.0f }; // 2Pバーの初期位置
	m_enemyhpBar->Load("Assets/Texture/hp.png", m_enemyHpBarPos.x, m_enemyHpBarPos.y, m_barMaxWidth, 80.0f);

	//m_skyDome->Draw();

	// ==================================================
	// 3. プレイヤー1 (自分) の生成
	// ==================================================
	CreateObj<Player>("Player");
	Player* player = GetObj<Player>("Player");
	// 1Pはキーボードの 'A' 'D' 'W' 'J' 'K' キーなどで操作
	player->SetInputType(PlayerInputType::PLAYER_1);

	// ==================================================
	// 4. 設定ファイルの読み込み (player_settings.ini)
	// ==================================================
	// デバッグシーンで調整した「移動速度」「サイズ」「攻撃パラメータ」などを反映する

	// 一時変数の初期化
	float moveSpeed = 2.0f;
	DirectX::XMFLOAT3 scale = { 1.0f, 1.0f, 1.0f };

	// 攻撃パラメータ取得用 (初期値はPlayerクラスのデフォルト)
	AttackParams lParams = player->GetLightPunchParams();
	AttackParams mParams = player->GetMediumPunchParams();

	std::ifstream ifs(SETTINGS_FILE);
	if (ifs.is_open())
	{
		// 基本パラメータ読み込み
		ifs >> moveSpeed;
		ifs >> scale.x >> scale.y >> scale.z;

		// 3部位(頭・体・足)分のくらい判定サイズ(Extents)と位置(Offset)を読み込む 
		for (int i = 0; i < (int)HurtboxType::COUNT; ++i) {
			DirectX::XMFLOAT2 ext, off;
			if (!ifs.eof()) ifs >> ext.x >> ext.y >> off.x >> off.y;
			// 読み込んだ値をプレイヤーの基本設定にセット
			player->SetHurtboxBase((HurtboxType)i, off, ext);
		}

		// --- 弱パンチ (LightPunch) パラメータ ---
		if (!ifs.eof()) ifs >> lParams.totalDuration;
		if (!ifs.eof()) ifs >> lParams.hitboxStart;
		if (!ifs.eof()) ifs >> lParams.hitboxEnd;
		if (!ifs.eof()) ifs >> lParams.hitboxOffset.x >> lParams.hitboxOffset.y;
		if (!ifs.eof()) ifs >> lParams.hitboxExtents.x >> lParams.hitboxExtents.y;
		if (!ifs.eof()) ifs >> lParams.damage;
		if (!ifs.eof()) ifs >> lParams.hitFrame;
		if (!ifs.eof()) ifs >> lParams.blockFrame;

		// 攻撃中のくらい判定補正 (Offset + Size)
		if (!ifs.eof()) ifs >> lParams.headOffsetVal.x >> lParams.headOffsetVal.y;
		if (!ifs.eof()) ifs >> lParams.headSizeVal.x >> lParams.headSizeVal.y;
		if (!ifs.eof()) ifs >> lParams.bodyOffsetVal.x >> lParams.bodyOffsetVal.y;
		if (!ifs.eof()) ifs >> lParams.bodySizeVal.x >> lParams.bodySizeVal.y;
		if (!ifs.eof()) ifs >> lParams.legsOffsetVal.x >> lParams.legsOffsetVal.y;
		if (!ifs.eof()) ifs >> lParams.legsSizeVal.x >> lParams.legsSizeVal.y;

		// --- 中パンチ (MediumPunch) パラメータ ---
		if (!ifs.eof()) ifs >> mParams.totalDuration;
		if (!ifs.eof()) ifs >> mParams.hitboxStart;
		if (!ifs.eof()) ifs >> mParams.hitboxEnd;
		if (!ifs.eof()) ifs >> mParams.hitboxOffset.x >> mParams.hitboxOffset.y;
		if (!ifs.eof()) ifs >> mParams.hitboxExtents.x >> mParams.hitboxExtents.y;
		if (!ifs.eof()) ifs >> mParams.damage;
		if (!ifs.eof()) ifs >> mParams.hitFrame;
		if (!ifs.eof()) ifs >> mParams.blockFrame;

		// 攻撃中のくらい判定補正
		if (!ifs.eof()) ifs >> mParams.headOffsetVal.x >> mParams.headOffsetVal.y;
		if (!ifs.eof()) ifs >> mParams.headSizeVal.x >> mParams.headSizeVal.y;
		if (!ifs.eof()) ifs >> mParams.bodyOffsetVal.x >> mParams.bodyOffsetVal.y;
		if (!ifs.eof()) ifs >> mParams.bodySizeVal.x >> mParams.bodySizeVal.y;
		if (!ifs.eof()) ifs >> mParams.legsOffsetVal.x >> mParams.legsOffsetVal.y;
		if (!ifs.eof()) ifs >> mParams.legsSizeVal.x >> mParams.legsSizeVal.y;

		ifs.close();
	}

	// 読み込んだ値をプレイヤー1に適用
	player->SetMoveSpeed(moveSpeed);
	player->SetScale(scale);
	player->GetLightPunchParams() = lParams;
	player->GetMediumPunchParams() = mParams;


	// ==================================================
	// 5. モデルとアニメーションのロード (1P)
	// ==================================================
	if (!player->Load("Assets/Model/knight/Idle.fbx", 0.02f, true, false))
	{
		MessageBox(NULL, "プレイヤーモデルの読み込みに失敗しました。", "Model Load Error", MB_OK);
	}
	// 各種アニメーションを名前付きで登録
	player->GetModel()->LoadAnimation("Assets/Model/knight/Walking.fbx", "Walk", true);
	player->GetModel()->LoadAnimation("Assets/Model/knight/WalkBack.fbx", "WalkBack", true);
	player->GetModel()->LoadAnimation("Assets/Model/knight/LightPunch.fbx", "LightPunch", true);
	player->GetModel()->LoadAnimation("Assets/Model/knight/MediumPunch.fbx", "MediumPunch", true);
	player->GetModel()->LoadAnimation("Assets/Model/knight/Jump.fbx", "Jump", true);

	// ★追加: やられアニメーションのロード
	player->GetModel()->LoadAnimation("Assets/Model/knight/Damage.fbx", "Damage", true);

	// 初期位置の設定 (左側に配置、右を向く)
	// Rotation Y = -PI/2 (-90度) で右向き
	player->SetPosition({ -2.0f, 0.0f, 0.0f });
	player->SetRotation({ 0.0f, DirectX::XM_PI / -2.0f, 0.0f });


	// ==================================================
	// 6. プレイヤー2 (相手) の生成
	// ==================================================
	CreateObj<Player>("Player2");
	Player* player2 = GetObj<Player>("Player2");
	// 2Pは矢印キー、テンキーなどで操作
	player2->SetInputType(PlayerInputType::PLAYER_2);

	// 2Pにも同じ設定を適用 (同キャラ対戦想定)
	player2->SetMoveSpeed(moveSpeed);
	player2->SetScale(scale);
	player2->GetLightPunchParams() = lParams;
	player2->GetMediumPunchParams() = mParams;

	// 2Pにも読み込んだ3部位の判定設定をコピー
	for (int i = 0; i < (int)HurtboxType::COUNT; ++i) {
		player2->SetHurtboxBase((HurtboxType)i,
			player->GetHurtboxBaseOffset((HurtboxType)i),
			player->GetHurtboxBaseExtents((HurtboxType)i));
	}

	// 2Pモデルとアニメーションのロード
	if (!player2->Load("Assets/Model/knight/Idle.fbx", 0.02f, true, false))
	{
		MessageBox(NULL, "プレイヤー2モデルの読み込みに失敗しました。", "Model Load Error", MB_OK);
	}
	player2->GetModel()->LoadAnimation("Assets/Model/knight/Walking.fbx", "Walk", true);
	player2->GetModel()->LoadAnimation("Assets/Model/knight/WalkBack.fbx", "WalkBack", true);
	player2->GetModel()->LoadAnimation("Assets/Model/knight/LightPunch.fbx", "LightPunch", true);
	player2->GetModel()->LoadAnimation("Assets/Model/knight/MediumPunch.fbx", "MediumPunch", true);
	player2->GetModel()->LoadAnimation("Assets/Model/knight/Jump.fbx", "Jump", true);
	player2->GetModel()->LoadAnimation("Assets/Model/knight/Damage.fbx", "Damage", true);

	// 初期位置の設定 (右側に配置、左を向く)
	// Rotation Y = PI/2 (90度) で左向き
	player2->SetPosition({ 2.0f, 0.0f, 0.0f });
	player2->SetRotation({ 0.0f, DirectX::XM_PI / 2.0f, 0.0f });

	// ==================================================
	// 7. カメラの初期配置
	// ==================================================
	CameraBase* pCamera = GetObj<CameraBase>("Camera");
	if (pCamera)
	{
		pCamera->SetPos({ 0.0f, 1.2f, -5.0f });
		pCamera->SetLook({ 0.0f, 1.0f, 4.0f });
	}

	// SimpleUI用のテクスチャ管理クラス生成
	g_uiTex = new Texture();
}

/**
 * @brief 終了処理
 * メモリの解放などを行う
 */
void SceneBlank::Uninit()
{
	// 画像メモリの解放 
	if (m_hpBar) delete m_hpBar;
	if (m_enemyhpBar) delete m_enemyhpBar;

	//スカイドームのメモリ開放
	if (m_skyDome) {
		delete m_skyDome;
		m_skyDome = nullptr;
	}

	if (g_uiTex) {
		delete g_uiTex;
		g_uiTex = nullptr;
	}
}

/**
 * @brief 更新処理 (毎フレーム呼ばれる)
 * ゲームロジック、物理演算、カメラ制御などはここで行う
 * @param tick 前回のフレームからの経過時間 (秒)
 */
void SceneBlank::Update(float tick)
{
	Player* player = GetObj<Player>("Player");
	Player* player2 = GetObj<Player>("Player2");

	// ==================================================
	// プレイヤーの状態更新
	// ==================================================
	// 入力受付、アニメーション進行、ステートマシン更新
	if (player) {
		player->Update(tick);
	}
	if (player2) {
		player2->Update(tick);
	}

	// ==================================================
	// ステージ端の制限 (見えない壁)
	// ==================================================
	// プレイヤーが STAGE_LIMIT_X  より外に出ないように座標を補正(clamp)する
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
		// ----------------------------------------------
		// A. 押し出し判定 (CheckCollisionで3部位すべてチェック)
		// ----------------------------------------------
		// どちらかの部位が重なっていたら isColliding = true
		bool isColliding = player->CheckCollision(player2);

		// 色変え用にフラグセット
		player->SetIsColliding(isColliding);
		player2->SetIsColliding(isColliding);

		// 攻撃中でなければ押し合い処理を行う
		// (攻撃中は位置がずれると判定がおかしくなることがあるため、押し合いを無効化する)
		if (isColliding && !player->IsAttacking() && !player2->IsAttacking())
		{
			// 押し出し計算のために、基準となる「体(BODY)」のボックスを取得
			DirectX::BoundingBox box1 = player->GetHurtbox(HurtboxType::BODY);
			DirectX::BoundingBox box2 = player2->GetHurtbox(HurtboxType::BODY);

			DirectX::XMFLOAT3 pos1 = player->GetPosition();
			DirectX::XMFLOAT3 pos2 = player2->GetPosition();

			// 重なり量(overlap)を計算
			float deltaX = box1.Center.x - box2.Center.x;
			float sumExtentsX = box1.Extents.x + box2.Extents.x;
			float overlapX = sumExtentsX - abs(deltaX); // X軸の重なり幅

			float deltaY = box1.Center.y - box2.Center.y;
			float sumExtentsY = box1.Extents.y + box2.Extents.y;
			float overlapY = sumExtentsY - abs(deltaY); // Y軸の重なり幅

			DirectX::XMFLOAT3 pushVector = { 0.0f, 0.0f, 0.0f };

			// X軸かY軸、重なりが浅い方で押し出す (基本は横移動なのでX軸で押し合うことが多い)
			if (overlapX < overlapY)
			{
				float pushAmount = overlapX / 2.0f; // お互いに半分ずつ動く
				float direction = (deltaX > 0.0f) ? 1.0f : -1.0f; // 相手がどっちにいるか
				pushVector.x = direction * pushAmount;
			}
			else
			{
				// 上に乗ったりした場合はY軸で押し出す
				float pushAmount = overlapY / 2.0f;
				float direction = (deltaY > 0.0f) ? 1.0f : -1.0f;
				pushVector.y = direction * pushAmount;
			}

			// 新しい位置を計算 (互いに逆方向へずらす)
			pos1.x += pushVector.x;
			pos2.x -= pushVector.x;
			pos1.y += pushVector.y;
			pos2.y -= pushVector.y;

			// 押し出された結果、壁の外に出ないように再度クランプ
			pos1.x = std::clamp(pos1.x, -STAGE_LIMIT_X, STAGE_LIMIT_X);
			pos2.x = std::clamp(pos2.x, -STAGE_LIMIT_X, STAGE_LIMIT_X);

			player->SetPosition(pos1);
			player2->SetPosition(pos2);
		}

		// ----------------------------------------------
		// B. 攻撃判定 (Hitbox) のヒットチェック & HP更新
		// ----------------------------------------------

		// ★ 1Pの攻撃 -> 2Pにヒットしたか？
		// 条件: 1Pが攻撃中、かつ まだヒット処理してない、かつ 1PのHitboxが2PのHurtbox(3部位どれか)に接触
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
			// ダメージ適用
			// 現在アクティブな技のパラメータを取得してダメージ計算
			AttackParams* params = player->GetCurrentAttackParams();
			int dmg = (params != nullptr) ? params->damage : 0;
			// ★追加: 技ごとのhitFrame(有利フレーム)を硬直時間として取得する
			int stun = (params != nullptr) ? params->hitFrame : 30; // 指定がない場合は30F

			player2->ReceiveDamage(dmg);

			// 多段ヒット防止のためフラグを立てる (アニメーション再生時にリセットされる)
			player->OnHit();

			// ★追加: 2Pを「やられ状態」に遷移させる (stunフレーム分硬直)
			player2->SetState(new PlayerStateDamage(stun));

			// --- 2P HPバーの更新 (右側) ---
			float ratio = player2->GetHpRatio();        // 残りHP割合 (0.0~1.0)
			float currentWidth = m_barMaxWidth * ratio; // 現在のバーの幅

			// 減った幅 (reduceWidth) を計算
			float reduceWidth = m_barMaxWidth - currentWidth;

			// サイズを更新
			m_enemyhpBar->SetSize(currentWidth, 80.0f);

			// 位置補正: 減った幅の半分だけ「左(-)」にずらす
			// これにより、中心基準描画の画像が、右端を固定して左へ縮んでいるように見える
			m_enemyhpBar->SetPosition(m_enemyHpBarPos.x - (reduceWidth / 2.0f), m_enemyHpBarPos.y);
		}

		// ★ 2Pの攻撃 -> 1Pにヒットしたか？
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
			// ダメージ適用
			AttackParams* params = player2->GetCurrentAttackParams();
			int dmg = (params != nullptr) ? params->damage : 0;
			// ★追加: 技ごとのhitFrame(有利フレーム)を硬直時間として取得する
			int stun = (params != nullptr) ? params->hitFrame : 30;

			player->ReceiveDamage(dmg);
			player2->OnHit(); // 多段ヒット防止

			// ★追加: 1Pを「やられ状態」に遷移させる
			player->SetState(new PlayerStateDamage(stun));

			// --- 1P HPバーの更新 (左側) ---
			float ratio = player->GetHpRatio();
			float currentWidth = m_barMaxWidth * ratio;
			float reduceWidth = m_barMaxWidth - currentWidth;

			// サイズを更新
			m_hpBar->SetSize(currentWidth, 80.0f);

			// 位置補正: 減った幅の半分だけ「右(+)」にずらす
			// これにより、中心基準描画の画像が、左端を固定して右へ縮んでいるように見える
			m_hpBar->SetPosition(m_hpBarPos.x + (reduceWidth / 2.0f), m_hpBarPos.y);
		}
	}


	// ==================================================
	// カメラ制御
	// ==================================================
	CameraBase* pCamera = GetObj<CameraBase>("Camera");
	if (pCamera && player && player2)
	{
		XMFLOAT3 p1Pos = player->GetPosition();
		XMFLOAT3 p2Pos = player2->GetPosition();

		// --- A. 中心座標 (X軸) ---
		float centerX = (p1Pos.x + p2Pos.x) * 0.5f;

		// カメラの中心もステージ端(CAMERA_LIMIT_X)を超えないように制限する
		// これにより、プレイヤーが端に行ってもカメラが追いきらず、端に追い詰められた演出になる
		centerX = std::clamp(centerX, -CAMERA_LIMIT_X, CAMERA_LIMIT_X);

		// --- B. 高さ調整 (Y軸) ---
		// どちらか高い方のプレイヤーに合わせて、見切れないように高さを微調整
		float maxY = (p1Pos.y > p2Pos.y) ? p1Pos.y : p2Pos.y;
		float targetLookY = 1.4f + (maxY * 0.2f); // 注視点の高さ
		float targetPosY = 1.5f + (maxY * 0.1f);  // カメラ位置の高さ

		// --- C. ズーム計算 (Z軸) ---
		// 2人の距離 (X軸) に応じてカメラを引く
		float distX = fabsf(p1Pos.x - p2Pos.x);

		float zoomFactorX = 0.45f; // 横距離に対する引き具合 (0.45 = 緩やか)
		float zoomFactorY = 0.8f;  // 高さに対する引き具合 (0.8 = ジャンプ対応)

		// 横と縦、どちらのために引くべきか大きい方を採用する (見切れ防止)
		float zoomAmount = 0.0f;
		float reqZoomX = (distX - 1.5f) * zoomFactorX; // 1.5m以上離れたら引く
		float reqZoomY = maxY * zoomFactorY;

		if (reqZoomX > reqZoomY) zoomAmount = reqZoomX;
		else zoomAmount = reqZoomY;

		if (zoomAmount < 0.0f) zoomAmount = 0.0f;

		// ベース位置 (-3.5f) から引く (Z軸マイナス方向へ)
		float baseZ = -3.5f;
		float targetZ = baseZ - zoomAmount;

		// ズームアウト制限 (-9.0f より奥には行かないようにする)
		if (targetZ < -9.0f) targetZ = -9.0f;

		// --- D. 座標の決定 ---
		XMFLOAT3 targetPos = { centerX, targetPosY, targetZ }; // カメラの位置
		XMFLOAT3 targetLook = { centerX, targetLookY, 0.0f };  // カメラが見る先

		// --- E. 滑らかに移動 (Lerp補間) ---
		// 4.0f * tick は「ゆっくり」追従する設定。重厚感を出す。
		float smoothSpeed = 4.0f * tick;

		XMFLOAT3 currentPos = pCamera->GetPos();
		XMFLOAT3 currentLook = pCamera->GetLook();

		// ベクトル演算で補間
		XMVECTOR vCurPos = XMLoadFloat3(&currentPos);
		XMVECTOR vTarPos = XMLoadFloat3(&targetPos);
		XMVECTOR vNewPos = XMVectorLerp(vCurPos, vTarPos, smoothSpeed);

		XMVECTOR vCurLook = XMLoadFloat3(&currentLook);
		XMVECTOR vTarLook = XMLoadFloat3(&targetLook);
		XMVECTOR vNewLook = XMVectorLerp(vCurLook, vTarLook, smoothSpeed);

		XMFLOAT3 newPos, newLook;
		XMStoreFloat3(&newPos, vNewPos);
		XMStoreFloat3(&newLook, vNewLook);

		// カメラに適用
		pCamera->SetPos(newPos);
		pCamera->SetLook(newLook);

		CameraBase* pCamera = GetObj<CameraBase>("Camera");
		if (pCamera && m_skyDome)
		{
			// カメラの位置を渡して、スカイドームが一緒についてくるようにする
			m_skyDome->Update(pCamera->GetPos());
		}

	}
}

/**
 * @brief 描画処理
 * 3Dモデル、エフェクト、UIの順に描画する
 */
void SceneBlank::Draw()
{
	// カメラとライトの準備
	CameraBase* pCamera = GetObj<CameraBase>("Camera");
	LightBase* pLight = GetObj<LightBase>("Light");

	// 定数バッファ(行列など)の更新準備
	DirectX::XMFLOAT4X4 mat[3];
	DirectX::XMStoreFloat4x4(&mat[0], DirectX::XMMatrixIdentity());
	mat[1] = pCamera->GetView();
	mat[2] = pCamera->GetProj();

	// ライト方向などの設定
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

	// シェーダー取得
	Shader* shader[] = {
		GetObj<Shader>("VS_SkinMeshAnimation"),
		GetObj<Shader>("PS_TexColor"),
	};

	if (m_skyDome)
	{
		m_skyDome->Draw(pCamera->GetView(), pCamera->GetProj(), GetObj<Shader>("VS_Object"));
	}

	// ==================================================
	// 1. プレイヤー1の描画
	// ==================================================
	Player* player = GetObj<Player>("Player");
	if (player) {
		// ワールド行列の計算 (Scale * Rotation * Translation)
		XMFLOAT3 pos = player->GetPosition();
		XMFLOAT3 rot = player->GetRotation();
		XMFLOAT3 pScale = player->GetScale();
		Matrix playerScaleMat = Matrix::CreateScale(pScale.x, pScale.y, pScale.z);
		Matrix modelBaseScaleMat = player->GetModel()->GetScaleBaseMatrix();
		Matrix rotMat = DirectX::XMMatrixRotationRollPitchYaw(rot.x, rot.y, rot.z);
		Matrix transMat = Matrix::CreateTranslation(pos.x, pos.y, pos.z);
		Matrix world = modelBaseScaleMat * playerScaleMat * rotMat * transMat;

		XMStoreFloat4x4(&mat[0], XMMatrixTranspose(world));

		// シェーダーにデータを渡す
		shader[0]->WriteBuffer(0, mat);
		shader[1]->WriteBuffer(0, light);
		shader[1]->WriteBuffer(1, camera);

		player->SetVertexShader(shader[0]);
		player->SetPixelShader(shader[1]);

		// 描画実行
		player->Draw();

		// デバッグ用ボックス描画 (くらい判定、攻撃判定)
		player->DrawBoundingBox();
		player->DrawHitbox();
	}

	// ==================================================
	// 2. プレイヤー2の描画
	// ==================================================
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

	// ==================================================
	// 3. 2D UIの描画 (SimpleUI / Image2D)
	// ==================================================

	// SimpleUIの準備 (リストクリア -> 行列初期化 -> 登録 -> 一括描画)
	SimpleUI::Clear();
	DirectX::XMFLOAT4X4 identity;
	DirectX::XMStoreFloat4x4(&identity, DirectX::XMMatrixIdentity());
	SimpleUI::SetMatrix(identity, identity, identity);

	// 各UIの描画登録
	if (m_hpBar) m_hpBar->Draw();
	if (m_enemyhpBar) m_enemyhpBar->Draw();

	// 描画実行
	SimpleUI::DrawAll();
}