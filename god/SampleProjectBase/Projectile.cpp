#include "Projectile.h"
#include "Player.h"
#include "Sprite.h"

Texture* Projectile::s_texture = nullptr;

Projectile::Projectile()
	: m_isActive(false)
	, m_owner(nullptr)
	, m_position({ 0,0,0 })
	, m_speed(0.0f)
	, m_damage(0)
	, m_size(1.0f)
	, m_isRight(true)
{
	if (!s_texture)
	{
		s_texture = new Texture();
		s_texture->Create("Assets/Texture/particle.png");
	}
}

Projectile::~Projectile()
{
}

void Projectile::Launch(Player* owner, float speed, int damage, float size, bool isRight)
{
	m_owner = owner;
	m_speed = speed;
	m_damage = damage;
	m_size = size;
	m_isRight = isRight;
	m_isActive = true;

	// プレイヤーの少し前から発射
	m_position = owner->GetPosition();
	m_position.y += 1.2f; // 胸の高さ
	m_position.z = 0.0f;
}

void Projectile::Update(float tick)
{
	if (!m_isActive) return;

	// 移動
	float move = m_speed * tick;
	if (m_isRight) m_position.x += move;
	else m_position.x -= move;

	// 当たり判定の更新
	m_hitbox.Center = m_position;
	m_hitbox.Extents = { m_size * 0.5f, m_size * 0.5f, 0.1f };

	// カメラ外判定（簡易的に原点からの距離）
	if (fabsf(m_position.x) > CAMERA_OUT_LIMIT)
	{
		m_isActive = false;
	}
}

void Projectile::Draw(const DirectX::XMFLOAT4X4& view, const DirectX::XMFLOAT4X4& projection)
{
	if (!m_isActive) return;

	Sprite::SetView(view);
	Sprite::SetProjection(projection);
	Sprite::SetTexture(s_texture);

	// 行列計算
	DirectX::XMMATRIX mScale = DirectX::XMMatrixScaling(m_size, m_size, 1.0f);
	DirectX::XMMATRIX mTrans = DirectX::XMMatrixTranslation(m_position.x, m_position.y, m_position.z);
	DirectX::XMMATRIX mWorld = mScale * mTrans;

	mWorld = DirectX::XMMatrixTranspose(mWorld);

	DirectX::XMFLOAT4X4 world;
	DirectX::XMStoreFloat4x4(&world, mWorld);
	Sprite::SetWorld(world);

	Sprite::SetOffset({ 0.0f, 0.0f });
	Sprite::SetSize({ 1.0f, 1.0f });
	Sprite::SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });

	Sprite::Draw();
}