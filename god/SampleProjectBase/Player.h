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

struct AttackParams
{
    float totalDuration = 0.5f;
    float hitboxStart = 0.1f;
    float hitboxEnd = 0.2f;

    DirectX::XMFLOAT2 hitboxOffset = { 1.0f, 1.2f };
    DirectX::XMFLOAT2 hitboxExtents = { 0.3f, 0.3f };

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

    void SetBoundingBoxExtents(const DirectX::XMFLOAT2& extents);
    DirectX::XMFLOAT2 GetBoundingBoxExtents() const;
    void SetBoundingBoxOffset(const DirectX::XMFLOAT2& offset);
    DirectX::XMFLOAT2 GetBoundingBoxOffset() const;
    DirectX::BoundingBox GetBoundingBox() const;
    void DrawBoundingBox();

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

    // ★修正: デバッグ再生時はブレンドを無効化（即座に切り替え）する
    void Debug_SetAnimation(const char* name, bool forceRestart = true) {
        PlayAnimation(name, forceRestart);
        m_blendFactor = 1.0f; // ブレンド率をMAXにして、即座にポーズを反映させる
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

    // アニメーションのブレンド率 (0.0～1.0)
    float m_blendFactor = 1.0f;
    const float m_transitionDuration = 0.2f;
    bool m_isAnimPaused = false;

    AttackParams m_lightPunchParams;
};