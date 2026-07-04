#include "SkyDome.h"
#include "DirectX.h"

using namespace DirectX;

SkyDome::SkyDome(){}
SkyDome::~SkyDome()
{
	if (m_pCullNone) { m_pCullNone->Release(); m_pCullNone = nullptr; }
	if (m_pDepthLessEqual) { m_pDepthLessEqual->Release(); m_pDepthLessEqual = nullptr; }
}

// モデル・シェーダー・ステートをまとめて用意する
bool SkyDome::Setup(SceneBase* scene)
{
	if (!scene) return false;

	// シェーダーが無ければ生成
	Shader* vsObj = scene->GetObj<Shader>("VS_Object");
	if (!vsObj)
	{
		vsObj = scene->CreateObj<VertexShader>("VS_Object");
		vsObj->Load("Assets/Shader/VS_Object.cso");
	}
	Shader* psTex = scene->GetObj<Shader>("PS_TexColor");
	if (!psTex)
	{
		psTex = scene->CreateObj<PixelShader>("PS_TexColor");
		psTex->Load("Assets/Shader/PS_TexColor.cso");
	}
	m_pVS = vsObj;

	// 背景モデルの読み込み
	Model* skyModel = scene->GetObj<Model>("SkyModel");
	if (!skyModel)
	{
		skyModel = scene->CreateObj<Model>("SkyModel");
		skyModel->Load("Assets/Model/SkyDome/SkyDome.fbx", 1.0f, true, true);
		skyModel->SetTexture("Assets/Model/SkyDome/SkyDome.png");
	}
	skyModel->SetPixelShader((PixelShader*)psTex);
	m_pModel = skyModel;

	// カリングなしと深度 LESS_EQUAL を生成
	D3D11_RASTERIZER_DESC rsDesc = {};
	rsDesc.FillMode = D3D11_FILL_SOLID;
	rsDesc.CullMode = D3D11_CULL_NONE;
	rsDesc.FrontCounterClockwise = FALSE;
	rsDesc.DepthClipEnable = FALSE;
	GetDevice()->CreateRasterizerState(&rsDesc, &m_pCullNone);

	D3D11_DEPTH_STENCIL_DESC dsDesc = {};
	dsDesc.DepthEnable = TRUE;
	dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	dsDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
	dsDesc.StencilEnable = FALSE;
	GetDevice()->CreateDepthStencilState(&dsDesc, &m_pDepthLessEqual);

	return true;
}

//=========================================
// 初期化
//=========================================
void SkyDome::Init(Model* pModel)
{
	m_pModel = pModel;
}

//=========================================
// 更新
//=========================================
void SkyDome::Update(const DirectX::XMFLOAT3& camPos)
{
	// スカイドームの中心を常にカメラの位置を合わせる
	//m_pos = camPos;

	// カメラ位置 + 手動オフセット
	m_pos.x = camPos.x + m_offset.x;
	m_pos.y = camPos.y + m_offset.y;
	m_pos.z = camPos.z + m_offset.z;
}

//=========================================
// 描画
//=========================================
void SkyDome::Draw(const DirectX::XMFLOAT4X4& viewMatrix, const DirectX::XMFLOAT4X4& projMatrix, Shader* pShader)
{
	if (!m_pModel || !pShader)return;

	// ワールド行列の計算
	DirectX::XMMATRIX matScale = DirectX::XMMatrixScaling(m_scale.x, m_scale.y, m_scale.z);
	DirectX::XMMATRIX matTrans = DirectX::XMMatrixTranslation(m_pos.x, m_pos.y, m_pos.z);
	
	DirectX::XMMATRIX worldMat = matScale * matTrans;

	// 定数バッファの準備
	DirectX::XMFLOAT4X4 mat[3];
	DirectX::XMStoreFloat4x4(&mat[0], DirectX::XMMatrixTranspose(worldMat)); // ワールド
	mat[1] = viewMatrix; // ビュー
	mat[2] = projMatrix; // プロジェクション

	// シェーダーに書き込み、セット
	pShader->WriteBuffer(0, mat);

	// 頂点シェーダーをセット
	m_pModel->SetVertexShader(pShader);
	//m_pModel->SetPixelShader(pShader);

	// 描画
	m_pModel->Draw(0);
}

// 最奥に描くため深度とカリングを切り替えて描画する
void SkyDome::DrawWithState(const DirectX::XMFLOAT4X4& viewMatrix, const DirectX::XMFLOAT4X4& projMatrix)
{
	if (!m_pModel || !m_pVS) return;

	// 深度を LESS_EQUAL、カリングなしに設定
	if (m_pDepthLessEqual) GetContext()->OMSetDepthStencilState(m_pDepthLessEqual, 0);
	if (m_pCullNone) GetContext()->RSSetState(m_pCullNone);

	Draw(viewMatrix, projMatrix, m_pVS);

	// 既定の裏面カリングへ戻す
	GetContext()->RSSetState(nullptr);
}
