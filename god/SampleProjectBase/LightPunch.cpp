#include "LightPunch.h"
#include "PlayerStateIdle.h"
#include "Player.h"

void LightPunch::OnEnter(Player* player)
{
	// 1. アニメーション再生開始 (この時点では速度1.0)
	player->PlayAnimation("LightPunch", true);

	// 2. パラメータ取得
	AttackParams& params = player->GetLightPunchParams();

	// 3. ★速度計算★
	// 元のアニメーションの長さ(フレーム数)を取得
	int originalFrames = player->GetModel()->GetAnimationTotalFrame("LightPunch");

	// 設定された「全体フレーム(秒)」を60FPS換算のフレーム数にする
	float targetFrames = params.totalDuration * 60.0f;
	if (targetFrames <= 0.0f) targetFrames = 1.0f; // 0除算防止

	// 倍率計算 (例: 元60F / 設定20F = 3.0倍速)
	float speed = (float)originalFrames / targetFrames;

	// 4. 計算した速度を適用
	player->SetAnimationSpeed(speed);


	// --- 以下、既存の初期化処理 ---
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
		player->SetState(new PlayerStateIdle());
		return;
	}
}