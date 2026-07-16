#include "math.h"
#include "SceneGame.h"
#include "CameraShake.h"
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
#include "PlayerAssetLoader.h"
#include "SceneRoot.h"
#include "Sound.h"
#include "SimpleFont.h"
#include <Xinput.h>

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
bool SceneGame::s_isGameSet = false;
bool SceneGame::s_requestTitle = false;
bool SceneGame::s_requestConfig = false;

/**
 * @brief シーンの初期化処理
 * シェーダー、モデル、UI、プレイヤーの生成と設定ロードを行う
 */
void SceneGame::Init()
{
	// 初期化
	m_hitStopTimer = 0.0f;
	m_shakeTimerP1 = 0.0f;
	m_shakeTimerP2 = 0.0f;
	m_shakeOffsetP1 = { 0.0f, 0.0f, 0.0f };
	m_shakeOffsetP2 = { 0.0f, 0.0f, 0.0f };
	m_cameraTrauma = 0.0f;
	m_cameraShakeKickDir = 0.0f;
	m_cameraShakeOffset = { 0.0f, 0.0f, 0.0f };

	// ラウンド情報の初期化
	m_winCountP1 = 0;
	m_winCountP2 = 0;
	m_isRoundOver = false;
	m_roundEndTimer = 0.0f;
	s_isGameSet = false;
	s_requestTitle = false;
	s_requestConfig = false;
	m_isResultMenu = false;
	m_resultMenuIndex = 0;
	m_isKOStage = false;

	// 決着メニューの文字描画用フォント初期化
	SimpleFont::Init(GetDevice(), GetContext());

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

	CreateObj<VertexShader>("VS_SpriteShadow");
	GetObj<VertexShader>("VS_SpriteShadow")->Load("Assets/Shader/VS_SpriteShadow.cso");
	CreateObj<PixelShader>("PS_WriteDepth");
	GetObj<PixelShader>("PS_WriteDepth")->Load("Assets/Shader/PS_WriteDepth.cso");
	CreateObj<PixelShader>("PS_Shadow");
	GetObj<PixelShader>("PS_Shadow")->Load("Assets/Shader/PS_Shadow.cso");

	// ==================================================
	// シャドウマップ・床の初期化
	// ==================================================
	D3D11_TEXTURE2D_DESC texDesc = {};
	texDesc.Width = 2048;
	texDesc.Height = 2048;
	texDesc.MipLevels = 1;
	texDesc.ArraySize = 1;
	texDesc.Format = DXGI_FORMAT_R32_FLOAT;
	texDesc.SampleDesc.Count = 1;
	texDesc.Usage = D3D11_USAGE_DEFAULT;
	texDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	GetDevice()->CreateTexture2D(&texDesc, nullptr, &m_shadowMapTex);
	GetDevice()->CreateRenderTargetView(m_shadowMapTex, nullptr, &m_shadowRTV);

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = texDesc.Format;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;
	GetDevice()->CreateShaderResourceView(m_shadowMapTex, &srvDesc, &m_shadowSRV);

	D3D11_TEXTURE2D_DESC depthDescS = {};
	depthDescS.Width = 2048;
	depthDescS.Height = 2048;
	depthDescS.MipLevels = 1;
	depthDescS.ArraySize = 1;
	depthDescS.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depthDescS.SampleDesc.Count = 1;
	depthDescS.Usage = D3D11_USAGE_DEFAULT;
	depthDescS.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	GetDevice()->CreateTexture2D(&depthDescS, nullptr, &m_shadowDepthTex);
	GetDevice()->CreateDepthStencilView(m_shadowDepthTex, nullptr, &m_shadowDSV);

	m_shadowViewport.TopLeftX = 0.0f;
	m_shadowViewport.TopLeftY = 0.0f;
	m_shadowViewport.Width = 2048.0f;
	m_shadowViewport.Height = 2048.0f;
	m_shadowViewport.MinDepth = 0.0f;
	m_shadowViewport.MaxDepth = 1.0f;

	// 影用サンプラーステートの作成
	D3D11_SAMPLER_DESC sampDesc = {};
	sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	GetDevice()->CreateSamplerState(&sampDesc, &m_pSamplerState);

	ShadowVertex vertices[] = {
		{ {-1.0f, 0.01f,  1.0f}, {0.0f, 0.0f} },
		{ { 1.0f, 0.01f,  1.0f}, {1.0f, 0.0f} },
		{ {-1.0f, 0.01f, -1.0f}, {0.0f, 1.0f} },
		{ { 1.0f, 0.01f, -1.0f}, {1.0f, 1.0f} },
	};
	D3D11_BUFFER_DESC bd = {};
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(ShadowVertex) * 4;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	D3D11_SUBRESOURCE_DATA initData = {};
	initData.pSysMem = vertices;
	GetDevice()->CreateBuffer(&bd, &initData, &m_quadVB);


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
	PlayerAssetLoader::LoadCommonAnimations(player);


	// ==================================================
	// プレイヤー2の生成 
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
	PlayerAssetLoader::LoadCommonAnimations(player2);

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

	m_hitEffects.Init(10);

	// ----------------------------------------------------
	// 描画設定の作成（スカイドーム表示用）
	// ----------------------------------------------------
	D3D11_DEPTH_STENCIL_DESC depthDesc = {};
	depthDesc.DepthEnable = TRUE;
	depthDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	depthDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL; // 1.0(最奥)も描画する
	depthDesc.StencilEnable = FALSE;
	GetDevice()->CreateDepthStencilState(&depthDesc, &m_pDepthState);

	// 影（床ポリゴン）用に深度書き込みをオフにしたステートを作成
	depthDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	GetDevice()->CreateDepthStencilState(&depthDesc, &m_pDepthStateNoWrite);

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

	D3D11_BLEND_DESC mulBlendDesc = {};
	mulBlendDesc.RenderTarget[0].BlendEnable = TRUE;
	mulBlendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_ZERO;
	mulBlendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_SRC_COLOR;
	mulBlendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	mulBlendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ZERO;
	mulBlendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
	mulBlendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	mulBlendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
	GetDevice()->CreateBlendState(&mulBlendDesc, &m_pMultiplyBlend);

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

	// プレイヤー描画ヘルパーを用意
	m_playerRenderer.Setup(this);

	// スカイドーム用：カリングなし
	rsDesc.CullMode = D3D11_CULL_NONE;
	rsDesc.DepthClipEnable = FALSE;
	GetDevice()->CreateRasterizerState(&rsDesc, &m_pCullNone);

	// 初期位置設定
	ResetRound();

	Sound::PlayBGM("Assets/Sound/BGM/ゲームシーン.mp3", 0.4f);
}

void SceneGame::Uninit()
{
	SimpleFont::Uninit();

	if (m_uiManager)
	{
		delete m_uiManager;
		m_uiManager = nullptr;
	}

	if (m_skyDome) { delete m_skyDome; m_skyDome = nullptr; }
	if (g_uiTex) { delete g_uiTex; g_uiTex = nullptr; }

	// 描画設定の解放
	if (m_pDepthState) { m_pDepthState->Release(); m_pDepthState = nullptr; }
	if (m_pDepthStateNoWrite) { m_pDepthStateNoWrite->Release(); m_pDepthStateNoWrite = nullptr; }
	if (m_pBlendState) { m_pBlendState->Release(); m_pBlendState = nullptr; }
	if (m_pMultiplyBlend) { m_pMultiplyBlend->Release(); m_pMultiplyBlend = nullptr; }
	if (m_pCullNone) { m_pCullNone->Release(); m_pCullNone = nullptr; }

	if (m_shadowMapTex) { m_shadowMapTex->Release(); m_shadowMapTex = nullptr; }
	if (m_shadowRTV) { m_shadowRTV->Release(); m_shadowRTV = nullptr; }
	if (m_shadowSRV) { m_shadowSRV->Release(); m_shadowSRV = nullptr; }
	if (m_shadowDepthTex) { m_shadowDepthTex->Release(); m_shadowDepthTex = nullptr; }
	if (m_shadowDSV) { m_shadowDSV->Release(); m_shadowDSV = nullptr; }
	if (m_pSamplerState) { m_pSamplerState->Release(); m_pSamplerState = nullptr; }
	if (m_quadVB) { m_quadVB->Release(); m_quadVB = nullptr; }
}

/**
 * @brief ラウンド開始・リセット処理
 * 位置、HP、向き、状態を初期状態に戻す
 */
void SceneGame::ResetRound()
{
	m_isRoundOver = false;
	m_roundEndTimer = 0.0f;
	SceneRoot::s_sceneFade = 0.0f; // ラウンド切替の暗転を解除
	m_hitStopTimer = 0.0f;
	m_shakeTimerP1 = 0.0f;
	m_shakeTimerP2 = 0.0f;
	m_cameraTrauma = 0.0f;
	m_cameraShakeKickDir = 0.0f;
	m_cameraShakeOffset = { 0.0f, 0.0f, 0.0f };
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

/**
 * @brief 試合全体をやり直す（再戦）
 * 勝敗数も0に戻し、READYフェーズから仕切り直す
 */
void SceneGame::ResetMatch()
{
	m_isResultMenu = false;
	m_resultMenuIndex = 0;
	m_winCountP1 = 0;
	m_winCountP2 = 0;
	ResetRound(); // 勝敗数0なのでREADYフェーズから開始される
}


void SceneGame::Update(float tick)
{
	Player* player = GetObj<Player>("Player");
	Player* player2 = GetObj<Player>("Player2");

#ifdef _DEBUG
	// デバッグコマンド。F1:2PのHPを1に F2:1PのHPを1に F3:両者全回復
	if (IsKeyTrigger(VK_F1) && player2) player2->Debug_SetHp(1);
	if (IsKeyTrigger(VK_F2) && player) player->Debug_SetHp(1);
	if (IsKeyTrigger(VK_F3)) { if (player) player->RefillHp(); if (player2) player2->RefillHp(); }
#endif

	// ==========================================================
	// 決着後の選択メニュー
	// リザルトシーンへは行かず、KO時の画面のまま止めてメニュー操作だけ受け付ける
	// ==========================================================
	if (m_isResultMenu)
	{
		const int itemCount = 3;
		auto menuUp = [&]() {
			if (IsKeyTrigger(VK_UP)) return true;
			for (int i = 0; i < 4; ++i) if (IsPadTrigger(i, XINPUT_GAMEPAD_DPAD_UP)) return true;
			return false;
			};
		auto menuDown = [&]() {
			if (IsKeyTrigger(VK_DOWN)) return true;
			for (int i = 0; i < 4; ++i) if (IsPadTrigger(i, XINPUT_GAMEPAD_DPAD_DOWN)) return true;
			return false;
			};
		auto menuConfirm = [&]() {
			if (IsKeyTrigger(VK_RETURN)) return true;
			for (int i = 0; i < 4; ++i) if (IsPadTrigger(i, XINPUT_GAMEPAD_A)) return true;
			return false;
			};

		if (menuDown() || menuUp())
		{
			int dir = menuDown() ? 1 : -1;
			m_resultMenuIndex = (m_resultMenuIndex + dir + itemCount) % itemCount;
			Sound::PlaySE("Assets/Sound/SE/UI/MoveSlider.mp3");
		}

		if (menuConfirm())
		{
			Sound::PlaySE("Assets/Sound/SE/UI/決定ボタンを押す6.mp3");

			if (m_resultMenuIndex == 0)      ResetMatch();          // 再戦
			else if (m_resultMenuIndex == 1) s_requestConfig = true; // キーコンフィグへ
			else if (m_resultMenuIndex == 2) s_requestTitle = true;  // タイトルへ
		}

		return; // ゲーム進行は止めたまま
	}

	// ==========================================================
	// ラウンド開始演出 (フェーズ管理)
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

				// KO表示を消すためにフェーズをPLAYING（UI表示なし）に戻す
				m_currentPhase = RoundPhase::PLAYING;
			}
		}
		else
		{
			// 通常のラウンド終了後待機（フェードアウトなど）
			// スローが終わってからフェード処理を開始する

			bool isDeathAnimFinished = true;

			if (player && player->GetHpRatio() <= 0.0f) {
				int total = player->GetModel()->GetAnimationTotalFrame("Death");
				if (player->Debug_GetFrame() < total - 1) isDeathAnimFinished = false;
			}
			if (player2 && player2->GetHpRatio() <= 0.0f) {
				int total = player2->GetModel()->GetAnimationTotalFrame("Death");
				if (player2->Debug_GetFrame() < total - 1) isDeathAnimFinished = false;
			}

			// アニメーションが終わってから初めてタイマーを進める
			if (isDeathAnimFinished)
			{
				m_roundEndTimer += tick;
			}

			// フェード処理
			if (!isGameSet && m_uiManager)
			{
				if (m_roundEndTimer < WAIT_BEFORE_FADE)
				{
					SceneRoot::s_sceneFade = 0.0f;
				}
				else
				{
					float progress = (m_roundEndTimer - WAIT_BEFORE_FADE) / FADE_DURATION;
					if (progress > 1.0f) progress = 1.0f;
					// 画面全体（グリッド線含む）を暗転させるためSceneRoot側でかける
					SceneRoot::s_sceneFade = 0.1f + (progress * 0.9f);
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
				// リザルトシーンへは遷移せず、KO時の画面のまま左に選択メニューを出す
				m_isResultMenu = true;
				m_resultMenuIndex = 0;
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
			m_hitEffects.Raw(),
			STAGE_LIMIT_X
		);

		// ヒット・ガードの効果音
		if (result.shakeTimerP1 > 0.0f || result.shakeTimerP2 > 0.0f)
		{
			// ガード＞飛び道具＞通常打撃の順に鳴らし分ける
			const char* seFile = "Assets/Sound/SE/Game/attack.mp3";
			if (result.wasBlocked)         seFile = "Assets/Sound/SE/Game/Hit-Block02-1(High).mp3";
			else if (result.wasProjectile) seFile = "Assets/Sound/SE/Game/波動拳ヒット.mp3";
			Sound::PlaySE(seFile);
		}

		// 結果の適用
		if (result.isRoundOver)
		{
			m_isRoundOver = true;
			m_winCountP1 += result.winCountP1ToAdd;
			m_winCountP2 += result.winCountP2ToAdd;

			if (player) player->SetInputType(PlayerInputType::AI);
			if (player2) player2->SetInputType(PlayerInputType::AI);

			// KOフェーズへ移行（UI描画用）
			m_currentPhase = RoundPhase::KO_CALL;
			Sound::PlaySE("Assets/Sound/ko.wav");

			// スローモーション演出開始 
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

			// 攻撃の強さ（ヒットストップ長）からカメラの揺れ量を決める
			float strength = CameraShake::TraumaFromHitStop(result.hitStopTimer);
			// のけぞる側に応じて横方向の初撃を決める
			float kickDir = 0.0f;
			if (result.shakeTimerP2 > 0.0f)      kickDir =  1.0f; // 2Pがのけぞる
			else if (result.shakeTimerP1 > 0.0f) kickDir = -1.0f; // 1Pがのけぞる
			AddCameraTrauma(strength, kickDir);
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

	// カメラシェイクの更新（ヒットストップ中も止めず実時間で減衰させる）
	UpdateCameraShake(tick);

	// ヒットエフェクトの更新
	m_hitEffects.Update(tick);

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
// ヒット時にカメラの揺れ（trauma）を加える。amountは0.0から1.0、dirXは横方向の初撃（-1/0/+1）
void SceneGame::AddCameraTrauma(float amount, float dirX)
{
	CameraShake::AddTrauma(m_cameraTrauma, amount);
	m_cameraShakeKickDir = dirX;
}

// 毎フレームのカメラシェイク更新。traumaを減衰させ、今フレームの揺れオフセットを計算する
void SceneGame::UpdateCameraShake(float tick)
{
	// 揺れ計算は CameraShake に委譲（デバッグシーンと共有）
	CameraShake::Tick(m_cameraTrauma, m_cameraShakeKickDir, tick, m_cameraShakeOffset);
}
void SceneGame::Draw()
{
	Player* player = GetObj<Player>("Player");
	Player* player2 = GetObj<Player>("Player2");
	CameraBase* pCamera = GetObj<CameraBase>("Camera");
	LightBase* pLight = GetObj<LightBase>("Light");

	// ==========================================================
	// シャドウマップ生成パス
	// ==========================================================
	ID3D11RenderTargetView* oldRTV = nullptr;
	ID3D11DepthStencilView* oldDSV = nullptr;
	GetContext()->OMGetRenderTargets(1, &oldRTV, &oldDSV);
	UINT numViewports = 1;
	D3D11_VIEWPORT oldViewport[1];
	GetContext()->RSGetViewports(&numViewports, oldViewport);

	GetContext()->OMSetRenderTargets(1, &m_shadowRTV, m_shadowDSV);
	GetContext()->RSSetViewports(1, &m_shadowViewport);

	// 前フレームのUI描画等で深度テストがオフになっているのを防ぐ
	float clearBlend[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	GetContext()->OMSetBlendState(nullptr, clearBlend, 0xffffffff);
	if (m_pDepthState) {
		GetContext()->OMSetDepthStencilState(m_pDepthState, 0);
	}
	else {
		GetContext()->OMSetDepthStencilState(nullptr, 0);
	}
	if (m_pCullNone) {
		GetContext()->RSSetState(m_pCullNone);
	}

	float clearColorDepth[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
	GetContext()->ClearRenderTargetView(m_shadowRTV, clearColorDepth);
	GetContext()->ClearDepthStencilView(m_shadowDSV, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	DirectX::XMFLOAT4X4 LView, LProj;

	XMVECTOR vLightDir = XMVectorSet(0.0f, -1.0f, 0.2f, 0.0f);
	vLightDir = XMVector3Normalize(vLightDir);
	XMVECTOR vLightPos = XMVectorScale(vLightDir, -20.0f); // 描画範囲を確保するため遠ざける
	XMVECTOR vUp = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	XMMATRIX matLView = XMMatrixLookAtLH(vLightPos, XMVectorZero(), vUp);
	XMMATRIX matLProj = XMMatrixOrthographicLH(20.0f, 20.0f, 1.0f, 50.0f);

	XMStoreFloat4x4(&LView, XMMatrixTranspose(matLView));
	XMStoreFloat4x4(&LProj, XMMatrixTranspose(matLProj));

	Shader* vsSkin = GetObj<Shader>("VS_SkinMeshAnimation");
	Shader* psDepth = GetObj<Shader>("PS_WriteDepth");

	auto DrawShadowPass = [&](Player* p) {
		if (!p) return;
		XMFLOAT3 pos = p->GetPosition();
		XMFLOAT3 rot = p->GetRotation();
		XMFLOAT3 pScale = p->GetScale();
		Matrix pS = Matrix::CreateScale(pScale.x, pScale.y, pScale.z);
		Matrix pM = p->GetModel()->GetScaleBaseMatrix();
		Matrix pR = DirectX::XMMatrixRotationRollPitchYaw(rot.x, rot.y, rot.z);
		Matrix pT = Matrix::CreateTranslation(pos.x, pos.y, pos.z);
		Matrix world = pM * pS * pR * pT;

		DirectX::XMFLOAT4X4 matWVP[3];
		XMStoreFloat4x4(&matWVP[0], XMMatrixTranspose(world));
		matWVP[1] = LView;
		matWVP[2] = LProj;
		vsSkin->WriteBuffer(0, matWVP);
		p->SetVertexShader(vsSkin);
		p->SetPixelShader(psDepth);
		p->Draw();
		};

	DrawShadowPass(player);
	DrawShadowPass(player2);

	GetContext()->OMSetRenderTargets(1, &oldRTV, oldDSV);
	GetContext()->RSSetViewports(numViewports, oldViewport);
	if (oldRTV) oldRTV->Release();
	if (oldDSV) oldDSV->Release();

	// カメラシェイク適用：描画の間だけカメラをずらし、3D描画後に元へ戻す（UIは揺らさない）
	XMFLOAT3 camShakeBasePos = pCamera->GetPos();
	XMFLOAT3 camShakeBaseLook = pCamera->GetLook();
	{
		// posとlookを同量ずらす＝視線方向を保ったまま画面全体が揺れる
		XMFLOAT3 shakenPos = { camShakeBasePos.x + m_cameraShakeOffset.x, camShakeBasePos.y + m_cameraShakeOffset.y, camShakeBasePos.z };
		XMFLOAT3 shakenLook = { camShakeBaseLook.x + m_cameraShakeOffset.x, camShakeBaseLook.y + m_cameraShakeOffset.y, camShakeBaseLook.z };
		pCamera->SetPos(shakenPos);
		pCamera->SetLook(shakenLook);
	}


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

	// ==================================================
	// 床への影描画 (乗算ブレンド)
	// ==================================================
	if (m_pMultiplyBlend)
	{
		GetContext()->OMSetBlendState(m_pMultiplyBlend, blendFactor, 0xffffffff);
	}
	if (m_pCullNone) GetContext()->RSSetState(m_pCullNone);

	//  影受け用の板ポリゴンが、背後のグリッドを隠さないように深度の書き込みをオフにする
	if (m_pDepthStateNoWrite)
	{
		GetContext()->OMSetDepthStencilState(m_pDepthStateNoWrite, 0);
	}

	Shader* vsShadow = GetObj<Shader>("VS_SpriteShadow");
	Shader* psShadow = GetObj<Shader>("PS_Shadow");

	DirectX::XMFLOAT4X4 matWVPFloor[3];
	XMMATRIX matFloorWorld = XMMatrixScaling(20.0f, 1.0f, 20.0f);
	XMStoreFloat4x4(&matWVPFloor[0], XMMatrixTranspose(matFloorWorld));
	matWVPFloor[1] = mat[1];
	matWVPFloor[2] = mat[2];
	vsShadow->WriteBuffer(0, matWVPFloor);

	SpriteParam pram = {
		{0.0f, 0.0f, 1.0f, 1.0f},
		{0.0f, 0.0f, 1.0f, 1.0f},
		{1.0f, 1.0f, 1.0f, 1.0f}
	};
	vsShadow->WriteBuffer(1, &pram);

	DirectX::XMFLOAT4X4 lightMatFloor[3];
	lightMatFloor[0] = matWVPFloor[0];
	lightMatFloor[1] = LView;
	lightMatFloor[2] = LProj;
	vsShadow->WriteBuffer(2, lightMatFloor);

	vsShadow->Bind();
	psShadow->Bind();

	// サンプラーとシャドウマップをセット
	GetContext()->PSSetSamplers(0, 1, &m_pSamplerState);
	GetContext()->PSSetShaderResources(0, 1, &m_shadowSRV);

	UINT stride = sizeof(ShadowVertex);
	UINT offsetVB = 0;
	GetContext()->IASetVertexBuffers(0, 1, &m_quadVB, &stride, &offsetVB);
	GetContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	GetContext()->Draw(4, 0);

	ID3D11ShaderResourceView* nullSRV = nullptr;
	GetContext()->PSSetShaderResources(0, 1, &nullSRV);
	ID3D11SamplerState* nullSamp = nullptr;
	GetContext()->PSSetSamplers(0, 1, &nullSamp);

	GetContext()->OMSetBlendState(nullptr, blendFactor, 0xffffffff);

	// プレイヤー等の描画に影響が出ないよう、通常の深度設定（書き込みあり）に戻す
	GetContext()->OMSetDepthStencilState(nullptr, 0);

	// 先に全員のアウトライン、次に本体を描く
	m_playerRenderer.DrawOutline(this, player, m_shakeOffsetP1);
	m_playerRenderer.DrawOutline(this, player2, m_shakeOffsetP2);
	m_playerRenderer.DrawBody(this, player, m_shakeOffsetP1);
	m_playerRenderer.DrawBody(this, player2, m_shakeOffsetP2);

#ifdef _DEBUG
	if (player) { player->DrawBoundingBox(); player->DrawHitbox(); player->DrawActiveHurtboxes(); }
	if (player2) { player2->DrawBoundingBox(); player2->DrawHitbox(); player2->DrawActiveHurtboxes(); }
#endif

	// ------------------------------------------------
	//  飛び道具 & エフェクト描画 
	// ------------------------------------------------

	// 画像反転時のカリング落ちを防ぐためカリングなしに変更
	if (m_pCullNone) GetContext()->RSSetState(m_pCullNone);

	// ブレンド設定: AlphaToCoverage ON
	if (m_pBlendState)
	{
		GetContext()->OMSetBlendState(m_pBlendState, blendFactor, 0xffffffff);
	}

	// 飛び道具 (Projectile)
	if (player && player->GetProjectile())
	{
		player->GetProjectile()->Draw(pCamera->GetView(), pCamera->GetProj());
#ifdef _DEBUG
		player->GetProjectile()->DrawHitbox(pCamera->GetView(), pCamera->GetProj());
#endif
	}
	if (player2 && player2->GetProjectile())
	{
		player2->GetProjectile()->Draw(pCamera->GetView(), pCamera->GetProj());
#ifdef _DEBUG
		player2->GetProjectile()->DrawHitbox(pCamera->GetView(), pCamera->GetProj());
#endif
	}

	//カメラ行列の準備
	DirectX::XMFLOAT4X4 view = pCamera->GetView();
	DirectX::XMFLOAT4X4 proj = pCamera->GetProj();

	m_hitEffects.Draw(view, proj);

	// カメラシェイクを元に戻す（UI描画や次フレームの追従に揺れを持ち込まない）
	pCamera->SetPos(camShakeBasePos);
	pCamera->SetLook(camShakeBaseLook);


	// ------------------------------------------------
	// UIの描画
	// ------------------------------------------------
	if (m_uiManager)
	{
		m_uiManager->Draw(m_currentPhase, m_winCountP1, m_winCountP2, ROUND_TO_WIN);
	}

	// 決着後の選択メニュー（KO画面の上に重ねて描画）
	if (m_isResultMenu)
	{
		DrawResultMenu();
	}

	GetContext()->OMSetBlendState(nullptr, blendFactor, 0xffffffff);
	GetContext()->OMSetDepthStencilState(nullptr, 0);
	GetContext()->RSSetState(nullptr);
}

/**
 * @brief 決着後、画面左に出す選択メニューの描画（1280x720基準）
 */
void SceneGame::DrawResultMenu()
{
	// ピクセル座標(px,py,pw,ph)を左上基準でNDCの矩形として登録するヘルパー
	auto rectPx = [](float px, float py, float pw, float ph, DirectX::XMFLOAT4 col) {
		float cx = px + pw * 0.5f;
		float cy = py + ph * 0.5f;
		float ndcX = (cx / 1280.0f) * 2.0f - 1.0f;
		float ndcY = 1.0f - (cy / 720.0f) * 2.0f;
		float ndcW = (pw / 1280.0f) * 2.0f;
		float ndcH = (ph / 720.0f) * 2.0f;
		SimpleUI::AddRect(ndcX, ndcY, ndcW, ndcH, col, nullptr);
		};

	const wchar_t* items[3] = { L"再戦", L"キーコンフィグへ", L"タイトルへ" };

	SimpleUI::Clear();
	// 背景の半透明パネル
	rectPx(40.0f, 292.0f, 380.0f, 280.0f, { 0.05f, 0.05f, 0.08f, 0.62f });
	// 各項目の枠（選択中は赤で強調）
	for (int i = 0; i < 3; ++i)
	{
		DirectX::XMFLOAT4 col = (i == m_resultMenuIndex)
			? DirectX::XMFLOAT4(0.72f, 0.12f, 0.14f, 0.95f)
			: DirectX::XMFLOAT4(0.16f, 0.16f, 0.19f, 0.78f);
		rectPx(60.0f, (float)(336 + i * 64), 340.0f, 52.0f, col);
	}
	SimpleUI::DrawAll();

	// 文字
	SimpleFont::Begin();
	SimpleFont::Draw(L"RESULT", 60.0f, 300.0f, 30.0f, { 1.0f, 1.0f, 1.0f, 1.0f });
	for (int i = 0; i < 3; ++i)
	{
		DirectX::XMFLOAT4 tc = (i == m_resultMenuIndex)
			? DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f)
			: DirectX::XMFLOAT4(0.80f, 0.80f, 0.85f, 1.0f);
		SimpleFont::Draw(items[i], 84.0f, (float)(344 + i * 64), 26.0f, tc);
	}
	SimpleFont::End();
}