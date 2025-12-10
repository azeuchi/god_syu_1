#pragma once
#include "PlayerState.h"

/**
 * @brief やられ (Damage) 状態
 * 攻撃を受けるとこの状態になり、指定された時間だけ操作不能になる
 */
class PlayerStateDamage : public PlayerState
{
public:
	// コンストラクタで硬直フレーム数を受け取る
	PlayerStateDamage(int stunFrames);

	void OnEnter(Player* player) override;
	void Update(Player* player, float tick) override;

	// やられ中は他の行動でキャンセルできない (操作不能)
	bool IsInterruptible() const override { return false; }

private:
	float m_stateTimer = 0.0f;
	float m_stunDuration = 0.0f; // 硬直時間 (秒)
};