#pragma once
#include "SceneBase.hpp"

/**
 * @brief メインのゲームシーン (GUIなし)
 */
class SceneBlank : public SceneBase
{
public:
	void Init();
	void Uninit();
	void Update(float tick);
	void Draw();

private:
	
};