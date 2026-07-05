#include "PlayerStateCrouch.h"
#include "PlayerStateIdle.h"
#include "Player.h"

void PlayerStateCrouch::OnEnter(Player* player)
{
	player->PlayAnimation("CrouchIdle");

	// しゃがみ中は移動しないので速度をゼロにする
	DirectX::XMFLOAT3 vel = player->GetVelocity();
	vel.x = 0.0f;
	vel.z = 0.0f;
	player->SetVelocity(vel);
}

void PlayerStateCrouch::UpdateBehavior(Player* player, float tick)
{
	// しゃがみ状態であることを通知（当たり判定をしゃがみ用に切り替える）
	player->SetIsCrouching(true);

	const PlayerInputs& inputs = player->GetInputs();

	// 下入力がなくなったら、立ち状態(Idle)に戻る
	if (!inputs.moveDown)
	{
		player->SetState(new PlayerStateIdle());
		return;
	}

}