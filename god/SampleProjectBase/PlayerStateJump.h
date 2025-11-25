#pragma once
#include "PlayerState.h"

/**
 * @brief ジャンプ (Jump) 状態
 */
class PlayerStateJump : public PlayerState
{
public:
	void OnEnter(Player* player) override;
	void Update(Player* player, float tick) override;

	bool IsInterruptible() const override { return true; }

private:
	float m_elapsedTime = 0.0f;
	const float JUMP_ANIM_DURATION = 0.5f; // ジャンプアニメの長さ (秒)
	
};