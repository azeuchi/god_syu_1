#pragma once
#include "Player.h"
#include "HitEffect.h"
#include <vector>

// 衝突処理の結果を受け取るための構造体
struct CollisionResult
{
	bool isRoundOver;
	int winCountP1ToAdd;
	int winCountP2ToAdd;
	float hitStopTimer;
	float shakeTimerP1;
	float shakeTimerP2;

	CollisionResult()
		: isRoundOver(false)
		, winCountP1ToAdd(0)
		, winCountP2ToAdd(0)
		, hitStopTimer(0.0f)
		, shakeTimerP1(0.0f)
		, shakeTimerP2(0.0f)
	{
	}
};

class BattleCollision
{
public:
	// メインの衝突更新処理
	static CollisionResult UpdateInteractions(
		Player* p1,
		Player* p2,
		float tick,
		std::vector<HitEffect*>& effects,
		float stageLimitX
	);

private:
	// 衝突時の位置補正（押し出し）
	static void ResolvePushback(Player* p1, Player* p2, float stageLimitX);

	// 攻撃ヒット判定
	static void CheckAttackHit(
		Player* attacker,
		Player* defender,
		std::vector<HitEffect*>& effects,
		CollisionResult& result,
		float stageLimitX,
		bool isP1Attacking // AttackerがP1かどうか
	);

	// 飛び道具ヒット判定
	static void CheckProjectileHit(
		Player* attacker,
		Player* defender,
		std::vector<HitEffect*>& effects,
		CollisionResult& result,
		float stageLimitX,
		bool isP1Attacking
	);

	// エフェクト再生ヘルパー
	static void SpawnHitEffect(std::vector<HitEffect*>& effects, Player* target);

	// くらい判定取得ヘルパー
	static std::vector<DirectX::BoundingBox> GetTargetHurtboxes(const Player* target);
};