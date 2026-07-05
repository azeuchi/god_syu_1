#pragma once

class Player;

// プレイヤー共通のアセット読み込みヘルパー
namespace PlayerAssetLoader
{
	// 戦闘で使う共通アニメーションをまとめて読み込む
	void LoadCommonAnimations(Player* player);
}