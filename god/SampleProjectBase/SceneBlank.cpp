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

// 定数定義
const int ROUND_TO_WIN = 2;         // 2本先取で勝利

// フェード演出用定数
const float WAIT_BEFORE_FADE = 1.0f; // ラウンド終了後、フェード開始までの待機時間
const float FADE_DURATION = 2.0f;    // フェードにかける時間
const float ROUND_WAIT_TIME = WAIT_BEFORE_FADE + FADE_DURATION; // リセットまでの合計時間 (3.0秒)

// 静的メンバ変数の実体定義
bool SceneBlank::s_isGameSet = false;

/**
 * @brief シーンの初期化処理
 * シェーダー、モデル、UI、プレイヤーの生成と設定ロードを行う
 */
void SceneBlank::Init()
{
	// 初期化
	m_hitStopTimer = 0.0f;
	m_shakeTimerP1 = 0.0f;
	m_shakeTimerP2 = 0.0f;
	m_shakeOffsetP1 = { 0.0f, 0.0f, 0.0f };
	m_shakeOffsetP2 = { 0.0f, 0.0f, 0.0f };

	// ラウンド情報の初期化
	m_winCountP1 = 0;
	m_winCountP2 = 0;
	m_isRoundOver = false;
	m_roundEndTimer = 0.0f;
	s_isGameSet = false;

	// ==================================================
	// 1. シェーダーの読み込み
	// ==================================================
	Shader* shader[] = {
		CreateObj<VertexShader>("VS_SkinMeshAnimation"),
		CreateObj<PixelShader>("PS_TexColor"),
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

	// ==================================================
	// 2. 背景（スカイドーム）の読み込み
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
	// 3. UI（HPバー）の初期化
	// ==================================================
	m_barMaxWidth = 500.0f;

	m_hpBar = new Image2D();
	m_hpBarPos = { 330.0f, 80.0f };
	m_hpBar->Load("Assets/Texture/hp.png", m_hpBarPos.x, m_hpBarPos.y, m_barMaxWidth, 80.0f);

	m_enemyhpBar = new Image2D();
	m_enemyHpBarPos = { 950.0f, 80.0f };
	m_enemyhpBar->Load("Assets/Texture/hp.png", m_enemyHpBarPos.x, m_enemyHpBarPos.y, m_barMaxWidth, 80.0f);

	// フェード用画像
	m_fadeBlack = new Image2D();
	// 画面中心座標 (640, 360) に配置 (1280x720)
	m_fadeBlack->Load("Assets/Texture/black.png", 640.0f, 360.0f, 1280.0f, 720.0f);
	// 初期状態は透明な黒
	m_fadeBlack->SetColor(0.0f, 0.0f, 0.0f, 0.0f);


	// ==================================================
	// 4. プレイヤーの生成と設定ロード
	// ==================================================
	CreateObj<Player>("Player");
	Player* player = GetObj<Player>("Player");
	player->SetInputType(PlayerInputType::PLAYER_1);

	// デフォルト値
	float moveSpeed = 2.0f;
	DirectX::XMFLOAT3 scale = { 1.0f, 1.0f, 1.0f };

	// 攻撃パラメータの一時保存用
	AttackParams lParams = player->GetLightPunchParams();
	AttackParams mParams = player->GetMediumPunchParams();
	AttackParams hParams = player->GetHeavyKickParams();

	// ファイルから読み込み
	std::ifstream ifs(SETTINGS_FILE);
	if (ifs.is_open())
	{
		ifs >> moveSpeed;
		ifs >> scale.x >> scale.y >> scale.z;

		// 立ち状態の当たり判定
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

		// 弱攻撃設定
		if (!ifs.eof()) ifs >> lParams.totalDuration;
		if (!ifs.eof()) ifs >> lParams.hitboxStart;
		if (!ifs.eof()) ifs >> lParams.hitboxEnd;
		if (!ifs.eof()) ifs >> lParams.hitboxOffset.x >> lParams.hitboxOffset.y;
		if (!ifs.eof()) ifs >> lParams.hitboxExtents.x >> lParams.hitboxExtents.y;
		if (!ifs.eof()) ifs >> lParams.damage;
		if (!ifs.eof()) ifs >> lParams.hitFrame;
		if (!ifs.eof()) ifs >> lParams.blockFrame;
		if (!ifs.eof()) ifs >> lParams.hitStop;
		if (!ifs.eof()) ifs >> lParams.headOffsetVal.x >> lParams.headOffsetVal.y;
		if (!ifs.eof()) ifs >> lParams.headSizeVal.x >> lParams.headSizeVal.y;
		if (!ifs.eof()) ifs >> lParams.bodyOffsetVal.x >> lParams.bodyOffsetVal.y;
		if (!ifs.eof()) ifs >> lParams.bodySizeVal.x >> lParams.bodySizeVal.y;
		if (!ifs.eof()) ifs >> lParams.legsOffsetVal.x >> lParams.legsOffsetVal.y;
		if (!ifs.eof()) ifs >> lParams.legsSizeVal.x >> lParams.legsSizeVal.y;
		if (!ifs.eof()) ifs >> lParams.cancelEnabled;
		if (!ifs.eof()) ifs >> lParams.cancelStart;
		if (!ifs.eof()) ifs >> lParams.cancelEnd;
		if (!ifs.eof()) ifs >> lParams.cancelToLight >> lParams.cancelToMedium >> lParams.cancelToHeavy;

		// 中攻撃設定
		if (!ifs.eof()) ifs >> mParams.totalDuration;
		if (!ifs.eof()) ifs >> mParams.hitboxStart;
		if (!ifs.eof()) ifs >> mParams.hitboxEnd;
		if (!ifs.eof()) ifs >> mParams.hitboxOffset.x >> mParams.hitboxOffset.y;
		if (!ifs.eof()) ifs >> mParams.hitboxExtents.x >> mParams.hitboxExtents.y;
		if (!ifs.eof()) ifs >> mParams.damage;
		if (!ifs.eof()) ifs >> mParams.hitFrame;
		if (!ifs.eof()) ifs >> mParams.blockFrame;
		if (!ifs.eof()) ifs >> mParams.hitStop;
		if (!ifs.eof()) ifs >> mParams.headOffsetVal.x >> mParams.headOffsetVal.y;
		if (!ifs.eof()) ifs >> mParams.headSizeVal.x >> mParams.headSizeVal.y;
		if (!ifs.eof()) ifs >> mParams.bodyOffsetVal.x >> mParams.bodyOffsetVal.y;
		if (!ifs.eof()) ifs >> mParams.bodySizeVal.x >> mParams.bodySizeVal.y;
		if (!ifs.eof()) ifs >> mParams.legsOffsetVal.x >> mParams.legsOffsetVal.y;
		if (!ifs.eof()) ifs >> mParams.legsSizeVal.x >> mParams.legsSizeVal.y;
		if (!ifs.eof()) ifs >> mParams.cancelEnabled;
		if (!ifs.eof()) ifs >> mParams.cancelStart;
		if (!ifs.eof()) ifs >> mParams.cancelEnd;
		if (!ifs.eof()) ifs >> mParams.cancelToLight >> mParams.cancelToMedium >> mParams.cancelToHeavy;

		// 強攻撃設定
		if (!ifs.eof()) ifs >> hParams.totalDuration;
		if (!ifs.eof()) ifs >> hParams.hitboxStart;
		if (!ifs.eof()) ifs >> hParams.hitboxEnd;
		if (!ifs.eof()) ifs >> hParams.hitboxOffset.x >> hParams.hitboxOffset.y;
		if (!ifs.eof()) ifs >> hParams.hitboxExtents.x >> hParams.hitboxExtents.y;
		if (!ifs.eof()) ifs >> hParams.damage;
		if (!ifs.eof()) ifs >> hParams.hitFrame;
		if (!ifs.eof()) ifs >> hParams.blockFrame;
		if (!ifs.eof()) ifs >> hParams.hitStop;
		if (!ifs.eof()) ifs >> hParams.headOffsetVal.x >> hParams.headOffsetVal.y;
		if (!ifs.eof()) ifs >> hParams.headSizeVal.x >> hParams.headSizeVal.y;
		if (!ifs.eof()) ifs >> hParams.bodyOffsetVal.x >> hParams.bodyOffsetVal.y;
		if (!ifs.eof()) ifs >> hParams.bodySizeVal.x >> hParams.bodySizeVal.y;
		if (!ifs.eof()) ifs >> hParams.legsOffsetVal.x >> hParams.legsOffsetVal.y;
		if (!ifs.eof()) ifs >> hParams.legsSizeVal.x >> hParams.legsSizeVal.y;
		if (!ifs.eof()) ifs >> hParams.cancelEnabled;
		if (!ifs.eof()) ifs >> hParams.cancelStart;
		if (!ifs.eof()) ifs >> hParams.cancelEnd;
		if (!ifs.eof()) ifs >> hParams.cancelToLight >> hParams.cancelToMedium >> hParams.cancelToHeavy;

		ifs.close();
	}

	// プレイヤー1への設定適用
	player->SetMoveSpeed(moveSpeed);
	player->SetScale(scale);
	player->GetLightPunchParams() = lParams;
	player->GetMediumPunchParams() = mParams;
	player->GetHeavyKickParams() = hParams;


	if (!player->Load("Assets/Model/knight/Idle.fbx", 0.02f, true, false))
	{
		MessageBox(NULL, "プレイヤーモデルの読み込みに失敗しました。", "Model Load Error", MB_OK);
	}
	// アニメーション読み込み
	player->GetModel()->LoadAnimation("Assets/Model/knight/Walking.fbx", "Walk", true);
	player->GetModel()->LoadAnimation("Assets/Model/knight/WalkBack.fbx", "WalkBack", true);
	player->GetModel()->LoadAnimation("Assets/Model/knight/CrouchIdle.fbx", "CrouchIdle", true);
	player->GetModel()->LoadAnimation("Assets/Model/knight/LightPunch.fbx", "LightPunch", true);
	player->GetModel()->LoadAnimation("Assets/Model/knight/MediumPunch.fbx", "MediumPunch", true);
	player->GetModel()->LoadAnimation("Assets/Model/knight/HeavyKick.fbx", "HeavyKick", true);
	player->GetModel()->LoadAnimation("Assets/Model/knight/Jump.fbx", "Jump", true);
	player->GetModel()->LoadAnimation("Assets/Model/knight/Damage.fbx", "Damage", true);

	// 初期位置設定
	ResetRound();


	// ==================================================
	// 5. プレイヤー2の生成 (P1の設定を反転して流用)
	// ==================================================
	CreateObj<Player>("Player2");
	Player* player2 = GetObj<Player>("Player2");
	player2->SetInputType(PlayerInputType::PLAYER_2);
	player2->SetMoveSpeed(moveSpeed);

	DirectX::XMFLOAT3 scaleP2 = scale;
	scaleP2.x *= -1.0f; // X軸反転
	player2->SetScale(scaleP2);

	player2->GetLightPunchParams() = lParams;
	player2->GetMediumPunchParams() = mParams;
	player2->GetHeavyKickParams() = hParams;

	// 当たり判定情報のコピー
	for (int i = 0; i < (int)HurtboxType::COUNT; ++i) {
		player2->SetHurtboxBase((HurtboxType)i,
			player->GetHurtboxBaseOffset((HurtboxType)i),
			player->GetHurtboxBaseExtents((HurtboxType)i));
		player2->SetHurtboxCrouch((HurtboxType)i,
			player->GetHurtboxCrouchOffset((HurtboxType)i),
			player->GetHurtboxCrouchExtents((HurtboxType)i));
	}

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

	// 初期位置設定 (ResetRoundで上書きされるが念のため)
	player2->SetPosition({ 2.0f, 0.0f, 0.0f });
	player2->SetRotation({ 0.0f, DirectX::XM_PI / 2.0f, 0.0f });

	// カメラの初期位置
	CameraBase* pCamera = GetObj<CameraBase>("Camera");
	if (pCamera)
	{
		pCamera->SetPos({ 0.0f, 1.2f, -5.0f });
		pCamera->SetLook({ 0.0f, 1.0f, 4.0f });
	}

	g_uiTex = new Texture();

	// ----------------------------------------------------
	// 描画設定の作成（スカイドーム表示用）
	// ----------------------------------------------------
	D3D11_DEPTH_STENCIL_DESC depthDesc = {};
	depthDesc.DepthEnable = TRUE;
	depthDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	depthDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL; // 1.0(最奥)も描画する
	depthDesc.StencilEnable = FALSE;
	GetDevice()->CreateDepthStencilState(&depthDesc, &m_pDepthState);

	// 背景ソリッド消え対策の深度無効ステート
	D3D11_DEPTH_STENCIL_DESC depthDisableDesc = {};
	depthDisableDesc.DepthEnable = FALSE;
	depthDisableDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	depthDisableDesc.DepthFunc = D3D11_COMPARISON_ALWAYS;
	depthDisableDesc.StencilEnable = FALSE;
	GetDevice()->CreateDepthStencilState(&depthDisableDesc, &m_pDepthDisableState);

	// 半透明合成用ブレンドステート
	D3D11_BLEND_DESC blendDesc = {};
	blendDesc.AlphaToCoverageEnable = FALSE;
	blendDesc.IndependentBlendEnable = FALSE;
	blendDesc.RenderTarget[0].BlendEnable = TRUE;
	blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
	blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
	GetDevice()->CreateBlendState(&blendDesc, &m_pBlendState);
}

void SceneBlank::Uninit()
{
	if (m_hpBar) delete m_hpBar;
	if (m_enemyhpBar) delete m_enemyhpBar;
	if (m_fadeBlack) { delete m_fadeBlack; m_fadeBlack = nullptr; }
	if (m_skyDome) { delete m_skyDome; m_skyDome = nullptr; }
	if (g_uiTex) { delete g_uiTex; g_uiTex = nullptr; }

	// 描画設定の解放
	if (m_pDepthState) { m_pDepthState->Release(); m_pDepthState = nullptr; }
	if (m_pDepthDisableState) { m_pDepthDisableState->Release(); m_pDepthDisableState = nullptr; }
	if (m_pBlendState) { m_pBlendState->Release(); m_pBlendState = nullptr; }
}

/**
 * @brief ラウンド開始・リセット処理
 * 位置、HP、向き、状態を初期状態に戻す
 */
void SceneBlank::ResetRound()
{
	m_isRoundOver = false;
	m_roundEndTimer = 0.0f;
	m_hitStopTimer = 0.0f;
	m_shakeTimerP1 = 0.0f;
	m_shakeTimerP2 = 0.0f;

	if (m_fadeBlack)
	{
		m_fadeBlack->SetColor(0.0f, 0.0f, 0.0f, 0.0f);
	}

	// 1Pリセット
	Player* player = GetObj<Player>("Player");
	if (player)
	{
		player->SetPosition({ -2.0f, 0.0f, 0.0f });
		player->SetRotation({ 0.0f, DirectX::XM_PI / -2.0f, 0.0f });
		player->Reset(); // HP全回復、ステートリセット
		player->SetInputType(PlayerInputType::PLAYER_1); // 操作可能に戻す
	}

	// 2Pリセット
	Player* player2 = GetObj<Player>("Player2");
	if (player2)
	{
		player2->SetPosition({ 2.0f, 0.0f, 0.0f });
		player2->SetRotation({ 0.0f, DirectX::XM_PI / 2.0f, 0.0f });
		player2->Reset();
		player2->SetInputType(PlayerInputType::PLAYER_2);
	}

	// HPバーの見た目もリセット
	if (m_hpBar) m_hpBar->SetSize(m_barMaxWidth, 80.0f);
	if (m_enemyhpBar) m_enemyhpBar->SetSize(m_barMaxWidth, 80.0f);
	// 位置も初期位置へ
	if (m_hpBar) m_hpBar->SetPosition(m_hpBarPos.x, m_hpBarPos.y);
	if (m_enemyhpBar) m_enemyhpBar->SetPosition(m_enemyHpBarPos.x, m_enemyHpBarPos.y);

	// カメラリセット
	CameraBase* pCamera = GetObj<CameraBase>("Camera");
	if (pCamera)
	{
		pCamera->SetPos({ 0.0f, 1.2f, -5.0f });
		pCamera->SetLook({ 0.0f, 1.0f, 4.0f });
	}

	DebugLog::log(DebugLog::INFO_LOG, "--- Round Start ---");
}


void SceneBlank::Update(float tick)
{
	Player* player = GetObj<Player>("Player");
	Player* player2 = GetObj<Player>("Player2");

	// ==========================================================
	// 1. ラウンド終了後の待機処理
	// ==========================================================
	if (m_isRoundOver)
	{
		m_roundEndTimer += tick;

		bool isGameSet = (m_winCountP1 >= ROUND_TO_WIN || m_winCountP2 >= ROUND_TO_WIN);

		// ゲームセットでない(次ラウンドがある)場合のみフェード
		if (!isGameSet && m_fadeBlack)
		{
			// 指定時間待機してから、だんだん濃くする
			if (m_roundEndTimer < WAIT_BEFORE_FADE)
			{
				// まだ待機時間内なので透明
				m_fadeBlack->SetColor(0.0f, 0.0f, 0.0f, 0.0f);
			}
			else
			{
				// 待機時間を過ぎたらフェード開始
				// 0.0f から 1.0f へ変化（進行度 progress）
				float progress = (m_roundEndTimer - WAIT_BEFORE_FADE) / FADE_DURATION;
				if (progress > 1.0f) progress = 1.0f;

				// ご要望通り 0.1 からスタート
				float alpha = 0.1f + (progress * 0.9f);

				// 色は黒(0,0,0)で、透明度だけを変える
				m_fadeBlack->SetColor(0.0f, 0.0f, 0.0f, alpha);
			}
		}

		// やられモーション等の更新のため、プレイヤーのUpdateは呼ぶが、
		// 操作は受け付けないように InputType を AI に変更しておく(ResetRoundで戻す)
		if (player) player->Update(tick);
		if (player2) player2->Update(tick);

		// カメラ更新は続ける
		// (後述のカメラ制御コードへ続く)

		// 待機時間(待機+フェード時間)が経過し、完全に暗くなったらリセット
		if (m_roundEndTimer >= ROUND_WAIT_TIME)
		{
			// どちらかが規定ラウンド数勝っていたらリザルトへ (静的フラグを立てる)
			if (m_winCountP1 >= ROUND_TO_WIN || m_winCountP2 >= ROUND_TO_WIN)
			{
				s_isGameSet = true;
			}
			else
			{
				// まだ決着がついていないなら次のラウンドへ
				ResetRound();
			}
		}
	}
	else
	{
		// ==========================================================
		// 2. 通常のゲーム進行 (内容は変更なし)
		// ==========================================================
		float playerTick = tick;
		if (m_hitStopTimer > 0.0f)
		{
			m_hitStopTimer -= tick;
			playerTick = 0.0f; // 時間停止
		}

		if (player) player->Update(playerTick);
		if (player2) player2->Update(playerTick);

		// 向きの制御、移動制限、押し出し処理などはラウンド中のみ有効

		// 向きの制御
		if (player && player2)
		{
			float x1 = player->GetPosition().x;
			float x2 = player2->GetPosition().x;
			float absScale1 = fabsf(player->GetScale().x);
			float absScale2 = fabsf(player2->GetScale().x);
			DirectX::XMFLOAT3 s1 = player->GetScale();
			DirectX::XMFLOAT3 s2 = player2->GetScale();
			float rotLeft = DirectX::XM_PI / 2.0f;
			float rotRight = DirectX::XM_PI / -2.0f;

			if (x1 < x2) {
				s1.x = absScale1; player->SetScale(s1); player->SetRotation({ 0.0f, rotRight, 0.0f });
				s2.x = -absScale2; player2->SetScale(s2); player2->SetRotation({ 0.0f, rotLeft, 0.0f });
			}
			else {
				s1.x = -absScale1; player->SetScale(s1); player->SetRotation({ 0.0f, rotLeft, 0.0f });
				s2.x = absScale2; player2->SetScale(s2); player2->SetRotation({ 0.0f, rotRight, 0.0f });
			}
		}

		// 移動制限
		if (player) {
			XMFLOAT3 pos = player->GetPosition();
			pos.x = std::clamp(pos.x, -STAGE_LIMIT_X, STAGE_LIMIT_X);
			player->SetPosition(pos);
		}
		if (player2) {
			XMFLOAT3 pos = player2->GetPosition();
			pos.x = std::clamp(pos.x, -STAGE_LIMIT_X, STAGE_LIMIT_X);
			player2->SetPosition(pos);
		}

		// 衝突判定と押し出し
		if (player && player2)
		{
			bool isColliding = player->CheckCollision(player2);
			player->SetIsColliding(isColliding);
			player2->SetIsColliding(isColliding);

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

				if (overlapX < overlapY) {
					float pushAmount = overlapX / 2.0f;
					float direction = (deltaX > 0.0f) ? 1.0f : -1.0f;
					pushVector.x = direction * pushAmount;
				}
				else {
					float pushAmount = overlapY / 2.0f;
					float direction = (deltaY > 0.0f) ? 1.0f : -1.0f;
					pushVector.y = direction * pushAmount;
				}

				pos1.x += pushVector.x; pos2.x -= pushVector.x;
				pos1.y += pushVector.y; pos2.y -= pushVector.y;
				pos1.x = std::clamp(pos1.x, -STAGE_LIMIT_X, STAGE_LIMIT_X);
				pos2.x = std::clamp(pos2.x, -STAGE_LIMIT_X, STAGE_LIMIT_X);
				player->SetPosition(pos1);
				player2->SetPosition(pos2);
			}

			// --- 攻撃判定 (P1 -> P2) ---
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
				AttackParams* params = player->GetCurrentAttackParams();
				int dmg = (params != nullptr) ? params->damage : 0;
				int stun = (params != nullptr) ? params->hitFrame : 30;

				player2->ReceiveDamage(dmg);
				player->OnHit();
				player2->SetState(new PlayerStateDamage(stun));

				float ratio = player2->GetHpRatio();
				float currentWidth = m_barMaxWidth * ratio;
				float reduceWidth = m_barMaxWidth - currentWidth;
				m_enemyhpBar->SetSize(currentWidth, 80.0f);
				m_enemyhpBar->SetPosition(m_enemyHpBarPos.x - (reduceWidth / 2.0f), m_enemyHpBarPos.y);

				float stopTime = (params != nullptr) ? params->hitStop : 0.1f;
				m_hitStopTimer = stopTime;
				m_shakeTimerP2 = stopTime;

				// KOチェック
				if (player2->GetHpRatio() <= 0.0f)
				{
					m_winCountP1++;
					m_isRoundOver = true;
					player->SetInputType(PlayerInputType::AI); // 操作不能に
					player2->SetInputType(PlayerInputType::AI);
					DebugLog::log(DebugLog::INFO_LOG, "P1 WIN ROUND!");
				}
			}

			// --- 攻撃判定 (P2 -> P1) ---
			// 既にラウンドが終わっていたら判定しない
			bool hit1 = false;
			if (!m_isRoundOver && player2->IsAttacking() && !player2->HasHit())
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

				float stopTime = (params != nullptr) ? params->hitStop : 0.1f;
				m_hitStopTimer = stopTime;
				m_shakeTimerP1 = stopTime;

				// KOチェック
				if (player->GetHpRatio() <= 0.0f)
				{
					m_winCountP2++;
					m_isRoundOver = true;
					player->SetInputType(PlayerInputType::AI);
					player2->SetInputType(PlayerInputType::AI);
					DebugLog::log(DebugLog::INFO_LOG, "P2 WIN ROUND!");
				}
			}
		}
	}

	// ヒットシェイク計算 (ラウンド終了後も振動は残ってOK)
	if (m_shakeTimerP1 > 0.0f) {
		m_shakeTimerP1 -= tick;
		float offsetX = ((float)(rand() % 100) / 100.0f - 0.5f) * 0.1f;
		float offsetY = ((float)(rand() % 100) / 100.0f - 0.5f) * 0.1f;
		m_shakeOffsetP1 = { offsetX, offsetY, 0.0f };
	}
	else {
		m_shakeOffsetP1 = { 0.0f, 0.0f, 0.0f };
	}

	if (m_shakeTimerP2 > 0.0f) {
		m_shakeTimerP2 -= tick;
		float offsetX = ((float)(rand() % 100) / 100.0f - 0.5f) * 0.1f;
		float offsetY = ((float)(rand() % 100) / 100.0f - 0.5f) * 0.1f;
		m_shakeOffsetP2 = { offsetX, offsetY, 0.0f };
	}
	else {
		m_shakeOffsetP2 = { 0.0f, 0.0f, 0.0f };
	}


	// カメラ制御 (ラウンド終了後もキャラを映し続ける)
	CameraBase* pCamera = GetObj<CameraBase>("Camera");
	if (pCamera && player && player2)
	{
		XMFLOAT3 p1Pos = player->GetPosition();
		XMFLOAT3 p2Pos = player2->GetPosition();

		float centerX = (p1Pos.x + p2Pos.x) * 0.5f;
		centerX = std::clamp(centerX, -CAMERA_LIMIT_X, CAMERA_LIMIT_X);

		float maxY = (p1Pos.y > p2Pos.y) ? p1Pos.y : p2Pos.y;
		float targetLookY = 1.4f + (maxY * 0.2f);
		float targetPosY = 1.5f + (maxY * 0.1f);

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

		if (m_skyDome)
		{
			m_skyDome->Update(pCamera->GetPos());
		}
	}
}

void SceneBlank::Draw()
{
	// ==========================================================
	// 描画設定のリセット・初期化
	// ==========================================================
	float blendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	GetContext()->OMSetBlendState(nullptr, blendFactor, 0xffffffff); // 不透明へリセット

	// スカイドーム(最奥)を描画するために LESS_EQUAL をセット
	if (m_pDepthState)
	{
		GetContext()->OMSetDepthStencilState(m_pDepthState, 0);
	}
	else
	{
		GetContext()->OMSetDepthStencilState(nullptr, 0);
	}


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

	// 1. 背景の描画
	if (m_skyDome)
	{
		m_skyDome->Draw(pCamera->GetView(), pCamera->GetProj(), GetObj<Shader>("VS_Object"));
	}

	// 2. プレイヤー1の描画
	Player* player = GetObj<Player>("Player");
	if (player) {
		XMFLOAT3 pos = player->GetPosition();
		XMFLOAT3 drawPos = { pos.x + m_shakeOffsetP1.x, pos.y + m_shakeOffsetP1.y, pos.z + m_shakeOffsetP1.z };

		XMFLOAT3 rot = player->GetRotation();
		XMFLOAT3 pScale = player->GetScale();
		Matrix playerScaleMat = Matrix::CreateScale(pScale.x, pScale.y, pScale.z);
		Matrix modelBaseScaleMat = player->GetModel()->GetScaleBaseMatrix();
		Matrix rotMat = DirectX::XMMatrixRotationRollPitchYaw(rot.x, rot.y, rot.z);
		Matrix transMat = Matrix::CreateTranslation(drawPos.x, drawPos.y, drawPos.z);
		Matrix world = modelBaseScaleMat * playerScaleMat * rotMat * transMat;

		XMStoreFloat4x4(&mat[0], XMMatrixTranspose(world));
		shader[0]->WriteBuffer(0, mat);
		shader[1]->WriteBuffer(0, light);
		shader[1]->WriteBuffer(1, camera);
		player->SetVertexShader(shader[0]);
		player->SetPixelShader(shader[1]);

		player->Draw();
#ifdef _DEBUG
		player->DrawBoundingBox();
		player->DrawHitbox();
#endif
	}

	// 3. プレイヤー2の描画
	Player* player2 = GetObj<Player>("Player2");
	if (player2) {
		XMFLOAT3 pos = player2->GetPosition();
		XMFLOAT3 drawPos = { pos.x + m_shakeOffsetP2.x, pos.y + m_shakeOffsetP2.y, pos.z + m_shakeOffsetP2.z };

		XMFLOAT3 rot = player2->GetRotation();
		XMFLOAT3 pScale = player2->GetScale();
		Matrix playerScaleMat = Matrix::CreateScale(pScale.x, pScale.y, pScale.z);
		Matrix modelBaseScaleMat = player2->GetModel()->GetScaleBaseMatrix();
		Matrix rotMat = DirectX::XMMatrixRotationRollPitchYaw(rot.x, rot.y, rot.z);
		Matrix transMat = Matrix::CreateTranslation(drawPos.x, drawPos.y, drawPos.z);
		Matrix world = modelBaseScaleMat * playerScaleMat * rotMat * transMat;

		XMStoreFloat4x4(&mat[0], XMMatrixTranspose(world));
		shader[0]->WriteBuffer(0, mat);
		shader[1]->WriteBuffer(0, light);
		shader[1]->WriteBuffer(1, camera);
		player2->SetVertexShader(shader[0]);
		player2->SetPixelShader(shader[1]);

		player2->Draw();
#ifdef _DEBUG
		player2->DrawBoundingBox();
		player2->DrawHitbox();
#endif
	}

	// 4. UIの描画
	SimpleUI::Clear();
	DirectX::XMFLOAT4X4 identity;
	DirectX::XMStoreFloat4x4(&identity, DirectX::XMMatrixIdentity());
	SimpleUI::SetMatrix(identity, identity, identity);

	if (m_hpBar) m_hpBar->Draw();
	if (m_enemyhpBar) m_enemyhpBar->Draw();

	// 通常UIの描画
	SimpleUI::DrawAll();

	// 5. フェード用黒画像 (半透明＆背景消え対策の設定で描画)
	bool isGameSet = (m_winCountP1 >= ROUND_TO_WIN || m_winCountP2 >= ROUND_TO_WIN);
	if (m_isRoundOver && !isGameSet && m_fadeBlack)
	{
		SimpleUI::Clear(); // リストをクリアしてフェード画像だけ登録
		m_fadeBlack->Draw();

		// フェード進行度を計算（Updateと同じ）
		float progress = 0.0f;
		if (m_roundEndTimer >= WAIT_BEFORE_FADE)
		{
			progress = (m_roundEndTimer - WAIT_BEFORE_FADE) / FADE_DURATION;
			if (progress > 1.0f) progress = 1.0f;
		}

	
		if (progress < 0.9f)
		{
			if (m_pBlendState && m_pDepthDisableState)
			{
				GetContext()->OMSetBlendState(m_pBlendState, blendFactor, 0xffffffff);
				GetContext()->OMSetDepthStencilState(m_pDepthDisableState, 0);
			}
			SimpleUI::DrawAll();

			// 設定を戻す
			GetContext()->OMSetBlendState(nullptr, blendFactor, 0xffffffff);
			GetContext()->OMSetDepthStencilState(nullptr, 0);
		}
	
		else
		{
			SimpleUI::DrawAll();
		}
	}

	// 念のためデフォルトに戻す
	GetContext()->OMSetDepthStencilState(nullptr, 0);
}