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
	int m_Frame = 0;
	bool m_playIdle = false;
};

#endif // __SCENE_BLANK_H___