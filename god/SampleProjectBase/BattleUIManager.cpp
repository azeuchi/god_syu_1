#include "BattleUIManager.h"
#include "SimpleUI.h"
#include "UISprite.h"
#include "Texture.h"
#include "Sprite.h" 

BattleUIManager::BattleUIManager()
	: m_hpBar(nullptr), m_enemyhpBar(nullptr), m_fadeBlack(nullptr)
	, m_hpFrame(nullptr), m_enemyhpFrame(nullptr)
	, m_imgRound1(nullptr), m_imgRound2(nullptr), m_imgFinalRound(nullptr), m_imgFight(nullptr)
	, m_pDepthStateUI(nullptr), m_pBlendState(nullptr)
	, m_barMaxWidth(400.0f)
	,m_barHeight(40.0f)
	, m_currentFadeAlpha(0.0f)
{
	m_hpBarPos = { 330.0f, 80.0f };
	m_enemyHpBarPos = { 950.0f, 80.0f };
}

BattleUIManager::~BattleUIManager()
{
	Uninit();
}

void BattleUIManager::Init()
{
	// ==================================================
	// UI（HPバー・ラウンド画像）の初期化
	// ==================================================
	m_barMaxWidth = 500.0f;
	m_currentFadeAlpha = 0.0f;

	m_hpBar = new Image2D();
	m_hpBar->Load("Assets/Texture/hp.png", m_hpBarPos.x, m_hpBarPos.y, m_barMaxWidth, m_barHeight);

	m_enemyhpBar = new Image2D();
	m_enemyhpBar->Load("Assets/Texture/hp.png", m_enemyHpBarPos.x, m_enemyHpBarPos.y, m_barMaxWidth, m_barHeight);

	m_hpFrame = new Image2D();
	m_hpFrame->Load("Assets/Texture/hpframe.png", 330.0f, 80.0f, 550.0f, 80.0f);

	m_enemyhpFrame = new Image2D();
	m_enemyhpFrame->Load("Assets/Texture/hpframe.png", 950.0f, 80.0f, 550.0f, 80.0f);

	// フェード用画像
	m_fadeBlack = new Image2D();
	m_fadeBlack->Load("Assets/Texture/black.png", 640.0f, 360.0f, 1280.0f, 720.0f);
	m_fadeBlack->SetColor(0.0f, 0.0f, 0.0f, 0.0f);

	// ラウンドコール画像
	m_imgRound1 = new Image2D();
	m_imgRound1->Load("Assets/Texture/ROUND1.png", 640.0f, 360.0f, 800.0f, 350.0f);

	m_imgRound2 = new Image2D();
	m_imgRound2->Load("Assets/Texture/ROUND2.png", 640.0f, 360.0f, 800.0f, 350.0f);

	m_imgFinalRound = new Image2D();
	m_imgFinalRound->Load("Assets/Texture/FINALROUND.png", 640.0f, 360.0f, 800.0f, 350.0f);

	m_imgFight = new Image2D();
	m_imgFight->Load("Assets/Texture/FIGHT.png", 640.0f, 360.0f, 800.0f, 350.0f);

	// UI用の深度ステート
	D3D11_DEPTH_STENCIL_DESC depthDescUI = {};
	depthDescUI.DepthEnable = TRUE;
	depthDescUI.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	depthDescUI.DepthFunc = D3D11_COMPARISON_ALWAYS; // 常に手前に描画
	depthDescUI.StencilEnable = FALSE;
	GetDevice()->CreateDepthStencilState(&depthDescUI, &m_pDepthStateUI);

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
}

void BattleUIManager::Uninit()
{
	if (m_hpBar) { delete m_hpBar; m_hpBar = nullptr; }
	if (m_enemyhpBar) { delete m_enemyhpBar; m_enemyhpBar = nullptr; }
	if (m_fadeBlack) { delete m_fadeBlack; m_fadeBlack = nullptr; }

	if (m_hpFrame) { delete m_hpFrame; m_hpFrame = nullptr; }
	if (m_enemyhpFrame) { delete m_enemyhpFrame; m_enemyhpFrame = nullptr; }

	if (m_imgRound1) { delete m_imgRound1; m_imgRound1 = nullptr; }
	if (m_imgRound2) { delete m_imgRound2; m_imgRound2 = nullptr; }
	if (m_imgFinalRound) { delete m_imgFinalRound; m_imgFinalRound = nullptr; }
	if (m_imgFight) { delete m_imgFight; m_imgFight = nullptr; }

	if (m_pDepthStateUI) { m_pDepthStateUI->Release(); m_pDepthStateUI = nullptr; }
	if (m_pBlendState) { m_pBlendState->Release(); m_pBlendState = nullptr; }
}

void BattleUIManager::Reset()
{
	// HPバー初期化
	if (m_hpBar) m_hpBar->SetSize(m_barMaxWidth, m_barHeight);
	if (m_enemyhpBar) m_enemyhpBar->SetSize(m_barMaxWidth, m_barHeight);
	if (m_hpBar) m_hpBar->SetPosition(m_hpBarPos.x, m_hpBarPos.y);
	if (m_enemyhpBar) m_enemyhpBar->SetPosition(m_enemyHpBarPos.x, m_enemyHpBarPos.y);

	m_currentFadeAlpha = 0.0f;
	if (m_fadeBlack)
	{
		m_fadeBlack->SetColor(0.0f, 0.0f, 0.0f, 0.0f);
	}
}

void BattleUIManager::UpdateHPBars(float p1Ratio, float p2Ratio)
{
	// P1
	float currentWidth1 = m_barMaxWidth * p1Ratio;
	float reduceWidth1 = m_barMaxWidth - currentWidth1;
	m_hpBar->SetSize(currentWidth1, m_barHeight);
	m_hpBar->SetPosition(m_hpBarPos.x + (reduceWidth1 / 2.0f), m_hpBarPos.y);

	// P2
	float currentWidth2 = m_barMaxWidth * p2Ratio;
	float reduceWidth2 = m_barMaxWidth - currentWidth2;
	m_enemyhpBar->SetSize(currentWidth2, m_barHeight);
	m_enemyhpBar->SetPosition(m_enemyHpBarPos.x - (reduceWidth2 / 2.0f), m_enemyHpBarPos.y);
}

void BattleUIManager::SetFadeAlpha(float alpha)
{
	m_currentFadeAlpha = alpha;
	if (m_fadeBlack)
	{
		m_fadeBlack->SetColor(0.0f, 0.0f, 0.0f, alpha);
	}
}

void BattleUIManager::Draw(RoundPhase currentPhase, int winCountP1, int winCountP2)
{
	// 深度設定: UI用 (Depth ON, Func ALWAYS)
	if (m_pDepthStateUI)
	{
		GetContext()->OMSetDepthStencilState(m_pDepthStateUI, 0);
	}

	Sprite::SetUVScale({ 1.0f, 1.0f });
	Sprite::SetUVPos({ 0.0f, 0.0f });

	// UIリストクリアと行列初期化
	SimpleUI::Clear();
	DirectX::XMFLOAT4X4 identity;
	DirectX::XMStoreFloat4x4(&identity, DirectX::XMMatrixIdentity());
	SimpleUI::SetMatrix(identity, identity, identity);

	// --- HPバー描画登録 ---
	if (m_hpBar) m_hpBar->Draw();
	if (m_enemyhpBar) m_enemyhpBar->Draw();

	//--- HPフレーム描画登録 ---
	if (m_hpFrame) m_hpFrame->Draw();
	if (m_enemyhpFrame) m_enemyhpFrame->Draw();

	// --- ラウンド演出の描画登録 ---
	if (currentPhase == RoundPhase::ROUND_CALL)
	{
		// 勝利数に応じて表示画像を切り替え
		if (winCountP1 == 0 && winCountP2 == 0)
		{
			if (m_imgRound1) m_imgRound1->Draw();
		}
		else if (winCountP1 == 1 && winCountP2 == 1)
		{
			// 1-1 ならファイナルラウンド
			if (m_imgFinalRound) m_imgFinalRound->Draw();
		}
		else
		{
			// それ以外 (1-0 or 0-1) ならラウンド2
			if (m_imgRound2) m_imgRound2->Draw();
		}
	}
	else if (currentPhase == RoundPhase::FIGHT_CALL)
	{
		if (m_imgFight) m_imgFight->Draw();
	}

	// --- フェード画像の描画登録 ---
	if (m_fadeBlack && m_currentFadeAlpha > 0.0f)
	{
		m_fadeBlack->Draw();
	}

	// まとめて描画実行
	SimpleUI::DrawAll();

	// 設定をデフォルトに戻す
	Sprite::SetUVScale({ 1.0f, 1.0f }); // 全体表示に戻す
	Sprite::SetUVPos({ 0.0f, 0.0f });   // 原点に戻す
}