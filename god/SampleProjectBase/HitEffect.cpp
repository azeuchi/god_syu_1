#include "HitEffect.h"
#include "DebugLog.h"
#include "Player.h"

static Texture* s_hitTexture = nullptr;

HitEffect::HitEffect()
	: m_isActive(false)
	, m_position({ 0,0,0 })
	, m_timer(0.0f)
	, m_frameIndex(0)
	, m_targetPlayer(nullptr)
{
	// 初回のみテクスチャ読み込み
	if (!s_hitTexture)
	{
		s_hitTexture = new Texture();
		s_hitTexture->Create("Assets/Texture/hit.png"); 
	}
}

HitEffect::~HitEffect()
{
}

void HitEffect::Activate(Player* target)
{
	m_isActive = true;
	m_targetPlayer = target; // ターゲットを記憶
	m_timer = 0.0f;
	m_frameIndex = 0;

	DebugLog::log(DebugLog::INFO_LOG, "HitEffect ACTIVATE! Pos:(%.2f, %.2f, %.2f)",
		m_position.x, m_position.y, m_position.z);

	if (m_targetPlayer)
	{
		m_position = m_targetPlayer->GetPosition();
		m_position.y += 1.5f; // 高さ調整
		m_position.z -= 0.5f; // 手前に出す
	}
}

void HitEffect::Update(float tick)
{
	if (!m_isActive) return;

	if (m_targetPlayer)
	{
		m_position = m_targetPlayer->GetPosition();
		m_position.y += 1.5f; // 高さ調整 
		m_position.z -= 0.5f; // 常に手前に
	}

	m_timer += tick;
	if (m_timer >= FRAME_TIME)
	{
		m_timer -= FRAME_TIME;
		m_frameIndex++;

		// 最後まで再生したら終了
		if (m_frameIndex >= MAX_FRAMES)
		{
			m_isActive = false;
			m_targetPlayer = nullptr;
		}
	}
}

void HitEffect::Draw(const DirectX::XMFLOAT4X4& view, const DirectX::XMFLOAT4X4& projection)
{
	if (!m_isActive) return;

	// 1. Spriteクラスへの設定
	Sprite::SetView(view);
	Sprite::SetProjection(projection);
	Sprite::SetTexture(s_hitTexture);

	// ただ単に「拡大」して「移動」するだけ
	DirectX::XMMATRIX mScale = DirectX::XMMatrixScaling(SPRITE_SIZE, SPRITE_SIZE, 1.0f);
	DirectX::XMMATRIX mTrans = DirectX::XMMatrixTranslation(m_position.x, m_position.y, m_position.z);

	// 回転などを挟まず、そのまま合成
	DirectX::XMMATRIX mWorld = mScale * mTrans;

	// 行列の転置 
	mWorld = DirectX::XMMatrixTranspose(mWorld);

	DirectX::XMFLOAT4X4 world;
	DirectX::XMStoreFloat4x4(&world, mWorld);
	Sprite::SetWorld(world);

	//  位置ズレ防止のリセット
	Sprite::SetOffset({ 0.0f, 0.0f });
	Sprite::SetSize({ 1.0f, 1.0f });

	// アニメーション設定
	float unitU = 1.0f / (float)MAX_FRAMES;
	Sprite::SetUVScale({ unitU, 1.0f });
	Sprite::SetUVPos({ unitU * m_frameIndex, 0.0f });
	Sprite::SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });

	// 描画
	Sprite::Draw();

	// 後始末
	Sprite::SetUVScale({ 1.0f, 1.0f });
	Sprite::SetUVPos({ 0.0f, 0.0f });
}