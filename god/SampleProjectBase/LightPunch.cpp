#include "LightPunch.h"
#include "PlayerStateIdle.h" // 攻撃後に戻るため
#include "Player.h"

// --- 技の性能定義 ---
// (Player.h の AttackParams 構造体に移動したため、ここの const は削除)


void LightPunch::OnEnter(Player* player)
{
	// 1. パンチアニメーションを再生
	player->PlayAnimation("LightPunch", true); //アニメーション名

	// 2. 状態タイマーをリセット
	m_stateTimer = 0.0f;

	// 3. 攻撃開始時はHitboxを無効化
	player->SetActiveHitbox(false);

	// 4. パンチ中は左右の移動速度を0にする
	DirectX::XMFLOAT3 vel = player->GetVelocity();
	vel.x = 0.0f;
	vel.z = 0.0f;
	player->SetVelocity(vel);
}

void LightPunch::Update(Player* player, float tick)
{
	m_stateTimer += tick; // 経過時間を加算

	// ★ プレイヤーから現在のライトパンチ性能を取得
	AttackParams& params = player->GetLightPunchParams();

	// --- 1. 攻撃判定（Hitbox）の制御 ---

	// 攻撃判定の発生フレームか？
	if (m_stateTimer >= params.hitboxStart && m_stateTimer < params.hitboxEnd)
	{
		// プレイヤーに設定されたパラメータでHitboxを更新
		player->UpdateHitbox(params.hitboxOffset, params.hitboxExtents);
		player->SetActiveHitbox(true);
	}
	else
	{
		// 攻撃判定の発生フレーム外なら、必ず無効化する
		player->SetActiveHitbox(false);
	}


	// --- 2. 状態の終了チェック ---

	// 技の全体フレームが終了したらIdle状態に戻る
	if (m_stateTimer >= params.totalDuration)
	{
		player->SetState(new PlayerStateIdle());
		return;
	}
}