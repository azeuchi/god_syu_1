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

	//ジャンプ中に割り込みできるか
	bool IsInterruptible() const override { return true; }
};