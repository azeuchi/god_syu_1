#include "LightPunch.h"
#include "PlayerStateIdle.h"
#include "Player.h"

// 遷移先のヘッダーをインクルード
#include "LightPunch.h"   // 連打キャンセル用
#include "MediumPunch.h"
#include "HeavyKick.h"

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

	// 攻撃判定の処理
	if (m_stateTimer >= params.hitboxStart && m_stateTimer < params.hitboxEnd)
	{
		player->UpdateHitbox(params.hitboxOffset, params.hitboxExtents);
		player->SetActiveHitbox(true);
	}
	else
	{
		player->SetActiveHitbox(false);
	}

	// キャンセル処理
	if (params.cancelEnabled)
	{
		// 受付期間内かチェック
		if (m_stateTimer >= params.cancelStart && m_stateTimer <= params.cancelEnd)
		{
			const PlayerInputs& inputs = player->GetInputs();

			// 弱パンチへのキャンセル
			if (params.cancelToLight && inputs.LightPunchi)
			{
				player->SetCurrentAttackParams(&player->GetLightPunchParams());
				player->SetState(new LightPunch());
				return; // 即座にリターンしてこのステートを終了
			}
			// 中パンチへのキャンセル
			if (params.cancelToMedium && inputs.MediumPunch)
			{
				player->SetCurrentAttackParams(&player->GetMediumPunchParams());
				player->SetState(new MediumPunch());
				return;
			}
			// 大キックへのキャンセル
			if (params.cancelToHeavy && inputs.HeavyKick)
			{
				player->SetCurrentAttackParams(&player->GetHeavyKickParams());
				player->SetState(new HeavyKick());
				return;
			}
		}
	}

	// 終了処理
	if (m_stateTimer >= params.totalDuration)
	{
		player->SetAnimationSpeed(1.0f);
		player->SetState(new PlayerStateIdle());
		return;
	}
}