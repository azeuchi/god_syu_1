#pragma once
#include "PlayerStateGround.h"

/**
 * @brief ‚µ‚á‚ª‚İ (Crouch) ó‘Ô
 * ‰ºƒL[‚ğ“ü—Í‚µ‚Ä‚¢‚éŠÔ‚Í‚±‚Ìó‘Ô‚É‚È‚éB
 */
class PlayerStateCrouch : public PlayerStateGround
{
public:
	void OnEnter(Player* player) override;
	void UpdateBehavior(Player* player, float tick) override;
};