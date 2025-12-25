#include "SceneResult.h"
#include "DebugLog.h"
#include "SimpleUI.h"

void SceneResult::Init()
{
	DebugLog::log(DebugLog::INFO_LOG, "--- Title Scene Init ---");
	// Image2Dのインスタンス生成
	m_pImage = new Image2D();
	m_returntitle = new Image2D();
	// 画像読み込み
	m_pImage->Load("Assets/Texture/background2.png", 640,360.0f, 1280, 720.0f);
	m_returntitle->Load("Assets/Texture/PressEnter.png", 640, 550, 500, 200.0f);



}

void SceneResult::Uninit()
{
	DebugLog::log(DebugLog::INFO_LOG, "--- Title Scene Uninit ---");

	// 使い終わったらメモリ解放
	if (m_pImage)
	{
		delete m_pImage;
		delete m_returntitle;
		m_pImage = nullptr;
		m_returntitle = nullptr;
	}
}

void SceneResult::Update(float tick)
{

}

void SceneResult::Draw()
{
	SimpleUI::Clear();
	DirectX::XMFLOAT4X4 identity;
	DirectX::XMStoreFloat4x4(&identity, DirectX::XMMatrixIdentity());
	SimpleUI::SetMatrix(identity, identity, identity);
	if (m_returntitle)m_returntitle->Draw();
	if (m_pImage) m_pImage->Draw();
	
	SimpleUI::DrawAll();
}