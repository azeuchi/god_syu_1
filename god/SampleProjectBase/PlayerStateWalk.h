#pragma once
#include "PlayerState.h"

/**
 * @brief ‘Oi (Walk) ó‘Ô
 */
class PlayerStateWalk : public PlayerState
{
public:
	void OnEnter(Player* player) override;
	void Update(Player* player, float tick) override;
};