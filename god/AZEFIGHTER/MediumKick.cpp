#include "MediumKick.h"
#include "PlayerStateIdle.h"
#include "Player.h"


void MediumKick::OnEnter(Player* player)
{
	player->PlayAnimation("MediumKick", true);

	AttackParams& params = player->GetMediumKickParams();

	int originalFrames = player->GetModel()->GetAnimationTotalFrame("MediumKick");
	float targetFrames = params.totalDuration * 60.0f;

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

void MediumKick::Update(Player* player, float tick)
{
	m_stateTimer += tick;
	AttackParams& params = player->GetMediumKickParams();

	// UŒ‚”»’è‚Ìˆ—
	if (m_stateTimer >= params.hitboxStart && m_stateTimer < params.hitboxEnd)
	{
		player->SetActiveHitbox(true);
	}
	else
	{
		player->SetActiveHitbox(false);
	}

	// I—¹”»’è
	if (m_stateTimer >= params.totalDuration)
	{
		player->SetState(new PlayerStateIdle());
		return;
	}

	// ƒLƒƒƒ“ƒZƒ‹ˆ—
	if (CheckCancel(player, m_stateTimer, params))
	{
		return;
	}
}