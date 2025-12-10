#include "MediumPunch.h"
#include "PlayerStateIdle.h" 
#include "Player.h"

void MediumPunch::OnEnter(Player* player)
{
	player->PlayAnimation("MediumPunch", true);

	AttackParams& params = player->GetMediumPunchParams();

	int originalFrames = player->GetModel()->GetAnimationTotalFrame("MediumPunch");
	float targetFrames = params.totalDuration * 60.0f;

	// ‘¬“xŒvZ‚É—]—T‚ğ‚½‚¹‚é
	if (targetFrames <= 1.0f) targetFrames = 1.0f;
	float speed = (float)originalFrames / (targetFrames - 0.9f);

	player->SetAnimationSpeed(speed);

	m_stateTimer = 0.0f;
	player->SetActiveHitbox(false);
	DirectX::XMFLOAT3 vel = player->GetVelocity();
	vel.x = 0.0f;
	vel.z = 0.0f;
	player->SetVelocity(vel);
}

void MediumPunch::Update(Player* player, float tick)
{
	m_stateTimer += tick;
	AttackParams& params = player->GetMediumPunchParams();

	if (m_stateTimer >= params.hitboxStart && m_stateTimer < params.hitboxEnd)
	{
		player->UpdateHitbox(params.hitboxOffset, params.hitboxExtents);
		player->SetActiveHitbox(true);
	}
	else
	{
		player->SetActiveHitbox(false);
	}

	if (m_stateTimer >= params.totalDuration)
	{
		player->SetState(new PlayerStateIdle());
		return;
	}
}