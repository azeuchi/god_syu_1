#include "BattleCollision.h"
#include "DebugLog.h"
#include "PlayerStateDamage.h"
#include "PlayerStateDown.h"
#include "PlayerStateGuard.h"

namespace
{
	// ガードされたときのヒットストップはヒットより短くし、当たった・防がれたの手応えを分ける
	constexpr float BLOCK_HITSTOP_RATE = 0.6f;

	// ガードされたときの押し戻し。技の重さ（ヒットストップの長さ）に比例させる。弱で約0.25、強で約0.35
	// ヒット時の押し戻し（knockback）は使わない。昇竜拳や足払いは当たると相手を大きく飛ばすが、
	// ガードされたときまで離れると反撃が届かなくなり、外したときのリスクが消えてしまうため
	float BlockPushback(float hitStop) { return 0.15f + hitStop * 3.0f; }

	// 飛び道具は距離で有利フレームが変わるので、硬直は固定にしている
	constexpr int PROJECTILE_HITSTUN = 15;
	constexpr int PROJECTILE_BLOCKSTUN = 11;
	constexpr float PROJECTILE_HITSTOP = 0.1f;
	constexpr float PROJECTILE_KNOCKBACK = 0.5f;
	constexpr float PROJECTILE_BLOCK_PUSHBACK = 0.35f;

	// キャンセルで出した技が当たるまで、相手ののけぞりを延ばすときの余裕（秒）。フレームの揺れを吸収する
	constexpr float CANCEL_GUARANTEE_MARGIN = 3.0f / 60.0f;

	// 当てた技をキャンセルしたとき、相手がまだのけぞっていれば、次の技の判定が出るまでのけぞりを延ばす。
	// キャンセルの受付中ならいつ押してもつながるので、技ごとにフレームを合わせなくてよい。
	// のけぞりが解けてから出したキャンセル（ガードされた後など）は延ばさない
	void GuaranteeCancel(Player* attacker, Player* defender)
	{
		if (!attacker->ConsumeCancel()) return;
		const AttackParams* params = attacker->GetCurrentAttackParams();
		if (params == nullptr || !defender->IsInHitstun()) return;
		defender->ExtendHitstun(params->hitboxStart + CANCEL_GUARANTEE_MARGIN);
	}

	// 2つの判定ボックスの重なり領域の中心を返す
	DirectX::XMFLOAT3 OverlapCenter(const DirectX::BoundingBox& a, const DirectX::BoundingBox& b)
	{
		float minX = (std::max)(a.Center.x - a.Extents.x, b.Center.x - b.Extents.x);
		float maxX = (std::min)(a.Center.x + a.Extents.x, b.Center.x + b.Extents.x);
		float minY = (std::max)(a.Center.y - a.Extents.y, b.Center.y - b.Extents.y);
		float maxY = (std::min)(a.Center.y + a.Extents.y, b.Center.y + b.Extents.y);
		return { (minX + maxX) * 0.5f, (minY + maxY) * 0.5f, a.Center.z };
	}
}
#include "Projectile.h"
#include <algorithm>

// 押し戻しを1フレーム分進める。壁に着いて進めなかった分は、押している側（相手）が下がる
static void ApplyPushback(Player* pushed, Player* opponent, float tick, float stageLimitX)
{
	const float step = pushed->TakePushbackStep(tick);
	if (step == 0.0f) return;

	DirectX::XMFLOAT3 pos = pushed->GetPosition();
	const float before = pos.x;
	pos.x = std::clamp(pos.x + step, -stageLimitX, stageLimitX);
	pushed->SetPosition(pos);

	const float blockedByWall = step - (pos.x - before);
	if (blockedByWall != 0.0f)
	{
		DirectX::XMFLOAT3 opp = opponent->GetPosition();
		opp.x = std::clamp(opp.x - blockedByWall, -stageLimitX, stageLimitX);
		opponent->SetPosition(opp);
	}
}

CollisionResult BattleCollision::UpdateInteractions(Player* p1, Player* p2, float tick, std::vector<std::unique_ptr<HitEffect>>& effects, float stageLimitX)
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

	ApplyPushback(p1, p2, tick, stageLimitX);
	ApplyPushback(p2, p1, tick, stageLimitX);

	GuaranteeCancel(p1, p2);
	GuaranteeCancel(p2, p1);

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

void BattleCollision::CheckAttackHit(Player* attacker, Player* defender, std::vector<std::unique_ptr<HitEffect>>& effects, CollisionResult& result, float stageLimitX, bool isP1Attacking)
{
	bool hit = false;
	DirectX::XMFLOAT3 hitPos = defender->GetPosition();
	hitPos.y += 1.5f; // 万一の保険（従来の位置）
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
						// エフェクトは判定が重なった領域の中心に出す
						hitPos = OverlapCenter(atk, hurt);
						break;
					}
				}
			}
		}
	}

	if (hit)
	{
		SpawnHitEffect(effects, hitPos);

		AttackParams* params = attacker->GetCurrentAttackParams();
		int dmg = (params != nullptr) ? params->damage : 0;
		int stun = (params != nullptr) ? params->hitFrame : 30;
		bool isDown = (params != nullptr) ? params->isDown : false;
		AttackLevel atkLevel = (params != nullptr) ? params->attackLevel : AttackLevel::HIGH;
		bool blocked = defender->TryGuard(atkLevel);
		result.wasBlocked = blocked;
		// hitFrame / blockFrame は有利フレーム。攻撃側の残り硬直を足して、のけぞり・ガード硬直の長さにする
		int blockStun = 10;
		if (params != nullptr)
		{
			float remainF = (params->totalDuration - attacker->GetAttackElapsedSec()) * 60.0f;
			int extra = (remainF > 0.0f) ? (int)(remainF + 0.5f) : 0;
			stun = params->hitFrame + extra;
			blockStun = params->blockFrame + extra;
		}

		defender->ReceiveDamage(dmg, atkLevel);
		attacker->OnHit();

		const float hitStop = (params != nullptr) ? params->hitStop : 0.1f;
		const float knockback = (params != nullptr) ? params->knockback : 0.0f;
		const float dir = (attacker->GetScale().x > 0.0f) ? 1.0f : -1.0f;
		defender->StartPushback(dir * (blocked ? BlockPushback(hitStop) : knockback));

		if (blocked)
		{
			defender->SetState(std::make_unique<PlayerStateGuard>(blockStun, defender->GetInputs().moveDown));
		}
		else if (isDown)
		{
			defender->SetState(std::make_unique<PlayerStateDown>());
		}
		else
		{
			defender->SetState(std::make_unique<PlayerStateDamage>(stun));
		}

		result.dbgStun = blocked ? blockStun : stun;
		const float stopTime = blocked ? hitStop * BLOCK_HITSTOP_RATE : hitStop;
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

void BattleCollision::CheckProjectileHit(Player* attacker, Player* defender, std::vector<std::unique_ptr<HitEffect>>& effects, CollisionResult& result, float stageLimitX, bool isP1Attacking)
{
	Projectile* proj = attacker->GetProjectile();
	// 無敵中（ダウン・起き上がり・昇竜拳の出始め）は弾がすり抜ける
	if (proj && proj->IsActive() && !defender->IsInvincible())
	{
		auto hurtboxes = GetTargetHurtboxes(defender);
		bool projHit = false;
		DirectX::XMFLOAT3 projHitPos = defender->GetPosition();
		projHitPos.y += 1.5f;
		for (const auto& hurt : hurtboxes)
		{
			if (proj->GetHitbox().Intersects(hurt))
			{
				projHit = true;
				projHitPos = OverlapCenter(proj->GetHitbox(), hurt);
				break;
			}
		}

		if (projHit)
		{
			result.wasProjectile = true;
			proj->Deactivate();
			SpawnHitEffect(effects, projHitPos);
			const bool blockedProj = defender->TryGuard(AttackLevel::HIGH);
			result.wasBlocked = blockedProj;
			defender->ReceiveDamage(proj->GetDamage(), AttackLevel::HIGH);

			const float dir = (attacker->GetScale().x > 0.0f) ? 1.0f : -1.0f;
			defender->StartPushback(dir * (blockedProj ? PROJECTILE_BLOCK_PUSHBACK : PROJECTILE_KNOCKBACK));

			if (blockedProj) defender->SetState(std::make_unique<PlayerStateGuard>(PROJECTILE_BLOCKSTUN, defender->GetInputs().moveDown));
			else             defender->SetState(std::make_unique<PlayerStateDamage>(PROJECTILE_HITSTUN));

			const float stopTime = blockedProj ? PROJECTILE_HITSTOP * BLOCK_HITSTOP_RATE : PROJECTILE_HITSTOP;
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
}

void BattleCollision::SpawnHitEffect(std::vector<std::unique_ptr<HitEffect>>& effects, const DirectX::XMFLOAT3& pos)
{
	for (auto& effect : effects)
	{
		if (!effect->IsActive())
		{
			effect->Activate(pos);
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