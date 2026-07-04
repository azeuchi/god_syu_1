#ifndef __SCENE_TRAINING_H__
#define __SCENE_TRAINING_H__

#include "SceneBase.hpp"
#include "Player.h"     // PlayerInputs
#include "HitEffectPool.h"
#include "SkyDome.h"
#include "PlayerRenderer.h"
#include <vector>
#include <d3d11.h>

/**
 * @brief トレーニングモード用シーン
 * 1Pを操作し、2Pをダミーとして「同じ技を振り続ける」「最初の1発以降ガードする」等を
 * 設定して挙動を確かめるための検証用シーン。
 */
class SceneTraining : public SceneBase
{
public:
	void Init();
	void Uninit();
	void Update(float tick);
	void Draw();

	// GUIボタンからのシーン遷移要求（SceneRootが監視する）
	static bool s_requestEnter;   // デバッグ画面 → トレーニング
	static bool s_requestGoGame;  // トレーニング → ゲーム
	static bool s_requestGoDebug; // トレーニング → デバッグ

private:
	void DrawImGui();
	void ResetPositions();
	// トレーニング設定からダミー(2P)の1フレーム分の入力を組み立てる
	PlayerInputs BuildDummyInputs(Player* dummy, float tick);

	// ヒットエフェクトのプール
	HitEffectPool m_hitEffects;

	// プレイヤー描画の共通ヘルパー
	PlayerRenderer m_playerRenderer;

	// 背景スカイドーム
	SkyDome* m_skyDome = nullptr;

	bool m_showImGui = true;

	// --- ダミー設定 ---
	int m_dummyStance = 0;   // 0:立ち 1:しゃがみ
	int m_dummyAction = 0;   // 0:なし 1:LP 2:MP 3:HP 4:MK 5:HK を繰り返す
	int m_dummyGuard = 0;    // 0:しない 1:常時 2:最初の1発以降
	bool m_infiniteHp = true;
	bool m_lockDummyPos = true;   // ダミーの位置を固定（コンボ確認用：離れないように）
	float m_dummyHomeX = 2.0f;    // 位置固定の基準X

	// 「最初の1発以降ガード」用の内部状態
	bool m_dummyHitLatch = false;       // 一度でも当てられたか
	float m_dummyNeutralTimer = 0.0f;   // 連続当たりが途切れてからの時間（ラッチ解除用）

	float m_repeatTimer = 0.0f;         // 攻撃を繰り返す間隔タイマー
	int m_hitCount = 0;                 // ダミーに当てた回数（ヒット/ガード合計）
	int m_blockCount = 0;               // うちガードされた回数
	bool m_lastBlocked = false;         // 直近の当たりがガードだったか
	int m_comboCount = 0;               // 現在のコンボ数（ヒット継続）
	int m_maxCombo = 0;                 // 最大コンボ
	float m_comboTimer = 0.0f;          // 直近ヒットからの経過（コンボ継続判定用）
	int m_lastStun = 0;                 // 直近に与えたのけぞり硬直（確認用）
};

#endif // __SCENE_TRAINING_H__
