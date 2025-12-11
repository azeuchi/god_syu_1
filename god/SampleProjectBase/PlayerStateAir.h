#pragma once
#include "PlayerState.h"

/**
 * @brief 空中状態の親クラス (Hierarchical State)
 * ジャンプ中などの「空中にいる状態」の共通処理を記述する
 */
class PlayerStateAir : public PlayerState
{
public:
	void Update(Player* player, float tick) override;

	// 子クラスが実装すべき固有の挙動
	virtual void UpdateBehavior(Player* player, float tick) = 0;
};