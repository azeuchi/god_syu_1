#include "PlayerStateDamage.h"
#include "PlayerStateIdle.h"
#include "Player.h"

static const float FRAME_TIME = 1.0f / 60.0f;

PlayerStateDamage::PlayerStateDamage(int stunFrames)
{
	if (stunFrames < 1) stunFrames = 1;
	m_stunDuration = stunFrames * FRAME_TIME;
}

void PlayerStateDamage::OnEnter(Player* player)
{
	player->PlayAnimation("Damage", true);

	// --- ‘¬“xŒvZ ---
	int originalFrames = player->GetModel()->GetAnimationTotalFrame("Damage");

	// d’¼ŠÔ(•b) ‚ğƒtƒŒ[ƒ€”‚ÉŠ·Z
	float stunFramesFrame = m_stunDuration * 60.0f;

	float extraFrames = 30.0f; 
	float targetFrames = stunFramesFrame + extraFrames;

	if (targetFrames <= 1.0f) targetFrames = 1.0f;

	float speed = (float)originalFrames / targetFrames;
	player->SetAnimationSpeed(speed);
	// ---------------------

	m_stateTimer = 0.0f;
	player->SetActiveHitbox(false);
	DirectX::XMFLOAT3 vel = player->GetVelocity();
	vel.x = 0.0f;
	vel.z = 0.0f;
	player->SetVelocity(vel);
}

void PlayerStateDamage::Update(Player* player, float tick)
{
	m_stateTimer += tick;

	if (m_stateTimer >= m_stunDuration)
	{
		// I—¹‚É‘¬“x‚ğ1.0‚É–ß‚µ‚Ä‚¨‚­
		player->SetAnimationSpeed(1.0f);
		player->SetState(std::make_unique<PlayerStateIdle>());
		return;
	}
}