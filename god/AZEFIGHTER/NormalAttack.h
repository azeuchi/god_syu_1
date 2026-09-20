#pragma once
#include "PlayerState.h"
#include "NormalMoves.h"

/**
 * @brief 通常技（立ち・しゃがみの弱中強パンチ／キック）の状態
 * 技ごとに違うのはアニメーション名とパラメータだけなので、1つのクラスを NormalMove で切り替えて使う
 */
class NormalAttack : public PlayerState
{
public:
	explicit NormalAttack(NormalMove move);

	// 攻撃パラメータの登録と状態の切り替えは必ずセットで行うので、技を出すときはこれを使う
	static void Start(Player* player, NormalMove move);

	void OnEnter(Player* player) override;
	void Update(Player* player, float tick) override;

	// 攻撃中は他の技で割り込めない
	bool IsInterruptible() const override { return false; }
	// しゃがみ技の間はくらい判定もしゃがみの形にする
	bool IsCrouch() const override;

private:
	NormalMove m_move;
	float m_stateTimer = 0.0f;
};
