#pragma once
#include <DirectXMath.h>
#include <DirectXCollision.h>
#include "Texture.h"

class Player;

/**
 * @brief ・ｽ・ｽﾑ難ｿｽ・ｽ・ｽi・ｽg・ｽ・ｽ・ｽ・ｽ・ｽj・ｽN・ｽ・ｽ・ｽX
 */
class Projectile
{
public:
	Projectile();
	~Projectile();

	// ・ｽe・ｽｭ射ゑｿｽ・ｽ・ｽ
	void Launch(Player* owner, float speed, int damage, float size, bool isRight);

	// ・ｽX・ｽV
	void Update(float tick);

	// ・ｽ`・ｽ・ｽ
	void Draw(const DirectX::XMFLOAT4X4& view, const DirectX::XMFLOAT4X4& projection);

#ifdef _DEBUG
	// ・ｽf・ｽo・ｽb・ｽO・ｽp・ｽ・ｽ・ｽ・ｽ・ｽ阡ｻ・ｽ・ｽ`・ｽ・ｽ
	void DrawHitbox(const DirectX::XMFLOAT4X4& view, const DirectX::XMFLOAT4X4& projection);
#endif

	bool IsActive() const { return m_isActive; }
	void Deactivate() { m_isActive = false; }

	const DirectX::BoundingBox& GetHitbox() const { return m_hitbox; }
	int GetDamage() const { return m_damage; }
	Player* GetOwner() const { return m_owner; }

private:
	bool m_isActive;
	Player* m_owner = nullptr;
	DirectX::XMFLOAT3 m_position;
	float m_speed;
	int m_damage;
	float m_size;
	bool m_isRight;

	DirectX::BoundingBox m_hitbox;
	static Texture* s_texture;
	// パーティクル用の丸ぼかしテクスチャ（コード生成）
	static Texture* s_softTex;

	// --- ・ｽ・ｽ・ｽ・ｽﾉ撒・ｽ・ｽ・ｽp・ｽ[・ｽe・ｽB・ｽN・ｽ・ｽ・ｽ・ｽ・ｽo ---
	struct Particle
	{
		DirectX::XMFLOAT3 pos;
		DirectX::XMFLOAT2 vel;
		float life = 0.0f;    // ・ｽc・ｽ・ｽ・ｽ・ｽ・ｽ・ｽi0・ｽﾈ会ｿｽ・ｽﾅ擾ｿｽ・ｽﾅ）
		float maxLife = 0.0f;
		float size = 0.0f;
	};
	static const int MAX_PARTICLES = 64;
	Particle m_particles[MAX_PARTICLES];
	float m_spawnTimer = 0.0f; // ・ｽ・ｽ・ｽﾌパ・ｽ[・ｽe・ｽB・ｽN・ｽ・ｽ・ｽ・ｽ・ｽ・ｽ・ｽﾜでのタ・ｽC・ｽ}・ｽ[

	// ・ｽ・ｽﾊ外・ｽi・ｽJ・ｽ・ｽ・ｽ・ｽ・ｽﾌ外・ｽj・ｽ・ｽ・ｽﾅゑｿｽ閾値
	const float CAMERA_OUT_LIMIT = 10.0f;
};