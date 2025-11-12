#pragma once
#include "PlayerState.h"

/**
 * @brief Œã‘Ş (WalkBack) ó‘Ô
 */
class PlayerStateWalkBack : public PlayerState
{
public:
	void OnEnter(Player* player) override;
	void Update(Player* player, float tick) override;
};