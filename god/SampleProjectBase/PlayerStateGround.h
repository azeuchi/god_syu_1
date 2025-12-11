#pragma once
#include "PlayerState.h"

/**
 * @brief 地上状態の親クラス (Hierarchical State)
 * 立ち、歩き、後退などは
 * すべてこのクラスを継承する。
 */
class PlayerStateGround : public PlayerState
{
public:
	// 親クラスとして共通の更新処理を行う
	void Update(Player* player, float tick) override;

	// 子クラスが実装すべきその状態固有の挙動
	virtual void UpdateBehavior(Player* player, float tick) = 0;
};