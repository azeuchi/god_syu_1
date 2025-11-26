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
#include "LightPunch.h" 
#include "PlayerStateJump.h" 

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
    , m_isAttacking(false)
    , m_isAnimPaused(false) // 初期化
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

    // 2. ★ FSM（状態）の一元的な遷移チェック ★
    if (m_currentState && m_currentState->IsInterruptible())
    {
        // 攻撃ボタンが押されたか
        if (m_inputs.attack1)
        {
            SetState(new LightPunch());
        }

        // ★ ジャンプ処理 (攻撃中でなければ飛べる)
        // ジャンプキーが押され、かつ現在ジャンプ中でなければ
        else if (m_inputs.jump && !m_isJumping)
        {
            Jump(); // 物理的に上に飛ばす
            SetState(new PlayerStateJump()); // ジャンプ状態へ遷移
        }
    }

    // 3. FSM（状態）の個別の更新
    if (m_currentState) {
        m_currentState->Update(this, tick);
    }

    // 4. 物理演算の更新
    UpdatePhysics(tick);

    // 5. アニメーションの更新
    UpdateAnimation(tick);

    // 6. モデルのボーン更新
    UpdateModelBlend();
}

/**
 * @brief 入力タイプに応じて m_inputs を更新する
 */
void Player::PollInputs()
{
    // 毎フレーム、入力をリセット
    m_inputs = {};

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
            // 'W' キーでジャンプ
            if (IsKeyTrigger('W')) {
                m_inputs.jump = true;
            }
        }
        // 'J' キーで攻撃
        if (IsKeyTrigger('J')) {
            m_inputs.attack1 = true;
        }
        break;

    case PlayerInputType::PLAYER_2:
        // 2Pは 矢印キー (左右)
        if (!IsKeyPress(VK_RBUTTON))
        {
            if (IsKeyPress(VK_LEFT)) {
                m_inputs.moveLeft = true;
            }
            else if (IsKeyPress(VK_RIGHT)) {
                m_inputs.moveRight = true;
            }
            // 上矢印キーでジャンプ
            if (IsKeyTrigger(VK_UP)) {
                m_inputs.jump = true;
            }
        }
        // テンキーの '1' で攻撃
        if (IsKeyTrigger(VK_NUMPAD1)) {
            m_inputs.attack1 = true;
        }
        break;

    case PlayerInputType::AI:
        break;
    }
}


void Player::UpdatePhysics(float tick)
{
    if (m_isJumping) {
        m_velocity.y -= 18.0f * tick; // 重力
    }
    m_position.x += m_velocity.x * tick;
    m_position.y += m_velocity.y * tick;
    m_position.z += m_velocity.z * tick;

    // 地面との接触
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

    //  一時停止中でなければフレームを進める
    if (!m_isAnimPaused)
    {
        m_currentAnim.frame++;
    }
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
    //  新しいアニメーションを再生する時は必ず一時停止を解除する
    m_isAnimPaused = false;

    if (!forceRestart && strcmp(m_currentAnim.name, name) == 0)
    {
        return;
    }
    m_previousAnim = m_currentAnim;
    m_currentAnim.name = name;
    m_currentAnim.frame = 0;
    m_blendFactor = 0.0f;
}

// アニメーション一時停止
void Player::SetAnimPause(bool pause)
{
    m_isAnimPaused = pause;
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
        m_velocity.y = 8.0f; // ジャンプ力
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


// --- 攻撃判定 (Hitbox) 用の関数 ---

/**
 * @brief 攻撃判定 (Hitbox) を有効/無効にする
 */
void Player::SetActiveHitbox(bool isActive)
{
    m_isAttacking = isActive;
}

/**
 * @brief 攻撃判定が有効か（Sceneがチェックするために使う）
 */
bool Player::IsAttacking() const
{
    return m_isAttacking;
}

/**
 * @brief 攻撃判定 (Hitbox) の位置とサイズを更新する
 */
void Player::UpdateHitbox(const DirectX::XMFLOAT2& offset, const DirectX::XMFLOAT2& extents)
{
    // 向きを考慮 (m_rotation.y が約 -PI/2 なら 1P、約 PI/2 なら 2P)
    float direction = (m_rotation.y < 0.0f) ? 1.0f : -1.0f;

    DirectX::XMFLOAT3 center = {
        m_position.x + (offset.x * direction), // 向きによってオフセットを反転
        m_position.y + offset.y,
        m_position.z
    };
    DirectX::XMFLOAT3 boxExtents = {
        extents.x,
        extents.y,
        0.1f // Zの厚みは固定
    };
    m_hitbox = DirectX::BoundingBox(center, boxExtents);
}

/**
 * @brief 有効な攻撃判定 (Hitbox) を取得する
 */
DirectX::BoundingBox Player::GetActiveHitbox() const
{
    return m_hitbox;
}

/**
 * @brief 攻撃判定 (Hitbox) をデバッグ描画する
 */
void Player::DrawHitbox()
{
    if (!m_isAttacking) return; // 攻撃中のみ描画

    using namespace DirectX;
    XMFLOAT3 corners[8];
    m_hitbox.GetCorners(corners);

    Geometory::SetColor(XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f)); // 赤色で描画

    // "手前"の面の4辺 (Bottom, Right, Top, Left)
    static const int edge[4][2] = { {0,1},{1,2},{2,3},{3,0} };
    for (int i = 0; i < 4; ++i) {
        Geometory::AddLine(corners[edge[i][0]], corners[edge[i][1]]);
    }
}