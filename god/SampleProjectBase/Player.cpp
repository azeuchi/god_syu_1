#include "Player.h"
#include "Input.h" 
#include <DirectXCollision.h>
#include "Geometory.h" 
#include <fstream>
#include "Shader.h"
#include "Model.h"

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
    , m_isAnimPaused(false)
    , m_hp(10000)
    , m_hasHit(false)
{
    m_currentAnim = { "Idle", 0 };
    m_previousAnim = { "Idle", 0 };
    SetState(new PlayerStateIdle());

    // 初期設定: 頭・体・足のくらい判定
    m_baseHurtboxOffsets[(int)HurtboxType::HEAD] = { 0.0f, 1.6f };
    m_baseHurtboxExtents[(int)HurtboxType::HEAD] = { 0.2f, 0.2f };

    m_baseHurtboxOffsets[(int)HurtboxType::BODY] = { 0.0f, 1.0f };
    m_baseHurtboxExtents[(int)HurtboxType::BODY] = { 0.3f, 0.4f };

    m_baseHurtboxOffsets[(int)HurtboxType::LEGS] = { 0.0f, 0.4f };
    m_baseHurtboxExtents[(int)HurtboxType::LEGS] = { 0.3f, 0.4f };
}

Player::~Player()
{
    if (m_currentState) {
        delete m_currentState;
        m_currentState = nullptr;
    }
}

void Player::Update(float tick)
{
    PollInputs();

    if (m_currentState && m_currentState->IsInterruptible())
    {
        if (m_inputs.attack1)
        {
            SetState(new LightPunch());
        }
        else if (m_inputs.jump && !m_isJumping)
        {
            Jump();
            SetState(new PlayerStateJump());
        }
    }

    if (m_currentState) {
        m_currentState->Update(this, tick);
    }

    UpdatePhysics(tick);
    UpdateAnimation(tick);
    UpdateModelBlend();
}

void Player::PollInputs()
{
    m_inputs = {};

    switch (m_inputType)
    {
    case PlayerInputType::PLAYER_1:
        if (!IsKeyPress(VK_RBUTTON))
        {
            if (IsKeyPress('A')) {
                m_inputs.moveLeft = true;
            }
            else if (IsKeyPress('D')) {
                m_inputs.moveRight = true;
            }
            if (IsKeyTrigger('W')) {
                m_inputs.jump = true;
            }
        }
        if (IsKeyTrigger('J')) {
            m_inputs.attack1 = true;
        }
        break;

    case PlayerInputType::PLAYER_2:
        if (!IsKeyPress(VK_RBUTTON))
        {
            if (IsKeyPress(VK_LEFT)) {
                m_inputs.moveLeft = true;
            }
            else if (IsKeyPress(VK_RIGHT)) {
                m_inputs.moveRight = true;
            }
          
        }
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
    m_isAnimPaused = false;
    m_hasHit = false;

    if (!forceRestart && strcmp(m_currentAnim.name, name) == 0)
    {
        return;
    }
    m_previousAnim = m_currentAnim;
    m_currentAnim.name = name;
    m_currentAnim.frame = 0;
    m_blendFactor = 0.0f;
}

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

void Player::SetInputType(PlayerInputType type)
{
    m_inputType = type;
}

PlayerInputType Player::GetInputType() const
{
    return m_inputType;
}

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
        m_velocity.y = 10.0f;
        m_isJumping = true;
    }
}
bool Player::GetIsJumping() const
{
    return m_isJumping;
}

void Player::SetHurtboxBase(HurtboxType type, const DirectX::XMFLOAT2& offset, const DirectX::XMFLOAT2& extents)
{
    if (type >= HurtboxType::COUNT) return;
    int idx = (int)type;
    m_baseHurtboxOffsets[idx] = offset;
    m_baseHurtboxExtents[idx] = extents;
}

//  攻撃中のOffsetとSize補正を適用
DirectX::BoundingBox Player::GetHurtbox(HurtboxType type) const
{
    if (type >= HurtboxType::COUNT) return DirectX::BoundingBox();

    int idx = (int)type;
    float direction = (m_rotation.y < 0.0f) ? 1.0f : -1.0f;

    // 基本値
    float offsetX = m_baseHurtboxOffsets[idx].x;
    float offsetY = m_baseHurtboxOffsets[idx].y;
    float extentX = m_baseHurtboxExtents[idx].x;
    float extentY = m_baseHurtboxExtents[idx].y;

    // 攻撃中ならパラメータの補正値を足す
    if (m_isAttacking)
    {
        const AttackParams& params = m_lightPunchParams;

        if (type == HurtboxType::HEAD) {
            offsetX += params.headOffsetVal.x;
            offsetY += params.headOffsetVal.y;
            extentX += params.headSizeVal.x; 
            extentY += params.headSizeVal.y;
        }
        else if (type == HurtboxType::BODY) {
            offsetX += params.bodyOffsetVal.x;
            offsetY += params.bodyOffsetVal.y;
            extentX += params.bodySizeVal.x; 
            extentY += params.bodySizeVal.y;
        }
        else if (type == HurtboxType::LEGS) {
            offsetX += params.legsOffsetVal.x;
            offsetY += params.legsOffsetVal.y;
            extentX += params.legsSizeVal.x; 
            extentY += params.legsSizeVal.y; 
        }
    }

    DirectX::XMFLOAT3 center = {
        m_position.x + (offsetX * direction),
        m_position.y + offsetY,
        m_position.z
    };
    DirectX::XMFLOAT3 extents = {
        extentX, extentY, 0.1f
    };

    return DirectX::BoundingBox(center, extents);
}

DirectX::XMFLOAT2 Player::GetHurtboxBaseOffset(HurtboxType type) const
{
    if (type >= HurtboxType::COUNT) return { 0.0f, 0.0f };
    return m_baseHurtboxOffsets[(int)type];
}
DirectX::XMFLOAT2 Player::GetHurtboxBaseExtents(HurtboxType type) const
{
    if (type >= HurtboxType::COUNT) return { 0.0f, 0.0f };
    return m_baseHurtboxExtents[(int)type];
}


void Player::DrawBoundingBox()
{
    using namespace DirectX;

    if (m_isColliding) Geometory::SetColor(XMFLOAT4(1.0f, 1.0f, 0.0f, 1.0f));
    else Geometory::SetColor(XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f));

    for (int i = 0; i < (int)HurtboxType::COUNT; ++i)
    {
        BoundingBox box = GetHurtbox((HurtboxType)i);
        XMFLOAT3 corners[8];
        box.GetCorners(corners);

        static const int edge[4][2] = { {0,1},{1,2},{2,3},{3,0} };
        for (int e = 0; e < 4; ++e) {
            Geometory::AddLine(corners[edge[e][0]], corners[edge[e][1]]);
        }
    }
}

bool Player::CheckCollision(const Player* other) const
{
    if (!other) return false;

    for (int i = 0; i < (int)HurtboxType::COUNT; ++i)
    {
        BoundingBox myBox = GetHurtbox((HurtboxType)i);
        for (int j = 0; j < (int)HurtboxType::COUNT; ++j)
        {
            BoundingBox otherBox = other->GetHurtbox((HurtboxType)j);
            if (myBox.Intersects(otherBox)) return true;
        }
    }
    return false;
}

void Player::SetIsColliding(bool isColliding) { m_isColliding = isColliding; }
bool Player::GetIsColliding() const { return m_isColliding; }

// --- 当たり判定 (後方互換用) ---
void Player::SetBoundingBoxExtents(const DirectX::XMFLOAT2& extents) {
    m_baseHurtboxExtents[(int)HurtboxType::BODY] = extents;
}
DirectX::XMFLOAT2 Player::GetBoundingBoxExtents() const {
    return m_baseHurtboxExtents[(int)HurtboxType::BODY];
}
void Player::SetBoundingBoxOffset(const DirectX::XMFLOAT2& offset) {
    m_baseHurtboxOffsets[(int)HurtboxType::BODY] = offset;
}
DirectX::XMFLOAT2 Player::GetBoundingBoxOffset() const {
    return m_baseHurtboxOffsets[(int)HurtboxType::BODY];
}
DirectX::BoundingBox Player::GetBoundingBox() const { return GetHurtbox(HurtboxType::BODY); }


void Player::SetMoveSpeed(float speed) { m_moveSpeed = speed; }
float Player::GetMoveSpeed() const { return m_moveSpeed; }
void Player::SetScale(const DirectX::XMFLOAT3& scale) { m_scale = scale; }
DirectX::XMFLOAT3 Player::GetScale() const { return m_scale; }


void Player::SetActiveHitbox(bool isActive)
{
    m_isAttacking = isActive;
}

bool Player::IsAttacking() const
{
    return m_isAttacking;
}

void Player::UpdateHitbox(const DirectX::XMFLOAT2& offset, const DirectX::XMFLOAT2& extents)
{
    float direction = (m_rotation.y < 0.0f) ? 1.0f : -1.0f;

    DirectX::XMFLOAT3 center = {
        m_position.x + (offset.x * direction),
        m_position.y + offset.y,
        m_position.z
    };
    DirectX::XMFLOAT3 boxExtents = {
        extents.x,
        extents.y,
        0.1f
    };
    m_hitbox = DirectX::BoundingBox(center, boxExtents);
}

DirectX::BoundingBox Player::GetActiveHitbox() const
{
    return m_hitbox;
}

void Player::DrawHitbox()
{
    if (!m_isAttacking) return;

    using namespace DirectX;
    XMFLOAT3 corners[8];
    m_hitbox.GetCorners(corners);

    Geometory::SetColor(XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f));

    static const int edge[4][2] = { {0,1},{1,2},{2,3},{3,0} };
    for (int i = 0; i < 4; ++i) {
        Geometory::AddLine(corners[edge[i][0]], corners[edge[i][1]]);
    }
}

void Player::ReceiveDamage(int damage)
{
    m_hp -= damage;
    if (m_hp < 0) m_hp = 0;
}

float Player::GetHpRatio() const
{
    return (float)m_hp / (float)m_maxHp;
}