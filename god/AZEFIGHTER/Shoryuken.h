#pragma once
#include "PlayerState.h"

struct AttackParams;

/**
 * @brief 昇竜拳 (Shoryuken) 状態
 * 出始めが無敵の対空技。飛び上がって落ちてくるまで無防備なので、外すと反撃を受ける
 */
class Shoryuken : public PlayerState
{
public:
	explicit Shoryuken(int strength); // 0:弱, 1:中, 2:強

	void OnEnter(Player* player) override;
	void Update(Player* player, float tick) override;

	bool IsInterruptible() const override { return false; }
	bool IsInvincible() const override { return m_stateTimer < m_invincibleEnd; }

	// Shoryuken.fbx の中で足が地面を離れる／着く位置（全体に対する割合）。
	// Maya のスクリプトで 24 フレーム中 2 フレーム目に離陸、18 フレーム目に着地させている。
	// 上昇速度はここから逆算するので、全体フレームを変えても見た目と物理の着地がずれない
	static constexpr float TAKEOFF_RATIO = 2.0f / 24.0f;
	static constexpr float LANDING_RATIO = 18.0f / 24.0f;

	// 頂点を過ぎたあとに残す前進の割合。上りで斜め前に進み、下りはほぼ真下に落ちる軌道にする
	static constexpr float FALL_DRIFT_RATIO = 0.3f;

	// 全体時間から、アニメーションと同じタイミングで着地する上昇初速を求める
	static float RiseVelocity(const AttackParams& params);

private:
	int m_strength = 0;
	AttackParams* m_params = nullptr;
	float m_stateTimer = 0.0f;
	float m_invincibleEnd = 0.0f;
	bool m_hasTakenOff = false;
	bool m_isFalling = false;
};
