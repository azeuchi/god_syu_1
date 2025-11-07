#ifndef __SCENE_BLANK_H__
#define __SCENE_BLANK_H__

#include "SceneBase.hpp"
#include "Ball.h"
#include <vector>

enum class PlayerState
{
	IDLE,
	WALKING,
	WALKING_BACK // Œã‘Şó‘Ô
};

class SceneBlank : public SceneBase
{
public:
	void Init();
	void Uninit();
	void Update(float tick);
	void Draw();
private:
	struct AnimationState
	{
		const char* name = nullptr;
		int frame = 0;
	};

	AnimationState m_currentState;
	AnimationState m_previousState;

	float m_blendFactor = 1.0f;
	const float m_transitionDuration = 0.2f;

	float m_fps = 0.0f;

	void SavePlayerSettings();

	// ImGui‚Ì•`‰æˆ—‚ğ‚Ü‚Æ‚ß‚éŠÖ”
	void DrawImGui();
};

#endif // __SCENE_BLANK_H___