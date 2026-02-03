#pragma once
#include "SceneBase.hpp"
#include "SkyDome.h"
#include <DirectXMath.h> 
#include <d3d11.h>       
#include "HitEffect.h"
#include "BattleUIManager.h" // UI管理クラス
#include "BattleCollision.h" // 衝突判定管理クラス

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
	// UIマネージャ
	BattleUIManager* m_uiManager;

	SkyDome* m_skyDome;

	//エフェクト管理用リスト
	std::vector<HitEffect*> m_hitEffects;

	RoundPhase m_currentPhase = RoundPhase::READY;
	float m_phaseTimer = 0.0f;

	// 描画設定
	ID3D11DepthStencilState* m_pDepthState = nullptr;   // スカイドーム用
	ID3D11BlendState* m_pBlendState = nullptr;          // 半透明用

	// アウトライン描画用ラスタライザーステート
	ID3D11RasterizerState* m_pCullFront = nullptr;      // 表面カリング（アウトライン用）
	ID3D11RasterizerState* m_pCullBack = nullptr;       // 裏面カリング（通常描画用）

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