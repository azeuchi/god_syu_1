#include "Player.h"
#include "Input.h" // キー入力用
#include <DirectXCollision.h>
#include "Geometory.h" //（線描画用）
#include <fstream>
#include "Shader.h"
#include "Model.h"

Player::Player()
    : m_model(std::make_shared<Model>())
    , m_position(0.0f, 0.0f, 0.0f)
    , m_rotation(0.0f, 0.0f, 0.0f)
    , m_velocity(0.0f, 0.0f, 0.0f)
    , m_isJumping(false)
    , m_boxExtents(1.0f, 1.0f, 1.0f)
    , m_boxOffset(0.0f, 0.0f, 0.0f)
{
}

Player::~Player() {}

bool Player::Load(const char* file, float scale, bool flip, bool simple)
{
    return m_model->Load(file, scale, flip, simple);
}

void Player::SetVertexShader(Shader* vs)
{
    m_model->SetVertexShader(vs);
}

void Player::SetPixelShader(Shader* ps)
{
    m_model->SetPixelShader(ps);
}

void Player::Draw()
{
    m_model->Draw();
}

Model* Player::GetModel()
{
    return m_model.get();
}

void Player::Update(float tick)
{
    // 右クリック中は移動しない
    if (!IsKeyPress(VK_RBUTTON)) {
        float speed = 5.0f * tick;
        if (IsKeyPress('A')) m_position.x -= speed;
        if (IsKeyPress('D')) m_position.x += speed;

        // ジャンプ（Wキーまたはスペースキーでジャンプ）
        if (!m_isJumping && (IsKeyTrigger('W') || IsKeyTrigger(VK_SPACE))) {
            m_velocity.y = 6.0f; // ジャンプ初速度
            m_isJumping = true;
        }
    }
    // 重力
    if (m_isJumping) {
        m_velocity.y -= 18.0f * tick; // 重力加速度
        m_position.y += m_velocity.y * tick;

        // 地面判定（y=0を地面とする）
        if (m_position.y <= 0.0f) {
            m_position.y = 0.0f;
            m_velocity.y = 0.0f;
            m_isJumping = false;
        }
    }
}

void Player::SetPosition(const DirectX::XMFLOAT3& pos)
{
    m_position = pos;
}

DirectX::XMFLOAT3 Player::GetPosition() const
{
    return m_position;
}

void Player::SetRotation(const DirectX::XMFLOAT3& rot)
{
    m_rotation = rot;
}

DirectX::XMFLOAT3 Player::GetRotation() const
{
    return m_rotation;
}

DirectX::BoundingBox Player::GetBoundingBox() const
{
    // オフセットを加味したAABB
    DirectX::XMFLOAT3 center = {
        m_position.x + m_boxOffset.x,
        m_position.y + m_boxOffset.y,
        m_position.z + m_boxOffset.z
    };
    return DirectX::BoundingBox(center, m_boxExtents);
}

// 当たり判定ボックスの描画
void Player::DrawBoundingBox()
{
    //using namespace DirectX;
    //using namespace DirectX::SimpleMath;

    //BoundingBox box = GetBoundingBox();
    //XMFLOAT3 corners[8];
    //box.GetCorners(corners);

    //// 赤色で描画
    //Geometory::SetColor(XMFLOAT4(1, 0, 0, 1));
    //// 12本のエッジを線で描画
    //static const int edge[12][2] = {
    //    {0,1},{1,2},{2,3},{3,0},
    //    {4,5},{5,6},{6,7},{7,4},
    //    {0,4},{1,5},{2,6},{3,7}
    //};
    //for (int i = 0; i < 12; ++i) {
    //    Geometory::AddLine(corners[edge[i][0]], corners[edge[i][1]]);
    //}
}