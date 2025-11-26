#pragma once
#include "SceneBase.hpp"
#include "Image2D.h"
/**
 * @brief メインのゲームシーン
 */
class SceneBlank : public SceneBase
{
public:
	void Init();
	void Uninit();
	void Update(float tick);
	void Draw();

private:
	Image2D* m_hpBar;
	Image2D* m_enemyhpBar;
};