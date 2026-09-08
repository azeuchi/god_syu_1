#include "HitEffectPool.h"
#include "HitEffect.h"

HitEffectPool::~HitEffectPool() = default;

void HitEffectPool::Init(int count)
{
	for (int i = 0; i < count; ++i) m_effects.push_back(std::make_unique<HitEffect>());
}

void HitEffectPool::Update(float tick)
{
	for (auto& e : m_effects) e->Update(tick);
}

void HitEffectPool::Draw(const DirectX::XMFLOAT4X4& view, const DirectX::XMFLOAT4X4& proj)
{
	for (auto& e : m_effects) e->Draw(view, proj);
}