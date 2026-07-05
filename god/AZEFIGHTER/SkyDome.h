#ifndef __SKYDOME_H__
#define __SKYDOME_H__

#include "Model.h"
#include "SceneBase.hpp"
#include "Shader.h"
#include <DirectXMath.h>
#include <d3d11.h>

class SkyDome
{
public:
	SkyDome();
	~SkyDome();

	// モデル・シェーダー・ステートをまとめて用意する
	bool Setup(SceneBase* scene);

	void Init(Model* pModel);
	void Update(const DirectX::XMFLOAT3& camPos);
	void Draw(const DirectX::XMFLOAT4X4& viewMatrix, const DirectX::XMFLOAT4X4& projMatrix, Shader* pShader);

	// 最奥描画用のステートを設定して描画する
	void DrawWithState(const DirectX::XMFLOAT4X4& viewMatrix, const DirectX::XMFLOAT4X4& projMatrix);

	DirectX::XMFLOAT3* GetScalePtr() { return &m_scale; }
	DirectX::XMFLOAT3* GetPosPtr() { return &m_pos; }
	DirectX::XMFLOAT3* GetOffsetPtr() { return &m_offset; }

private:
	Model* m_pModel = nullptr;
	Shader* m_pVS = nullptr; // VS_Object
	// 最奥描画用ステート
	ID3D11RasterizerState* m_pCullNone = nullptr;
	ID3D11DepthStencilState* m_pDepthLessEqual = nullptr;
	DirectX::XMFLOAT3 m_pos = { 0.0f,0.0f,0.0f }; // スカイドームの位置
	DirectX::XMFLOAT3 m_scale = { 80.0f,80.0f,80.0f }; // 巨大にする
	DirectX::XMFLOAT3 m_offset = { 0.0f, -18000.0f, 0.0f }; // ズレ補正用
};


#endif // !__SKYDOME_H__


