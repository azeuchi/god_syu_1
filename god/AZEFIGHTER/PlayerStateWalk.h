#pragma once
#include "PlayerStateGround.h"

/**
 * @brief ‘Oi (Walk) ó‘Ô
 */
class PlayerStateWalk : public PlayerStateGround
{
public:
	void OnEnter(Player* player) override;
	void UpdateBehavior(Player* player, float tick) override;
};