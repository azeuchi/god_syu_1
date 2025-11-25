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
    bool jump = false;
    bool attack1 = false;
};

struct AnimationState
{
    const char* name = nullptr;
    int frame = 0;
};

// ★技の性能を管理する構造体 (拡張)
struct AttackParams
{
    // --- タイミング (秒) ---
    float totalDuration = 0.5f;
    float hitboxStart = 0.1f;
    float hitboxEnd = 0.2f;

    // --- 攻撃判定 (Hitbox) の形状 ---
    DirectX::XMFLOAT2 hitboxOffset = { 1.0f, 1.2f };
    DirectX::XMFLOAT2 hitboxExtents = { 0.3f, 0.3f };

    // --- ゲームプレイ用パラメータ ---
    int damage = 10;       // ダメージ
    int hitFrame = 5;      // ヒット時の有利フレーム
    int blockFrame = -2;   // ガード時の有利フレーム
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

    // --- 状態管理 ---
    void Update(float tick);
    void SetState(PlayerState* newState);

    // --- 入力・ヘルパー ---
    void SetInputType(PlayerInputType type);
    PlayerInputType GetInputType() const;
    const PlayerInputs& GetInputs() const;

    void PlayAnimation(const char* name, bool forceRestart = false);
    void SetAnimPause(bool pause); // アニメーション一時停止

    float GetForwardMoveDot() const;

    // --- 操作用 ---
    void SetPosition(const DirectX::XMFLOAT3& pos);
    DirectX::XMFLOAT3 GetPosition() const;
    void SetRotation(const DirectX::XMFLOAT3& rot);
    DirectX::XMFLOAT3 GetRotation() const;
    DirectX::XMFLOAT3 GetVelocity() const;
    void SetVelocity(const DirectX::XMFLOAT3& vel);
    void Jump();
    bool GetIsJumping() const;

    // --- 当たり判定 ---
    void SetBoundingBoxExtents(const DirectX::XMFLOAT2& extents);
    DirectX::XMFLOAT2 GetBoundingBoxExtents() const;
    void SetBoundingBoxOffset(const DirectX::XMFLOAT2& offset);
    DirectX::XMFLOAT2 GetBoundingBoxOffset() const;
    DirectX::BoundingBox GetBoundingBox() const;
    void DrawBoundingBox();

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

    // --- アニメーション ---
    void UpdateAnimation(float tick);
    void UpdateModelBlend();

    // --- パラメータ取得 ---
    AttackParams& GetLightPunchParams() { return m_lightPunchParams; }

    // --- デバッグ用 ---
    void Debug_SetAnimation(const char* name, bool forceRestart = true) {
        PlayAnimation(name, forceRestart);
    }
    int Debug_GetFrame() const {
        return m_currentAnim.frame;
    }
    void Debug_SetFrame(int frame) {
        m_currentAnim.frame = frame;
    }

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

    DirectX::XMFLOAT2 m_boxOffset = { 0.0f, 1.0f };
    DirectX::XMFLOAT2 m_boxExtents = { 0.5f, 1.0f };
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
};