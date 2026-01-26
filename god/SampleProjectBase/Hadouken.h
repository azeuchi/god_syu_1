#pragma once
#include "PlayerState.h"

/**
 * @brief ”g“®Œ (Hadouken) ó‘Ô
 */
class Hadouken : public PlayerState
{
public:
	Hadouken(int strength); // 0:ã, 1:’†, 2:‹­
	void OnEnter(Player* player) override;
	void Update(Player* player, float tick) override;

	bool IsInterruptible() const override { return false; }

private:
	float m_stateTimer = 0.0f;
	int m_strength = 0;
	bool m_hasFired = false;
	const float FIRE_FRAME = 0.25f; // ’e‚ªo‚éƒ^ƒCƒ~ƒ“ƒO (•b)
};