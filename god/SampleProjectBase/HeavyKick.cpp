#include "HeavyKick.h"
#include "PlayerStateIdle.h" 
#include "Player.h"

void HeavyKick::OnEnter(Player* player)
{
	player->PlayAnimation("HeavyKick", true);

	AttackParams& params = player->GetHeavyKickParams();

	int originalFrames = player->GetModel()->GetAnimationTotalFrame("HeavyKick");
	float targetFrames = params.totalDuration * 60.0f;

	// 速度計算
	if (targetFrames <= 1.0f) targetFrames = 1.0f;
	float speed = (float)originalFrames / targetFrames;

	player->SetAnimationSpeed(speed);

	m_stateTimer = 0.0f;
	player->SetActiveHitbox(false);
	DirectX::XMFLOAT3 vel = player->GetVelocity();
	vel.x = 0.0f;
	vel.z = 0.0f;
	player->SetVelocity(vel);
}

void HeavyKick::Update(Player* player, float tick)
{
	m_stateTimer += tick;
	AttackParams& params = player->GetHeavyKickParams();

	// 判定の発生・消失チェック
	if (m_stateTimer >= params.hitboxStart && m_stateTimer < params.hitboxEnd)
	{
		player->UpdateHitbox(params.hitboxOffset, params.hitboxExtents);
		player->SetActiveHitbox(true);
	}
	else
	{
		player->SetActiveHitbox(false);
	}

	// 終了チェック
	if (m_stateTimer >= params.totalDuration)
	{
		// 終了時に速度を1.0に戻しておく
		player->SetAnimationSpeed(1.0f);
		player->SetState(new PlayerStateIdle());
		return;
	}
}