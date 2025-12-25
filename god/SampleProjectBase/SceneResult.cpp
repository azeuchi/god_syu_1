#include "SceneResult.h"
#include "DebugLog.h"
#include "SimpleUI.h"
#include "Shader.h"

void SceneResult::Init()
{
	DebugLog::log(DebugLog::INFO_LOG, "--- Result Scene Init ---");

	// シェーダー読み込み 
	Shader* vsObj = CreateObj<VertexShader>("VS_Object");
	Shader* psColor = CreateObj<PixelShader>("PS_TexColor");
	vsObj->Load("Assets/Shader/VS_Object.cso");
	psColor->Load("Assets/Shader/PS_TexColor.cso");

	// Image2Dのインスタンス生成
	m_pImage = new Image2D();
	m_returntitle = new Image2D();
	// 画像読み込み
	m_pImage->Load("Assets/Texture/background2.png", 640, 360.0f, 1280, 720.0f);
	m_returntitle->Load("Assets/Texture/PressEnter.png", 640, 550, 500, 200.0f);

	// 1. 透過設定の作成 (BlendState)
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

	// 2. 奥行き無効設定の作成 (DepthStencilState)
	
	D3D11_DEPTH_STENCIL_DESC depthDescUI = {};
	depthDescUI.DepthEnable = FALSE; //  奥行き判定をOFFにする
	depthDescUI.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	depthDescUI.DepthFunc = D3D11_COMPARISON_ALWAYS; // 常に手前に描く
	depthDescUI.StencilEnable = FALSE;
	GetDevice()->CreateDepthStencilState(&depthDescUI, &m_pDepthStateUI);
}

void SceneResult::Uninit()
{
	DebugLog::log(DebugLog::INFO_LOG, "--- Result Scene Uninit ---");

	// 使い終わったらメモリ解放
	if (m_pImage)
	{
		delete m_pImage;
		m_pImage = nullptr;
	}
	if (m_returntitle)
	{
		delete m_returntitle;
		m_returntitle = nullptr;
	}

	if (m_pBlendState) { m_pBlendState->Release(); m_pBlendState = nullptr; }
	if (m_pDepthStateUI) { m_pDepthStateUI->Release(); m_pDepthStateUI = nullptr; }
}

void SceneResult::Update(float tick)
{

}

void SceneResult::Draw()
{
	// シェーダー適用
	Shader* vs = GetObj<Shader>("VS_Object");
	Shader* ps = GetObj<Shader>("PS_TexColor");
	if (vs) vs->Bind();
	if (ps) ps->Bind();

	// 1. 透過を有効にする
	float blendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	if (m_pBlendState)
	{
		GetContext()->OMSetBlendState(m_pBlendState, blendFactor, 0xffffffff);
	}

	// 2. 奥行き判定を無効にする (2D重ね合わせ用)
	if (m_pDepthStateUI)
	{
		GetContext()->OMSetDepthStencilState(m_pDepthStateUI, 0);
	}

	SimpleUI::Clear();
	DirectX::XMFLOAT4X4 identity;
	DirectX::XMStoreFloat4x4(&identity, DirectX::XMMatrixIdentity());
	SimpleUI::SetMatrix(identity, identity, identity);

	if (m_pImage) m_pImage->Draw();
	if (m_returntitle) m_returntitle->Draw();

	SimpleUI::DrawAll();

	// 設定をリセットして帰る
	GetContext()->OMSetBlendState(nullptr, blendFactor, 0xffffffff);
	GetContext()->OMSetDepthStencilState(nullptr, 0);
}