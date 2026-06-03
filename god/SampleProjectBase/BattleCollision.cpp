#include "BattleCollision.h"
#include "DebugLog.h"
#include "PlayerStateDamage.h"
#include "PlayerStateDown.h"
#include "Projectile.h"
#include <algorithm>

CollisionResult BattleCollision::UpdateInteractions(Player* p1, Player* p2, float tick, std::vector<HitEffect*>& effects, float stageLimitX)
{
	CollisionResult result;

	if (!p1 || !p2) return result;

	// 判定ボックスの更新
	p1->UpdateAttackBoxes();
	p2->UpdateAttackBoxes();

	// 向きの制御
	float x1 = p1->GetPosition().x;
	float x2 = p2->GetPosition().x;
	float absScale1 = fabsf(p1->GetScale().x);
	float absScale2 = fabsf(p2->GetScale().x);
	DirectX::XMFLOAT3 s1 = p1->GetScale();
	DirectX::XMFLOAT3 s2 = p2->GetScale();
	float rotLeft = DirectX::XM_PI / 2.0f;
	float rotRight = DirectX::XM_PI / -2.0f;

	if (x1 < x2) {
		s1.x = absScale1; p1->SetScale(s1); p1->SetRotation({ 0.0f, rotRight, 0.0f });
		s2.x = -absScale2; p2->SetScale(s2); p2->SetRotation({ 0.0f, rotLeft, 0.0f });
	}
	else {
		s1.x = -absScale1; p1->SetScale(s1); p1->SetRotation({ 0.0f, rotLeft, 0.0f });
		s2.x = absScale2; p2->SetScale(s2); p2->SetRotation({ 0.0f, rotRight, 0.0f });
	}

	// 移動制限
	DirectX::XMFLOAT3 pos1 = p1->GetPosition();
	pos1.x = std::clamp(pos1.x, -stageLimitX, stageLimitX);
	p1->SetPosition(pos1);

	DirectX::XMFLOAT3 pos2 = p2->GetPosition();
	pos2.x = std::clamp(pos2.x, -stageLimitX, stageLimitX);
	p2->SetPosition(pos2);

	// 衝突判定と押し出し
	ResolvePushback(p1, p2, stageLimitX);

	// 攻撃判定 (P1 -> P2)
	CheckAttackHit(p1, p2, effects, result, stageLimitX, true);

	// 飛び道具判定 (P1 -> P2)
	CheckProjectileHit(p1, p2, effects, result, stageLimitX, true);

	// 攻撃判定 (P2 -> P1)
	if (!result.isRoundOver) // ラウンド終了してなければ判定
	{
		CheckAttackHit(p2, p1, effects, result, stageLimitX, false);
	}

	// 飛び道具判定 (P2 -> P1)
	if (!result.isRoundOver)
	{
		CheckProjectileHit(p2, p1, effects, result, stageLimitX, false);
	}

	return result;
}

void BattleCollision::ResolvePushback(Player* p1, Player* p2, float stageLimitX)
{
	bool isColliding = p1->CheckCollision(p2);
	p1->SetIsColliding(isColliding);
	p2->SetIsColliding(isColliding);

	if (isColliding && !p1->IsAttacking() && !p2->IsAttacking())
	{
		DirectX::BoundingBox box1 = p1->GetHurtbox(HurtboxType::BODY);
		DirectX::BoundingBox box2 = p2->GetHurtbox(HurtboxType::BODY);
		DirectX::XMFLOAT3 pos1 = p1->GetPosition();
		DirectX::XMFLOAT3 pos2 = p2->GetPosition();

		float deltaX = box1.Center.x - box2.Center.x;
		float sumExtentsX = box1.Extents.x + box2.Extents.x;
		float overlapX = sumExtentsX - abs(deltaX);

		// 押し出し方向と量の計算 (X軸のみ)
		float pushAmount = overlapX / 2.0f;
		float direction = (deltaX > 0.0f) ? 1.0f : -1.0f;

		// 互いに外側へ押し出す
		pos1.x += direction * pushAmount;
		pos2.x -= direction * pushAmount;

		pos1.x = std::clamp(pos1.x, -stageLimitX, stageLimitX);
		pos2.x = std::clamp(pos2.x, -stageLimitX, stageLimitX);
		p1->SetPosition(pos1);
		p2->SetPosition(pos2);
	}
}

void BattleCollision::CheckAttackHit(Player* attacker, Player* defender, std::vector<HitEffect*>& effects, CollisionResult& result, float stageLimitX, bool isP1Attacking)
{
	bool hit = false;
	if (attacker->IsAttacking() && !attacker->HasHit())
	{
		if (!defender->IsInvincible())
		{
			const auto& hitboxes = attacker->GetActiveHitboxes();
			auto hurtboxes = GetTargetHurtboxes(defender);

			for (const auto& atk : hitboxes)
			{
				if (hit) break;
				for (const auto& hurt : hurtboxes)
				{
					if (atk.Intersects(hurt))
					{
						hit = true;
						break;
					}
				}
			}
		}
	}

	if (hit)
	{
		SpawnHitEffect(effects, defender);

		AttackParams* params = attacker->GetCurrentAttackParams();
		int dmg = (params != nullptr) ? params->damage : 0;
		int stun = (params != nullptr) ? params->hitFrame : 30;
		bool isDown = (params != nullptr) ? params->isDown : false;

		defender->ReceiveDamage(dmg);
		attacker->OnHit();

		float kb = (params != nullptr) ? params->knockback : 0.0f;
		float dir = (attacker->GetScale().x > 0.0f) ? 1.0f : -1.0f;

		DirectX::XMFLOAT3 defPos = defender->GetPosition();
		float originalX = defPos.x;
		float targetX = originalX + (dir * kb);
		float clampedX = std::clamp(targetX, -stageLimitX, stageLimitX);
		defPos.x = clampedX;
		defender->SetPosition(defPos);

		float movedDist = fabsf(clampedX - originalX);
		float pushBackDist = kb - movedDist;

		if (pushBackDist > 0.0f)
		{
			DirectX::XMFLOAT3 atkPos = attacker->GetPosition();
			atkPos.x -= dir * pushBackDist;
			atkPos.x = std::clamp(atkPos.x, -stageLimitX, stageLimitX);
			attacker->SetPosition(atkPos);
		}

		if (isDown)
		{
			defender->SetState(new PlayerStateDown());
		}
		else
		{
			defender->SetState(new PlayerStateDamage(stun));
		}

		float stopTime = (params != nullptr) ? params->hitStop : 0.1f;
		result.hitStopTimer = stopTime;

		if (isP1Attacking) result.shakeTimerP2 = stopTime;
		else result.shakeTimerP1 = stopTime;

		if (defender->GetHpRatio() <= 0.0f)
		{
			if (isP1Attacking) result.winCountP1ToAdd++;
			else result.winCountP2ToAdd++;

			result.isRoundOver = true;
			attacker->SetInputType(PlayerInputType::AI);
			defender->SetInputType(PlayerInputType::AI);
			DebugLog::log(DebugLog::INFO_LOG, isP1Attacking ? "P1 WIN ROUND!" : "P2 WIN ROUND!");
		}
	}
}

void BattleCollision::CheckProjectileHit(Player* attacker, Player* defender, std::vector<HitEffect*>& effects, CollisionResult& result, float stageLimitX, bool isP1Attacking)
{
	Projectile* proj = attacker->GetProjectile();
	if (proj && proj->IsActive())
	{
		auto hurtboxes = GetTargetHurtboxes(defender);
		bool projHit = false;
		for (const auto& hurt : hurtboxes)
		{
			if (proj->GetHitbox().Intersects(hurt))
			{
				projHit = true;
				break;
			}
		}

		if (projHit)
		{
			proj->Deactivate();
			SpawnHitEffect(effects, defender);
			defender->ReceiveDamage(proj->GetDamage());

			float kb = 0.5f; // 飛び道具は固定のノックバックとするかパラメータ化するか
			float dir = (attacker->GetScale().x > 0.0f) ? 1.0f : -1.0f;
			DirectX::XMFLOAT3 defPos = defender->GetPosition();
			float originalX = defPos.x;
			float targetX = originalX + (dir * kb);
			float clampedX = std::clamp(targetX, -stageLimitX, stageLimitX);
			defPos.x = clampedX;
			defender->SetPosition(defPos);

			float movedDist = fabsf(clampedX - originalX);
			float pushBackDist = kb - movedDist;

			if (pushBackDist > 0.0f)
			{
				DirectX::XMFLOAT3 atkPos = attacker->GetPosition();
				atkPos.x -= dir * pushBackDist;
				atkPos.x = std::clamp(atkPos.x, -stageLimitX, stageLimitX);
				attacker->SetPosition(atkPos);
			}

			defender->SetState(new PlayerStateDamage(15));
			result.hitStopTimer = 0.1f;

			if (isP1Attacking) result.shakeTimerP2 = 0.1f;
			else result.shakeTimerP1 = 0.1f;

			if (defender->GetHpRatio() <= 0.0f)
			{
				if (isP1Attacking) result.winCountP1ToAdd++;
				else result.winCountP2ToAdd++;

				result.isRoundOver = true;
				attacker->SetInputType(PlayerInputType::AI);
				defender->SetInputType(PlayerInputType::AI);
				DebugLog::log(DebugLog::INFO_LOG, isP1Attacking ? "P1 WIN ROUND!" : "P2 WIN ROUND!");
			}
		}
	}
}

void BattleCollision::SpawnHitEffect(std::vector<HitEffect*>& effects, Player* target)
{
	for (auto effect : effects)
	{
		if (!effect->IsActive())
		{
			effect->Activate(target);
			break;
		}
	}
}

std::vector<DirectX::BoundingBox> BattleCollision::GetTargetHurtboxes(const Player* target)
{
	// 基本のくらい判定(頭・体・足)は常時有効
	std::vector<DirectX::BoundingBox> boxes;
	boxes.push_back(target->GetHurtbox(HurtboxType::HEAD));
	boxes.push_back(target->GetHurtbox(HurtboxType::BODY));
	boxes.push_back(target->GetHurtbox(HurtboxType::LEGS));

	// 攻撃中は、その瞬間アクティブな技固有の追加くらい判定を加算する
	if (target->IsAttacking())
	{
		const auto& extra = target->GetActiveHurtboxes();
		boxes.insert(boxes.end(), extra.begin(), extra.end());
	}
	return boxes;
}