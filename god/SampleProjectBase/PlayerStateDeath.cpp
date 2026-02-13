#include "PlayerStateDeath.h"
#include "Player.h"

void PlayerStateDeath::OnEnter(Player* player)
{
	player->PlayAnimation("Death", false);
	player->SetAnimationSpeed(2.0f);
	player->SetActiveHitbox(false);

	DirectX::XMFLOAT3 zeroVel = { 0.0f, 0.0f, 0.0f };
	player->SetVelocity(zeroVel);
}

void PlayerStateDeath::Update(Player* player, float tick)
{
	int totalFrame = player->GetModel()->GetAnimationTotalFrame("Death");
	int currentFrame = player->Debug_GetFrame();

	if (currentFrame >= totalFrame - 1)
	{
		player->SetAnimPause(true);
	}
}