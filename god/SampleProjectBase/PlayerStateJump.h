#pragma once
#include "PlayerStateAir.h"

/**
 * @brief ジャンプ (Jump) 状態
 */
class PlayerStateJump : public PlayerStateAir
{
public:
	void OnEnter(Player* player) override;
	void UpdateBehavior(Player* player, float tick) override;

	bool IsInterruptible() const override { return true; }

private:
	float m_elapsedTime = 0.0f;
	const float JUMP_ANIM_DURATION = 0.5f; // ジャンプアニメの長さ (秒)

};