#include "Projectile.h"
#include "Player.h"
#include "Sprite.h"
#include "Geometory.h" // ・ｽ・ｽ・ｽ・ｽ・ｽ阡ｻ・ｽ・ｽ`・ｽ・ｽp・ｽﾉ追会ｿｽ
#include <stdlib.h>
#include <math.h>

Texture* Projectile::s_texture = nullptr;
Texture* Projectile::s_softTex = nullptr;

namespace
{
	// 0.0・ｽ・ｽ・ｽ・ｽ1.0・ｽﾌ暦ｿｽ・ｽ・ｽ・ｽi・ｽp・ｽ[・ｽe・ｽB・ｽN・ｽ・ｽ・ｽﾌ散・ｽ・ｽﾎゑｿｽp・ｽj
	float Rand01() { return (float)rand() / (float)RAND_MAX; }
}

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

	// パーティクル用の丸いぼかしテクスチャをコードで生成する
	// （中心が不透明で外へ向かって透明になる白い円。色は描画時に付ける）
	if (!s_softTex)
	{
		const int W = 64;
		static BYTE pixels[W * W * 4];
		for (int y = 0; y < W; ++y)
		{
			for (int x = 0; x < W; ++x)
			{
				float dx = (x + 0.5f) / W * 2.0f - 1.0f;
				float dy = (y + 0.5f) / W * 2.0f - 1.0f;
				float r = sqrtf(dx * dx + dy * dy);
				float fall = 1.0f - r;
				if (fall < 0.0f) fall = 0.0f;
				fall *= 1.8f; if (fall > 1.0f) fall = 1.0f; // 中心を不透明に飽和させる（半透明破棄対策）
				BYTE a = (BYTE)(fall * 255.0f);
				BYTE* px = &pixels[(y * W + x) * 4];
				px[0] = 255; px[1] = 255; px[2] = 255; px[3] = a;
			}
		}
		s_softTex = new Texture();
		s_softTex->Create(DXGI_FORMAT_R8G8B8A8_UNORM, W, W, pixels);
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
	m_spawnTimer = 0.0f;

	// ・ｽv・ｽ・ｽ・ｽC・ｽ・ｽ・ｽ[・ｽﾌ擾ｿｽ・ｽ・ｽ・ｽO・ｽ・ｽ・ｽ逕ｭ・ｽ・ｽ
	m_position = owner->GetPosition();
	m_position.y += 1.2f; // ・ｽ・ｽﾌ搾ｿｽ・ｽ・ｽ
	m_position.z = 0.0f;
}

void Projectile::Update(float tick)
{
	// ・ｽp・ｽ[・ｽe・ｽB・ｽN・ｽ・ｽ・ｽﾍ弾・ｽ・ｽ・ｽ・ｽ・ｽ・ｽ・ｽ・ｽ・ｽ・ｽ・ｽ・ｽc・ｽ・ｽ・ｽ・ｽX・ｽV・ｽ・ｽ・ｽﾄ趣ｿｽ・ｽR・ｽﾉ擾ｿｽ・ｽ・ｽ
	for (auto& p : m_particles)
	{
		if (p.life <= 0.0f) continue;
		p.life -= tick;
		p.pos.x += p.vel.x * tick;
		p.pos.y += p.vel.y * tick;
	}

	if (!m_isActive) return;

	// ・ｽﾚ難ｿｽ
	float move = m_speed * tick;
	if (m_isRight) m_position.x += move;
	else m_position.x -= move;

	// ・ｽ・ｽ・ｽ・ｽ・ｽ阡ｻ・ｽ・ｽﾌ更・ｽV
	m_hitbox.Center = m_position;
	m_hitbox.Extents = { m_size * 0.5f, m_size * 0.5f, 0.1f };

	// ・ｽe・ｽﾌ鯉ｿｽ・ｽ・ｽ・ｽ・ｽ・ｽp・ｽ[・ｽe・ｽB・ｽN・ｽ・ｽ・ｽｭ撰ｿｽ・ｽ・ｽ・ｽ・ｽ・ｽ・ｽ
	const float dir = m_isRight ? 1.0f : -1.0f;
	m_spawnTimer -= tick;
	while (m_spawnTimer <= 0.0f)
	{
		m_spawnTimer += 0.016f; // ・ｽ・ｽ・ｽ謔ｻ・ｽ・ｽ・ｽt・ｽ・ｽ・ｽ[・ｽ・ｽ1・ｽ・ｽ
		for (auto& p : m_particles)
		{
			if (p.life > 0.0f) continue;
			// ・ｽe・ｽﾌ鯉ｿｽ・ｽ[・ｽ・ｽ・ｽ迴ｭ・ｽ・ｽ・ｽU・ｽ轤ｵ・ｽﾄ出・ｽ・ｽ
			p.pos = { m_position.x - dir * m_size * 0.4f,
					  m_position.y + (Rand01() - 0.5f) * m_size * 0.5f,
					  m_position.z };
			// ・ｽ・ｽ・ｽ・ｽﾖ暦ｿｽ・ｽ・ｽﾂつ上下・ｽﾉ軽・ｽ・ｽ・ｽﾎらけ・ｽ・ｽ
			p.vel = { -dir * (0.8f + Rand01() * 1.2f),
					  (Rand01() - 0.5f) * 0.8f };
			p.maxLife = 0.25f + Rand01() * 0.25f;
			p.life = p.maxLife;
			p.size = m_size * (0.25f + Rand01() * 0.25f);
			break;
		}
	}

	// ・ｽJ・ｽ・ｽ・ｽ・ｽ・ｽO・ｽ・ｽ・ｽ・ｽ
	if (fabsf(m_position.x) > CAMERA_OUT_LIMIT)
	{
		m_isActive = false;
	}
}

void Projectile::Draw(const DirectX::XMFLOAT4X4& view, const DirectX::XMFLOAT4X4& projection)
{
	Sprite::SetView(view);
	Sprite::SetProjection(projection);
	Sprite::SetTexture(s_softTex); // 粒は生成した丸ぼかしを使う
	Sprite::SetOffset({ 0.0f, 0.0f });
	Sprite::SetSize({ 1.0f, 1.0f });
	Sprite::SetUVPos({ 0.0f, 0.0f });
	Sprite::SetUVScale({ 1.0f, 1.0f });

	// --- ・ｽp・ｽ[・ｽe・ｽB・ｽN・ｽ・ｽ・ｽi・ｽe・ｽ・ｽ・ｽ諱・ｿｽ・ｽ・ｽﾉ描・ｽ・ｽ・ｽj---
	for (const auto& p : m_particles)
	{
		if (p.life <= 0.0f) continue;
		float t = p.life / p.maxLife;                 // 1.0・ｽ・ｽ・ｽ・ｽ0.0・ｽ・ｽ
		float sc = p.size * (0.2f + 0.8f * t); // 縮みながら消える
		DirectX::XMMATRIX mS = DirectX::XMMatrixScaling(sc, sc, 1.0f);
		DirectX::XMMATRIX mT = DirectX::XMMatrixTranslation(p.pos.x, p.pos.y, p.pos.z);
		DirectX::XMFLOAT4X4 world;
		DirectX::XMStoreFloat4x4(&world, DirectX::XMMatrixTranspose(mS * mT));
		Sprite::SetWorld(world);
		// ・ｽﾂみゑｿｽ・ｽ・ｽ・ｽ・ｽ・ｽ・ｽ・ｽF・ｽﾅ、・ｽ・ｽ・ｽ・ｽ・ｽﾉ会ｿｽ・ｽ・ｽ・ｽﾄ費ｿｽ・ｽ・ｽ・ｽﾈゑｿｽ
		Sprite::SetColor({ 0.5f, 0.75f, 1.0f, 0.9f });
		Sprite::Draw();
	}

	if (!m_isActive) return;

	// --- ・ｽe・ｽ{・ｽﾌ（・ｽ・ｽ・ｽﾌまま：・ｽ鞫懶ｿｽ・ｽ1・ｽ・ｽ・ｽ`・ｽ・ｽ・ｽ・ｽ・ｽ・ｽ・ｽj---
	// ・ｽ・ｽ・ｽ・ｽ・ｽ・ｽ・ｽﾌ場合・ｽ・ｽX・ｽX・ｽP・ｽ[・ｽ・ｽ・ｽ・ｽ・ｽ}・ｽC・ｽi・ｽX・ｽﾉゑｿｽ・ｽﾄ画像・ｽ・ｽ・ｽ・ｽ・ｽE・ｽ・ｽ・ｽ]・ｽ・ｽ・ｽ・ｽ・ｽ・ｽ
	Sprite::SetTexture(s_texture); // 本体は元の画像に戻す
	float scaleX = m_isRight ? m_size : -m_size;
	DirectX::XMMATRIX mScale = DirectX::XMMatrixScaling(scaleX, m_size, 1.0f);
	DirectX::XMMATRIX mTrans = DirectX::XMMatrixTranslation(m_position.x, m_position.y, m_position.z);
	DirectX::XMMATRIX mWorld = mScale * mTrans;

	mWorld = DirectX::XMMatrixTranspose(mWorld);

	DirectX::XMFLOAT4X4 world;
	DirectX::XMStoreFloat4x4(&world, mWorld);
	Sprite::SetWorld(world);

	Sprite::SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });

	Sprite::Draw();
}

#ifdef _DEBUG
void Projectile::DrawHitbox(const DirectX::XMFLOAT4X4& view, const DirectX::XMFLOAT4X4& projection)
{
	if (!m_isActive) return;

	using namespace DirectX;

	// ・ｽ・ｽﾑ難ｿｽ・ｽ・ｽﾌ費ｿｽ・ｽ・ｽﾍオ・ｽ・ｽ・ｽ・ｽ・ｽW・ｽﾅ描・ｽ謔ｵ・ｽﾄ鯉ｿｽ・ｽﾂゑｿｽ・ｽ竄ｷ・ｽ・ｽ・ｽ・ｽ・ｽ・ｽ
	Geometory::SetColor(XMFLOAT4(1.0f, 0.5f, 0.0f, 1.0f));

	XMFLOAT3 corners[8];
	m_hitbox.GetCorners(corners);

	// Player・ｽﾌデ・ｽo・ｽb・ｽO・ｽ`・ｽ・ｽﾆ難ｿｽ・ｽl・ｽ・ｽZ・ｽ・ｽ・ｽ・ｽO・ｽﾌ厄ｿｽ(4・ｽ・ｽ・ｽ_)・ｽ・ｽ・ｽ・ｽ・ｽ・ｽﾅ四・ｽp・ｽ`・ｽ・ｽ`・ｽ・ｽ
	static const int edge[4][2] = { {0,1},{1,2},{2,3},{3,0} };
	for (int i = 0; i < 4; ++i) {
		Geometory::AddLine(corners[edge[i][0]], corners[edge[i][1]]);
	}
}
#endif