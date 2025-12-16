#ifndef __SCENE_TITLE_H__
#define __SCENE_TITLE_H__

#include "SceneBase.hpp"
#include "Image2D.h" 

class SceneTitle : public SceneBase
{
public:
	void Init() override;
	void Uninit() override;
	void Update(float tick) override;
	void Draw() override;

private:
	Image2D* m_pImage = nullptr; // 画像管理用ポインタ
	Image2D* m_background = nullptr; // 背景画像管理用ポインタ
	Image2D* m_PressEnter = nullptr; // エンターキー
};

#endif // __SCENE_TITLE_H__