#include "PlayerStateDown.h"
#include "PlayerStateWakeUp.h"
#include "Player.h"
#include <cmath>

void PlayerStateDown::OnEnter(Player* player)
{
	player->PlayAnimation("Down", true);
	player->SetAnimationSpeed(2.0f); // アニメーション速度

	// 向きの取得 (Scale.x > 0 なら右向き)
	// 自分が向いている方向の「逆」に吹っ飛ばす
	float dir = (player->GetScale().x > 0.0f) ? 1.0f : -1.0f;

	// 初速の設定
	DirectX::XMFLOAT3 vel;
	vel.x = -dir * 1.0f; // 後ろへ
	vel.y = 2.5f;       // 上へ
	vel.z = 0.0f;
	player->SetVelocity(vel);

	// わずかに浮かせて「空中にいる」状態を作る
	DirectX::XMFLOAT3 pos = player->GetPosition();
	pos.y = 0.1f;
	player->SetPosition(pos);

	// 重力を有効にする
	player->ForceJumpState(true);

	player->SetActiveHitbox(false);
	m_hasLanded = false;
	m_groundTimer = 0.0f;
}

void PlayerStateDown::Update(Player* player, float tick)
{
	// --- アニメーション制御 ---
	// 最後のフレーム付近で止めておく
	int totalFrame = player->GetModel()->GetAnimationTotalFrame("Down");
	int currentFrame = player->Debug_GetFrame();

	if (currentFrame >= totalFrame - 2)
	{
		player->SetAnimPause(true);
	}

	// --- 接地判定 ---
	// Player::UpdatePhysicsにより、地面につくと GetIsJumping() が false になる
	if (!player->GetIsJumping())
	{
		if (!m_hasLanded)
		{
			// 着地した瞬間
			m_hasLanded = true;

			// その場で停止
			DirectX::XMFLOAT3 zeroVel = { 0.0f, 0.0f, 0.0f };
			player->SetVelocity(zeroVel);
		}

		// 接地後の待機時間 (1.0秒)
		m_groundTimer += tick;
		if (m_groundTimer >= DOWN_TIME)
		{
			// 起き上がりへ移行
			player->SetState(new PlayerStateWakeUp());
		}
	}
}