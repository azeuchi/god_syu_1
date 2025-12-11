#pragma once
#include "PlayerStateGround.h"

/**
 * @brief ‚µ‚á‚ª‚İ (Crouch) ó‘Ô
 */
class PlayerStateCrouch : public PlayerStateGround
{
public:
	void OnEnter(Player* player) override;
	void UpdateBehavior(Player* player, float tick) override;

	// ‚±‚Ìó‘Ô‚Íu‚µ‚á‚ª‚İv‚Å‚ ‚é‚ÆéŒ¾
	bool IsCrouch() const override { return true; }
};