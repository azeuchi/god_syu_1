#pragma once
#include "Model.h"
#include <DirectXMath.h>
#include <DirectXCollision.h>
#include <string>
#include "Geometory.h"

// 前方宣言
class PlayerState;
class Shader;

enum class PlayerInputType
{
    PLAYER_1,
    PLAYER_2,
    AI
};

struct PlayerInputs
{
    bool moveLeft = false;
    bool moveRight = false;
    bool jump = false;//ジャンプ
    bool attack1 = false;//弱パンチ
    bool attack2 = false;//中パンチ
};

struct AnimationState
{
    const char* name = nullptr;
    int frame = 0;
};

enum class HurtboxType
{
    HEAD,
    BODY,
    LEGS,
    COUNT
};

struct AttackParams
{
    // --- タイミング ---
    float totalDuration = 0.5f;
    float hitboxStart = 0.1f;
    float hitboxEnd = 0.2f;

    // --- 攻撃判定 (Hitbox: 赤枠) ---
    DirectX::XMFLOAT2 hitboxOffset = { 1.0f, 1.2f };
    DirectX::XMFLOAT2 hitboxExtents = { 0.3f, 0.3f };

    // --- 攻撃中のくらい判定補正 (緑枠の変化) ---
    DirectX::XMFLOAT2 headOffsetVal = { 0.0f, 0.0f };
    DirectX::XMFLOAT2 headSizeVal = { 0.0f, 0.0f }; 

    DirectX::XMFLOAT2 bodyOffsetVal = { 0.0f, 0.0f };
    DirectX::XMFLOAT2 bodySizeVal = { 0.0f, 0.0f }; 

    DirectX::XMFLOAT2 legsOffsetVal = { 0.0f, 0.0f };
    DirectX::XMFLOAT2 legsSizeVal = { 0.0f, 0.0f }; 

    // --- パラメータ ---
    int damage = 10;
    int hitFrame = 5;
    int blockFrame = -2;
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

    void Update(float tick);
    void SetState(PlayerState* newState);

    void SetInputType(PlayerInputType type);
    PlayerInputType GetInputType() const;
    const PlayerInputs& GetInputs() const;

    void PlayAnimation(const char* name, bool forceRestart = false);
    void SetAnimPause(bool pause);

    float GetForwardMoveDot() const;

    void SetPosition(const DirectX::XMFLOAT3& pos);
    DirectX::XMFLOAT3 GetPosition() const;
    void SetRotation(const DirectX::XMFLOAT3& rot);
    DirectX::XMFLOAT3 GetRotation() const;
    DirectX::XMFLOAT3 GetVelocity() const;
    void SetVelocity(const DirectX::XMFLOAT3& vel);
    void Jump();
    bool GetIsJumping() const;

    // --- 当たり判定 ---
    void SetHurtboxBase(HurtboxType type, const DirectX::XMFLOAT2& offset, const DirectX::XMFLOAT2& extents);
    DirectX::BoundingBox GetHurtbox(HurtboxType type) const;

    DirectX::XMFLOAT2 GetHurtboxBaseOffset(HurtboxType type) const;
    DirectX::XMFLOAT2 GetHurtboxBaseExtents(HurtboxType type) const;

    void DrawBoundingBox();
    bool CheckCollision(const Player* other) const;

    // (互換性用)
    void SetBoundingBoxExtents(const DirectX::XMFLOAT2& extents);
    DirectX::XMFLOAT2 GetBoundingBoxExtents() const;
    void SetBoundingBoxOffset(const DirectX::XMFLOAT2& offset);
    DirectX::XMFLOAT2 GetBoundingBoxOffset() const;
    DirectX::BoundingBox GetBoundingBox() const;

    // --- 攻撃判定 ---
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

    void UpdateAnimation(float tick);
    void UpdateModelBlend();

    AttackParams& GetLightPunchParams() { return m_lightPunchParams; }
    AttackParams& GetMediumPunchParams() { return m_mediumPunchParams; }

    void Debug_SetAnimation(const char* name, bool forceRestart = true) {
        PlayAnimation(name, forceRestart);
        m_blendFactor = 1.0f;
    }
    int Debug_GetFrame() const {
        return m_currentAnim.frame;
    }
    void Debug_SetFrame(int frame) {
        m_currentAnim.frame = frame;
    }

    void ReceiveDamage(int damage);
    float GetHpRatio() const;

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

    // 3部位分の配列
    DirectX::XMFLOAT2 m_baseHurtboxOffsets[(int)HurtboxType::COUNT];
    DirectX::XMFLOAT2 m_baseHurtboxExtents[(int)HurtboxType::COUNT];
    bool m_isColliding = false;

    DirectX::BoundingBox m_hitbox;
    bool m_isAttacking = false;

    PlayerState* m_currentState;
    PlayerInputType m_inputType;
    PlayerInputs m_inputs;

    AnimationState m_currentAnim;
    AnimationState m_previousAnim;
    float m_blendFactor = 1.0f;
    const float m_transitionDuration = 0.2f;
    bool m_isAnimPaused = false;

    AttackParams m_lightPunchParams;
    AttackParams m_mediumPunchParams;

    int m_hp;
    const int m_maxHp = 10000;
    bool m_hasHit = false;
};