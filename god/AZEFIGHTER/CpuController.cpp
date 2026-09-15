#include "CpuController.h"
#include "Projectile.h"
#include <algorithm>
#include <cfloat>
#include <cmath>
#include <cstdarg>
#include <cwchar>

using namespace DirectX;

namespace
{
	const float FRAME_SEC = 1.0f / 60.0f;

	// 攻撃するときはリーチを少し短く見積もり、先端ぎりぎりで振って空振りするのを防ぐ
	const float ATTACK_REACH_SCALE = 0.9f;
	// ガードするときは逆に長く見積もる。届くか怪しい技はガードしておく方が失うものが少ない
	const float GUARD_REACH_SCALE = 1.15f;

	// 飛び道具が当たるまでこの時間を切ったら、ガードかジャンプかを決める
	const float PROJECTILE_REACT_SEC = 0.4f;
	// 通常技が届かず、この距離より離れているときだけ波動拳を選択肢に入れる
	const float FIREBALL_MIN_DISTANCE = 4.0f;
	// 連携をガードされたら、硬直が解けたあとしばらく後ろを入れておく
	const float GUARD_AFTER_BLOCK_SEC = 0.4f;

	const CpuProfile PROFILES[(int)CpuLevel::COUNT] =
	{
		//  名前       反応 確認 出し切り 考え直す間隔  攻め   構え   波動拳 ジャンプ
		{ L"EASY",   26,  40,  0.3f,    20, 36,     0.35f, 0.25f, 0.20f, 0.0f },
		{ L"NORMAL", 17,  20,  0.5f,    12, 24,     0.45f, 0.65f, 0.35f, 0.3f },
		{ L"HARD",   11,  12,  0.7f,     6, 14,     0.55f, 0.85f, 0.40f, 0.6f },
	};

	// 相手の癖を覚えるときの減衰。新しい技を見るたびに過去の回数をこの割合に縮める
	const float HABIT_DECAY = 0.85f;

	int ToFrames(float sec)
	{
		return (int)std::lround(sec * 60.0f);
	}

	std::wstring Format(const wchar_t* fmt, ...)
	{
		wchar_t buf[256];
		va_list args;
		va_start(args, fmt);
		vswprintf_s(buf, fmt, args);
		va_end(args);
		return buf;
	}

	const wchar_t* LevelName(AttackLevel level)
	{
		switch (level)
		{
		case AttackLevel::MID: return L"中段";
		case AttackLevel::LOW: return L"下段";
		default:               return L"上段";
		}
	}

	void HoldForward(bool facingRight, PlayerInputs& in)
	{
		if (facingRight) in.moveRight = true; else in.moveLeft = true;
	}

	void HoldBack(bool facingRight, PlayerInputs& in)
	{
		if (facingRight) in.moveLeft = true; else in.moveRight = true;
	}

	bool FacingRight(const Player& player)
	{
		return player.GetRotation().y < 0.0f;
	}

	// 攻撃側がこの位置でこの技を出したとき、判定のどれかが相手のくらい判定に重なるか
	// キーフレームをすべて調べるので、持続のどこかで届けば true になる
	bool HitboxReaches(const AttackParams& params, float attackerX, float attackerY, bool facingRight,
		const BoundingBox* hurt, int hurtCount, float shiftX, float shiftY, float reachScale)
	{
		const float dir = facingRight ? 1.0f : -1.0f;
		for (const AnimatedBox& animBox : params.hitboxes)
		{
			for (const BoxKeyframe& key : animBox.keyframes)
			{
				for (int i = 0; i < hurtCount; ++i)
				{
					BoundingBox target = hurt[i];
					target.Center.x += shiftX;
					target.Center.y += shiftY;

					const BoundingBox hit(
						XMFLOAT3(attackerX + key.data.offset.x * dir * reachScale, attackerY + key.data.offset.y, target.Center.z),
						XMFLOAT3(key.data.extents.x * reachScale, key.data.extents.y, 0.1f));
					if (hit.Intersects(target)) return true;
				}
			}
		}
		return false;
	}
}

CpuController::CpuController(CpuLevel level)
	: m_level(level)
	, m_rng(std::random_device{}())
{
}

const CpuProfile& CpuController::GetProfile(CpuLevel level)
{
	return PROFILES[(int)level];
}

void CpuController::Reset(Player& self)
{
	m_clock = 0.0f;
	m_gameClock = 0.0f;
	m_head = 0;
	m_count = 0;
	m_seenIndex = 0;

	m_ownMove = nullptr;
	m_ownMoveElapsed = 0.0f;
	m_contact = false;
	m_combo = ComboDecision::WAITING;
	m_inheritedCombo = ComboDecision::WAITING;

	m_guardAfterBlockTimer = 0.0f;
	m_hitstunGuardRolled = false;
	m_hitstunGuard = false;
	m_prevSeenProjectile = false;
	m_jumpOverThisProjectile = false;

	m_intent = Intent::WAIT;
	m_guardLow = false;
	m_wasThreatened = false;
	m_lastSeenMove = MOVE_NONE;
	m_lastSeenElapsed = 0.0f;
	// 相手の癖（m_midUsage / m_lowUsage）はラウンドをまたいで覚えておく
	m_thinkTimer = 0.0f;
	m_scriptLength = 0;
	m_scriptStep = 0;
	m_scriptFrame = 0;

	m_prevInput = {};
	m_debug.decision.clear();
	m_debug.reason.clear();

	// ラウンド前後は注入入力のまま操作ロックされるので、最後の入力が残らないよう空にする
	self.SetInjectedInputs({});
}

void CpuController::Update(Player& self, const Player& opponent, float realTick, float gameTick)
{
	m_clock += realTick;
	m_gameClock += gameTick;
	Record(opponent);

	m_seenIndex = PerceiveIndex(ReactionSec());
	const Snapshot& seen = m_history[m_seenIndex];

	// 新しい飛び道具が見えたら、ジャンプでかわすかをその弾について1回だけ決める
	if (seen.projectileActive && !m_prevSeenProjectile)
	{
		m_jumpOverThisProjectile = Roll(Profile().jumpOverRate);
	}
	m_prevSeenProjectile = seen.projectileActive;

	RememberOpponentHabit(opponent, seen);

	PlayerInputs in;

	if (self.GetHpRatio() <= 0.0f || seen.dead)
	{
		// 決着後は何もしない
	}
	else if (self.IsInHitstun())
	{
		// のけぞり中は操作が効かないが、後ろを入れておけば解けた瞬間からガードになる。
		// 構えの堅さと同じ確率で、のけぞり1回につき1度だけ決める
		if (!m_hitstunGuardRolled)
		{
			m_hitstunGuardRolled = true;
			m_hitstunGuard = Roll(Profile().guardRate);
		}
		if (m_hitstunGuard) HoldBack(FacingRight(self), in);
		Explain(L"のけぞり中", [&] {
			return std::wstring(m_hitstunGuard ? L"解けた瞬間にガードできるよう後ろを入れておく" : L"");
			});
	}
	else if (!self.IsActionable())
	{
		m_hitstunGuardRolled = false;
		if (self.GetCurrentAttackParams()) ThinkDuringOwnMove(self, in);
	}
	else
	{
		m_hitstunGuardRolled = false;
		m_ownMove = nullptr;

		const bool reacted =
			ThinkDefense(self, opponent, seen, in) ||
			ThinkPunish(self, opponent, seen, in) ||
			ThinkAntiAir(self, seen, in);

		if (reacted)
		{
			// 入力途中のコマンドより、目の前の状況への対応を優先する
			m_scriptLength = 0;
		}
		else if (!GuardAfterBlock(self, realTick, in) && !RunScript(in))
		{
			ThinkNeutral(self, opponent, seen, realTick, in);
		}
	}

	// 攻撃ボタンとジャンプは「押した瞬間」だけ有効な入力なので、2フレーム続けて押さない。
	// 人間の連打と同じく、一度離さないと次の入力にならないようにしている
	if (m_prevInput.LightPunch)  in.LightPunch = false;
	if (m_prevInput.MediumPunch) in.MediumPunch = false;
	if (m_prevInput.HeavyPunch)  in.HeavyPunch = false;
	if (m_prevInput.MediumKick)  in.MediumKick = false;
	if (m_prevInput.HeavyKick)   in.HeavyKick = false;
	if (m_prevInput.jump)        in.jump = false;

	self.SetInjectedInputs(in);
	m_prevInput = in;

	if (m_debugEnabled) UpdateDebugHeader(seen);
}

// ============================================================
// 観測
// ============================================================

void CpuController::Record(const Player& opponent)
{
	Snapshot& s = m_history[m_head];
	m_head = (m_head + 1) % HISTORY_SIZE;
	m_count = (std::min)(m_count + 1, HISTORY_SIZE);

	const XMFLOAT3 pos = opponent.GetPosition();
	const XMFLOAT3 vel = opponent.GetVelocity();
	s.time = m_clock;
	s.gameTime = m_gameClock;
	s.x = pos.x;
	s.y = pos.y;
	s.vx = vel.x;
	s.vy = vel.y;
	s.facingRight = FacingRight(opponent);
	s.jumping = opponent.GetIsJumping();
	s.hitstun = opponent.IsInHitstun();
	s.invincible = opponent.IsInvincible();
	s.dead = (opponent.GetHpRatio() <= 0.0f);
	s.move = IdentifyMove(opponent);
	s.moveElapsed = opponent.GetAttackElapsedSec();
	s.hurtCount = CollectHurtboxes(opponent, s.hurt);

	const Projectile* projectile = opponent.GetProjectile();
	s.projectileActive = (projectile && projectile->IsActive());
	if (s.projectileActive) s.projectile = projectile->GetHitbox();
}

// delaySec 以上前の観測のうち、いちばん新しいものを返す。
// ラウンド開始直後でまだ十分古い観測が無いときは、いちばん古いものを使う
int CpuController::PerceiveIndex(float delaySec) const
{
	// 経過時間の誤差で1フレームずれないよう、半フレームの余裕を持たせる
	const float limit = m_clock - delaySec + FRAME_SEC * 0.5f;
	const int newest = (m_head - 1 + HISTORY_SIZE) % HISTORY_SIZE;
	for (int i = 0; i < m_count; ++i)
	{
		const int index = (newest - i + HISTORY_SIZE) % HISTORY_SIZE;
		if (m_history[index].time <= limit) return index;
	}
	return (m_head - m_count + HISTORY_SIZE) % HISTORY_SIZE;
}

// 弾の速さは内部の値を読まず、見えた位置の移り変わりから求める。
// ヒットストップ中は弾が止まって見えるので、動いている区間が見つかるまで少しさかのぼる
float CpuController::EstimateProjectileSpeed(int seenIndex) const
{
	const Snapshot& current = m_history[seenIndex];
	if (!current.projectileActive) return 0.0f;

	const int oldest = (m_head - m_count + HISTORY_SIZE) % HISTORY_SIZE;
	int index = seenIndex;
	for (int step = 0; step < 8 && index != oldest; ++step)
	{
		index = (index - 1 + HISTORY_SIZE) % HISTORY_SIZE;
		const Snapshot& past = m_history[index];
		if (!past.projectileActive) break;

		const float dx = current.projectile.Center.x - past.projectile.Center.x;
		const float dt = current.gameTime - past.gameTime;
		if (std::fabs(dx) > 0.0001f && dt > 0.0f) return dx / dt;
	}
	return 0.0f;
}

// ============================================================
// 自分の技の最中：ヒット確認とキャンセル
// ============================================================

void CpuController::ThinkDuringOwnMove(const Player& self, PlayerInputs& in)
{
	const AttackParams* move = self.GetCurrentAttackParams();
	const float elapsed = self.GetAttackElapsedSec();

	// 新しい技が始まったら、その技についての記憶を作り直す
	if (move != m_ownMove || elapsed < m_ownMoveElapsed)
	{
		m_ownMove = move;
		m_contact = false;
		m_combo = (m_clock <= m_inheritDeadline) ? m_inheritedCombo : ComboDecision::WAITING;
		m_inheritedCombo = ComboDecision::WAITING;
	}
	m_ownMoveElapsed = elapsed;

	if (!m_contact && self.HasHit())
	{
		m_contact = true;
		m_contactTime = m_clock;
	}

	if (!m_contact || !move->cancelEnabled || move->isDown) return;
	if (m_combo == ComboDecision::DONE || m_combo == ComboDecision::DROP || m_combo == ComboDecision::BLOCK_CONFIRMED) return;

	const int moveId = IdentifyMove(self);

	if (m_combo == ComboDecision::WAITING)
	{
		// のけぞりが見た目に出るのは触れた次のフレームから
		const float knowTime = m_contactTime + ConfirmSec() + FRAME_SEC;
		if (m_clock >= knowTime)
		{
			const Snapshot& atConfirm = m_history[PerceiveIndex(ConfirmSec())];
			if (atConfirm.hitstun || atConfirm.invincible)
			{
				m_combo = ComboDecision::HIT_CONFIRMED;
			}
			else
			{
				m_combo = ComboDecision::BLOCK_CONFIRMED;
				m_guardAfterBlockTimer = GUARD_AFTER_BLOCK_SEC;
				Explain(L"連携をやめる", [&] {
					return Format(L"%s をガードされたのを %dF で確認", MoveName(moveId), Profile().confirmFrames);
					});
				return;
			}
		}
		else if (knowTime - m_clock > move->cancelEnd - elapsed)
		{
			// 見分けがつく前にキャンセル受付が終わる。出し切るかどうかをここで一度だけ決める
			m_combo = Roll(Profile().comboCommitRate) ? ComboDecision::COMMIT : ComboDecision::DROP;
			if (m_combo == ComboDecision::DROP)
			{
				Explain(L"連携をやめる", [&] {
					return Format(L"ヒット確認(%dF)が受付終了に間に合わない", Profile().confirmFrames);
					});
				return;
			}
		}
		else
		{
			return;
		}
	}

	if (elapsed < move->cancelStart) return;

	// BattleCollision と同じ式で、今キャンセルしたときに相手に残っているのけぞりを出す
	const int stunLeft = move->hitFrame + ToFrames(move->totalDuration - elapsed);

	// 触れたのが最後の観測より後なら、そのぶんのノックバックで相手は離れている
	const Snapshot& seen = m_history[m_seenIndex];
	const bool facingRight = FacingRight(self);
	const float shiftX = (m_contactTime > seen.time) ? (facingRight ? move->knockback : -move->knockback) : 0.0f;
	const XMFLOAT3 myPos = self.GetPosition();

	int best = MOVE_NONE;
	int bestStartup = 0;
	for (int id = 0; id < NORMAL_MOVE_COUNT; ++id)
	{
		if (!CancelAllowed(*move, id)) continue;

		const AttackParams& next = ParamsOf(self, id);
		// ボタンを押したフレームのぶん1フレーム足す
		const int startup = ToFrames(next.hitboxStart) + 1;
		if (startup > stunLeft) continue;
		if (!HitboxReaches(next, myPos.x, myPos.y, facingRight, seen.hurt.data(), seen.hurtCount, shiftX, 0.0f, ATTACK_REACH_SCALE)) continue;

		if (best == MOVE_NONE || next.damage > ParamsOf(self, best).damage)
		{
			best = id;
			bestStartup = startup;
		}
	}

	const bool confirmed = (m_combo == ComboDecision::HIT_CONFIRMED);
	if (best == MOVE_NONE)
	{
		m_combo = ComboDecision::DONE;
		Explain(L"連携おわり", [&] { return Format(L"のけぞり残り %dF に間に合う技がない", stunLeft); });
		return;
	}

	PressMove(best, in);
	m_combo = ComboDecision::DONE;
	// 1発目で確認できていれば、そのあとの技は確認し直さずに繋ぐ
	InheritCombo(confirmed ? ComboDecision::HIT_CONFIRMED : ComboDecision::COMMIT);

	Explain(L"キャンセル", [&] {
		return Format(L"%s→%s（%s） のけぞり残り%dF ≧ 発生%dF",
			MoveName(moveId), MoveName(best), confirmed ? L"ヒット確認済み" : L"確認せず出し切り", stunLeft, bestStartup);
		});
}

// ============================================================
// 反応：ガード、反撃、着地狩り
// ============================================================

bool CpuController::ThinkDefense(const Player& self, const Player& opponent, const Snapshot& seen, PlayerInputs& in)
{
	const bool facingRight = FacingRight(self);

	// 打撃。見えた時点の経過時間に、見てから今までの時間を足して「今どこまで進んでいるか」を出す
	if (seen.move != MOVE_NONE && seen.move < NORMAL_MOVE_COUNT)
	{
		const AttackParams& attack = ParamsOf(opponent, seen.move);
		const float elapsedNow = seen.moveElapsed + (m_gameClock - seen.gameTime);
		if (elapsedNow < attack.hitboxEnd)
		{
			std::array<BoundingBox, MAX_HURTBOXES> myHurt;
			const int hurtCount = CollectHurtboxes(self, myHurt);
			if (HitboxReaches(attack, seen.x, seen.y, seen.facingRight, myHurt.data(), hurtCount, 0.0f, 0.0f, GUARD_REACH_SCALE))
			{
				HoldBack(facingRight, in);
				// 下段はしゃがみ、中段は立ち。上段はどちらでも防げるので今の姿勢のまま
				in.moveDown = (attack.attackLevel == AttackLevel::LOW) ||
					(attack.attackLevel == AttackLevel::HIGH && self.GetIsCrouching());

				Explain(in.moveDown ? L"しゃがみガード" : L"立ちガード", [&] {
					return Format(L"%s（%s・発生%dF）を %dF 目で認識",
						MoveName(seen.move), LevelName(attack.attackLevel), ToFrames(attack.hitboxStart), ToFrames(seen.moveElapsed));
					});
				return true;
			}
		}
	}

	// 飛び道具
	if (!seen.projectileActive) return false;

	const float speed = EstimateProjectileSpeed(m_seenIndex);
	const XMFLOAT3 myPos = self.GetPosition();
	const float projectileX = seen.projectile.Center.x + speed * (m_gameClock - seen.gameTime);
	const float toMe = myPos.x - projectileX;
	if (speed == 0.0f || toMe * speed <= 0.0f) return false;

	const BoundingBox body = self.GetHurtbox(HurtboxType::BODY);
	const float gap = std::fabs(toMe) - seen.projectile.Extents.x - body.Extents.x;
	const float timeToHit = (std::max)(gap, 0.0f) / std::fabs(speed);
	if (timeToHit >= PROJECTILE_REACT_SEC) return false;

	if (m_jumpOverThisProjectile)
	{
		// 今ジャンプしたとき、弾が体の幅を通り過ぎるあいだ、くらい判定の下端が弾より上にあるかを放物線で調べる
		float hurtBottomFromFeet = FLT_MAX;
		for (int type = 0; type < (int)HurtboxType::COUNT; ++type)
		{
			const BoundingBox box = self.GetHurtbox((HurtboxType)type);
			hurtBottomFromFeet = (std::min)(hurtBottomFromFeet, box.Center.y - box.Extents.y - myPos.y);
		}
		const float needHeight = (seen.projectile.Center.y + seen.projectile.Extents.y) - hurtBottomFromFeet;
		const float passTime = (seen.projectile.Extents.x + body.Extents.x) * 2.0f / std::fabs(speed);

		const float v = Player::JUMP_VELOCITY;
		const float g = Player::GRAVITY;
		const float discriminant = v * v - 2.0f * g * needHeight;
		if (discriminant > 0.0f)
		{
			// 高さが needHeight を超えている時間帯 [rise, fall]
			const float rise = (v - std::sqrt(discriminant)) / g;
			const float fall = (v + std::sqrt(discriminant)) / g;
			if (timeToHit >= rise && timeToHit + passTime <= fall)
			{
				in.jump = true;
				Explain(L"ジャンプで回避", [&] {
					return Format(L"弾の到達まで%dF、通過する%dFのあいだ弾より高くいられる", ToFrames(timeToHit), ToFrames(passTime));
					});
				return true;
			}
			if (timeToHit > rise)
			{
				// まだ早い。今跳ぶと弾が通り過ぎる前に着地してしまう
				Explain(L"跳ぶタイミング待ち", [&] { return Format(L"弾の到達まで%dF", ToFrames(timeToHit)); });
				return true;
			}
		}
	}

	HoldBack(facingRight, in);
	in.moveDown = self.GetIsCrouching();
	Explain(L"飛び道具をガード", [&] { return Format(L"到達まで%dF", ToFrames(timeToHit)); });
	return true;
}

bool CpuController::ThinkPunish(const Player& self, const Player& opponent, const Snapshot& seen, PlayerInputs& in)
{
	if (seen.move == MOVE_NONE || seen.invincible) return false;

	const AttackParams& attack = ParamsOf(opponent, seen.move);
	const bool isFireball = (seen.move >= MOVE_FIREBALL_L);
	const float elapsedNow = seen.moveElapsed + (m_gameClock - seen.gameTime);

	// 攻撃判定が残っているあいだは反撃ではなくガードの場面
	if (!isFireball && elapsedNow < attack.hitboxEnd) return false;

	const int recoveryLeft = ToFrames(attack.totalDuration - elapsedNow);
	if (recoveryLeft <= 0) return false;

	const bool facingRight = FacingRight(self);
	const XMFLOAT3 myPos = self.GetPosition();

	int best = MOVE_NONE;
	int bestStartup = 0;
	for (int id = 0; id < NORMAL_MOVE_COUNT; ++id)
	{
		const AttackParams& mine = ParamsOf(self, id);
		const int startup = ToFrames(mine.hitboxStart) + 1;
		if (startup > recoveryLeft) continue;
		if (!HitboxReaches(mine, myPos.x, myPos.y, facingRight, seen.hurt.data(), seen.hurtCount, 0.0f, 0.0f, ATTACK_REACH_SCALE)) continue;

		if (best == MOVE_NONE || mine.damage > ParamsOf(self, best).damage)
		{
			best = id;
			bestStartup = startup;
		}
	}
	if (best == MOVE_NONE) return false;

	PressMove(best, in);
	// 相手が動けないのは分かっているので、当たったあとも確認し直さずに繋ぐ
	InheritCombo(ComboDecision::HIT_CONFIRMED);

	Explain(L"反撃", [&] {
		return Format(L"%s の硬直残り%dF ≧ %s の発生%dF", MoveName(seen.move), recoveryLeft, MoveName(best), bestStartup);
		});
	return true;
}

bool CpuController::ThinkAntiAir(const Player& self, const Snapshot& seen, PlayerInputs& in)
{
	if (!seen.jumping) return false;

	// 見えた高さと速度から、物理式で今の状態と着地までの時間を出す
	const float g = Player::GRAVITY;
	const float dt = m_gameClock - seen.gameTime;
	const float y = seen.y + seen.vy * dt - 0.5f * g * dt * dt;
	const float vy = seen.vy - g * dt;
	if (y <= 0.0f) return false;

	const float timeToLand = (vy + std::sqrt(vy * vy + 2.0f * g * y)) / g;
	const int landFrames = ToFrames(timeToLand);
	const float landShiftX = seen.vx * (dt + timeToLand);

	const bool facingRight = FacingRight(self);
	const XMFLOAT3 myPos = self.GetPosition();

	int best = MOVE_NONE;
	bool anyReach = false;
	for (int id = 0; id < NORMAL_MOVE_COUNT; ++id)
	{
		const AttackParams& mine = ParamsOf(self, id);
		if (!HitboxReaches(mine, myPos.x, myPos.y, facingRight, seen.hurt.data(), seen.hurtCount, landShiftX, -seen.y, ATTACK_REACH_SCALE)) continue;
		anyReach = true;

		// 着地の瞬間に攻撃判定が出ている技だけを選ぶ
		const int startup = ToFrames(mine.hitboxStart) + 1;
		const int activeEnd = ToFrames(mine.hitboxEnd);
		if (landFrames < startup || landFrames >= activeEnd) continue;

		if (best == MOVE_NONE || mine.damage > ParamsOf(self, best).damage) best = id;
	}

	if (best != MOVE_NONE)
	{
		PressMove(best, in);
		Explain(L"着地狩り", [&] {
			const AttackParams& p = ParamsOf(self, best);
			return Format(L"着地まで%dF → %s（持続 %dF〜%dF）", landFrames, MoveName(best), ToFrames(p.hitboxStart) + 1, ToFrames(p.hitboxEnd));
			});
		return true;
	}

	if (anyReach)
	{
		// 近くに降りてくるが、まだ技を出すには早い。むやみに動かず待つ
		Explain(L"着地を待つ", [&] { return Format(L"着地まで%dF", landFrames); });
		return true;
	}
	return false;
}

bool CpuController::GuardAfterBlock(const Player& self, float tick, PlayerInputs& in)
{
	if (m_guardAfterBlockTimer <= 0.0f) return false;
	m_guardAfterBlockTimer -= tick;
	HoldBack(FacingRight(self), in);
	Explain(L"ガードを固める", [] { return std::wstring(L"連携をガードされた直後なので反撃に備える"); });
	return true;
}

// ============================================================
// 立ち回り
// ============================================================

void CpuController::ThinkNeutral(const Player& self, const Player& opponent, const Snapshot& seen, float tick, PlayerInputs& in)
{
	const CpuProfile& profile = Profile();
	const bool facingRight = FacingRight(self);
	const XMFLOAT3 myPos = self.GetPosition();
	const float distance = std::fabs(seen.x - myPos.x);

	// 今の間合いで届く通常技
	int reachable[NORMAL_MOVE_COUNT];
	int reachableCount = 0;
	if (!seen.invincible)
	{
		for (int id = 0; id < NORMAL_MOVE_COUNT; ++id)
		{
			if (HitboxReaches(ParamsOf(self, id), myPos.x, myPos.y, facingRight, seen.hurt.data(), seen.hurtCount, 0.0f, 0.0f, ATTACK_REACH_SCALE))
			{
				reachable[reachableCount++] = id;
			}
		}
	}

	// 相手の技が届く間合いか。どの通常技も発生が反応速度より速いので、
	// ここに入ったら「見てから」ではなく、先にガードを固めておかないと防げない
	const bool threatened = !seen.invincible && InThreatRange(self, opponent, seen);

	// 次の判断を待たずに考え直す場面。前進中に技が届いた、相手の間合いに入った
	if (m_intent == Intent::FORWARD && reachableCount > 0) m_thinkTimer = 0.0f;
	if (threatened && !m_wasThreatened) m_thinkTimer = 0.0f;
	m_wasThreatened = threatened;

	m_thinkTimer -= tick;
	if (m_thinkTimer <= 0.0f)
	{
		m_thinkTimer = RandomRange(profile.thinkMinFrames, profile.thinkMaxFrames) * FRAME_SEC;

		if (seen.invincible)
		{
			m_intent = Intent::WAIT;
			Explain(L"様子見", [] { return std::wstring(L"相手はダウン中で無敵"); });
		}
		else if (reachableCount > 0 && Roll(profile.aggression))
		{
			const int move = ChooseStarter(self, reachable, reachableCount);
			PressMove(move, in);
			m_intent = Intent::WAIT;
			Explain(L"攻撃", [&] { return Format(L"%s が届く間合い（距離 %.2f）", MoveName(move), distance); });
			return;
		}
		else if (reachableCount == 0 && !threatened && distance >= FIREBALL_MIN_DISTANCE && self.CanFireProjectile() && Roll(profile.fireballRate))
		{
			StartFireballScript(facingRight, RandomRange(0, 2));
			m_intent = Intent::WAIT;
			Explain(L"波動拳", [&] { return Format(L"通常技が届かない距離（%.2f）", distance); });
			RunScript(in);
			return;
		}
		else if (threatened)
		{
			if (Roll(profile.guardRate))
			{
				// 立ちとしゃがみのどちらで待つかは読み。相手が最近よく使う段に合わせる
				const float lowRate = LowGuessRate();
				m_guardLow = Roll(lowRate);
				m_intent = Intent::GUARD;
				Explain(m_guardLow ? L"しゃがみで固める" : L"立ちで固める", [&] {
					return Format(L"相手の技が届く間合い。相手が下段を使う割合 %d%%", (int)std::lround(lowRate * 100.0f));
					});
			}
			else
			{
				// 固めなかった。こちらの技が届かないなら、前に出て間合いを詰める
				m_intent = (reachableCount > 0) ? Intent::WAIT : Intent::FORWARD;
				Explain(m_intent == Intent::FORWARD ? L"前進" : L"様子見", [] { return std::wstring(L"相手の技が届く間合いだが、ガードせずに動く"); });
			}
		}
		else
		{
			// どちらの技も届かない間合い。攻め気が強いほど前に出る
			const float r = Rand01();
			if (r < profile.aggression)              m_intent = Intent::FORWARD;
			else if (r < profile.aggression + 0.15f) m_intent = Intent::BACK;
			else                                     m_intent = Intent::WAIT;

			const wchar_t* label = (m_intent == Intent::FORWARD) ? L"前進" : (m_intent == Intent::BACK) ? L"後退" : L"様子見";
			Explain(label, [&] { return Format(L"お互いの技が届かない距離（%.2f）", distance); });
		}
	}

	switch (m_intent)
	{
	case Intent::FORWARD: HoldForward(facingRight, in); break;
	case Intent::BACK:    HoldBack(facingRight, in);    break;
	case Intent::GUARD:
		HoldBack(facingRight, in);
		in.moveDown = m_guardLow;
		break;
	case Intent::WAIT: break;
	}
}

// 相手が今いる場所から、反応速度のあいだに歩いて詰めた位置で技を出したとして、
// どれかの通常技が自分のくらい判定に届くか
bool CpuController::InThreatRange(const Player& self, const Player& opponent, const Snapshot& seen) const
{
	std::array<BoundingBox, MAX_HURTBOXES> myHurt;
	const int hurtCount = CollectHurtboxes(self, myHurt);

	const float towardMe = (self.GetPosition().x > seen.x) ? 1.0f : -1.0f;
	const float approachX = seen.x + towardMe * opponent.GetMoveSpeed() * ReactionSec();
	const bool facingMe = (towardMe > 0.0f);

	for (int id = 0; id < NORMAL_MOVE_COUNT; ++id)
	{
		if (HitboxReaches(ParamsOf(opponent, id), approachX, seen.y, facingMe, myHurt.data(), hurtCount, 0.0f, 0.0f, GUARD_REACH_SCALE))
		{
			return true;
		}
	}
	return false;
}

// 見えた相手の技を、段ごとに数えておく（1つの技につき1回）
void CpuController::RememberOpponentHabit(const Player& opponent, const Snapshot& seen)
{
	const bool isNewMove = (seen.move != MOVE_NONE) &&
		(seen.move != m_lastSeenMove || seen.moveElapsed < m_lastSeenElapsed);

	if (isNewMove && seen.move < NORMAL_MOVE_COUNT)
	{
		const AttackLevel level = ParamsOf(opponent, seen.move).attackLevel;
		if (level != AttackLevel::HIGH)
		{
			m_midUsage *= HABIT_DECAY;
			m_lowUsage *= HABIT_DECAY;
			if (level == AttackLevel::LOW) m_lowUsage += 1.0f;
			else                           m_midUsage += 1.0f;
		}
	}
	m_lastSeenMove = seen.move;
	m_lastSeenElapsed = seen.moveElapsed;
}

// しゃがみでガードを固める確率。何も見ていないうちは五分五分から始める
float CpuController::LowGuessRate() const
{
	return (m_lowUsage + 1.0f) / (m_lowUsage + m_midUsage + 2.0f);
}

// 届く技の中から始動技を選ぶ。キャンセルで繋げられる技と、威力の高い技ほど選ばれやすくする
int CpuController::ChooseStarter(const Player& self, const int* candidates, int count)
{
	float weights[NORMAL_MOVE_COUNT];
	float total = 0.0f;
	for (int i = 0; i < count; ++i)
	{
		const AttackParams& p = ParamsOf(self, candidates[i]);
		weights[i] = (1.0f + p.damage / 400.0f) * (p.cancelEnabled ? 1.5f : 1.0f);
		total += weights[i];
	}

	float r = Rand01() * total;
	for (int i = 0; i < count; ++i)
	{
		if (r < weights[i]) return candidates[i];
		r -= weights[i];
	}
	return candidates[count - 1];
}

// 下 → 斜め下前 → 前＋パンチ。コマンドの受付時間(0.3秒)に余裕を持って収まる長さにしている
void CpuController::StartFireballScript(bool facingRight, int strength)
{
	PlayerInputs down;
	down.moveDown = true;

	PlayerInputs downForward = down;
	HoldForward(facingRight, downForward);

	PlayerInputs forwardPunch;
	HoldForward(facingRight, forwardPunch);
	if (strength == 0)      forwardPunch.LightPunch = true;
	else if (strength == 1) forwardPunch.MediumPunch = true;
	else                    forwardPunch.HeavyPunch = true;

	m_script[0] = { down, 2 };
	m_script[1] = { downForward, 2 };
	m_script[2] = { forwardPunch, 1 };
	m_scriptLength = 3;
	m_scriptStep = 0;
	m_scriptFrame = 0;
}

bool CpuController::RunScript(PlayerInputs& in)
{
	if (m_scriptStep >= m_scriptLength) return false;

	in = m_script[m_scriptStep].input;
	if (++m_scriptFrame >= m_script[m_scriptStep].frames)
	{
		m_scriptFrame = 0;
		++m_scriptStep;
	}
	return true;
}

// ============================================================
// 補助
// ============================================================

float CpuController::ReactionSec() const
{
	return Profile().reactionFrames * FRAME_SEC;
}

float CpuController::ConfirmSec() const
{
	return Profile().confirmFrames * FRAME_SEC;
}

bool CpuController::Roll(float probability)
{
	return Rand01() < probability;
}

float CpuController::Rand01()
{
	return std::uniform_real_distribution<float>(0.0f, 1.0f)(m_rng);
}

int CpuController::RandomRange(int minValue, int maxValue)
{
	return std::uniform_int_distribution<int>(minValue, maxValue)(m_rng);
}

const AttackParams& CpuController::ParamsOf(const Player& player, int move)
{
	switch (move)
	{
	case MOVE_LP:         return player.GetLightPunchParams();
	case MOVE_MP:         return player.GetMediumPunchParams();
	case MOVE_HP:         return player.GetHeavyPunchParams();
	case MOVE_MK:         return player.GetMediumKickParams();
	case MOVE_HK:         return player.GetHeavyKickParams();
	case MOVE_FIREBALL_L: return player.GetHadoukenLParams();
	case MOVE_FIREBALL_M: return player.GetHadoukenMParams();
	default:              return player.GetHadoukenHParams();
	}
}

// 今出している技を、パラメータの実体のアドレスで見分ける。
// のけぞり・ダウン中や、技が終わって動ける状態に戻ったあとは技の最中として扱わない
int CpuController::IdentifyMove(const Player& player)
{
	const AttackParams* current = player.GetCurrentAttackParams();
	if (!current || player.IsActionable() || player.IsInHitstun() || player.IsInvincible()) return MOVE_NONE;

	for (int id = 0; id < MOVE_COUNT; ++id)
	{
		if (current == &ParamsOf(player, id)) return id;
	}
	return MOVE_NONE;
}

int CpuController::CollectHurtboxes(const Player& player, std::array<BoundingBox, MAX_HURTBOXES>& out)
{
	int count = 0;
	for (int type = 0; type < (int)HurtboxType::COUNT; ++type)
	{
		out[count++] = player.GetHurtbox((HurtboxType)type);
	}
	for (const BoundingBox& box : player.GetActiveHurtboxes())
	{
		if (count >= MAX_HURTBOXES) break;
		out[count++] = box;
	}
	return count;
}

bool CpuController::CancelAllowed(const AttackParams& from, int to)
{
	switch (to)
	{
	case MOVE_LP: return from.cancelToLight;
	case MOVE_MP: return from.cancelToMedium;
	case MOVE_HP: return from.cancelToHeavyPunch;
	case MOVE_MK: return from.cancelToMediumKick;
	case MOVE_HK: return from.cancelToHeavy;
	default:      return false;
	}
}

void CpuController::PressMove(int move, PlayerInputs& in)
{
	switch (move)
	{
	case MOVE_LP: in.LightPunch = true;  break;
	case MOVE_MP: in.MediumPunch = true; break;
	case MOVE_HP: in.HeavyPunch = true;  break;
	case MOVE_MK: in.MediumKick = true;  break;
	case MOVE_HK: in.HeavyKick = true;   break;
	default: break;
	}
}

const wchar_t* CpuController::MoveName(int move)
{
	static const wchar_t* NAMES[MOVE_COUNT] =
	{
		L"弱パンチ", L"中パンチ", L"強パンチ", L"中キック", L"強キック",
		L"弱波動拳", L"中波動拳", L"強波動拳"
	};
	return (move >= 0 && move < MOVE_COUNT) ? NAMES[move] : L"なし";
}

void CpuController::UpdateDebugHeader(const Snapshot& seen)
{
	const CpuProfile& profile = Profile();
	m_debug.header = Format(L"CPU %s　反応 %dF　ヒット確認 %dF", profile.name, profile.reactionFrames, profile.confirmFrames);

	std::wstring state;
	if (seen.dead)                   state = L"決着";
	else if (seen.move != MOVE_NONE) state = Format(L"%s の %dF 目", MoveName(seen.move), ToFrames(seen.moveElapsed));
	else if (seen.invincible)        state = L"ダウン中";
	else if (seen.hitstun)           state = L"のけぞり中";
	else if (seen.jumping)           state = L"ジャンプ中";
	else                             state = L"技を出していない";
	m_debug.perception = Format(L"%dF 前の相手：%s", profile.reactionFrames, state.c_str());
}

// 次に始まる技へ、ヒット確認の結果を引き継ぐ。
// 押したボタンが入力バッファ(0.1秒)のうちに技にならなければ、引き継ぎは無効にする
void CpuController::InheritCombo(ComboDecision decision)
{
	m_inheritedCombo = decision;
	m_inheritDeadline = m_clock + 0.1f;
}
