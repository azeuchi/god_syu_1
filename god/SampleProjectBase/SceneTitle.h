#ifndef __SCENE_TITLE_H__
#define __SCENE_TITLE_H__

#include "SceneBase.hpp"
#include "Image2D.h"
#include "Model.h"
#include "SkyDome.h"
#include <string>
#include <vector>
#include <d3d11.h>

struct Particle
{
	float x, y;
	float speed;
	float size;
	float life;
	float maxLife;
	float drift;
};

class SceneTitle : public SceneBase
{
public:
	void Init() override;
	void Uninit() override;
	void Update(float tick) override;
	void Draw() override;

private:
	// 画像管理用ポインタ
	Image2D* m_pImage = nullptr;
	Image2D* m_PressEnter = nullptr;
	float m_blinkTimer = 0.0f;

	// パーティクル管理用
	Image2D* m_pParticleImg = nullptr;
	std::vector<Particle> m_particles;
	float m_spawnTimer = 0.0f;

	// 3D演出用ポインタ
	Model* m_pTitleModel = nullptr;
	SkyDome* m_skyDome = nullptr;

	// アニメーション制御
	std::string m_currentAnimName;
	float m_currentFrame;
	bool m_isLoop;
	float m_actionTimer;

	// カメラ制御
	float m_cameraAngle = 0.0f;

	// 描画ステート管理
	ID3D11BlendState* m_pBlendState = nullptr;
	ID3D11DepthStencilState* m_pDepthStateUI = nullptr;
	ID3D11DepthStencilState* m_pDepthState3D = nullptr;
};

#endif // __SCENE_TITLE_H__