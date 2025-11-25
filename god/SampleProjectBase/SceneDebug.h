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
	bool m_isAttacking = false;         // 攻撃アニメーション再生中フラグ
	int m_currentFrame = 0;             // 現在のフレーム

	void SavePlayerSettings();
	void DrawImGui();
};

#endif // __SCENE_DEBUG_H__