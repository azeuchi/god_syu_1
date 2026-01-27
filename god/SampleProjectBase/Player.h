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

enum class PlayerInputType
{
	PLAYER_1, // 1P
	PLAYER_2, // 2P
	AI        // AI
};

//ステート
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

struct AnimationState
{
	const char* name = nullptr;
	float frame = 0.0f;
};

enum class HurtboxType
{
	HEAD,
	BODY,
	LEGS,
	COUNT
};

struct AnimSpeedModifier
{
	float startFrame = 0.0f;
	float endFrame = 0.0f;
	float speed = 1.0f;
};

struct BoxData
{
	DirectX::XMFLOAT2 offset = { 0.0f, 0.0f };
	DirectX::XMFLOAT2 extents = { 0.5f, 0.5f };
};

struct BoxKeyframe
{
	float frame = 0.0f;
	BoxData data;
};

struct AnimatedBox
{
	std::vector<BoxKeyframe> keyframes;
};

struct AttackParams
{
	// --- タイミング (秒) ---
	float totalDuration = 0.5f;
	float hitboxStart = 0.1f;
	float hitboxEnd = 0.2f;

	// 複数の攻撃判定 (赤枠) のリスト
	std::vector<AnimatedBox> hitboxes;

	// 攻撃中のくらい判定 (緑枠) のリスト
	std::vector<AnimatedBox> hurtboxes;

	// --- ゲームプレイ用パラメータ ---
	int damage = 10;
	int hitFrame = 5;
	int blockFrame = -2;

	float hitStop = 0.1f;
	float knockback = 0.5f;
	bool isDown = false;

	// --- キャンセル設定 ---
	bool cancelEnabled = false;
	float cancelStart = 0.0f;
	float cancelEnd = 0.0f;

	bool cancelToLight = false;
	bool cancelToMedium = false;
	bool cancelToHeavyPunch = false;
	bool cancelToMediumKick = false;
	bool cancelToHeavy = false;

	std::vector<AnimSpeedModifier> speedModifiers;

	// --- 飛び道具用パラメータ ---
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

	void ReceiveDamage(int damage);
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

private:
	// 初期パラメータ設定用関数
	void InitDefaultParameters();

	void UpdatePhysics(float tick);
	void PollInputs();

	std::shared_ptr<Model> m_model;
	DirectX::XMFLOAT3 m_position;
	DirectX::XMFLOAT3 m_rotation;
	DirectX::XMFLOAT3 m_scale;
	DirectX::XMFLOAT3 m_velocity;
	bool m_isJumping;
	float m_moveSpeed;

	DirectX::XMFLOAT2 m_baseHurtboxOffsets[(int)HurtboxType::COUNT];
	DirectX::XMFLOAT2 m_baseHurtboxExtents[(int)HurtboxType::COUNT];

	DirectX::XMFLOAT2 m_crouchHurtboxOffsets[(int)HurtboxType::COUNT];
	DirectX::XMFLOAT2 m_crouchHurtboxExtents[(int)HurtboxType::COUNT];

	bool m_isColliding = false;
	bool m_isCrouching = false;

	// 複数の攻撃判定を保持
	std::vector<DirectX::BoundingBox> m_activeHitboxes;
	std::vector<DirectX::BoundingBox> m_activeHurtboxes;

	bool m_isAttacking = false;

	PlayerState* m_currentState;
	PlayerInputType m_inputType;
	PlayerInputs m_inputs;

	AnimationState m_currentAnim;
	AnimationState m_previousAnim;
	float m_blendFactor = 1.0f;
	const float m_transitionDuration = 0.1f;
	bool m_isAnimPaused = false;

	float m_animSpeed = 1.0f;

	AttackParams m_lightPunchParams;
	AttackParams m_mediumPunchParams;
	AttackParams m_heavyPunchParams;
	AttackParams m_mediumKickParams;
	AttackParams m_heavyKickParams;

	// 波動拳パラメータ
	AttackParams m_hadoukenLParams;
	AttackParams m_hadoukenMParams;
	AttackParams m_hadoukenHParams;

	AttackParams* m_pActiveAttackParams = nullptr;

	//  攻撃の経過時間を正確に測るタイマー
	float m_attackTimer = 0.0f;

	int m_hp;
	const int m_maxHp = 10000;
	bool m_hasHit = false;

	// 飛び道具インスタンス
	Projectile* m_projectile = nullptr;

	// コマンドバッファ
	float m_cmdTimerDown = 0.0f;
	float m_cmdTimerDownForward = 0.0f;
	const float CMD_WINDOW = 0.3f;
};