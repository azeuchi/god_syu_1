#include "UISprite.h"
#include "SimpleUI.h"

UISprite::UISprite()
	: m_texture(nullptr)
	, m_position(0.0f, 0.0f)
	, m_size(100.0f, 100.0f)
	, m_color(1.0f, 1.0f, 1.0f, 1.0f) // デフォルトは白（画像そのままの色）
	, m_isActive(true)
{
}

UISprite::~UISprite()
{
	if (m_texture)
	{
		delete m_texture;
		m_texture = nullptr;
	}
}

void UISprite::Init(const char* fileName, float x, float y, float width, float height)
{
	// テクスチャの読み込み
	m_texture = new Texture();

	m_texture->Create(fileName);

	m_position = { x, y };
	m_size = { width, height };
}

void UISprite::Draw()
{
	if (!m_isActive) return;

	// --- ピクセル座標 を NDC座標 (-1.0 ~ 1.0) に変換 ---
	// 左上原点のピクセル座標を、DirectXのクリップ空間座標に変換します

	// X: 0～1280 -> -1.0～1.0
	float ndcX = (m_position.x / SCREEN_WIDTH) * 2.0f - 1.0f;

	// Y: 0～720  ->  1.0～-1.0 (Y軸は上下反転)
	float ndcY = 1.0f - (m_position.y / SCREEN_HEIGHT) * 2.0f;

	// 幅・高さ: 画面に対する比率 * 2.0
	float ndcW = (m_size.x / SCREEN_WIDTH) * 2.0f;
	float ndcH = (m_size.y / SCREEN_HEIGHT) * 2.0f;

	// SimpleUI に登録
	SimpleUI::AddRect(ndcX, ndcY, ndcW, ndcH, m_color, m_texture);
}