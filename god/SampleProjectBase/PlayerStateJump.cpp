#include "PlayerStateJump.h"
#include "PlayerStateIdle.h"
#include "PlayerStateWalk.h"
#include "Player.h"

void PlayerStateJump::OnEnter(Player* player)
{
	// ジャンプアニメーションを再生
	player->PlayAnimation("Jump", true);
	m_elapsedTime = 0.0f;

	// --- 空中制御 (ジャンプした瞬間の入力で軌道を固定) ---
	DirectX::XMFLOAT3 vel = player->GetVelocity();
	const PlayerInputs& inputs = player->GetInputs();

	// ここで歩き速度ではなく、新しく作ったジャンプ速度を取得します
	float jumpSpeed = player->GetJumpSpeed();

	if (inputs.moveLeft) {
		vel.x = -jumpSpeed;
	}
	else if (inputs.moveRight) {
		vel.x = jumpSpeed;
	}
	else {
		vel.x = 0.0f;
	}

	player->SetVelocity(vel);
}

void PlayerStateJump::UpdateBehavior(Player* player, float tick)
{
	// ---  着地チェック ---

	// --- アニメーション制御 (最後まで再生したら止める) ---
	m_elapsedTime += tick;
	if (m_elapsedTime >= JUMP_ANIM_DURATION)
	{
		player->SetAnimPause(true); // 最後のポーズで固定
	}
}