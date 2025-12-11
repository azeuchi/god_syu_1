#pragma once

class Player;

/**
 * @brief プレイヤーの状態のインターフェース（設計図）
 */
class PlayerState
{
public:
	virtual ~PlayerState() {}

	/**
	 * @brief この状態になった「瞬間」に1度だけ呼ばれる
	 */
	virtual void OnEnter(Player* player) = 0;

	/**
	 * @brief この状態の「間」ずっと（毎フレーム）呼ばれる
	 */
	virtual void Update(Player* player, float tick) = 0;

	/**
	 * @brief この状態が（攻撃などで）割り込み可能か？
	 */
	virtual bool IsInterruptible() const { return true; }

	/**
	 * @brief この状態はしゃがみ扱いか？
	 * デフォルトはfalse。しゃがみステートでtrueを返す。
	 */
	virtual bool IsCrouch() const { return false; }
};