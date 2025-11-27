#pragma once
#include "SceneBase.hpp"
#include "Image2D.h"
#include <DirectXMath.h> // XMFLOAT2用

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

	// 画像を追加する関数
	void AddUI(const char* filePath, float x, float y, float w, float h);

private:
	Image2D* m_hpBar;
	Image2D* m_enemyhpBar;

	//HPバーの初期位置・サイズを記憶しておく変数
	DirectX::XMFLOAT2 m_hpBarPos;      // 1Pの初期座標
	DirectX::XMFLOAT2 m_enemyHpBarPos; // 2Pの初期座標
	float m_barMaxWidth = 500.0f;      // バーの最大幅
};