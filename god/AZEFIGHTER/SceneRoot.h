#ifndef __SCENE_ROOT_H__
#define __SCENE_ROOT_H__

#include "SceneBase.hpp"
#include <d3d11.h>

class SceneRoot : public SceneBase
{
public:
	void Init();
	void Uninit();
	void Update(float tick);
	void Draw();
	bool isSceneChange();
	std::string GetSceneName();

	// ゲーム内の演出から画面全体を暗転させる値。ラウンド切替で使用
	static float s_sceneFade;

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

	// シーン遷移フェード
	float m_fadeAlpha = 0.0f; // 現在の暗転の濃さ
	int m_pendingScene = -1;  // フェード後に切り替えるシーン。-1なら待ち無し
	ID3D11BlendState* m_pFadeBlend = nullptr;
	ID3D11DepthStencilState* m_pFadeDepth = nullptr;
};

#endif // __SCENE_ROOT_H__