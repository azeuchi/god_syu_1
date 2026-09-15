#pragma once
#include <array>
#include <random>
#include <string>
#include <DirectXCollision.h>
#include "Player.h"
#include "MatchConfig.h"

// 難易度ごとの設定値
// 見てから対応できるかどうかは反応速度だけで決め、読みや好みで変わる行動だけを確率にしている
struct CpuProfile
{
	const wchar_t* name;
	int   reactionFrames;   // 相手の動きを判断に使えるようになるまでの遅れ
	int   confirmFrames;    // 自分の技がヒットしたかガードされたかを見分けるまでの遅れ
	float comboCommitRate;  // 見分けが間に合わないとき、確認せずに連携を出し切る確率
	int   thinkMinFrames;   // 立ち回りを考え直す間隔の下限
	int   thinkMaxFrames;   // 同じく上限。毎回ばらつかせて、行動のリズムを読まれにくくする
	float aggression;       // 技が届く間合いで攻撃を選ぶ確率
	float guardRate;        // 相手の技が届く間合いで、攻めないときにガードを固めておく確率
	float fireballRate;     // 技が届かない遠い間合いで波動拳を選ぶ確率
	float jumpOverRate;     // 飛び越えられる飛び道具を、ガードではなくジャンプでかわす確率
};

// 思考表示（F6）に出す文字列
struct CpuDebugInfo
{
	std::wstring header;
	std::wstring perception;
	std::wstring decision;
	std::wstring reason;
};

/**
 * @brief CPU対戦の思考ルーチン
 *
 * ・ステートを直接切り替えず、PlayerInputs（仮想のボタン入力）だけを作って注入する。
 *   入力バッファ、キャンセル条件、コマンドの受付時間など、人間と同じルールの上で動かすため
 * ・相手の情報は反応速度ぶん古いものしか使わない。
 *   ただし自分の技が相手に触れたことは、ヒットストップで体に伝わるので遅れなしで扱う
 * ・技の発生・持続・硬直・リーチは、その場で AttackParams から読む。
 *   デバッグシーンで調整した値が、そのまま CPU の判断にも反映される
 */
class CpuController
{
public:
	explicit CpuController(CpuLevel level);

	// self の Update より前に毎フレーム呼ぶ。作った入力は self に注入する
	// realTick は反応の遅れを測る実時間、gameTick はヒットストップで止まるゲーム内の時間
	void Update(Player& self, const Player& opponent, float realTick, float gameTick);

	// ラウンドの区切りで記憶を消し、注入していた入力も空にする
	void Reset(Player& self);

	// 思考表示の文字列は、表示しているときだけ組み立てる
	void SetDebugEnabled(bool enabled) { m_debugEnabled = enabled; }
	const CpuDebugInfo& GetDebugInfo() const { return m_debug; }

	static const CpuProfile& GetProfile(CpuLevel level);

private:
	enum MoveId
	{
		MOVE_NONE = -1,
		MOVE_LP, MOVE_MP, MOVE_HP, MOVE_MK, MOVE_HK,
		MOVE_FIREBALL_L, MOVE_FIREBALL_M, MOVE_FIREBALL_H,
		MOVE_COUNT
	};
	static constexpr int NORMAL_MOVE_COUNT = MOVE_FIREBALL_L;

	// 観測を何フレーム分さかのぼれるか。最も遅い反応速度より十分長くしておく
	static constexpr int HISTORY_SIZE = 128;
	static constexpr int MAX_HURTBOXES = 8;

	// ある瞬間に相手がどう見えていたか
	struct Snapshot
	{
		float time = 0.0f;     // 実時間。反応の遅れを数えるのに使う
		float gameTime = 0.0f; // ゲーム内の時間。相手の技や弾が今どこまで進んだかの予測に使う
		float x = 0.0f;
		float y = 0.0f;
		float vx = 0.0f;
		float vy = 0.0f;
		bool facingRight = false;
		bool jumping = false;
		bool hitstun = false;
		bool invincible = false;
		bool dead = false;
		int move = MOVE_NONE;
		float moveElapsed = 0.0f;
		// 基本のくらい判定に加えて、技の最中だけ伸びる手足の判定も含める
		std::array<DirectX::BoundingBox, MAX_HURTBOXES> hurt{};
		int hurtCount = 0;
		bool projectileActive = false;
		DirectX::BoundingBox projectile{};
	};

	// 自分の技が相手に触れたあと、次の技を出すかどうか
	enum class ComboDecision
	{
		WAITING,          // ヒットかガードかの見分け待ち
		HIT_CONFIRMED,    // ヒットを確認した
		BLOCK_CONFIRMED,  // ガードされたのを確認した
		COMMIT,           // 確認は間に合わないが出し切る
		DROP,             // 確認が間に合わないのでやめる
		DONE              // この技での判断は済んだ
	};

	// 立ち回りの方針。次に考え直すまで続ける
	enum class Intent
	{
		WAIT,
		FORWARD,
		BACK,
		GUARD
	};

	// 波動拳のように、何フレームかにまたがる入力の1区切り
	struct ScriptStep
	{
		PlayerInputs input;
		int frames = 0;
	};

	// 観測
	void Record(const Player& opponent);
	int PerceiveIndex(float delaySec) const;
	float EstimateProjectileSpeed(int seenIndex) const;

	// 判断。上にあるものほど優先する
	void ThinkDuringOwnMove(const Player& self, PlayerInputs& in);
	bool ThinkDefense(const Player& self, const Player& opponent, const Snapshot& seen, PlayerInputs& in);
	bool ThinkPunish(const Player& self, const Player& opponent, const Snapshot& seen, PlayerInputs& in);
	bool ThinkAntiAir(const Player& self, const Snapshot& seen, PlayerInputs& in);
	bool GuardAfterBlock(const Player& self, float tick, PlayerInputs& in);
	bool RunScript(PlayerInputs& in);
	void ThinkNeutral(const Player& self, const Player& opponent, const Snapshot& seen, float tick, PlayerInputs& in);

	// 補助
	void StartFireballScript(bool facingRight, int strength);
	void InheritCombo(ComboDecision decision);
	bool InThreatRange(const Player& self, const Player& opponent, const Snapshot& seen) const;
	void RememberOpponentHabit(const Player& opponent, const Snapshot& seen);
	float LowGuessRate() const;
	int ChooseStarter(const Player& self, const int* candidates, int count);
	void UpdateDebugHeader(const Snapshot& seen);
	const CpuProfile& Profile() const { return GetProfile(m_level); }
	float ReactionSec() const;
	float ConfirmSec() const;
	bool Roll(float probability);
	float Rand01();
	int RandomRange(int minValue, int maxValue);

	static const AttackParams& ParamsOf(const Player& player, int move);
	static int IdentifyMove(const Player& player);
	static int CollectHurtboxes(const Player& player, std::array<DirectX::BoundingBox, MAX_HURTBOXES>& out);
	static bool CancelAllowed(const AttackParams& from, int to);
	static void PressMove(int move, PlayerInputs& in);
	static const wchar_t* MoveName(int move);

	// 表示が有効なときだけ reason を組み立てる。毎フレームの文字列生成を避けるため関数で受け取る
	template<class MakeReason>
	void Explain(const wchar_t* decision, MakeReason&& makeReason)
	{
		if (!m_debugEnabled) return;
		m_debug.decision = decision;
		m_debug.reason = makeReason();
	}

	CpuLevel m_level;
	std::mt19937 m_rng;

	float m_clock = 0.0f;
	float m_gameClock = 0.0f;
	std::array<Snapshot, HISTORY_SIZE> m_history{};
	int m_head = 0;
	int m_count = 0;
	int m_seenIndex = 0;

	// 自分の技の追跡
	const AttackParams* m_ownMove = nullptr;
	float m_ownMoveElapsed = 0.0f;
	bool m_contact = false;
	float m_contactTime = 0.0f;
	ComboDecision m_combo = ComboDecision::WAITING;
	// キャンセルで出した次の技に、確認済みかどうかを引き継ぐ
	ComboDecision m_inheritedCombo = ComboDecision::WAITING;
	float m_inheritDeadline = 0.0f; // ボタンが技にならなかったとき、無関係な技へ引き継がないための期限

	float m_guardAfterBlockTimer = 0.0f;
	bool m_hitstunGuardRolled = false;
	bool m_hitstunGuard = false;

	bool m_prevSeenProjectile = false;
	bool m_jumpOverThisProjectile = false;

	Intent m_intent = Intent::WAIT;
	bool m_guardLow = false;       // ガードを固めるときにしゃがむか
	bool m_wasThreatened = false;  // 前のフレームで相手の技が届く間合いにいたか

	// 相手が使った中段・下段の回数。古いものほど軽くして、最近の癖を重く見る
	float m_midUsage = 0.0f;
	float m_lowUsage = 0.0f;
	int m_lastSeenMove = MOVE_NONE;
	float m_lastSeenElapsed = 0.0f;
	float m_thinkTimer = 0.0f;

	std::array<ScriptStep, 4> m_script{};
	int m_scriptLength = 0;
	int m_scriptStep = 0;
	int m_scriptFrame = 0;

	PlayerInputs m_prevInput;

	bool m_debugEnabled = false;
	CpuDebugInfo m_debug;
};
