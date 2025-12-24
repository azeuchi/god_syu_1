#include "MediumPunch.h"
#include "PlayerStateIdle.h"
#include "Player.h"

#include "LightPunch.h"
#include "HeavyKick.h"

void MediumPunch::OnEnter(Player* player)
{
	player->PlayAnimation("MediumPunch", true);

	AttackParams& params = player->GetMediumPunchParams();

	int originalFrames = player->GetModel()->GetAnimationTotalFrame("MediumPunch");
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

void MediumPunch::Update(Player* player, float tick)
{
	m_stateTimer += tick;
	AttackParams& params = player->GetMediumPunchParams();

	// UŒ‚”»’è‚Ìˆ—
	if (m_stateTimer >= params.hitboxStart && m_stateTimer < params.hitboxEnd)
	{
		player->UpdateHitbox(params.hitboxOffset, params.hitboxExtents);
		player->SetActiveHitbox(true);
	}
	else
	{
		player->SetActiveHitbox(false);
	}

	// ƒLƒƒƒ“ƒZƒ‹ˆ—
	if (params.cancelEnabled)
	{
		if (m_stateTimer >= params.cancelStart && m_stateTimer <= params.cancelEnd)
		{
			const PlayerInputs& inputs = player->GetInputs();

			if (params.cancelToLight && inputs.LightPunch)
			{
				player->SetCurrentAttackParams(&player->GetLightPunchParams());
				player->SetState(new LightPunch());
				return;
			}
			if (params.cancelToMedium && inputs.MediumPunch)
			{
				player->SetCurrentAttackParams(&player->GetMediumPunchParams());
				player->SetState(new MediumPunch());
				return;
			}
			if (params.cancelToHeavy && inputs.HeavyKick)
			{
				player->SetCurrentAttackParams(&player->GetHeavyKickParams());
				player->SetState(new HeavyKick());
				return;
			}
		}
	}

	// I—¹ˆ—
	if (m_stateTimer >= params.totalDuration)
	{
		player->SetAnimationSpeed(1.0f);
		player->SetState(new PlayerStateIdle());
		return;
	}
}