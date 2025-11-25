#include "PlayerStateJump.h"
#include "PlayerStateIdle.h"
#include "PlayerStateWalk.h"
#include "Player.h"

void PlayerStateJump::OnEnter(Player* player)
{
	// ジャンプアニメーションを再生
	player->PlayAnimation("Jump", true);
	m_elapsedTime = 0.0f;
}

void PlayerStateJump::Update(Player* player, float tick)
{
	// --- 1. 着地チェック ---
	if (!player->GetIsJumping())
	{
		// 着地したら待機状態へ遷移
		player->SetState(new PlayerStateIdle());
		return;
	}

	// --- 2. アニメーション制御 (最後まで再生したら止める) ---
	m_elapsedTime += tick;
	if (m_elapsedTime >= JUMP_ANIM_DURATION)
	{
		player->SetAnimPause(true); // 最後のポーズで固定
	}

	// --- 3. 空中制御 (Air Control) ---
	DirectX::XMFLOAT3 vel = player->GetVelocity();
	const PlayerInputs& inputs = player->GetInputs();
	float moveSpeed = player->GetMoveSpeed();

	if (inputs.moveLeft) {
		vel.x = -moveSpeed;
	}
	else if (inputs.moveRight) {
		vel.x = moveSpeed;
	}
	else {
		vel.x = 0.0f;
	}

	player->SetVelocity(vel);
}