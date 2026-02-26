#pragma once
#include "SceneBase.hpp"
#include <vector>

// 前方宣言
class HitEffect;

class SceneKeyConfig : public SceneBase
{
public:
	void Init();
	void Uninit();
	void Update(float tick);
	void Draw();

	static bool s_isConfigSet;

private:
	void DrawImGui();
	void DrawKeyBindButton(const char* label, int* key);
	const char* GetKeyName(int vk);

	int* m_waitBindKeyPtr = nullptr;

	// エフェクト管理用リスト（攻撃を当てた時の確認用）
	std::vector<HitEffect*> m_hitEffects;
};