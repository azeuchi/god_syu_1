#ifndef __SCENE_BLANK_H__
#define __SCENE_BLANK_H__

#include "SceneBase.hpp"
#include "Ball.h"
#include <vector>

class SceneBlank : public SceneBase
{
public:
	void Init();
	void Uninit();
	void Update(float tick);
	void Draw();
private:
	
	float m_fps = 0.0f;

	// ImGui‚Ì•\Ž¦ƒtƒ‰ƒO
	bool m_showImGui;

	void SavePlayerSettings();
	void DrawImGui();
};

#endif // __SCENE_BLANK_H___