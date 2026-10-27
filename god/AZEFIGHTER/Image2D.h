#pragma once
#include "Texture.h"
#include <memory>
#include <DirectXMath.h>

class Image2D
{
public:
	Image2D();
	~Image2D();

	/**
	 * @brief 画像を読み込んで設定する
	 * @param filePath 画像ファイルのパス (例: "Assets/Texture/hp.png")
	 * @param x 画面のX座標 (ピクセル)
	 * @param y 画面のY座標 (ピクセル)
	 * @param w 画像の幅 (ピクセル)
	 * @param h 画像の高さ (ピクセル)
	 */
	void Load(const char* filePath, float x, float y, float w, float h);

	/**
	 * @brief 描画登録 (UpdateかDrawの中で呼ぶ)
	 */
	void Draw();

	// 場所・サイズ・透明度を指定して描画する (パーティクル等で使用)
	void Draw(float x, float y, float w, float h, float alpha);

	// 位置やサイズを後から変えたい場合用
	void SetPosition(float x, float y) { m_x = x; m_y = y; }
	void SetSize(float w, float h) { m_width = w; m_height = h; }

	// 色や透明度を変更する関数
	void SetColor(float r, float g, float b, float a) { m_color = { r, g, b, a }; }

	// 画像のどこを切り出して貼るか（0?1）。HPバーの減った分を削るのに使う
	void SetUV(float u, float v, float uw, float vh) { m_uvPos = { u, v }; m_uvScale = { uw, vh }; }

private:
	std::unique_ptr<Texture> m_texture;
	float m_x, m_y;
	float m_width, m_height;
	DirectX::XMFLOAT4 m_color; // 色保存用
	DirectX::XMFLOAT2 m_uvPos = { 0.0f, 0.0f };   // 切り出し開始位置
	DirectX::XMFLOAT2 m_uvScale = { 1.0f, 1.0f }; // 切り出し範囲

	const float SCREEN_W = 1280.0f;
	const float SCREEN_H = 720.0f;
};