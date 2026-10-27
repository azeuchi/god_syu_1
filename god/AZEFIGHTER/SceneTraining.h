#ifndef __SCENE_TRAINING_H__
#define __SCENE_TRAINING_H__

#include "SceneBase.hpp"
#include <memory>
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
	// 倒れた側を起こし、両者のHPを満タンに戻す
	void Revive(Player* p1, Player* p2);
	// トレーニング設定からダミー(2P)の1フレーム分の入力を組み立てる
	PlayerInputs BuildDummyInputs(Player* dummy, float tick);

	// ヒットエフェクトのプール
	HitEffectPool m_hitEffects;

	// プレイヤー描画の共通ヘルパー
	PlayerRenderer m_playerRenderer;

	// 背景スカイドーム
	std::unique_ptr<SkyDome> m_skyDome;

	// エフェクト描画用（カリングなし・半透明合成）
	ID3D11RasterizerState* m_pCullNone = nullptr;
	ID3D11BlendState* m_pBlendState = nullptr;

	bool m_showImGui = true;

	// ヒットストップと揺れ（ゲーム本編と同じ演出）
	float m_hitStopTimer = 0.0f;
	float m_shakeTimerP1 = 0.0f;
	float m_shakeTimerP2 = 0.0f;
	DirectX::XMFLOAT3 m_shakeOffsetP1 = { 0.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT3 m_shakeOffsetP2 = { 0.0f, 0.0f, 0.0f };
	float m_cameraTrauma = 0.0f;
	float m_cameraShakeKickDir = 0.0f;
	DirectX::XMFLOAT3 m_cameraShakeOffset = { 0.0f, 0.0f, 0.0f };

	// --- ダミー設定 ---
	int m_dummyStance = 0;   // 0:立ち 1:しゃがみ
	int m_dummyAction = 0;   // 0:なし 1:LP 2:MP 3:HP 4:MK 5:HK を繰り返す
	int m_dummyGuard = 0;    // 0:しない 1:常時 2:最初の1発以降
	bool m_infiniteHp = true;
	bool m_lockDummyPos = false;  // ダミーの位置を固定（コンボ確認用：離れないように）。既定は実戦どおり動かす
	float m_reviveTimer = 0.0f;   // 倒れてから起き上がるまでの残り時間
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
	int m_lastStun = 0;                 // 直近に与えたのけぞり硬直（確認用）
};

#endif // __SCENE_TRAINING_H__
