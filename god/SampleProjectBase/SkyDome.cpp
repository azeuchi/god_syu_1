#include "SkyDome.h"
#include "DirectX.h"

using namespace DirectX;

SkyDome::SkyDome(){}
SkyDome::~SkyDome()
{
	if (m_pCullNone) { m_pCullNone->Release(); m_pCullNone = nullptr; }
	if (m_pDepthLessEqual) { m_pDepthLessEqual->Release(); m_pDepthLessEqual = nullptr; }
}

// 背景一式（モデル・シェーダー・ステート）をまとめて用意する
bool SkyDome::Setup(SceneBase* scene)
{
	if (!scene) return false;

	// 必要なシェーダーを用意（無ければ生成）
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

	// 最奥描画用：カリングなし＋深度 LESS_EQUAL のステートを生成
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
// ・ｽ・ｽ・ｽ・ｽ・ｽ・ｽ
//=========================================
void SkyDome::Init(Model* pModel)
{
	m_pModel = pModel;
}

//=========================================
// ・ｽX・ｽV
//=========================================
void SkyDome::Update(const DirectX::XMFLOAT3& camPos)
{
	// ・ｽX・ｽJ・ｽC・ｽh・ｽ[・ｽ・ｽ・ｽﾌ抵ｿｽ・ｽS・ｽ・ｽ・ｽ・ｽﾉカ・ｽ・ｽ・ｽ・ｽ・ｽﾌ位置・ｽ・ｽ・ｽ・ｽ・ｽ墲ｹ・ｽ・ｽ
	//m_pos = camPos;

	// ・ｽJ・ｽ・ｽ・ｽ・ｽ・ｽﾊ置 + ・ｽ闢ｮ・ｽI・ｽt・ｽZ・ｽb・ｽg
	m_pos.x = camPos.x + m_offset.x;
	m_pos.y = camPos.y + m_offset.y;
	m_pos.z = camPos.z + m_offset.z;
}

//=========================================
// ・ｽ`・ｽ・ｽ
//=========================================
void SkyDome::Draw(const DirectX::XMFLOAT4X4& viewMatrix, const DirectX::XMFLOAT4X4& projMatrix, Shader* pShader)
{
	if (!m_pModel || !pShader)return;

	// ・ｽ・ｽ・ｽ[・ｽ・ｽ・ｽh・ｽs・ｽ・ｽﾌ計・ｽZ
	DirectX::XMMATRIX matScale = DirectX::XMMatrixScaling(m_scale.x, m_scale.y, m_scale.z);
	DirectX::XMMATRIX matTrans = DirectX::XMMatrixTranslation(m_pos.x, m_pos.y, m_pos.z);
	
	DirectX::XMMATRIX worldMat = matScale * matTrans;

	// ・ｽ關費ｿｽo・ｽb・ｽt・ｽ@・ｽﾌ擾ｿｽ・ｽ・ｽ
	DirectX::XMFLOAT4X4 mat[3];
	DirectX::XMStoreFloat4x4(&mat[0], DirectX::XMMatrixTranspose(worldMat)); // ・ｽ・ｽ・ｽ[・ｽ・ｽ・ｽh
	mat[1] = viewMatrix; // ・ｽr・ｽ・ｽ・ｽ[
	mat[2] = projMatrix; // ・ｽv・ｽ・ｽ・ｽW・ｽF・ｽN・ｽV・ｽ・ｽ・ｽ・ｽ

	// ・ｽV・ｽF・ｽ[・ｽ_・ｽ[・ｽﾉ擾ｿｽ・ｽ・ｽ・ｽ・ｽ・ｽﾝ、・ｽZ・ｽb・ｽg
	pShader->WriteBuffer(0, mat);

	// ・ｽ・ｽ・ｽ_・ｽV・ｽF・ｽ[・ｽ_・ｽ[・ｽ・ｽ・ｽZ・ｽb・ｽg
	m_pModel->SetVertexShader(pShader);
	//m_pModel->SetPixelShader(pShader);

	// ・ｽ`・ｽ・ｽ
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

	// 後続のモデル描画のため既定（裏面カリング）へ戻す
	GetContext()->RSSetState(nullptr);
}