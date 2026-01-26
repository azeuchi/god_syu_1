#include "Hadouken.h"
#include "PlayerStateIdle.h"
#include "Player.h"
#include "Projectile.h"

Hadouken::Hadouken(int strength)
	: m_strength(strength)
	, m_hasFired(false)
	, m_stateTimer(0.0f)
{
}

void Hadouken::OnEnter(Player* player)
{
	player->PlayAnimation("Hadouken", true);

	AttackParams* params = nullptr;
	if (m_strength == 0) params = &player->GetHadoukenLParams();
	else if (m_strength == 1) params = &player->GetHadoukenMParams();
	else params = &player->GetHadoukenHParams();

	player->SetCurrentAttackParams(params);

	// ƒ‚[ƒVƒ‡ƒ“’†‚Ì‘¬“x‚ðƒ[ƒ‚É
	player->SetVelocity({ 0, 0, 0 });
	player->SetActiveHitbox(false);
}

void Hadouken::Update(Player* player, float tick)
{
	m_stateTimer += tick;
	AttackParams* params = player->GetCurrentAttackParams();

	// ’e‚Ì”­ŽË
	if (!m_hasFired && m_stateTimer >= FIRE_FRAME)
	{
		m_hasFired = true;
		Projectile* proj = player->GetProjectile();
		if (proj)
		{
			bool isRight = (player->GetRotation().y < 0.0f);
			proj->Launch(player, params->projectileSpeed, params->damage, params->projectileSize, isRight);
		}
	}

	// I—¹”»’è
	if (m_stateTimer >= params->totalDuration)
	{
		player->SetState(new PlayerStateIdle());
		return;
	}
}