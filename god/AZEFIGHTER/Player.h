#pragma once
#include "Model.h"
#include <DirectXMath.h>
#include <DirectXCollision.h>
#include <string>
#include <vector>
#include <memory>
#include "Geometory.h"
#include "NormalMoves.h"

// 前方宣言
class PlayerState;
class Shader;
class Projectile;

// デバイスのタイプ
enum class InputDeviceType
{
	KEYBOARD,
	PAD_0,
	PAD_1,
	PAD_2,
	PAD_3
};

struct KeyConfig
{
	int up;
	int down;
	int left;
	int right;
	int lightPunch;
	int mediumPunch;
	int heavyPunch;
	int lightKick;
	int mediumKick;
	int heavyKick;
};

struct PadConfig
{
	int up;
	int down;
	int left;
	int right;
	int lightPunch;
	int mediumPunch;
	int heavyPunch;
	int lightKick;
	int mediumKick;
	int heavyKick;
};

extern KeyConfig g_keyConfigP1;
extern KeyConfig g_keyConfigP2;

extern PadConfig g_padConfigP1;
extern PadConfig g_padConfigP2;

extern InputDeviceType g_inputDeviceP1;
extern InputDeviceType g_inputDeviceP2;

// プレイヤーの操作権限（1P、2P、AI）
enum class PlayerInputType
{
	PLAYER_1, // 1P
	PLAYER_2, // 2P
	AI,       // 注入された入力で動く。歩き状態には遷移しない（ラウンド前後の操作ロック、トレーニングのダミー）
	CPU       // 注入された入力で動く。状態遷移は人間と同じ（CPU対戦）
};

//ステート
// 現在のフレームで入力されているキー・ボタン情報
struct PlayerInputs
{
	bool moveLeft = false;
	bool moveRight = false;
	bool moveDown = false;
	bool jump = false;

	bool LightPunch = false;
	bool MediumPunch = false;
	bool HeavyPunch = false;
	bool LightKick = false;
	bool MediumKick = false;
	bool HeavyKick = false;
};

// 再生中のアニメーション名と現在のフレーム数を管理
struct AnimationState
{
	const char* name = nullptr;
	float frame = 0.0f;
};

// くらい判定（やられ判定）の部位
enum class HurtboxType
{
	HEAD,
	BODY,
	LEGS,
	COUNT
};

// 攻撃の判定レベル（上段・中段・下段）
enum class AttackLevel
{
	HIGH,
	MID,
	LOW
};

// アニメーションの特定区間で再生速度を変更するためのモディファイア
struct AnimSpeedModifier
{
	float startFrame = 0.0f;
	float endFrame = 0.0f;
	float speed = 1.0f;
};

// 当たり判定の基準となるオフセットとサイズ
struct BoxData
{
	DirectX::XMFLOAT2 offset = { 0.0f, 0.0f };
	DirectX::XMFLOAT2 extents = { 0.5f, 0.5f };
};

// 特定のフレームにおける当たり判定の情報（キーフレーム）
struct BoxKeyframe
{
	float frame = 0.0f;
	BoxData data;
};

// キーフレーム間で補間される当たり判定のコンテナ
struct AnimatedBox
{
	std::vector<BoxKeyframe> keyframes;
};

// 技ごとの追加くらい判定（指定フレーム区間の間だけ出る固定サイズのボックス）
// 基本のくらい判定は常時有効で、これはその上に区間限定で加算される
struct WindowedHurtbox
{
	DirectX::XMFLOAT2 offset = { 0.0f, 1.0f };  // 中心オフセット
	DirectX::XMFLOAT2 extents = { 0.3f, 0.3f }; // 半サイズ
	int startFrame = 0; // 出現フレーム(60FPS基準)
	int endFrame = 0;   // 消滅フレーム(60FPS基準)
};

// 攻撃技に関するすべてのパラメータをまとめた構造体
struct AttackParams
{
	// --- タイミング (秒) ---
	// 攻撃モーションの全体フレーム（秒）
	float totalDuration = 0.5f;
	// 攻撃判定（Hitbox）が発生するタイミング
	float hitboxStart = 0.1f;
	// 攻撃判定（Hitbox）が消失するタイミング
	float hitboxEnd = 0.2f;

	// 複数の攻撃判定 (赤枠) のリスト
	std::vector<AnimatedBox> hitboxes;

	// 攻撃中のくらい判定 (緑枠) のリスト
	// 区間限定で出る追加のくらい判定。基本のくらい判定は常時有効なので別管理
	std::vector<WindowedHurtbox> moveHurtboxes;

	// 攻撃の属性（ガード方向の判定に使用）
	AttackLevel attackLevel = AttackLevel::HIGH;

	// --- ゲームプレイ用パラメータ ---
	// 与える基本ダメージ
	int damage = 10;
	// ヒット時の有利フレーム
	int hitFrame = 5;
	// ガード時の不利（有利）フレーム
	int blockFrame = -2;

	// 攻撃ヒット時にお互いの動きを止める時間
	float hitStop = 0.1f;
	// ヒット時のノックバック量
	float knockback = 0.5f;
	// 相手をダウンさせる攻撃かどうか
	bool isDown = false;

	// --- キャンセル設定 ---
	// 別の技へのキャンセル入力を受け付けるかどうか
	bool cancelEnabled = false;
	// キャンセル受付開始時間
	float cancelStart = 0.0f;
	// キャンセル受付終了時間
	float cancelEnd = 0.0f;

	// どの強度・種類の技へキャンセル可能か
	bool cancelToLight = false;
	bool cancelToMedium = false;
	bool cancelToHeavyPunch = false;
	bool cancelToLightKick = false;
	bool cancelToMediumKick = false;
	bool cancelToHeavy = false;

	// 攻撃モーション中の再生速度変化のリスト
	std::vector<AnimSpeedModifier> speedModifiers;

	// --- 飛び道具用パラメータ ---
	// 発射する飛び道具の速度とサイズ
	float projectileSpeed = 5.0f;
	float projectileSize = 1.0f;

	// --- 無敵・移動 (昇竜拳など) ---
	// 技の開始からこの時間まで、打撃も飛び道具も受けない (秒)
	float invincibleEnd = 0.0f;
	// 飛び上がるときの前方向の速さ
	float forwardSpeed = 0.0f;
};


class Player
{
public:
	Player();
	~Player();
	bool Load(const char* file, float scale = 1.0f, bool flip = false, bool simple = false);
	void SetVertexShader(Shader* vs);
	void SetPixelShader(Shader* ps);
	void Draw();
	Model* GetModel();

	void SetCurrentFrame(float frame);

	void Update(float tick);
	void SetState(std::unique_ptr<PlayerState> newState);

	bool IsInvincible() const;
	bool IsInHitstun() const;
	// 自分の意思で次の行動を始められるか（攻撃中・のけぞり中・ダウン中・空中は不可）
	bool IsActionable() const;

	void SetInputType(PlayerInputType type);
	PlayerInputType GetInputType() const;

	const PlayerInputs& GetInputs() const;
	void SetInjectedInputs(const PlayerInputs& in);
	void RefillHp() { m_hp = m_maxHp; }
	static bool s_variableSpeed; // 緩急（可変フレーム再生）のオン/オフ。等速と見比べる用
	static constexpr float GRAVITY = 55.0f;       // 重力加速度。CPUが着地のタイミングを予測するのにも使う
	static constexpr float JUMP_VELOCITY = 19.0f; // ジャンプの初速
	static constexpr float MODEL_SCALE = 0.014f;   // モデルの cm をゲームの座標に直す倍率
	// 後ろ歩きは前歩きより遅くする。下がりながら安全に間合いを外しにくくする、格闘ゲームの定番
	static constexpr float BACK_WALK_SPEED_RATE = 0.8f;
#ifdef _DEBUG
	// デバッグコマンド用
	void Debug_SetHp(int hp) { m_hp = hp; }
#endif
	void SetGuardAllLevels(bool b) { m_guardAllLevels = b; }

	void PlayAnimation(const char* name, bool forceRestart = false);
	void SetAnimPause(bool pause);
	void SetAnimationSpeed(float speed);
	bool IsAnimEnd() const;

	float GetForwardMoveDot() const;

	void SetPosition(const DirectX::XMFLOAT3& pos);
	DirectX::XMFLOAT3 GetPosition() const;
	void SetRotation(const DirectX::XMFLOAT3& rot);
	DirectX::XMFLOAT3 GetRotation() const;
	DirectX::XMFLOAT3 GetVelocity() const;
	void SetVelocity(const DirectX::XMFLOAT3& vel);
	void Jump();
	void ForceJumpState(bool isJumping);
	bool GetIsJumping() const;

	// --- 当たり判定 (Hurtbox) ---
	void SetHurtboxBase(HurtboxType type, const DirectX::XMFLOAT2& offset, const DirectX::XMFLOAT2& extents);
	DirectX::XMFLOAT2 GetHurtboxBaseOffset(HurtboxType type) const;
	DirectX::XMFLOAT2 GetHurtboxBaseExtents(HurtboxType type) const;

	void SetHurtboxCrouch(HurtboxType type, const DirectX::XMFLOAT2& offset, const DirectX::XMFLOAT2& extents);
	DirectX::XMFLOAT2 GetHurtboxCrouchOffset(HurtboxType type) const;
	DirectX::XMFLOAT2 GetHurtboxCrouchExtents(HurtboxType type) const;

	DirectX::BoundingBox GetHurtbox(HurtboxType type) const;

	void SetIsCrouching(bool isCrouching);
	bool GetIsCrouching() const;

	void DrawBoundingBox();
	bool CheckCollision(const Player* other) const;


	void SetBoundingBoxExtents(const DirectX::XMFLOAT2& extents);
	DirectX::XMFLOAT2 GetBoundingBoxExtents() const;
	void SetBoundingBoxOffset(const DirectX::XMFLOAT2& offset);
	DirectX::XMFLOAT2 GetBoundingBoxOffset() const;
	DirectX::BoundingBox GetBoundingBox() const;

	// --- 攻撃判定 (Hitbox) 用 ---
	const std::vector<DirectX::BoundingBox>& GetActiveHitboxes() const;
	const std::vector<DirectX::BoundingBox>& GetActiveHurtboxes() const;

	bool IsAttacking() const;
	void SetActiveHitbox(bool isActive);

	// パラメータに基づいてボックスを更新する関数
	void UpdateAttackBoxes();
	void UpdateInputBuffer(float tick);
	void DrawActiveHurtboxes();

	void DrawHitbox();

	void SetIsColliding(bool isColliding);
	bool GetIsColliding() const;
	void SetMoveSpeed(float speed);
	float GetMoveSpeed() const;
	float GetWalkSpeed(bool back) const; // 前歩き・後ろ歩きの移動速度
	void SetJumpSpeed(float speed);
	float GetJumpSpeed() const;
	void SetScale(const DirectX::XMFLOAT3& scale);
	DirectX::XMFLOAT3 GetScale() const;

	void UpdateAnimation(float tick);
	void UpdateModelBlend();

	// 通常技のパラメータ。NormalMove で引く
	AttackParams& GetNormalParams(NormalMove move) { return m_normalParams[(int)move]; }

	// 波動拳用パラメータ
	AttackParams& GetHadoukenLParams() { return m_hadoukenLParams; }
	AttackParams& GetHadoukenMParams() { return m_hadoukenMParams; }
	AttackParams& GetHadoukenHParams() { return m_hadoukenHParams; }

	// 昇竜拳用パラメータ
	AttackParams& GetShoryukenLParams() { return m_shoryukenLParams; }
	AttackParams& GetShoryukenMParams() { return m_shoryukenMParams; }
	AttackParams& GetShoryukenHParams() { return m_shoryukenHParams; }

	void SetCurrentAttackParams(AttackParams* params);
	AttackParams* GetCurrentAttackParams() const;

	// 読み取り専用のパラメータ参照。CPU が相手の技のフレームデータを読むのに使う
	const AttackParams& GetNormalParams(NormalMove move) const { return m_normalParams[(int)move]; }
	const AttackParams& GetHadoukenLParams() const { return m_hadoukenLParams; }
	const AttackParams& GetHadoukenMParams() const { return m_hadoukenMParams; }
	const AttackParams& GetHadoukenHParams() const { return m_hadoukenHParams; }
	const AttackParams& GetShoryukenLParams() const { return m_shoryukenLParams; }
	const AttackParams& GetShoryukenMParams() const { return m_shoryukenMParams; }
	const AttackParams& GetShoryukenHParams() const { return m_shoryukenHParams; }

	void Debug_SetAnimation(const char* name, bool forceRestart = true) {
		PlayAnimation(name, forceRestart);
		m_blendFactor = 1.0f;
	}
	int Debug_GetFrame() const {
		return (int)m_currentAnim.frame;
	}
	void Debug_SetFrame(int frame) {
		m_currentAnim.frame = (float)frame;
	}

	// 攻撃タイマー設定
	void SetAttackTimer(float timer) { m_attackTimer = timer; }
	float GetAttackTimer() const { return m_attackTimer; }
	float GetAttackElapsedSec() const { return m_attackElapsedSec; }

	void ReceiveDamage(int damage, AttackLevel atkLevel = AttackLevel::HIGH);
	float GetHpRatio() const;

	void Reset();

	bool HasHit() const { return m_hasHit; }
	void OnHit() { m_hasHit = true; }

	// 飛び道具制御
	Projectile* GetProjectile() { return m_projectile.get(); }
	const Projectile* GetProjectile() const { return m_projectile.get(); }
	bool CanFireProjectile() const;

	// コマンド判定
	void UpdateCommandTimer(float tick);
	bool CheckHadoukenCommand() const;
	bool CheckShoryukenCommand() const;

	// ガード判定関連
	bool IsGuarding() const;
	bool IsGuardingHigh() const;
	bool IsGuardingLow() const;
	bool TryGuard(AttackLevel atkLevel) const;

	// 押し戻し。瞬間移動させず、数フレームかけて減速しながら滑らせる
	void StartPushback(float distance);  // 符号つき。+X 方向が正
	float TakePushbackStep(float tick);  // このフレームに動く量を返し、残りから引く

private:
	// 初期パラメータ設定用関数
	void InitDefaultParameters();

	void UpdatePhysics(float tick);
	void PollInputs();

	// プレイヤーモデルのインスタンス
	std::shared_ptr<Model> m_model;

	// トランスフォーム情報
	DirectX::XMFLOAT3 m_position;
	DirectX::XMFLOAT3 m_rotation;
	DirectX::XMFLOAT3 m_scale;
	DirectX::XMFLOAT3 m_velocity;

	bool m_isJumping;
	float m_moveSpeed;
	float m_jumpSpeed;

	// 立ち状態の部位ごとのくらい判定ベースオフセット・サイズ
	DirectX::XMFLOAT2 m_baseHurtboxOffsets[(int)HurtboxType::COUNT];
	DirectX::XMFLOAT2 m_baseHurtboxExtents[(int)HurtboxType::COUNT];

	// しゃがみ状態の部位ごとのくらい判定ベースオフセット・サイズ
	DirectX::XMFLOAT2 m_crouchHurtboxOffsets[(int)HurtboxType::COUNT];
	DirectX::XMFLOAT2 m_crouchHurtboxExtents[(int)HurtboxType::COUNT];

	// 他のプレイヤーと重なっているかどうかのフラグ
	bool m_isColliding = false;
	// 現在しゃがんでいるかどうかのフラグ
	bool m_isCrouching = false;
	float m_pushbackLeft = 0.0f; // まだ滑り終えていない押し戻しの量（符号つき）

	// 複数の攻撃判定を保持
	// 現在アクティブな攻撃判定と、攻撃モーション中専用のくらい判定
	std::vector<DirectX::BoundingBox> m_activeHitboxes;
	std::vector<DirectX::BoundingBox> m_activeHurtboxes;

	// 攻撃中であるかを示すフラグ
	bool m_isAttacking = false;

	// ステートパターン用ポインタ
	std::unique_ptr<PlayerState> m_currentState;
	// プレイヤーの操作タイプ（人間かAIか）
	PlayerInputType m_inputType;
	// 現在の入力状況
	PlayerInputs m_inputs;
	PlayerInputs m_injectedInputs;
	bool m_guardAllLevels = false;
	float m_atkBuf[ATTACK_BUTTON_COUNT] = {};
	float m_attackElapsedSec = 0.0f;

	// アニメーションブレンド用管理変数
	AnimationState m_currentAnim;
	AnimationState m_previousAnim;
	float m_blendFactor = 1.0f;
	float m_transitionDuration = 0.1f; // 前のモーションと混ぜる時間（秒）。切り替え先で変わる
	bool m_isAnimPaused = false;

	// 現在のアニメーション再生倍率
	float m_animSpeed = 1.0f;

	// 各攻撃技のパラメータ実体
	AttackParams m_normalParams[NORMAL_MOVE_COUNT];

	// 波動拳パラメータ
	AttackParams m_hadoukenLParams;
	AttackParams m_hadoukenMParams;
	AttackParams m_hadoukenHParams;

	// 昇竜拳パラメータ
	AttackParams m_shoryukenLParams;
	AttackParams m_shoryukenMParams;
	AttackParams m_shoryukenHParams;

	// 現在実行中の攻撃技のパラメータへのポインタ
	AttackParams* m_pActiveAttackParams = nullptr;

	//  攻撃の経過時間を正確に測るタイマー
	float m_attackTimer = 0.0f;

	// 体力管理
	int m_hp;
	const int m_maxHp = 10000;

	// 攻撃が既にヒットしたか（多段ヒット防止用）
	bool m_hasHit = false;

	// 飛び道具インスタンス
	std::unique_ptr<Projectile> m_projectile;

	// コマンドバッファ
	// 必殺技コマンド入力を保持するタイマー
	float m_cmdTimerDown = 0.0f;
	float m_cmdTimerDownForward = 0.0f;
	// 昇竜拳 (下、下) の途中経過
	float m_cmdTimerDownTap = 0.0f;   // 1回目の下を押してからの受付時間
	float m_cmdTimerDownTwice = 0.0f; // 2回目の下を押してからの受付時間
	bool m_prevMoveDown = false;      // 押した瞬間を取るための、前のフレームの下入力
	// コマンドの入力受付猶予時間
	const float CMD_WINDOW = 0.3f;
};