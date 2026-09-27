#include "PlayerStateGuard.h"
#include "PlayerStateIdle.h"
#include "PlayerStateCrouch.h"
#include "Player.h"

static const float FRAME_TIME = 1.0f / 60.0f;

PlayerStateGuard::PlayerStateGuard(int stunFrames, bool crouch)
	: m_crouch(crouch)
{
	if (stunFrames < 1) stunFrames = 1;
	m_stunDuration = stunFrames * FRAME_TIME;
}

// 反動から構えに戻るまでを硬直の長さにちょうど収め、硬直の進み具合と同じところから再生する
void PlayerStateGuard::PlayGuardAnimation(Player* player) const
{
	const char* anim = m_crouch ? "CrouchGuard" : "Guard";
	const int originalFrames = player->GetModel()->GetAnimationTotalFrame(anim);
	player->PlayAnimation(anim, true);
	player->SetAnimationSpeed((float)originalFrames / (m_stunDuration * 60.0f));
	player->SetCurrentFrame(originalFrames * (m_stateTimer / m_stunDuration));
}

void PlayerStateGuard::OnEnter(Player* player)
{
	m_stateTimer = 0.0f;
	PlayGuardAnimation(player);

	player->SetActiveHitbox(false);
	DirectX::XMFLOAT3 vel = player->GetVelocity();
	vel.x = 0.0f;
	vel.z = 0.0f;
	player->SetVelocity(vel);
}

void PlayerStateGuard::Update(Player* player, float tick)
{
	m_stateTimer += tick;

	if (m_stateTimer >= m_stunDuration)
	{
		player->SetAnimationSpeed(1.0f);
		if (player->GetInputs().moveDown) player->SetState(std::make_unique<PlayerStateCrouch>());
		else                              player->SetState(std::make_unique<PlayerStateIdle>());
		return;
	}

	const bool wantCrouch = player->GetInputs().moveDown;
	if (wantCrouch != m_crouch)
	{
		m_crouch = wantCrouch;
		PlayGuardAnimation(player);
	}
}
