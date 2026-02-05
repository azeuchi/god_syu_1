#pragma once
#include "Player.h"

/**
 * @brief プレイヤーのパラメータ（設定ファイル）の読み込みと管理を行うクラス
 */
class PlayerParameterLoader
{
public:
	/**
	 * @brief 指定されたファイルからパラメータを読み込み、プレイヤーに適用する
	 * @param player 設定を適用するプレイヤー
	 */
	static void LoadSettings(Player* player);

	static void LoadCommonSettings(Player* player, const char* filePath);
	static void LoadAttackParams(AttackParams& params, const char* filePath);

	/**
	 * @brief 基準となるプレイヤー(src)から、別のプレイヤー(dst)へパラメータをコピーする
	 * (攻撃データや当たり判定などを同期するために使用)
	 * @param src コピー元 (P1)
	 * @param dst コピー先 (P2)
	 */
	static void CopyParameters(Player* src, Player* dst);
};