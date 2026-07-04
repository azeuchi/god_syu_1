#include "Projectile.h"
#include "Player.h"
#include "Sprite.h"
#include "Geometory.h" // 当たり判定描画用に追加
#include <stdlib.h>
#include <math.h>

Texture* Projectile::s_texture = nullptr;
Texture* Projectile::s_softTex = nullptr;

namespace
{
	// 0.0から1.0の乱数（パーティクルの散らばり用）
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

	// プレイヤーの少し前から発射
	m_position = owner->GetPosition();
	m_position.y += 1.2f; // 手の高さ
	m_position.z = 0.0f;
}

void Projectile::Update(float tick)
{
	// パーティクルは弾が消えた後も残りを更新して自然に消す
	for (auto& p : m_particles)
	{
		if (p.life <= 0.0f) continue;
		p.life -= tick;
		p.pos.x += p.vel.x * tick;
		p.pos.y += p.vel.y * tick;
	}

	if (!m_isActive) return;

	// 移動
	float move = m_speed * tick;
	if (m_isRight) m_position.x += move;
	else m_position.x -= move;

	// 当たり判定の更新
	m_hitbox.Center = m_position;
	m_hitbox.Extents = { m_size * 0.5f, m_size * 0.5f, 0.1f };

	// 弾の後方からパーティクルを発生させる
	const float dir = m_isRight ? 1.0f : -1.0f;
	m_spawnTimer -= tick;
	while (m_spawnTimer <= 0.0f)
	{
		m_spawnTimer += 0.016f; // およそ毎フレーム1個
		for (auto& p : m_particles)
		{
			if (p.life > 0.0f) continue;
			// 弾の後ろ端から少し散らして出す
			p.pos = { m_position.x - dir * m_size * 0.4f,
					  m_position.y + (Rand01() - 0.5f) * m_size * 0.5f,
					  m_position.z };
			// 後方へ流れつつ上下に軽くばらける
			p.vel = { -dir * (0.8f + Rand01() * 1.2f),
					  (Rand01() - 0.5f) * 0.8f };
			p.maxLife = 0.25f + Rand01() * 0.25f;
			p.life = p.maxLife;
			p.size = m_size * (0.25f + Rand01() * 0.25f);
			break;
		}
	}

	// カメラ外判定
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

	// --- パーティクル（弾より先＝後ろに描く）---
	for (const auto& p : m_particles)
	{
		if (p.life <= 0.0f) continue;
		float t = p.life / p.maxLife;                 // 1.0から0.0へ
		float sc = p.size * (0.2f + 0.8f * t);        // 縮みながら消える
		DirectX::XMMATRIX mS = DirectX::XMMatrixScaling(sc, sc, 1.0f);
		DirectX::XMMATRIX mT = DirectX::XMMatrixTranslation(p.pos.x, p.pos.y, p.pos.z);
		DirectX::XMFLOAT4X4 world;
		DirectX::XMStoreFloat4x4(&world, DirectX::XMMatrixTranspose(mS * mT));
		Sprite::SetWorld(world);
		// 青みがかった色（不透明寄りにして描画設定に消されないようにする）
		Sprite::SetColor({ 0.5f, 0.75f, 1.0f, 0.9f });
		Sprite::Draw();
	}

	if (!m_isActive) return;

	// --- 弾本体（元のまま：画像を1枚描くだけ）---
	// 左向きの場合はXスケールをマイナスにして画像を左右反転させる
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

	// 飛び道具の判定はオレンジで描画して見つけやすくする
	Geometory::SetColor(XMFLOAT4(1.0f, 0.5f, 0.0f, 1.0f));

	XMFLOAT3 corners[8];
	m_hitbox.GetCorners(corners);

	// Playerのデバッグ描画と同様にZ軸手前の面(4頂点)を結んで四角形を描く
	static const int edge[4][2] = { {0,1},{1,2},{2,3},{3,0} };
	for (int i = 0; i < 4; ++i) {
		Geometory::AddLine(corners[edge[i][0]], corners[edge[i][1]]);
	}
}
#endif