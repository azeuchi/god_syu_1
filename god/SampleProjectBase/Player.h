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

    // 当たり判定AABB取得
    DirectX::BoundingBox GetBoundingBox() const;
    // 当たり判定ボックス描画
    void DrawBoundingBox();

private:
    DirectX::XMFLOAT3 m_boxOffset = { 0.0f, 0.0f, 0.0f };

    std::shared_ptr<Model> m_model;
    DirectX::XMFLOAT3 m_position; // プレイヤー座標
    DirectX::XMFLOAT3 m_rotation; // プレイヤーの回転（ラジアン）
    DirectX::XMFLOAT3 m_scale;    // プレイヤーのスケール
    DirectX::XMFLOAT3 m_velocity;
    bool m_isJumping;

    // 当たり判定ボックスの大きさ（中心から各面までの距離）
    DirectX::XMFLOAT3 m_boxExtents = { 0.5f, 0.5f, 0.5f };
};