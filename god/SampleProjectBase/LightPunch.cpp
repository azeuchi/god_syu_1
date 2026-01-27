#include "LightPunch.h"
#include "PlayerStateIdle.h"
#include "Player.h"

void LightPunch::OnEnter(Player* player)
{
	player->PlayAnimation("LightPunch", true);

	AttackParams& params = player->GetLightPunchParams();

	int originalFrames = player->GetModel()->GetAnimationTotalFrame("LightPunch");
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

void LightPunch::Update(Player* player, float tick)
{
	m_stateTimer += tick;
	AttackParams& params = player->GetLightPunchParams();

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