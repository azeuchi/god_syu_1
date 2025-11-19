#pragma once
#include "PlayerState.h"

/**
 * @brief ライトパンチ (LightPunch) 状態
 */
class LightPunch : public PlayerState
{
public:
	void OnEnter(Player* player) override;
	void Update(Player* player, float tick) override;

	// 攻撃中は他の技で割り込めない
	virtual bool IsInterruptible() const override { return false; }

private:
	float m_stateTimer = 0.0f; // この状態が開始してからの経過時間
};