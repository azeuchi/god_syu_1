#include "Image2D.h"
#include "SimpleUI.h" 
#include "DebugLog.h"

Image2D::Image2D()
	: m_texture(nullptr)
	, m_x(0.0f), m_y(0.0f)
	, m_width(100.0f), m_height(100.0f)
{
}

Image2D::~Image2D()
{
	// 使い終わったらテクスチャをメモリから削除
	if (m_texture)
	{
		delete m_texture;
		m_texture = nullptr;
	}
}

void Image2D::Load(const char* filePath, float x, float y, float w, float h)
{
	m_x = x;
	m_y = y;
	m_width = w;
	m_height = h;

	// テクスチャ読み込み
	m_texture = new Texture();
	if (FAILED(m_texture->Create(filePath)))
	{
	
	}
}

void Image2D::Draw()
{
	if (!m_texture) return;

	// --- 座標変換 (ピクセル -> NDC) ---
	// 画面左上(0,0) を基準に計算します
	float ndcX = (m_x / SCREEN_W) * 2.0f - 1.0f;
	float ndcY = 1.0f - (m_y / SCREEN_H) * 2.0f;
	float ndcW = (m_width / SCREEN_W) * 2.0f;
	float ndcH = (m_height / SCREEN_H) * 2.0f;

	// SimpleUIに登録
	// 色は白 {1,1,1,1} (画像そのままの色)
	SimpleUI::AddRect(ndcX, ndcY, ndcW, ndcH, { 1.0f, 1.0f, 1.0f, 1.0f }, m_texture);
}