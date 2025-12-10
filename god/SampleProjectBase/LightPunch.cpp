#include "LightPunch.h"
#include "PlayerStateIdle.h"
#include "Player.h"

void LightPunch::OnEnter(Player* player)
{
	player->PlayAnimation("LightPunch", true);

	AttackParams& params = player->GetLightPunchParams();

	// --- 速度計算 ---
	int originalFrames = player->GetModel()->GetAnimationTotalFrame("LightPunch");

	// 設定時間の90で再生が完了するように速度を設定する
	float targetFrames = (params.totalDuration * 0.9f) * 60.0f;

	if (targetFrames <= 1.0f) targetFrames = 1.0f;

	float speed = (float)originalFrames / targetFrames;
	player->SetAnimationSpeed(speed);
	// ----------------

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
		// 終了時に速度を1.0に戻しておく
		player->SetAnimationSpeed(1.0f);
		player->SetState(new PlayerStateIdle());
		return;
	}
}