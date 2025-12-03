#include "MediumPunch.h"
#include "PlayerStateIdle.h" 
#include "Player.h"

void MediumPunch::OnEnter(Player* player)
{
	// 1. アニメーション再生開始
	player->PlayAnimation("MediumPunch", true);

	// 2. パラメータ取得 (中パンチ)
	AttackParams& params = player->GetMediumPunchParams();

	// 速度計算
	int originalFrames = player->GetModel()->GetAnimationTotalFrame("MediumPunch");
	float targetFrames = params.totalDuration * 60.0f;
	if (targetFrames <= 0.0f) targetFrames = 1.0f;
	float speed = (float)originalFrames / targetFrames;

	// 4. 速度適用
	player->SetAnimationSpeed(speed);


	// --- 初期化処理 ---
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