#pragma once
#include <d3d11.h>
#include <DirectXMath.h>

class SceneBase;
class Player;

// プレイヤーをゲームと同じ見た目（アウトライン付き）で描画するヘルパー
// 必要なシェーダーと反転対応のカリングステートを内部で保持する
class PlayerRenderer
{
public:
	~PlayerRenderer();

	// 必要なシェーダーとカリングステートを用意する
	bool Setup(SceneBase* scene);

	// アウトライン（裏面を膨らませた輪郭）を描く
	void DrawOutline(SceneBase* scene, Player* player, const DirectX::XMFLOAT3& offset = { 0.0f, 0.0f, 0.0f });
	// 本体（通常）を描く
	void DrawBody(SceneBase* scene, Player* player, const DirectX::XMFLOAT3& offset = { 0.0f, 0.0f, 0.0f });

private:
	ID3D11RasterizerState* m_pCullFront = nullptr; // 表面カリング
	ID3D11RasterizerState* m_pCullBack = nullptr;  // 裏面カリング
};