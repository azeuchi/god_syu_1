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

// プレイヤーの入力タイプを明確化
enum class PlayerInputType
{
	PLAYER_1, // 1P
	PLAYER_2, // 2P
	AI        // AI
};

// 抽象化された入力状態を保持する構造体
struct PlayerInputs
{
	bool moveLeft = false;
	bool moveRight = false;
	bool moveDown = false;
	bool jump = false;

	// 名前を変更
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

// くらい判定 (Hurtbox) の部位定義
enum class HurtboxType
{
	HEAD,
	BODY,
	LEGS,
	COUNT
};

// アニメーションの特定区間の速度を変更する設定
struct AnimSpeedModifier
{
	float startFrame = 0.0f; // 開始フレーム
	float endFrame = 0.0f;   // 終了フレーム
	float speed = 1.0f;      // 速度倍率 (0.5=半分, 2.0=倍速)
};

// 技の性能を管理する構造体
struct AttackParams
{
	// --- タイミング (秒) ---
	float totalDuration = 0.5f; // 全体動作時間
	float hitboxStart = 0.1f;   // 攻撃判定発生
	float hitboxEnd = 0.2f;     // 攻撃判定終了

	// --- 攻撃判定 (Hitbox: 赤枠) の形状 ---
	DirectX::XMFLOAT2 hitboxOffset = { 1.0f, 1.2f };
	DirectX::XMFLOAT2 hitboxExtents = { 0.3f, 0.3f };

	// --- 攻撃中のくらい判定補正 (緑枠の変化) ---
	DirectX::XMFLOAT2 headOffsetVal = { 0.0f, 0.0f };
	DirectX::XMFLOAT2 headSizeVal = { 0.0f, 0.0f };

	DirectX::XMFLOAT2 bodyOffsetVal = { 0.0f, 0.0f };
	DirectX::XMFLOAT2 bodySizeVal = { 0.0f, 0.0f };

	DirectX::XMFLOAT2 legsOffsetVal = { 0.0f, 0.0f };
	DirectX::XMFLOAT2 legsSizeVal = { 0.0f, 0.0f };

	// --- ゲームプレイ用パラメータ ---
	int damage = 10;       // ダメージ量
	int hitFrame = 5;      // ヒット時の有利フレーム
	int blockFrame = -2;   // ガード時の有利フレーム

	// ヒットストップ時間 (秒)
	float hitStop = 0.1f;

	// ノックバック距離 (ヒット時に相手を押し下げる距離)
	float knockback = 0.5f;

	// ダウン属性 (trueならダウンさせる)
	bool isDown = false;

	// --- キャンセル設定 ---
	bool cancelEnabled = false;     // キャンセルが可能か
	float cancelStart = 0.0f;       // キャンセル受付開始時間 (秒)
	float cancelEnd = 0.0f;         // キャンセル受付終了時間 (秒)

	// どの技にキャンセルできるか (ルート設定)
	bool cancelToLight = false;      // 弱パンチへ
	bool cancelToMedium = false;     // 中パンチへ
	bool cancelToHeavyPunch = false; // 大パンチへ
	bool cancelToMediumKick = false; // 中キックへ
	bool cancelToHeavy = false;      // 大キックへ

	// --- アニメーション速度制御 ---
	// 特定フレーム区間の速度を変えるためのリスト
	std::vector<AnimSpeedModifier> speedModifiers;
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

	// 現在のアニメーションフレームを強制的に指定する
	void SetCurrentFrame(float frame);

	// --- 状態管理 (ステートパターン) ---
	void Update(float tick);
	void SetState(PlayerState* newState);

	// 現在のステートが無敵かどうかを確認する
	bool IsInvincible() const;

	// --- 状態 (State) から呼ばれるヘルパー関数 ---
	void SetInputType(PlayerInputType type);
	PlayerInputType GetInputType() const;

	const PlayerInputs& GetInputs() const;

	void PlayAnimation(const char* name, bool forceRestart = false);
	void SetAnimPause(bool pause);
	void SetAnimationSpeed(float speed);

	float GetForwardMoveDot() const;

	// --- 操作用 ---
	void SetPosition(const DirectX::XMFLOAT3& pos);
	DirectX::XMFLOAT3 GetPosition() const;
	void SetRotation(const DirectX::XMFLOAT3& rot);
	DirectX::XMFLOAT3 GetRotation() const;
	DirectX::XMFLOAT3 GetVelocity() const;
	void SetVelocity(const DirectX::XMFLOAT3& vel);
	void Jump();
	void ForceJumpState(bool isJumping); // 強制的にジャンプフラグを立てる（ダウン吹き飛び用）
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
	DirectX::BoundingBox GetActiveHitbox() const;
	bool IsAttacking() const;
	void SetActiveHitbox(bool isActive);
	void UpdateHitbox(const DirectX::XMFLOAT2& offset, const DirectX::XMFLOAT2& extents);
	void DrawHitbox();

	void SetIsColliding(bool isColliding);
	bool GetIsColliding() const;
	void SetMoveSpeed(float speed);
	float GetMoveSpeed() const;
	void SetScale(const DirectX::XMFLOAT3& scale);
	DirectX::XMFLOAT3 GetScale() const;

	// --- アニメーション ---
	void UpdateAnimation(float tick);
	void UpdateModelBlend();

	// --- ImGui 調整用にパラメータを取得する関数 ---
	AttackParams& GetLightPunchParams() { return m_lightPunchParams; }
	AttackParams& GetMediumPunchParams() { return m_mediumPunchParams; }
	AttackParams& GetHeavyPunchParams() { return m_heavyPunchParams; }
	AttackParams& GetMediumKickParams() { return m_mediumKickParams; }
	AttackParams& GetHeavyKickParams() { return m_heavyKickParams; }

	// 現在アクティブな技のパラメータを取得・設定する
	void SetCurrentAttackParams(AttackParams* params);
	AttackParams* GetCurrentAttackParams() const;

	// --- デバッグ用 ---
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

	// --- HP関連の関数 ---
	void ReceiveDamage(int damage);
	float GetHpRatio() const;

	// ラウンド開始時のリセット
	void Reset();

	// --- 攻撃ヒット管理 ---
	bool HasHit() const { return m_hasHit; }
	void OnHit() { m_hasHit = true; }

private:
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

	DirectX::BoundingBox m_hitbox;
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

	AttackParams* m_pActiveAttackParams = nullptr;

	int m_hp;
	const int m_maxHp = 10000;
	bool m_hasHit = false;
};