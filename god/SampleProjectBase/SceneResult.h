#ifndef __SCENE_RESULT_H__
#define __SCENE_RESULT_H__

#include "SceneBase.hpp"
#include "Image2D.h" 

class SceneResult : public SceneBase
{
public:
	void Init() override;
	void Uninit() override;
	void Update(float tick) override;
	void Draw() override;

private:
	Image2D* m_pImage = nullptr; // 画像管理用ポインタ
	Image2D* m_returntitle = nullptr;
};

#endif // __SCENE_RESULT_H__