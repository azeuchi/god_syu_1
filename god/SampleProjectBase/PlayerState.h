#pragma once

// Playerクラスを「前方宣言」する
// (Player.h を include すると循環参照になるため)
class Player;

/**
 * @brief プレイヤーの状態のインターフェース（設計図）
 */
class PlayerState
{
public:
	// 仮想デストラクタ (継承されるクラスのお作法)
	virtual ~PlayerState() {}

	/**
	 * @brief この状態になった「瞬間」に1度だけ呼ばれる
	 * @param player 状態を変更するプレイヤー
	 */
	virtual void OnEnter(Player* player) = 0;

	/**
	 * @brief この状態の「間」ずっと（毎フレーム）呼ばれる
	 * @param player 状態を更新するプレイヤー
	 * @param tick 経過時間
	 */
	virtual void Update(Player* player, float tick) = 0;

	/**
	 * @brief この状態が（攻撃などで）割り込み可能か？
	 * @return true (割り込み可能), false (割り込み不可)
	 */
	virtual bool IsInterruptible() const { return true; }
};