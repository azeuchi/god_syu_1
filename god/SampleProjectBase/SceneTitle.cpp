#include "SceneTitle.h"
#include "DebugLog.h"
#include "SimpleUI.h"
#include <math.h>

void SceneTitle::Init()
{
	DebugLog::log(DebugLog::INFO_LOG, "--- Title Scene Init ---");
	// Image2Dのインスタンス生成
	m_background = new Image2D();
	m_pImage = new Image2D();
	m_PressEnter = new Image2D();
	// 画像読み込み
	m_pImage->Load("Assets/Texture/AZEFIGHTER.png", 650.0f, 150.0f, 676, 369.0f);
	m_background->Load("Assets/Texture/background4.png", 640, 360.0f, 1280, 720.0f);
	m_PressEnter->Load("Assets/Texture/PressEnter.png", 640, 550, 500, 200.0f);

	m_blinkTimer = 0.0f;
}

void SceneTitle::Uninit()
{
	DebugLog::log(DebugLog::INFO_LOG, "--- Title Scene Uninit ---");

	// 使い終わったらメモリ解放
	if (m_pImage)
	{
		delete m_pImage;
		delete m_background;
		delete m_PressEnter;
		m_background = nullptr;
		m_pImage = nullptr;
		m_PressEnter = nullptr;
	}
}

void SceneTitle::Update(float tick)
{
	m_blinkTimer += tick;

	// 基本のサイン波 (0.0 〜 1.0)
	float wave = (sinf(m_blinkTimer * 3.0f) + 1.0f) * 0.5f;

	// 最小値を 0.3 (30%) に設定して、0.3 〜 1.0 の範囲で点滅させる
	// 計算式: 最小値 + (最大値 - 最小値) * wave
	float minAlpha = 0.3f;
	float alpha = minAlpha + (1.0f - minAlpha) * wave;

	if (m_PressEnter)
	{
		m_PressEnter->SetColor(1.0f, 1.0f, 1.0f, alpha);
	}
}

void SceneTitle::Draw()
{
	SimpleUI::Clear();
	DirectX::XMFLOAT4X4 identity;
	DirectX::XMStoreFloat4x4(&identity, DirectX::XMMatrixIdentity());
	SimpleUI::SetMatrix(identity, identity, identity);

	if (m_background) m_background->Draw();
	if (m_pImage) m_pImage->Draw();
	if (m_PressEnter) m_PressEnter->Draw();

	SimpleUI::DrawAll();
}