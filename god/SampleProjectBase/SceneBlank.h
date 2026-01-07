#pragma once
#include "SceneBase.hpp"
#include "Image2D.h"
#include "SkyDome.h"
#include <DirectXMath.h> // XMFLOAT2用
#include <d3d11.h>       // DirectX用

/**
 * @brief メインのゲームシーン
 * 格闘ゲームの本番動作用シーン
 */
class SceneBlank : public SceneBase
{
public:
	void Init();   // 初期化
	void Uninit(); // 終了
	void Update(float tick); // 更新
	void Draw();   // 描画

	// SceneRootから参照するための静的フラグ
	static bool s_isGameSet;

private:
	// 次のラウンドのために位置やHPをリセットする関数
	void ResetRound();

private:
	// HPバー表示用クラス
	Image2D* m_hpBar;
	Image2D* m_enemyhpBar;
	SkyDome* m_skyDome;

	// フェード用画像
	Image2D* m_fadeBlack;

	// ラウンド開始演出用画像
	Image2D* m_imgRound1 = nullptr;
	Image2D* m_imgRound2 = nullptr;
	Image2D* m_imgFinalRound = nullptr;
	Image2D* m_imgFight = nullptr;

	// 演出用ステート定義
	enum class RoundPhase {
		READY,      // 開始前の溜め (ラウンド1のみ使用)
		ROUND_CALL, // ラウンド数表示
		FIGHT_CALL, // FIGHT表示
		PLAYING     // 試合中
	};

	RoundPhase m_currentPhase = RoundPhase::READY;
	float m_phaseTimer = 0.0f;

	// HPバーの初期位置・サイズを記憶しておく変数
	DirectX::XMFLOAT2 m_hpBarPos;      // 1Pの初期座標
	DirectX::XMFLOAT2 m_enemyHpBarPos; // 2Pの初期座標
	float m_barMaxWidth = 500.0f;      // バーの最大幅 (満タン時)

	// 描画設定
	ID3D11DepthStencilState* m_pDepthState = nullptr;   // スカイドーム用
	ID3D11DepthStencilState* m_pDepthStateUI = nullptr; // UI用 (Depth ON, ALWAYS)
	ID3D11BlendState* m_pBlendState = nullptr;          // UI用 (AlphaToCoverage)

	// ヒットストップ演出用タイマー
	float m_hitStopTimer = 0.0f;

	// ヒットシェイク（振動）制御用
	float m_shakeTimerP1 = 0.0f; // 1Pの振動残り時間
	float m_shakeTimerP2 = 0.0f; // 2Pの振動残り時間
	DirectX::XMFLOAT3 m_shakeOffsetP1 = { 0.0f, 0.0f, 0.0f }; // 1Pの描画ずらし量
	DirectX::XMFLOAT3 m_shakeOffsetP2 = { 0.0f, 0.0f, 0.0f }; // 2Pの描画ずらし量

	// ラウンド管理用
	int m_winCountP1 = 0;      // 1Pの勝利ラウンド数
	int m_winCountP2 = 0;      // 2Pの勝利ラウンド数
	bool m_isRoundOver = false; // ラウンドが終わったかどうかのフラグ
	float m_roundEndTimer = 0.0f; // ラウンド終了後の待機タイマー (2秒)
};