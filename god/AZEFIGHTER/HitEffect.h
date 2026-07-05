#pragma once
#include <DirectXMath.h>
#include "Sprite.h"

class Player;

class HitEffect
{
public:
	HitEffect();
	~HitEffect();

	// エフェクトを発生させる
	void Activate(Player* target);

	// 更新（アニメーション進行）
	void Update(float tick);

	// 描画
	void Draw(const DirectX::XMFLOAT4X4& view, const DirectX::XMFLOAT4X4& projection);

	bool IsActive() const { return m_isActive; }

private:
	bool m_isActive;
	DirectX::XMFLOAT3 m_position;
	float m_timer;
	int m_frameIndex;

	//追従するプレイヤーを記憶するポインタ
	Player* m_targetPlayer = nullptr;

	// 設定定数
	const float FRAME_TIME = 0.05f; // 1コマの表示時間(秒)
	const int MAX_FRAMES = 5;       // hit.pngは5コマ
	const float SPRITE_SIZE = 1.0f; // エフェクトの大きさ
};
