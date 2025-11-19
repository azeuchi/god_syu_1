#ifndef __SCENE_DEBUG_H__
#define __SCENE_DEBUG_H__

#include "SceneBase.hpp"

class SceneDebug : public SceneBase
{
public:
	void Init();
	void Uninit();
	void Update(float tick);
	void Draw();

private:
	float m_fps = 0.0f;
	bool m_showImGui;

	// --- デバッグ用メンバー ---
	bool m_isPaused = false;            // アニメーション一時停止フラグ
	int m_selectedAnimIndex = 0;        // 選択中のアニメーションインデックス
	int m_currentFrame = 0;             // 現在のフレーム（ImGui表示・入力用）
	const char* m_animNames[4] = {      // 配列サイズを 3 -> 4 に変更
		"Idle", "Walk", "WalkBack", "LightPunch" // "Punch" を追加
	};

	void SavePlayerSettings();
	void DrawImGui();
};

#endif // __SCENE_DEBUG_H__