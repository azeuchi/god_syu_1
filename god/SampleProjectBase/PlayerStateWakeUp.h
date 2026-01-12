#pragma once
#include "PlayerState.h"

/**
 * @brief 起き上がり (WakeUp) 状態
 * ダウンから復帰するモーション。無敵時間がある。
 */
class PlayerStateWakeUp : public PlayerState
{
public:
	void OnEnter(Player* player) override;
	void Update(Player* player, float tick) override;

	// 起き上がり動作中はキャンセル不能
	bool IsInterruptible() const override { return false; }

	// 起き上がり中も無敵
	bool IsInvincible() const override { return true; }
};