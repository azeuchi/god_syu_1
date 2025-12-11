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
    bool jump = false;    // ジャンプ
    bool attack1 = false; // 弱パンチ
    bool attack2 = false; // 中パンチ
};

struct AnimationState
{
    const char* name = nullptr;
    float frame = 0.0f; // ★変更: int -> float (再生速度調整のため)
};

// くらい判定 (Hurtbox) の部位定義
enum class HurtboxType
{
    HEAD, // 頭 (上段・ジャンプ攻撃用)
    BODY, // 体 (中段・基本判定)
    LEGS, // 足 (下段・足払い用)
    COUNT
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


    // --- 状態 (State) から呼ばれるヘルパー関数 ---
    void SetInputType(PlayerInputType type);
    PlayerInputType GetInputType() const;

    // 抽象化された入力を State が取得するための関数
    const PlayerInputs& GetInputs() const;

    void PlayAnimation(const char* name, bool forceRestart = false);
    // アニメーションの一時停止を設定する関数
    void SetAnimPause(bool pause);

    // ★追加: アニメーション再生速度を設定 (1.0 = 等倍, 2.0 = 倍速)
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
    bool GetIsJumping() const;


    // --- 当たり判定 (Hurtbox) ---
    void SetHurtboxBase(HurtboxType type, const DirectX::XMFLOAT2& offset, const DirectX::XMFLOAT2& extents);

    DirectX::BoundingBox GetHurtbox(HurtboxType type) const;

    DirectX::XMFLOAT2 GetHurtboxBaseOffset(HurtboxType type) const;
    DirectX::XMFLOAT2 GetHurtboxBaseExtents(HurtboxType type) const;

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
    AttackParams& GetMediumPunchParams() { return m_mediumPunchParams; }

    // 現在アクティブな技のパラメータを取得・設定する
    void SetCurrentAttackParams(AttackParams* params);
    AttackParams* GetCurrentAttackParams() const;


    // --- デバッグ用 ---
    void Debug_SetAnimation(const char* name, bool forceRestart = true) {
        PlayAnimation(name, forceRestart);
        m_blendFactor = 1.0f;
    }
    // int へのキャストを追加
    int Debug_GetFrame() const {
        return (int)m_currentAnim.frame;
    }
    void Debug_SetFrame(int frame) {
        m_currentAnim.frame = (float)frame;
    }

    // --- HP関連の関数 ---
    void ReceiveDamage(int damage); // ダメージを受ける
    float GetHpRatio() const;       // HPの割合(0.0～1.0)を取得 (UI用)

    // --- 攻撃ヒット管理 ---
    bool HasHit() const { return m_hasHit; } // 今の攻撃はもう当たったか？
    void OnHit() { m_hasHit = true; }        // 当たったことにする

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
    DirectX::XMFLOAT2 m_baseHurtboxOffsets[(int)HurtboxType::COUNT];
    DirectX::XMFLOAT2 m_baseHurtboxExtents[(int)HurtboxType::COUNT];
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
    const float m_transitionDuration = 0.1f;
    bool m_isAnimPaused = false; // アニメーション一時停止フラグ

    // 再生スピード倍率 
    float m_animSpeed = 1.0f;

    // --- 技パラメータ用メンバー ---
    AttackParams m_lightPunchParams;
    AttackParams m_mediumPunchParams;

    // 現在アクティブなパラメータへのポインタ
    AttackParams* m_pActiveAttackParams = nullptr;

    // --- HPとヒット管理 ---
    int m_hp;
    const int m_maxHp = 10000; // 最大HP 10000固定
    bool m_hasHit = false;     // 現在の攻撃が既にヒットしたか
};