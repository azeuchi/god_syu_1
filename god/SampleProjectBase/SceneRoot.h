#ifndef __SCENE_ROOT_H__
#define __SCENE_ROOT_H__

#include "SceneBase.hpp"

class SceneRoot : public SceneBase
{
public:
	void Init();
	void Uninit();
	void Update(float tick);
	void Draw();
	bool isSceneChange();
	std::string GetSceneName();

private:
	// 内部処理用：シーン構築
	void ChangeScene();

	// シーン遷移を簡単にするためのヘルパー関数
	// 引数に次のシーンの種類を渡す
	void Transition(int nextScene);

private:
	int m_index = 0;
	std::string m_sceneName;
	bool m_isChangeScene = false;
};

#endif // __SCENE_ROOT_H__