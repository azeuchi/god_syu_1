#pragma once
#include <vector>
#include <DirectXMath.h>

class HitEffect;

// ヒットエフェクトのプール
class HitEffectPool
{
public:
	~HitEffectPool();

	void Init(int count);
	void Update(float tick);
	void Draw(const DirectX::XMFLOAT4X4& view, const DirectX::XMFLOAT4X4& proj);

	// BattleCollision::UpdateInteractions に渡すための実体参照
	std::vector<HitEffect*>& Raw() { return m_effects; }

private:
	std::vector<HitEffect*> m_effects;
};