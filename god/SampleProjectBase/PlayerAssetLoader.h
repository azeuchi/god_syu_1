#pragma once

class Player;

// プレイヤー共通のアセット読み込みヘルパー
namespace PlayerAssetLoader
{
	// 戦闘で使う共通アニメーションをまとめて読み込む（全シーン共通）
	void LoadCommonAnimations(Player* player);
}