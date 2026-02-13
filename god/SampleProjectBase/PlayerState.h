#pragma once

class Player;
// AttackParamsを引数で使うため前方宣言
struct AttackParams;

/**
 * @brief プレイヤーの状態のインターフェース（設計図）
 */
class PlayerState
{
public:
	virtual ~PlayerState() {}

	/**
	 * @brief この状態になった瞬間に1度だけ呼ばれる
	 */
	virtual void OnEnter(Player* player) = 0;

	/**
	 * @brief この状態の間ずっと（毎フレーム）呼ばれる
	 */
	virtual void Update(Player* player, float tick) = 0;

	/**
	 * @brief この状態が攻撃など割り込み可能か？
	 */
	virtual bool IsInterruptible() const { return true; }

	/**
	 * @brief この状態はしゃがみ扱いか？
	 * デフォルトはfalse。しゃがみステートでtrueを返す。
	 */
	virtual bool IsCrouch() const { return false; }

	/**
	 * @brief この状態は無敵か？ (ダウン中、起き上がり中など)
	 * デフォルトはfalse。
	 */
	virtual bool IsInvincible() const { return false; }

	/**
	 * @brief この状態は死亡状態か？
	 * 上書き防止チェックのために追加
	 */
	virtual bool IsDeathState() const { return false; }

protected:
	/**
	 * @brief キャンセル処理の共通化
	 * パラメータのキャンセル設定を見て、入力があれば次の技へ遷移する
	 * @return 遷移した場合は true を返す
	 */
	bool CheckCancel(Player* player, float stateTimer, const AttackParams& params);
};