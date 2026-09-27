#pragma once
#include "PlayerState.h"

/**
 * @brief ガード硬直
 * 攻撃をガードしてから、決められたフレームだけ動けない。
 * ・硬直中は後ろを離してもガードが続くので、連係の途中で崩れない
 * ・立ちとしゃがみは硬直中でも下の入力で切り替えられる。上下の揺さぶりへの対応はここで行う
 * ・モーションは硬直の長さに合わせて再生し、最後のフレームで構えに戻る
 */
class PlayerStateGuard : public PlayerState
{
public:
	PlayerStateGuard(int stunFrames, bool crouch);

	void OnEnter(Player* player) override;
	void Update(Player* player, float tick) override;

	bool IsInterruptible() const override { return false; }
	bool IsCrouch() const override { return m_crouch; }
	bool IsBlockstun() const override { return true; }

private:
	void PlayGuardAnimation(Player* player) const;

	float m_stateTimer = 0.0f;
	float m_stunDuration = 0.0f; // 硬直時間（秒）
	bool m_crouch = false;
};
