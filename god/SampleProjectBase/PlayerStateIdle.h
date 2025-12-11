#pragma once
#include "PlayerStateGround.h"

/**
 * @brief ‘Ò‹@ (Idle) ó‘Ô
 */
class PlayerStateIdle : public PlayerStateGround
{
public:
	void OnEnter(Player* player) override;
	void UpdateBehavior(Player* player, float tick) override;
};