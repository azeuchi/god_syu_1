#include "PlayerStateIdle.h"
#include "PlayerStateWalk.h"
#include "PlayerStateWalkBack.h"
#include "Player.h"
// #include "Input.h"

void PlayerStateIdle::OnEnter(Player* player)
{
	player->PlayAnimation("Idle");
}

void PlayerStateIdle::UpdateBehavior(Player* player, float tick)
{
	DirectX::XMFLOAT3 vel = player->GetVelocity();
	vel.x = 0.0f;
	vel.z = 0.0f;

	// 抽象化された入力を取得
	const PlayerInputs& inputs = player->GetInputs();

	if (inputs.moveLeft) {
		vel.x = -player->GetMoveSpeed();
	}
	else if (inputs.moveRight) {
		vel.x = player->GetMoveSpeed();
	}

	player->SetVelocity(vel);


	// --- 2. 別の状態への「遷移チェック」 ---
	if (player->GetInputType() != PlayerInputType::AI)
	{
		float moveDot = player->GetForwardMoveDot();

		if (moveDot > 0.5f) {
			player->SetState(new PlayerStateWalk());
			return;
		}

		if (moveDot < -0.5f) {
			player->SetState(new PlayerStateWalkBack());
			return;
		}
	}
}