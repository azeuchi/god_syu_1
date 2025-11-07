#pragma once
#include "Model.h"
#include <DirectXMath.h>
#include <DirectXCollision.h>
#include <string>
#include "Geometory.h"

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

    // --- 操作用 ---
    void Update(float tick);
    void SetPosition(const DirectX::XMFLOAT3& pos);
    DirectX::XMFLOAT3 GetPosition() const;
    void SetRotation(const DirectX::XMFLOAT3& rot);
    DirectX::XMFLOAT3 GetRotation() const;

    // 速度を取得するための関数
    DirectX::XMFLOAT3 GetVelocity() const;

    // --- 当たり判定用 ---
    // 当たり判定AABB取得
    DirectX::BoundingBox GetBoundingBox() const;
    // 当たり判定ボックス描画
    void DrawBoundingBox();

    // 当たり判定ボックスのサイズ（中心から各面への距離）を設定/取得
    void SetBoundingBoxExtents(const DirectX::XMFLOAT3& extents);
    DirectX::XMFLOAT3 GetBoundingBoxExtents() const;

    // 当たり判定ボックスのオフセット（中心からのズレ）を設定/取得
    void SetBoundingBoxOffset(const DirectX::XMFLOAT3& offset);
    DirectX::XMFLOAT3 GetBoundingBoxOffset() const;

    // 移動速度のセッター/ゲッター
    void SetMoveSpeed(float speed);
    float GetMoveSpeed() const;

    // スケールのセッター/ゲッター
    void SetScale(const DirectX::XMFLOAT3& scale);
    DirectX::XMFLOAT3 GetScale() const;

private:
    std::shared_ptr<Model> m_model;
    DirectX::XMFLOAT3 m_position; // プレイヤー座標
    DirectX::XMFLOAT3 m_rotation; // プレイヤーの回転（ラジアン）
    DirectX::XMFLOAT3 m_scale;    // プレイヤーのスケール
    DirectX::XMFLOAT3 m_velocity;
    bool m_isJumping;

    float m_moveSpeed; // 水平方向の移動速度

    // --- 当たり判定用メンバー変数 ---
    // ボックスの中心位置オフセット
    DirectX::XMFLOAT3 m_boxOffset = { 0.0f, 1.0f, 0.0f };
    // ボックスの大きさ（中心から各面までの距離）
    // (横幅1.0f, 高さ2.0f, 奥行き1.0f のボックスになる)
    DirectX::XMFLOAT3 m_boxExtents = { 0.5f, 1.0f, 0.5f };
};