#pragma once
#include "PlayerState.h"

/**
 * @brief ‘Ò‹@ (Idle) ó‘Ô
 */
class PlayerStateIdle : public PlayerState
{
public:
	void OnEnter(Player* player) override;
	void Update(Player* player, float tick) override;
};