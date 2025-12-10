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

	// ★変更: 自動進行を停止
	player->SetAnimationSpeed(0.0f);

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

	// ★追加: フレームの手動制御
	float progress = m_stateTimer / m_stunDuration;
	if (progress > 1.0f) progress = 1.0f;

	int totalAnimFrames = player->GetModel()->GetAnimationTotalFrame("Damage");
	float currentFrame = progress * (float)totalAnimFrames;
	player->SetCurrentFrame(currentFrame);


	if (m_stateTimer >= m_stunDuration)
	{
		player->SetAnimationSpeed(1.0f);
		player->SetState(new PlayerStateIdle());
		return;
	}
}
