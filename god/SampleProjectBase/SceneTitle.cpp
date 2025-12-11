#include "SceneTitle.h"
#include "DebugLog.h"
#include "SimpleUI.h"

void SceneTitle::Init()
{
	DebugLog::log(DebugLog::INFO_LOG, "--- Title Scene Init ---");
	// Image2Dのインスタンス生成
	m_pImage = new Image2D();
	// 画像読み込み
	m_pImage->Load("Assets/Texture/AZEFIGHTER.png", 650.0f, 150.0f, 676, 369.0f);
}

void SceneTitle::Uninit()
{
	DebugLog::log(DebugLog::INFO_LOG, "--- Title Scene Uninit ---");

	// 使い終わったらメモリ解放
	if (m_pImage)
	{
		delete m_pImage;
		m_pImage = nullptr;
	}
}

void SceneTitle::Update(float tick)
{

}

void SceneTitle::Draw()
{
	SimpleUI::Clear();
	DirectX::XMFLOAT4X4 identity;
	DirectX::XMStoreFloat4x4(&identity, DirectX::XMMatrixIdentity());
	SimpleUI::SetMatrix(identity, identity, identity);

	if (m_pImage) m_pImage->Draw();

	SimpleUI::DrawAll();
}