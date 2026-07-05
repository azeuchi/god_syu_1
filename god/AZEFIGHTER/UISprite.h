#pragma once
#include <DirectXMath.h>
#include "Texture.h"

class UISprite
{
public:
	UISprite();
	~UISprite();

	/**
	 * @brief UIの初期化
	 * @param fileName 画像ファイルパス (例: "Assets/Texture/hp_bar.png")
	 * @param x 画面上のX座標 (ピクセル)
	 * @param y 画面上のY座標 (ピクセル)
	 * @param width 幅 (ピクセル)
	 * @param height 高さ (ピクセル)
	 */
	void Init(const char* fileName, float x, float y, float width, float height);

	/**
	 * @brief 描画登録 (UpdateかDrawの中で呼ぶ)
	 */
	void Draw();

	// --- 値の変更用 ---
	void SetPosition(float x, float y) { m_position = { x, y }; }
	void SetSize(float w, float h) { m_size = { w, h }; }
	void SetColor(float r, float g, float b, float a) { m_color = { r, g, b, a }; }
	void SetActive(bool active) { m_isActive = active; } // 表示/非表示切り替え

private:
	Texture* m_texture;
	DirectX::XMFLOAT2 m_position;
	DirectX::XMFLOAT2 m_size;
	DirectX::XMFLOAT4 m_color;
	bool m_isActive;

	// 画面サイズ
	const float SCREEN_WIDTH = 1280.0f;
	const float SCREEN_HEIGHT = 720.0f;
};