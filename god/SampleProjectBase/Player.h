#pragma once
#include "Model.h"
#include <DirectXMath.h>
#include <DirectXCollision.h>
#include <string>
#include "Geometory.h"

// 前方宣言
class PlayerState;
class Shader;

// プレイヤーの入力タイプを明確化
enum class PlayerInputType
{
    PLAYER_1, // 1P (例: 'A','D'キー)
    PLAYER_2, // 2P (例: 矢印キー)
    AI        // AI (何もしない)
};

// 抽象化された入力状態を保持する構造体
struct PlayerInputs
{
    bool moveLeft = false;
    bool moveRight = false;
    bool jump = false;    // ★追加: ジャンプ
    bool attack1 = false; // 弱パンチ
};

struct AnimationState
{
    const char* name = nullptr;
    int frame = 0;
};

// 技の性能を管理する構造体
struct AttackParams
{
    // タイミング (秒)
    float totalDuration = 0.5f;
    float hitboxStart = 0.1f;
    float hitboxEnd = 0.2f;

    // Hitbox (攻撃判定) の形状
    DirectX::XMFLOAT2 hitboxOffset = { 1.0f, 1.2f };
    DirectX::XMFLOAT2 hitboxExtents = { 0.3f, 0.3f };
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


    // --- 状態管理 (ステートパターン) ---
    void Update(float tick);
    void SetState(PlayerState* newState);


    // --- 状態 (State) から呼ばれるヘルパー関数 ---
    void SetInputType(PlayerInputType type);
    PlayerInputType GetInputType() const;

    // 抽象化された入力を State が取得するための関数
    const PlayerInputs& GetInputs() const;

    void PlayAnimation(const char* name, bool forceRestart = false);
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
    DirectX::BoundingBox GetBoundingBox() const; // 「くらい判定 (Hurtbox)」
    void DrawBoundingBox();

    // --- 攻撃判定 (Hitbox) 用 ---
    DirectX::BoundingBox GetActiveHitbox() const;
    bool IsAttacking() const;
    void SetActiveHitbox(bool isActive);
    void UpdateHitbox(const DirectX::XMFLOAT2& offset, const DirectX::XMFLOAT2& extents);
    void DrawHitbox(); // デバッグ描画用

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

    // --- デバッグ用に追加 ---
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

    // 入力タイプに応じて m_inputs を更新する
    void PollInputs();

    std::shared_ptr<Model> m_model;
    DirectX::XMFLOAT3 m_position;
    DirectX::XMFLOAT3 m_rotation;
    DirectX::XMFLOAT3 m_scale;
    DirectX::XMFLOAT3 m_velocity;
    bool m_isJumping;
    float m_moveSpeed;

    // --- 当たり判定用メンバー変数 ---
    DirectX::XMFLOAT2 m_boxOffset = { 0.0f, 1.0f };
    DirectX::XMFLOAT2 m_boxExtents = { 0.5f, 1.0f };
    bool m_isColliding = false;

    // --- 攻撃判定 (Hitbox) 用メンバー ---
    DirectX::BoundingBox m_hitbox;     // 攻撃判定用のBox
    bool m_isAttacking = false; // 攻撃判定がアクティブか

    // --- FSM (ステートパターン) 用メンバー ---
    PlayerState* m_currentState;

    // --- 入力タイプ用メンバー ---
    PlayerInputType m_inputType;

    // 抽象化された入力状態
    PlayerInputs m_inputs;

    // --- アニメーション用メンバー ---
    AnimationState m_currentAnim;
    AnimationState m_previousAnim;
    float m_blendFactor = 1.0f;
    const float m_transitionDuration = 0.2f;

    // --- 技パラメータ用メンバー ---
    AttackParams m_lightPunchParams;
};