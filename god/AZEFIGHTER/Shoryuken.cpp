#include "Shoryuken.h"
#include "PlayerStateIdle.h"
#include "Player.h"

Shoryuken::Shoryuken(int strength)
	: m_strength(strength)
{
}

float Shoryuken::RiseVelocity(const AttackParams& params)
{
	// 投げ上げの滞空時間 t = 2v / g より v = g * t / 2
	const float airTime = params.totalDuration * (LANDING_RATIO - TAKEOFF_RATIO);
	return Player::GRAVITY * airTime * 0.5f;
}

void Shoryuken::OnEnter(Player* player)
{
	player->PlayAnimation("Shoryuken", true);

	if (m_strength == 0)      m_params = &player->GetShoryukenLParams();
	else if (m_strength == 1) m_params = &player->GetShoryukenMParams();
	else                      m_params = &player->GetShoryukenHParams();

	player->SetCurrentAttackParams(m_params);
	m_invincibleEnd = m_params->invincibleEnd;

	// アニメーション全体が、パラメータの全体時間にちょうど収まる再生速度にする
	const int originalFrames = player->GetModel()->GetAnimationTotalFrame("Shoryuken");
	float targetFrames = m_params->totalDuration * 60.0f;
	if (targetFrames <= 1.0f) targetFrames = 1.0f;
	player->SetAnimationSpeed((float)originalFrames / targetFrames);

	m_stateTimer = 0.0f;
	m_hasTakenOff = false;
	m_isFalling = false;
	player->SetActiveHitbox(false);
	player->SetVelocity({ 0.0f, 0.0f, 0.0f });
}

void Shoryuken::Update(Player* player, float tick)
{
	m_stateTimer += tick;
	const AttackParams& params = *m_params;

	// アニメーションで足が地面を離れる瞬間に、物理でも飛び上がらせる
	if (!m_hasTakenOff && m_stateTimer >= params.totalDuration * TAKEOFF_RATIO)
	{
		m_hasTakenOff = true;
		const float dir = (player->GetRotation().y < 0.0f) ? 1.0f : -1.0f;
		player->SetVelocity({ dir * params.forwardSpeed, RiseVelocity(params), 0.0f });
		player->ForceJumpState(true);
	}

	if (m_hasTakenOff && !m_isFalling && player->GetVelocity().y <= 0.0f)
	{
		m_isFalling = true;
		DirectX::XMFLOAT3 vel = player->GetVelocity();
		vel.x *= FALL_DRIFT_RATIO;
		player->SetVelocity(vel);
	}

	player->SetActiveHitbox(m_stateTimer >= params.hitboxStart && m_stateTimer < params.hitboxEnd);

	if (m_stateTimer >= params.totalDuration)
	{
		player->SetAnimationSpeed(1.0f);
		player->SetState(std::make_unique<PlayerStateIdle>());
	}
}
