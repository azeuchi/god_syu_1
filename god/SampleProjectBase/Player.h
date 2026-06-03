#pragma once
#include "Model.h"
#include <DirectXMath.h>
#include <DirectXCollision.h>
#include <string>
#include <vector>
#include "Geometory.h"

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
	AI        // AI
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
	bool cancelToMediumKick = false;
	bool cancelToHeavy = false;

	// 攻撃モーション中の再生速度変化のリスト
	std::vector<AnimSpeedModifier> speedModifiers;

	// --- 飛び道具用パラメータ ---
	// 発射する飛び道具の速度とサイズ
	float projectileSpeed = 5.0f;
	float projectileSize = 1.0f;
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
	void SetState(PlayerState* newState);

	bool IsInvincible() const;

	void SetInputType(PlayerInputType type);
	PlayerInputType GetInputType() const;

	const PlayerInputs& GetInputs() const;

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

	void DrawHitbox();

	void SetIsColliding(bool isColliding);
	bool GetIsColliding() const;
	void SetMoveSpeed(float speed);
	float GetMoveSpeed() const;
	void SetJumpSpeed(float speed);
	float GetJumpSpeed() const;
	void SetScale(const DirectX::XMFLOAT3& scale);
	DirectX::XMFLOAT3 GetScale() const;

	void UpdateAnimation(float tick);
	void UpdateModelBlend();

	AttackParams& GetLightPunchParams() { return m_lightPunchParams; }
	AttackParams& GetMediumPunchParams() { return m_mediumPunchParams; }
	AttackParams& GetHeavyPunchParams() { return m_heavyPunchParams; }
	AttackParams& GetMediumKickParams() { return m_mediumKickParams; }
	AttackParams& GetHeavyKickParams() { return m_heavyKickParams; }

	// 波動拳用パラメータ
	AttackParams& GetHadoukenLParams() { return m_hadoukenLParams; }
	AttackParams& GetHadoukenMParams() { return m_hadoukenMParams; }
	AttackParams& GetHadoukenHParams() { return m_hadoukenHParams; }

	void SetCurrentAttackParams(AttackParams* params);
	AttackParams* GetCurrentAttackParams() const;

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

	void ReceiveDamage(int damage, AttackLevel atkLevel = AttackLevel::HIGH);
	float GetHpRatio() const;

	void Reset();

	bool HasHit() const { return m_hasHit; }
	void OnHit() { m_hasHit = true; }

	// 飛び道具制御
	Projectile* GetProjectile() { return m_projectile; }
	bool CanFireProjectile() const;

	// コマンド判定
	void UpdateCommandTimer(float tick);
	bool CheckHadoukenCommand() const;

	// ガード判定関連
	bool IsGuarding() const;
	bool IsGuardingHigh() const;
	bool IsGuardingLow() const;
	bool TryGuard(AttackLevel atkLevel) const;

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

	// 複数の攻撃判定を保持
	// 現在アクティブな攻撃判定と、攻撃モーション中専用のくらい判定
	std::vector<DirectX::BoundingBox> m_activeHitboxes;
	std::vector<DirectX::BoundingBox> m_activeHurtboxes;

	// 攻撃中であるかを示すフラグ
	bool m_isAttacking = false;

	// ステートパターン用ポインタ
	PlayerState* m_currentState;
	// プレイヤーの操作タイプ（人間かAIか）
	PlayerInputType m_inputType;
	// 現在の入力状況
	PlayerInputs m_inputs;

	// アニメーションブレンド用管理変数
	AnimationState m_currentAnim;
	AnimationState m_previousAnim;
	float m_blendFactor = 1.0f;
	const float m_transitionDuration = 0.1f;
	bool m_isAnimPaused = false;

	// 現在のアニメーション再生倍率
	float m_animSpeed = 1.0f;

	// 各攻撃技のパラメータ実体
	AttackParams m_lightPunchParams;
	AttackParams m_mediumPunchParams;
	AttackParams m_heavyPunchParams;
	AttackParams m_mediumKickParams;
	AttackParams m_heavyKickParams;

	// 波動拳パラメータ
	AttackParams m_hadoukenLParams;
	AttackParams m_hadoukenMParams;
	AttackParams m_hadoukenHParams;

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
	Projectile* m_projectile = nullptr;

	// コマンドバッファ
	// 必殺技コマンド入力を保持するタイマー
	float m_cmdTimerDown = 0.0f;
	float m_cmdTimerDownForward = 0.0f;
	// コマンドの入力受付猶予時間
	const float CMD_WINDOW = 0.3f;
};