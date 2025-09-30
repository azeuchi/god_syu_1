#ifndef __SCENE_BLANK_H__
#define __SCENE_BLANK_H__

#include "SceneBase.hpp"
#include "Ball.h"
#include <vector>

//変更: プレイヤーの状態enumはシンプルに
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
	// アニメーションの状態を管理する構造体
	struct AnimationState
	{
		const char* name = nullptr;
		int frame = 0;
	};

	// アニメーション管理変数を刷新
	AnimationState m_currentState;      // 現在のアニメーション
	AnimationState m_previousState;     // 遷移前のアニメーション

	float m_blendFactor = 1.0f;         // ブレンド率 (0.0: previous -> 1.0: current)
	const float m_transitionDuration = 0.2f; // アニメーション遷移にかかる時間（秒）
};

#endif // __SCENE_BLANK_H___