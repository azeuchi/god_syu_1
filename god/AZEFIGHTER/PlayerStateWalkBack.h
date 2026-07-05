#pragma once
#include "PlayerStateGround.h"

/**
 * @brief å„ëﬁ (WalkBack) èÛë‘
 */
class PlayerStateWalkBack : public PlayerStateGround
{
public:
	void OnEnter(Player* player) override;
	void UpdateBehavior(Player* player, float tick) override;
};