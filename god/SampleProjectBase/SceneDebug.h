#ifndef __SCENE_DEBUG_H__
#define __SCENE_DEBUG_H__

#include "SceneBase.hpp"

/**
 * @brief デバッグ調整用シーン
 * * ゲームのステートマシン(FSM)を使わず、手動でアニメーションや判定を制御するシーン。
 * 以下の機能を持つ:
 * 1. 攻撃技のパラメータ調整 (発生フレーム、持続、硬直、判定サイズなど)
 * 2. プレイヤーの基本「くらい判定(Hurtbox)」の調整
 * 3. アニメーションのコマ送り確認
 * 4. 調整結果のファイル保存/読み込み
 */
class SceneDebug : public SceneBase
{
public:
	// --- シーンライフサイクル ---
	void Init();   // 初期化 (リソース読み込み、変数の初期化)
	void Uninit(); // 終了処理 (リソース解放)
	void Update(float tick); // 更新処理 (アニメーション進行、入力処理)
	void Draw();   // 描画処理 (モデル描画、GUI描画)

private:
	float m_fps = 0.0f; // 現在のFPS
	bool m_showImGui;   // GUIの表示/非表示フラグ

	// --- デバッグ制御用メンバ変数 ---

	// 現在、攻撃アニメーションを再生中かどうか
	// (trueなら攻撃判定の描画や、終了フレームの監視を行う)
	bool m_isAttacking = false;

	// アニメーション一時停止フラグ
	// (trueならアニメーションを進めない、またはコマ送りモード)
	bool m_isPaused = false;

	// 現在の再生フレーム数 (ImGuiでの表示・操作用)
	// Playerクラス内部の float フレームとは別に、整数で管理して同期させる
	int m_currentFrame = 0;

	// アニメーション再生速度制御用タイマー
	// デバッグシーンは可変フレームレートで動くが、アニメーションは60FPS基準で
	// 正確にコマ送りしたいため、時間を積算して制御する
	float m_animTimer = 0.0f;

	// --- 内部関数 ---

	// 現在のプレイヤー設定を "player_settings.ini" に保存する
	void SavePlayerSettings();

	// 調整用GUI (ImGui) の描画を行う
	void DrawImGui();
};

#endif // __SCENE_DEBUG_H__