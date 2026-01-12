#include "PlayerStateWakeUp.h"
#include "PlayerStateIdle.h"
#include "Player.h"

void PlayerStateWakeUp::OnEnter(Player* player)
{
	player->PlayAnimation("WakeUp", true);
	player->SetAnimPause(false);

	// 速度はゼロ
	player->SetVelocity({ 0.0f, 0.0f, 0.0f });
	player->SetActiveHitbox(false);
}

void PlayerStateWakeUp::Update(Player* player, float tick)
{
	// アニメーションが終了したらアイドルに戻る
	int totalFrame = player->GetModel()->GetAnimationTotalFrame("WakeUp");
	int currentFrame = player->Debug_GetFrame();

	// 少し余裕を持って判定
	if (currentFrame >= totalFrame - 1)
	{
		player->SetState(new PlayerStateIdle());
		return;
	}
}