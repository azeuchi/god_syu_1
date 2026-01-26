#pragma once
#include <DirectXMath.h>
#include <DirectXCollision.h>
#include "Texture.h"

class Player;

/**
 * @brief 飛び道具（波動拳）クラス
 */
class Projectile
{
public:
	Projectile();
	~Projectile();

	// 弾を発射する
	void Launch(Player* owner, float speed, int damage, float size, bool isRight);

	// 更新
	void Update(float tick);

	// 描画
	void Draw(const DirectX::XMFLOAT4X4& view, const DirectX::XMFLOAT4X4& projection);

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

	// 画面外（カメラ外）判定の閾値
	const float CAMERA_OUT_LIMIT = 10.0f;
};