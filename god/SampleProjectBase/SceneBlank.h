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
	ID3D11DepthStencilState* m_pDepthState = nullptr;         // スカイドーム用（Z書き込みあり）
	ID3D11DepthStencilState* m_pDepthStateNoWrite = nullptr; // 影・床用（Z書き込みなし）
	ID3D11BlendState* m_pBlendState = nullptr;                // 半透明用

	// アウトライン・カリング設定用ラスタライザーステート
	ID3D11RasterizerState* m_pCullFront = nullptr;      // 表面カリング（P1アウトライン、P2通常用）
	ID3D11RasterizerState* m_pCullBack = nullptr;       // 裏面カリング（P1通常、P2アウトライン用）
	ID3D11RasterizerState* m_pCullNone = nullptr;       // カリングなし（スカイドーム用）

	// シャドウマップ・床用設定
	ID3D11Texture2D* m_shadowMapTex = nullptr;
	ID3D11RenderTargetView* m_shadowRTV = nullptr;
	ID3D11ShaderResourceView* m_shadowSRV = nullptr;
	ID3D11Texture2D* m_shadowDepthTex = nullptr;
	ID3D11DepthStencilView* m_shadowDSV = nullptr;
	ID3D11SamplerState* m_pSamplerState = nullptr;      // 影用サンプラー
	D3D11_VIEWPORT m_shadowViewport = {};
	ID3D11BlendState* m_pMultiplyBlend = nullptr;

	ID3D11Buffer* m_quadVB = nullptr; // 床のポリゴン用

	struct SpriteParam {
		DirectX::XMFLOAT4 offset_size;
		DirectX::XMFLOAT4 uvPos_scale;
		DirectX::XMFLOAT4 color;
	};

	struct ShadowVertex {
		DirectX::XMFLOAT3 pos;
		DirectX::XMFLOAT2 uv;
	};

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

	// KO演出用
	bool m_isKOStage = false; // 現在KO演出フェーズかどうか

	// スローモーション演出用
	bool m_isSlowMotion = false;        // スローモーション中か
	float m_slowMotionTimer = 0.0f;     // スローモーション残り時間
	float m_slowMotionDuration = 0.0f;  // スローモーションの全体時間（補間計算用）

	// KO時のカメラズーム用
	DirectX::XMFLOAT3 m_cameraZoomStartPos;  // ズーム開始地点
	DirectX::XMFLOAT3 m_cameraZoomStartLook; // ズーム開始時の注視点
	DirectX::XMFLOAT3 m_cameraZoomTargetPos; // ズーム目標地点
	DirectX::XMFLOAT3 m_cameraZoomTargetLook;// ズーム目標注視点
};