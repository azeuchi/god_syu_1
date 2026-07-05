#pragma once
#include "PlayerState.h"

/**
 * @brief ‘åƒpƒ“ƒ` (HeavyPunch) ó‘Ô
 */
class HeavyPunch : public PlayerState
{
public:
	void OnEnter(Player* player) override;
	void Update(Player* player, float tick) override;

	// UŒ‚’†‚Í‘¼‚Ì‹Z‚ÅŠ„‚è‚ß‚È‚¢
	bool IsInterruptible() const override { return false; }

private:
	float m_stateTimer = 0.0f;
};