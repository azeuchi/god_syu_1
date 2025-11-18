#include "Player.h"
#include "Input.h" 
#include <DirectXCollision.h>
#include "Geometory.h" 
#include <fstream>
#include "Shader.h"
#include "Model.h"

// ステートパターン用
#include "PlayerState.h" 
#include "PlayerStateIdle.h" 

#include <DirectXMath.h>


Player::Player()
    : m_model(std::make_shared<Model>())
    , m_position(0.0f, 0.0f, 0.0f)
    , m_rotation(0.0f, 0.0f, 0.0f)
    , m_scale(1.0f, 1.0f, 1.0f)
    , m_velocity(0.0f, 0.0f, 0.0f)
    , m_isJumping(false)
    , m_moveSpeed(2.0f)
    , m_currentState(nullptr)
    , m_inputType(PlayerInputType::AI)
    , m_blendFactor(1.0f)
{
    m_currentAnim = { "Idle", 0 };
    m_previousAnim = { "Idle", 0 };
    SetState(new PlayerStateIdle());
}

Player::~Player()
{
    if (m_currentState) {
        delete m_currentState;
        m_currentState = nullptr;
    }
}

/**
 * @brief FSM, 物理, アニメーションをすべて更新する
 */
void Player::Update(float tick)
{
    // 1. 入力をポーリングして m_inputs を更新
    PollInputs();

    // 2. FSM（状態）の更新
    //    (m_inputs を使って継続処理や遷移チェックを行う)
    if (m_currentState) {
        m_currentState->Update(this, tick);
    }

    // 3. 物理演算の更新
    UpdatePhysics(tick);

    // 4. アニメーションの更新
    UpdateAnimation(tick);

    // 5. モデルのボーン更新
    UpdateModelBlend();
}

/**
 * @brief 入力タイプに応じて m_inputs を更新する
 */
void Player::PollInputs()
{
    // 毎フレーム、入力をリセット
    m_inputs = {}; // m_inputs.moveLeft = false, etc.

    switch (m_inputType)
    {
    case PlayerInputType::PLAYER_1:
        // 1Pは 'A' 'D' キー
        if (!IsKeyPress(VK_RBUTTON))
        {
            if (IsKeyPress('A')) {
                m_inputs.moveLeft = true;
            }
            else if (IsKeyPress('D')) {
                m_inputs.moveRight = true;
            }
        }
   
        break;

    case PlayerInputType::PLAYER_2:
        // 2Pは 矢印キー (左右)

        if (!IsKeyPress(VK_RBUTTON))
        {
            if (IsKeyPress(VK_LEFT)) { // 左矢印キー
                m_inputs.moveLeft = true;
            }
            else if (IsKeyPress(VK_RIGHT)) { // 右矢印キー
                m_inputs.moveRight = true;
            }
        }
        
        break;

    case PlayerInputType::AI:
        // AIは何もしない (m_inputs はリセットされたまま)
        break;
    }
}


void Player::UpdatePhysics(float tick)
{
    if (m_isJumping) {
        m_velocity.y -= 18.0f * tick;
    }
    m_position.x += m_velocity.x * tick;
    m_position.y += m_velocity.y * tick;
    m_position.z += m_velocity.z * tick;
    if (m_position.y <= 0.0f) {
        m_position.y = 0.0f;
        if (m_isJumping) {
            m_velocity.y = 0.0f;
            m_isJumping = false;
        }
    }
}
void Player::UpdateAnimation(float tick)
{
    if (m_blendFactor < 1.0f)
    {
        m_blendFactor += tick / m_transitionDuration;
        if (m_blendFactor > 1.0f) m_blendFactor = 1.0f;
    }
    m_currentAnim.frame++;
}
void Player::UpdateModelBlend()
{
    m_model->UpdateWithBlend(
        m_currentAnim.name, m_currentAnim.frame,
        m_previousAnim.name, m_previousAnim.frame,
        m_blendFactor);
}
void Player::SetState(PlayerState* newState)
{
    if (newState != nullptr)
    {
        if (m_currentState) {
            delete m_currentState;
        }
        m_currentState = newState;
        m_currentState->OnEnter(this);
    }
}
void Player::PlayAnimation(const char* name, bool forceRestart)
{
    if (!forceRestart && strcmp(m_currentAnim.name, name) == 0)
    {
        return;
    }
    m_previousAnim = m_currentAnim;
    m_currentAnim.name = name;
    m_currentAnim.frame = 0;
    m_blendFactor = 0.0f;
}
float Player::GetForwardMoveDot() const
{
    using namespace DirectX::SimpleMath;
    Vector3 velocity = m_velocity;
    Vector3 rotation = m_rotation;
    if (Vector2(velocity.x, velocity.z).LengthSquared() <= 0.01f)
    {
        return 0.0f;
    }
    Matrix rotMat = Matrix::CreateRotationY(rotation.y);
    Vector3 forward = Vector3::Transform(Vector3(0.0f, 0.0f, -1.0f), rotMat);
    Vector3 moveDir = velocity;
    moveDir.y = 0.0f;
    moveDir.Normalize();
    return forward.Dot(moveDir);
}


/**
 * @brief プレイヤーの入力タイプを設定
 */
void Player::SetInputType(PlayerInputType type)
{
    m_inputType = type;
}

/**
 * @brief プレイヤーの入力タイプを取得
 */
PlayerInputType Player::GetInputType() const
{
    return m_inputType;
}

/**
 * @brief 抽象化された入力を State が取得するための関数
 */
const PlayerInputs& Player::GetInputs() const
{
    return m_inputs;
}


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
DirectX::XMFLOAT3 Player::GetVelocity() const
{
    return m_velocity;
}
void Player::SetVelocity(const DirectX::XMFLOAT3& vel)
{
    m_velocity = vel;
}
void Player::Jump()
{
    if (!m_isJumping) {
        m_velocity.y = 6.0f;
        m_isJumping = true;
    }
}
bool Player::GetIsJumping() const
{
    return m_isJumping;
}

/**
 * @brief BoundingBoxをZ軸固定で生成する
 */
DirectX::BoundingBox Player::GetBoundingBox() const
{
    DirectX::XMFLOAT3 center = {
        m_position.x + m_boxOffset.x, // 2D offset (X)
        m_position.y + m_boxOffset.y, // 2D offset (Y)
        m_position.z                  // Zはオフセットなし (プレイヤーのZ座標に従う)
    };
    DirectX::XMFLOAT3 extents = {
        m_boxExtents.x, // 2D extents (X)
        m_boxExtents.y, // 2D extents (Y)
        0.1f            // Zの厚みは固定 (0だとIntersectsが機能しないため)
    };
    return DirectX::BoundingBox(center, extents);
}

/**
 * @brief 当たり判定を2Dの四角形で描画する
 */
void Player::DrawBoundingBox()
{
    using namespace DirectX;
    BoundingBox box = GetBoundingBox();
    XMFLOAT3 corners[8];
    box.GetCorners(corners);

    if (m_isColliding) {
        Geometory::SetColor(XMFLOAT4(1.0f, 1.0f, 0.0f, 1.0f));
    }
    else {
        Geometory::SetColor(XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f));
    }

    // Z軸を除いた、XY平面の4辺のみを描画
    static const int edge[4][2] = {
        {0,1},{1,2},{2,3},{3,0} // "手前"の面の4辺 (Bottom, Right, Top, Left)
    };
    for (int i = 0; i < 4; ++i) {
        Geometory::AddLine(corners[edge[i][0]], corners[edge[i][1]]);
    }
}
void Player::SetIsColliding(bool isColliding) { m_isColliding = isColliding; }
bool Player::GetIsColliding() const { return m_isColliding; }

// --- 当たり判定---
void Player::SetBoundingBoxExtents(const DirectX::XMFLOAT2& extents) { m_boxExtents = extents; }
DirectX::XMFLOAT2 Player::GetBoundingBoxExtents() const { return m_boxExtents; }
void Player::SetBoundingBoxOffset(const DirectX::XMFLOAT2& offset) { m_boxOffset = offset; }
DirectX::XMFLOAT2 Player::GetBoundingBoxOffset() const { return m_boxOffset; }

void Player::SetMoveSpeed(float speed) { m_moveSpeed = speed; }
float Player::GetMoveSpeed() const { return m_moveSpeed; }
void Player::SetScale(const DirectX::XMFLOAT3& scale) { m_scale = scale; }
DirectX::XMFLOAT3 Player::GetScale() const { return m_scale; }