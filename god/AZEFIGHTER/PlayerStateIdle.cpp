#include "PlayerStateIdle.h"
#include "PlayerStateWalk.h"
#include "PlayerStateWalkBack.h"
#include "PlayerStateCrouch.h" 
#include "Player.h"
// #include "Input.h"

// 待機モーションは 30fps で打たれているので、1キーを1フレームで再生すると2倍速になる
static const float IDLE_ANIM_SPEED = 0.5f;

void PlayerStateIdle::OnEnter(Player* player)
{
	player->PlayAnimation("Idle");
	player->SetAnimationSpeed(IDLE_ANIM_SPEED);
}

void PlayerStateIdle::UpdateBehavior(Player* player, float tick)
{
	DirectX::XMFLOAT3 vel = player->GetVelocity();
	vel.x = 0.0f;
	vel.z = 0.0f;

	// 抽象化された入力を取得
	const PlayerInputs& inputs = player->GetInputs();
	const bool back = player->GetForwardMoveDot() < 0.0f;
	if (inputs.moveLeft) {
		vel.x = -player->GetWalkSpeed(back);
	}
	else if (inputs.moveRight) {
		vel.x = player->GetWalkSpeed(back);
	}

	player->SetVelocity(vel);


	// --- 2. 別の状態への「遷移チェック」 ---
	if (player->GetInputType() != PlayerInputType::AI)
	{
		float moveDot = player->GetForwardMoveDot();

		if (moveDot > 0.5f) {
			player->SetState(std::make_unique<PlayerStateWalk>());
			return;
		}

		if (moveDot < -0.5f) {
			player->SetState(std::make_unique<PlayerStateWalkBack>());
			return;
		}
	}
}