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
#include <cmath>
#include "Projectile.h"
#include "PlayerParameterLoader.h" 

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
	m_isKOStage = false;

	// スロー演出用初期化
	m_isSlowMotion = false;
	m_slowMotionTimer = 0.0f;
	m_cameraZoomStartPos = { 0,0,0 };
	m_cameraZoomTargetPos = { 0,0,0 };

	// ==================================================
	// シェーダーの読み込み
	// ==================================================
	Shader* shader[] = {
		CreateObj<VertexShader>("VS_SkinMeshAnimation"),
		CreateObj<PixelShader>("PS_TexColor"),
		CreateObj<VertexShader>("VS_Object"),
		CreateObj<VertexShader>("VS_SkinMeshOutline"),
		CreateObj<PixelShader>("PS_Outline")
	};
	const char* file[] = {
		"Assets/Shader/VS_SkinMeshAnimation.cso",
		"Assets/Shader/PS_TexColor.cso",
		"Assets/Shader/VS_Object.cso",
		"Assets/Shader/VS_SkinMeshOutline.cso",
		"Assets/Shader/PS_Outline.cso"
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
	// 背景（スカイドーム）の読み込み
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
	// UIマネージャの初期化
	// ==================================================
	m_uiManager = new BattleUIManager();
	m_uiManager->Init();


	// ==================================================
	//  プレイヤーの生成と設定ロード
	// ==================================================
	CreateObj<Player>("Player");
	Player* player = GetObj<Player>("Player");
	player->SetInputType(PlayerInputType::PLAYER_1);

	// 設定ファイルからパラメータを読み込み
	PlayerParameterLoader::LoadSettings(player);

	if (!player->Load("Assets/Model/knight/Idle.fbx", 0.014f, true, false))
	{
		MessageBox(NULL, "プレイヤーモデルの読み込みに失敗しました。", "Model Load Error", MB_OK);
	}
	// アニメーション読み込み
	player->GetModel()->LoadAnimation("Assets/Model/knight/Walking.fbx", "Walk", true);
	player->GetModel()->LoadAnimation("Assets/Model/knight/WalkBack.fbx", "WalkBack", true);
	player->GetModel()->LoadAnimation("Assets/Model/knight/CrouchIdle.fbx", "CrouchIdle", true);
	player->GetModel()->LoadAnimation("Assets/Model/knight/LightPunch.fbx", "LightPunch", true);
	player->GetModel()->LoadAnimation("Assets/Model/knight/MediumPunch.fbx", "MediumPunch", true);
	player->GetModel()->LoadAnimation("Assets/Model/knight/HeavyPunch.fbx", "HeavyPunch", true);
	player->GetModel()->LoadAnimation("Assets/Model/knight/MediumKick.fbx", "MediumKick", true);
	player->GetModel()->LoadAnimation("Assets/Model/knight/HeavyKick.fbx", "HeavyKick", true);
	player->GetModel()->LoadAnimation("Assets/Model/knight/Jump.fbx", "Jump", true);
	player->GetModel()->LoadAnimation("Assets/Model/knight/Damage.fbx", "Damage", true);
	player->GetModel()->LoadAnimation("Assets/Model/knight/Down.fbx", "Down", true);
	player->GetModel()->LoadAnimation("Assets/Model/knight/WakeUp.fbx", "WakeUp", true);
	player->GetModel()->LoadAnimation("Assets/Model/knight/Hadouken.fbx", "Hadouken", true);

	// 初期位置設定
	ResetRound();


	// ==================================================
	// 5. プレイヤー2の生成 
	// ==================================================
	CreateObj<Player>("Player2");
	Player* player2 = GetObj<Player>("Player2");
	player2->SetInputType(PlayerInputType::PLAYER_2);
	player2->SetMoveSpeed(player->GetMoveSpeed());

	DirectX::XMFLOAT3 scaleP2 = player->GetScale();
	scaleP2.x *= -1.0f; // X軸反転
	player2->SetScale(scaleP2);

	// P1からパラメータをコピー
	PlayerParameterLoader::CopyParameters(player, player2);

	if (!player2->Load("Assets/Model/knight/Idle.fbx", 0.014f, true, false))
	{
		MessageBox(NULL, "プレイヤー2モデルの読み込みに失敗しました。", "Model Load Error", MB_OK);
	}
	player2->GetModel()->LoadAnimation("Assets/Model/knight/Walking.fbx", "Walk", true);
	player2->GetModel()->LoadAnimation("Assets/Model/knight/WalkBack.fbx", "WalkBack", true);
	player2->GetModel()->LoadAnimation("Assets/Model/knight/LightPunch.fbx", "LightPunch", true);
	player2->GetModel()->LoadAnimation("Assets/Model/knight/MediumPunch.fbx", "MediumPunch", true);
	player2->GetModel()->LoadAnimation("Assets/Model/knight/HeavyPunch.fbx", "HeavyPunch", true);
	player2->GetModel()->LoadAnimation("Assets/Model/knight/MediumKick.fbx", "MediumKick", true);
	player2->GetModel()->LoadAnimation("Assets/Model/knight/HeavyKick.fbx", "HeavyKick", true);
	player2->GetModel()->LoadAnimation("Assets/Model/knight/Jump.fbx", "Jump", true);
	player2->GetModel()->LoadAnimation("Assets/Model/knight/Damage.fbx", "Damage", true);
	player2->GetModel()->LoadAnimation("Assets/Model/knight/CrouchIdle.fbx", "CrouchIdle", true);
	player2->GetModel()->LoadAnimation("Assets/Model/knight/Down.fbx", "Down", true);
	player2->GetModel()->LoadAnimation("Assets/Model/knight/WakeUp.fbx", "WakeUp", true);
	player2->GetModel()->LoadAnimation("Assets/Model/knight/Hadouken.fbx", "Hadouken", true);

	// 初期位置設定
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

	for (int i = 0; i < 10; i++)
	{
		m_hitEffects.push_back(new HitEffect());
	}

	// ----------------------------------------------------
	// 描画設定の作成（スカイドーム表示用）
	// ----------------------------------------------------
	D3D11_DEPTH_STENCIL_DESC depthDesc = {};
	depthDesc.DepthEnable = TRUE;
	depthDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	depthDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL; // 1.0(最奥)も描画する
	depthDesc.StencilEnable = FALSE;
	GetDevice()->CreateDepthStencilState(&depthDesc, &m_pDepthState);

	// 半透明合成用ブレンドステート 
	D3D11_BLEND_DESC blendDesc = {};
	blendDesc.AlphaToCoverageEnable = TRUE;
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

	// ----------------------------------------------------
	// アウトライン用ラスタライザーステートの作成
	// ----------------------------------------------------
	D3D11_RASTERIZER_DESC rsDesc = {};
	rsDesc.FillMode = D3D11_FILL_SOLID;
	rsDesc.FrontCounterClockwise = FALSE; // 時計回りが表面
	rsDesc.DepthBias = 0;
	rsDesc.SlopeScaledDepthBias = 0.0f;
	rsDesc.DepthBiasClamp = 0.0f;
	rsDesc.DepthClipEnable = TRUE;
	rsDesc.ScissorEnable = FALSE;
	rsDesc.MultisampleEnable = FALSE;
	rsDesc.AntialiasedLineEnable = FALSE;

	// 表面カリング（P1アウトライン用：表面を消して裏面を描く）
	rsDesc.CullMode = D3D11_CULL_FRONT;
	GetDevice()->CreateRasterizerState(&rsDesc, &m_pCullFront);

	// 裏面カリング（通常描画用：裏面を消して表面を描く）
	rsDesc.CullMode = D3D11_CULL_BACK;
	GetDevice()->CreateRasterizerState(&rsDesc, &m_pCullBack);

	// スカイドーム用：カリングなし
	rsDesc.CullMode = D3D11_CULL_NONE;
	rsDesc.DepthClipEnable = FALSE; // スカイドーム用は深度クリップも切っておくと安全
	GetDevice()->CreateRasterizerState(&rsDesc, &m_pCullNone);
}

void SceneBlank::Uninit()
{
	if (m_uiManager)
	{
		delete m_uiManager;
		m_uiManager = nullptr;
	}

	if (m_skyDome) { delete m_skyDome; m_skyDome = nullptr; }
	if (g_uiTex) { delete g_uiTex; g_uiTex = nullptr; }

	// 描画設定の解放
	if (m_pDepthState) { m_pDepthState->Release(); m_pDepthState = nullptr; }
	if (m_pBlendState) { m_pBlendState->Release(); m_pBlendState = nullptr; }
	if (m_pCullFront) { m_pCullFront->Release(); m_pCullFront = nullptr; }
	if (m_pCullBack) { m_pCullBack->Release(); m_pCullBack = nullptr; }
	if (m_pCullNone) { m_pCullNone->Release(); m_pCullNone = nullptr; }
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
	m_isKOStage = false;

	// スロー演出リセット
	m_isSlowMotion = false;
	m_slowMotionTimer = 0.0f;


	// フェーズ初期化
	if (m_winCountP1 == 0 && m_winCountP2 == 0)
	{
		m_currentPhase = RoundPhase::READY;
	}
	else
	{
		m_currentPhase = RoundPhase::ROUND_CALL;
	}

	m_phaseTimer = 0.0f;

	if (m_uiManager)
	{
		m_uiManager->Reset();
	}

	// 1Pリセット
	Player* player = GetObj<Player>("Player");
	if (player)
	{
		player->SetPosition({ -2.0f, 0.0f, 0.0f });
		player->SetRotation({ 0.0f, DirectX::XM_PI / -2.0f, 0.0f });
		player->Reset(); // HP全回復、ステートリセット
		player->SetInputType(PlayerInputType::AI);
	}

	// 2Pリセット
	Player* player2 = GetObj<Player>("Player2");
	if (player2)
	{
		player2->SetPosition({ 2.0f, 0.0f, 0.0f });
		player2->SetRotation({ 0.0f, DirectX::XM_PI / 2.0f, 0.0f });
		player2->Reset();
		player2->SetInputType(PlayerInputType::AI);
	}

	// カメラリセット
	CameraBase* pCamera = GetObj<CameraBase>("Camera");
	if (pCamera)
	{
		pCamera->SetPos({ 0.0f, 1.2f, -5.0f });
		pCamera->SetLook({ 0.0f, 1.0f, 4.0f });
	}

	DebugLog::log(DebugLog::INFO_LOG, "--- Round Start Sequence ---");
}


void SceneBlank::Update(float tick)
{
	Player* player = GetObj<Player>("Player");
	Player* player2 = GetObj<Player>("Player2");

	// ==========================================================
	// 0. ラウンド開始演出 (フェーズ管理)
	// ラウンド中(PLAYING)でもなく、KO演出中(KO_CALL)でもない場合は
	// 開始前演出（READY/ROUND/FIGHT）を行う
	// ==========================================================
	if (m_currentPhase != RoundPhase::PLAYING && m_currentPhase != RoundPhase::KO_CALL)
	{
		m_phaseTimer += tick;

		if (m_currentPhase == RoundPhase::READY)
		{
			if (m_phaseTimer >= 1.0f)
			{
				m_currentPhase = RoundPhase::ROUND_CALL;
				m_phaseTimer = 0.0f;
			}
		}
		else if (m_currentPhase == RoundPhase::ROUND_CALL)
		{
			if (m_phaseTimer >= 1.5f)
			{
				m_currentPhase = RoundPhase::FIGHT_CALL;
				m_phaseTimer = 0.0f;
			}
		}
		else if (m_currentPhase == RoundPhase::FIGHT_CALL)
		{
			if (m_phaseTimer >= 1.0f)
			{
				m_currentPhase = RoundPhase::PLAYING;
				m_phaseTimer = 0.0f;

				// 操作権限をプレイヤーに戻す
				if (player) player->SetInputType(PlayerInputType::PLAYER_1);
				if (player2) player2->SetInputType(PlayerInputType::PLAYER_2);
				DebugLog::log(DebugLog::INFO_LOG, "--- FIGHT! ---");
			}
		}

		if (player) player->Update(tick);
		if (player2) player2->Update(tick);

		CameraBase* pCamera = GetObj<CameraBase>("Camera");
		if (m_skyDome && pCamera)
		{
			m_skyDome->Update(pCamera->GetPos());
		}
		return;
	}


	// ==========================================================
	// ラウンド終了後の待機処理 (スローモーション演出含む)
	// ==========================================================
	if (m_isRoundOver)
	{
		bool isGameSet = (m_winCountP1 >= ROUND_TO_WIN || m_winCountP2 >= ROUND_TO_WIN);

		// スローモーション演出処理
		if (m_isSlowMotion)
		{
			// 時間進行（実時間はそのまま進む）
			m_slowMotionTimer -= tick;

			// プレイヤーの更新はゆっくりにする (通常の10%)
			float slowTick = tick * 0.1f;
			if (player) player->Update(slowTick);
			if (player2) player2->Update(slowTick);

			// カメラのズーム演出 (線形補間)
			CameraBase* pCamera = GetObj<CameraBase>("Camera");
			if (pCamera && m_slowMotionDuration > 0.0f)
			{
				float t = 1.0f - (m_slowMotionTimer / m_slowMotionDuration); // 0.0 -> 1.0
				if (t > 1.0f) t = 1.0f;

				// イージング（少し滑らかに）
				float easeT = t * t * (3.0f - 2.0f * t);

				XMVECTOR vStartPos = XMLoadFloat3(&m_cameraZoomStartPos);
				XMVECTOR vTargetPos = XMLoadFloat3(&m_cameraZoomTargetPos);
				XMVECTOR vCurrentPos = XMVectorLerp(vStartPos, vTargetPos, easeT);

				XMVECTOR vStartLook = XMLoadFloat3(&m_cameraZoomStartLook);
				XMVECTOR vTargetLook = XMLoadFloat3(&m_cameraZoomTargetLook);
				XMVECTOR vCurrentLook = XMVectorLerp(vStartLook, vTargetLook, easeT);

				XMFLOAT3 newPos, newLook;
				XMStoreFloat3(&newPos, vCurrentPos);
				XMStoreFloat3(&newLook, vCurrentLook);
				pCamera->SetPos(newPos);
				pCamera->SetLook(newLook);
			}

			// スローモーション終了判定
			if (m_slowMotionTimer <= 0.0f)
			{
				m_isSlowMotion = false;
				// スロー終了後、カメラはズームしたままにするなら何もしない
			}
		}
		else
		{
			// 通常のラウンド終了後待機（フェードアウトなど）
			// スローが終わってからフェード処理を開始する
			m_roundEndTimer += tick;

			// フェード処理
			if (!isGameSet && m_uiManager)
			{
				if (m_roundEndTimer < WAIT_BEFORE_FADE)
				{
					m_uiManager->SetFadeAlpha(0.0f);
				}
				else
				{
					float progress = (m_roundEndTimer - WAIT_BEFORE_FADE) / FADE_DURATION;
					if (progress > 1.0f) progress = 1.0f;
					float alpha = 0.1f + (progress * 0.9f);
					m_uiManager->SetFadeAlpha(alpha);
				}
			}

			// 倒れた後の動きは通常速度に戻すか、停止させるか。ここでは通常速度で少し動かす
			if (player) player->Update(tick);
			if (player2) player2->Update(tick);
		}

		// リセット処理へ移行
		// フェードが終わるまで待つ
		if (m_roundEndTimer >= ROUND_WAIT_TIME)
		{
			if (m_winCountP1 >= ROUND_TO_WIN || m_winCountP2 >= ROUND_TO_WIN)
			{
				s_isGameSet = true;
			}
			else
			{
				ResetRound();
			}
		}
	}
	else
	{
		// ==========================================================
		// 通常のゲーム進行
		// ==========================================================
		float playerTick = tick;
		if (m_hitStopTimer > 0.0f)
		{
			m_hitStopTimer -= tick;
			playerTick = 0.0f; // 時間停止
		}

		if (player) player->Update(playerTick);
		if (player2) player2->Update(playerTick);

		// ==========================================================
		// 衝突・攻撃判定を一括処理
		// ==========================================================
		CollisionResult result = BattleCollision::UpdateInteractions(
			player,
			player2,
			playerTick,
			m_hitEffects,
			STAGE_LIMIT_X
		);

		// 結果の適用
		if (result.isRoundOver)
		{
			m_isRoundOver = true;
			m_winCountP1 += result.winCountP1ToAdd;
			m_winCountP2 += result.winCountP2ToAdd;

			// KOフェーズへ移行（UI描画用）
			m_currentPhase = RoundPhase::KO_CALL;

			// ★★★ スローモーション演出開始 ★★★
			m_isSlowMotion = true;
			m_slowMotionDuration = 1.5f; // 1.5秒間スローにする
			m_slowMotionTimer = m_slowMotionDuration;

			// カメラのズームターゲット計算
			CameraBase* pCamera = GetObj<CameraBase>("Camera");
			if (pCamera)
			{
				m_cameraZoomStartPos = pCamera->GetPos();
				m_cameraZoomStartLook = pCamera->GetLook();

				XMFLOAT3 p1Pos = player->GetPosition();
				XMFLOAT3 p2Pos = player2->GetPosition();

				// 中間地点を計算
				float centerX = (p1Pos.x + p2Pos.x) * 0.5f;
				float centerY = (p1Pos.y + p2Pos.y) * 0.5f + 0.9f; // 注視点は胸の高さあたり

			
				// 現在のカメラ位置(StartPos)から、注視点(TargetLook)への方向ベクトルを求める
				XMVECTOR vStartPos = XMLoadFloat3(&m_cameraZoomStartPos);
				XMVECTOR vTargetLook = XMVectorSet(centerX, centerY, 0.0f, 0.0f);

				XMVECTOR vDir = XMVectorSubtract(vStartPos, vTargetLook); // 注視点 -> カメラ へのベクトル
				vDir = XMVector3Normalize(vDir);

				// 注視点から一定距離（2.5f）離れた位置をターゲットとする
			
				float zoomDist = 2.5f;
				XMVECTOR vTargetPos = XMVectorAdd(vTargetLook, XMVectorScale(vDir, zoomDist));

				XMStoreFloat3(&m_cameraZoomTargetPos, vTargetPos);
				XMStoreFloat3(&m_cameraZoomTargetLook, { centerX, centerY, 0.0f });
			}
		}

		if (result.hitStopTimer > 0.0f)
		{
			m_hitStopTimer = result.hitStopTimer;
		}

		if (result.shakeTimerP1 > 0.0f) m_shakeTimerP1 = result.shakeTimerP1;
		if (result.shakeTimerP2 > 0.0f) m_shakeTimerP2 = result.shakeTimerP2;

		// UI更新
		if (m_uiManager && player && player2)
		{
			m_uiManager->UpdateHPBars(player->GetHpRatio(), player2->GetHpRatio());
		}
	}

	// ヒットシェイク計算 
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
	// ヒットエフェクトの更新
	for (auto effect : m_hitEffects)
	{
		effect->Update(tick);
	}

	// カメラ制御 (通常時のみ。スローモーション中は上で制御するためスキップ)
	if (!m_isSlowMotion)
	{
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
			// ラウンド終了後の待機中はカメラをゆっくり動かす（スロー後）
			if (m_isRoundOver) smoothSpeed = 1.0f * tick;

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
	else
	{
		// スローモーション中でもスカイドームの位置更新は必要
		CameraBase* pCamera = GetObj<CameraBase>("Camera");
		if (m_skyDome && pCamera)
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
		GetObj<Shader>("VS_SkinMeshOutline"),
		GetObj<Shader>("PS_Outline")
	};

	// 背景の描画
	if (m_skyDome)
	{
		if (m_pCullNone)
		{
			GetContext()->RSSetState(m_pCullNone);
		}
		m_skyDome->Draw(pCamera->GetView(), pCamera->GetProj(), GetObj<Shader>("VS_Object"));
	}

	Player* player = GetObj<Player>("Player");
	Player* player2 = GetObj<Player>("Player2");

	// ==================================================
	// アウトライン描画パス
	// ==================================================

	if (player) {
		// スケールXがマイナスなら反転している
		bool isFlipped = (player->GetScale().x < 0.0f);

		// 反転している場合 -> 裏面を描画したいので「表面カリング(CullBack)」を使う (逆転)
		// 通常の場合       -> 裏面を描画したいので「表面カリング(CullFront)」を使う
		if (isFlipped) {
			if (m_pCullBack) GetContext()->RSSetState(m_pCullBack);
		}
		else {
			if (m_pCullFront) GetContext()->RSSetState(m_pCullFront);
		}

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
		shader[2]->WriteBuffer(0, mat); // WVPのみ
		player->SetVertexShader(shader[2]);
		player->SetPixelShader(shader[3]);
		player->Draw();
	}

	if (player2) {
		bool isFlipped = (player2->GetScale().x < 0.0f);
		if (isFlipped) {
			if (m_pCullBack) GetContext()->RSSetState(m_pCullBack);
		}
		else {
			if (m_pCullFront) GetContext()->RSSetState(m_pCullFront);
		}

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
		shader[2]->WriteBuffer(0, mat);
		player2->SetVertexShader(shader[2]);
		player2->SetPixelShader(shader[3]);
		player2->Draw();
	}

	// ==================================================
	// 通常描画パス (表面を描画)
	// ==================================================

	if (player) {
		bool isFlipped = (player->GetScale().x < 0.0f);

		// 反転している場合 -> 表面を描画したいので「裏面カリング(CullFront)」を使う (逆転)
		// 通常の場合       -> 表面を描画したいので「裏面カリング(CullBack)」を使う
		if (isFlipped) {
			if (m_pCullFront) GetContext()->RSSetState(m_pCullFront);
		}
		else {
			if (m_pCullBack) GetContext()->RSSetState(m_pCullBack);
		}

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

	if (player2) {
		bool isFlipped = (player2->GetScale().x < 0.0f);
		if (isFlipped) {
			if (m_pCullFront) GetContext()->RSSetState(m_pCullFront);
		}
		else {
			if (m_pCullBack) GetContext()->RSSetState(m_pCullBack);
		}

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

	// ------------------------------------------------
	//  飛び道具 & エフェクト描画 
	// ------------------------------------------------

	// 通常のカリング(CullBack)に戻す
	if (m_pCullBack) GetContext()->RSSetState(m_pCullBack);

	// ブレンド設定: AlphaToCoverage ON
	if (m_pBlendState)
	{
		GetContext()->OMSetBlendState(m_pBlendState, blendFactor, 0xffffffff);
	}

	// 飛び道具 (Projectile)
	if (player && player->GetProjectile())
	{
		player->GetProjectile()->Draw(pCamera->GetView(), pCamera->GetProj());
	}
	if (player2 && player2->GetProjectile())
	{
		player2->GetProjectile()->Draw(pCamera->GetView(), pCamera->GetProj());
	}

	//カメラ行列の準備
	DirectX::XMFLOAT4X4 view = pCamera->GetView();
	DirectX::XMFLOAT4X4 proj = pCamera->GetProj();

	for (auto effect : m_hitEffects)
	{
		effect->Draw(view, proj);
	}


	// ------------------------------------------------
	// UIの描画 
	// ------------------------------------------------
	if (m_uiManager)
	{
		m_uiManager->Draw(m_currentPhase, m_winCountP1, m_winCountP2);
	}

	GetContext()->OMSetBlendState(nullptr, blendFactor, 0xffffffff);
	GetContext()->OMSetDepthStencilState(nullptr, 0);
	GetContext()->RSSetState(nullptr);
}