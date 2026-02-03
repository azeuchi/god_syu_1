#pragma once
#include "Image2D.h"
#include <DirectXMath.h>
#include <d3d11.h>

// 演出用ステート定義
enum class RoundPhase {
	READY,      // 開始前の溜め (ラウンド1のみ使用)
	ROUND_CALL, // ラウンド数表示
	FIGHT_CALL, // FIGHT表示
	PLAYING     // 試合中
};

class BattleUIManager
{
public:
	BattleUIManager();
	~BattleUIManager();

	void Init();
	void Uninit();

	// ラウンド開始時のリセット
	void Reset();

	// HPバーの更新
	void UpdateHPBars(float p1Ratio, float p2Ratio);

	// フェード状態の描画設定（ラウンド終了時など）
	void SetFadeAlpha(float alpha);

	// 描画
	void Draw(RoundPhase currentPhase, int winCountP1, int winCountP2);

private:
	// HPバー
	Image2D* m_hpBar;
	Image2D* m_enemyhpBar;

	Image2D* m_hpFrame;
	Image2D* m_enemyhpFrame;

	DirectX::XMFLOAT2 m_hpBarPos;      // 1Pの初期座標
	DirectX::XMFLOAT2 m_enemyHpBarPos; // 2Pの初期座標

	DirectX::XMFLOAT2 m_hpFramePos;      // 1Pの初期座標
	DirectX::XMFLOAT2 m_enemyHpFramerPos; // 2Pの初期座標

	float m_barMaxWidth;               // バーの最大幅
	float m_barHeight;                    // HPバーの高さを管理する変数

	// フェード用画像
	Image2D* m_fadeBlack;
	float m_currentFadeAlpha;          // フェードの現在の透明度を保持

	// ラウンド開始演出用画像
	Image2D* m_imgRound1;
	Image2D* m_imgRound2;
	Image2D* m_imgFinalRound;
	Image2D* m_imgFight;

	// 描画設定
	ID3D11DepthStencilState* m_pDepthStateUI; // UI用 (Depth ON, ALWAYS)
	ID3D11BlendState* m_pBlendState;          // UI用 (AlphaToCoverage)
};