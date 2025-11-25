#include "PlayerStateJump.h"
#include "PlayerStateIdle.h"
#include "PlayerStateWalk.h"
#include "Player.h"

void PlayerStateJump::OnEnter(Player* player)
{
	// ジャンプアニメーションを再生 (ループしない設定にすると自然です)
	// 第2引数 true で最初から再生
	player->PlayAnimation("Jump", true);
}

void PlayerStateJump::Update(Player* player, float tick)
{
	// --- 1. 着地チェック ---
	// Player::UpdatePhysics で地面につくと GetIsJumping() が false になります
	if (!player->GetIsJumping())
	{
		// 着地したら待機状態へ遷移
		player->SetState(new PlayerStateIdle());
		return;
	}

	// --- 2. 空中制御 (Air Control) ---
	// ジャンプ中も左右移動できるようにする場合の処理

	DirectX::XMFLOAT3 vel = player->GetVelocity();
	// Y軸 (重力/ジャンプ力) は変更せず、X/Z軸だけ入力を反映する

	const PlayerInputs& inputs = player->GetInputs();

	// 空中での移動速度（必要なら減速させても良いですが、今回は地上と同じにします）
	float moveSpeed = player->GetMoveSpeed();

	if (inputs.moveLeft) {
		vel.x = -moveSpeed;
	}
	else if (inputs.moveRight) {
		vel.x = moveSpeed;
	}
	else {
		// キーを離したら慣性を消す場合 (格ゲーっぽい挙動)
		vel.x = 0.0f;
	}

	player->SetVelocity(vel);
}