#include "SkyDome.h"

using namespace DirectX;

SkyDome::SkyDome(){}
SkyDome::~SkyDome(){}

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