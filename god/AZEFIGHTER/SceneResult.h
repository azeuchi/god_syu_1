#ifndef __SCENE_RESULT_H__
#define __SCENE_RESULT_H__

#include "SceneBase.hpp"
#include <memory>
#include "Image2D.h" 
#include <d3d11.h>

class SceneResult : public SceneBase
{
public:
	void Init() override;
	void Uninit() override;
	void Update(float tick) override;
	void Draw() override;

private:
	std::unique_ptr<Image2D> m_pImage; // 画像管理用ポインタ
	std::unique_ptr<Image2D> m_returntitle;

	// 透過処理用
	ID3D11BlendState* m_pBlendState = nullptr;
	// 2D重ね合わせ用（奥行き判定無効化）
	ID3D11DepthStencilState* m_pDepthStateUI = nullptr;
};

#endif // __SCENE_RESULT_H__