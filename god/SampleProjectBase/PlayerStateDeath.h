#pragma once
#include "PlayerState.h"

/**
 * @brief €–S (Death) ó‘Ô
 * HP‚ª0‚É‚È‚Á‚½‚É‘JˆÚ‚·‚éB
 * DeathƒAƒjƒ[ƒVƒ‡ƒ“‚ğÄ¶‚µAI—¹Œã‚Í‚»‚Ì‚Ü‚Ü“|‚ê‚Ä~‚Ü‚éB
 */
class PlayerStateDeath : public PlayerState
{
public:
	void OnEnter(Player* player) override;
	void Update(Player* player, float tick) override;

	// ‘€ì•s”\
	bool IsInterruptible() const override { return false; }
	// €‘ÌR‚è–h~‚Ì‚½‚ß–³“Gˆµ‚¢‚É‚·‚é
	bool IsInvincible() const override { return true; }

	// ‚±‚ê‚Í€–Só‘Ô‚Å‚ ‚é‚ÆéŒ¾iã‘‚«–h~—pj
	bool IsDeathState() const override { return true; }
};