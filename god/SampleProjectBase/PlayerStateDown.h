#pragma once
#include "PlayerState.h"

/**
 * @brief ダウン (Down) 状態
 * 吹き飛んで地面に落ち、しばらく倒れたままになるステート
 */
class PlayerStateDown : public PlayerState
{
public:
	void OnEnter(Player* player) override;
	void Update(Player* player, float tick) override;

	// ダウン中は操作不能
	bool IsInterruptible() const override { return false; }

	// ダウン中は無敵
	bool IsInvincible() const override { return true; }

private:
	bool m_hasLanded = false;     // 着地したか？
	float m_groundTimer = 0.0f;   // 接地後の経過時間
	const float DOWN_TIME = 1.0f; // 接地してから起き上がるまでの時間
};