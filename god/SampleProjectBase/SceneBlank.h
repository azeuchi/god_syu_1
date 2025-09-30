#ifndef __SCENE_BLANK_H__
#define __SCENE_BLANK_H__

#include "SceneBase.hpp"
#include "Ball.h"
#include <vector>

// プレイヤーの状態を定義するenum
enum class PlayerState
{
	IDLE,
	WALKING,
};

class SceneBlank : public SceneBase
{
public:
	void Init();
	void Uninit();
	void Update(float tick);
	void Draw();
private:
	//各アニメーションのフレーム数を管理する変数に変更
	int m_idleFrame = 0;
	int m_walkFrame = 0;

	// プレイヤーの現在の状態を保持する変数
	PlayerState m_playerState = PlayerState::IDLE;
};

#endif // __SCENE_BLANK_H___