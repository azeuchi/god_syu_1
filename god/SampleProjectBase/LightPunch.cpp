#include "LightPunch.h"
#include "PlayerStateIdle.h"
#include "Player.h"
void LightPunch::OnEnter(Player* player)
{
	// アニメーション再生開始
	player->PlayAnimation("LightPunch", true);

	// ★変更: プレイヤー側の自動更新を止めるため、速度を0にする
	// これにより、Update関数内で手動制御できるようになります
	player->SetAnimationSpeed(0.0f);

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

	// =========================================================
	// ★追加: 経過時間に応じたフレーム数の計算と適用
	// =========================================================
	// 1. 現在の進行度 (0.0 ～ 1.0) を計算
	float progress = m_stateTimer / params.totalDuration;
	if (progress > 1.0f) progress = 1.0f;

	// 2. 元のアニメーションの総フレーム数を取得
	int totalAnimFrames = player->GetModel()->GetAnimationTotalFrame("LightPunch");

	// 3. 進行度に合わせてフレームを決定 (例: 50%なら総フレームの半分)
	float currentFrame = progress * (float)totalAnimFrames;

	// 4. プレイヤーに適用
	player->SetCurrentFrame(currentFrame);
	// =========================================================


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
		// 終了時は速度を1.0に戻しておく
		player->SetAnimationSpeed(1.0f);
		player->SetState(new PlayerStateIdle());
		return;
	}
}
