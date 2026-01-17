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
#include "MediumPunch.h" 
#include "HeavyPunch.h"
#include "MediumKick.h"
#include "HeavyKick.h" 
#include "PlayerStateJump.h" 
#include "PlayerStateDamage.h"
#include "PlayerStateCrouch.h"

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
	, m_pActiveAttackParams(nullptr)
	, m_animSpeed(1.0f)
{
	m_currentAnim = { "Idle", 0 };
	m_previousAnim = { "Idle", 0 };
	SetState(new PlayerStateIdle());

	// デフォルトパラメータの初期化
	InitDefaultParameters();
}

Player::~Player()
{
	if (m_currentState) {
		delete m_currentState;
		m_currentState = nullptr;
	}
}

void Player::InitDefaultParameters()
{
	// --- Global Settings ---
	m_moveSpeed = 2.102f;
	m_scale = { 1.0f, 1.0f, 1.0f };

	// Standing Hurtboxes
	// Head
	m_baseHurtboxExtents[(int)HurtboxType::HEAD] = { 0.33f, 0.33f };
	m_baseHurtboxOffsets[(int)HurtboxType::HEAD] = { 0.024f, 1.923f };
	// Body
	m_baseHurtboxExtents[(int)HurtboxType::BODY] = { 0.598f, 0.522f };
	m_baseHurtboxOffsets[(int)HurtboxType::BODY] = { 0.011f, 1.198f };
	// Legs
	m_baseHurtboxExtents[(int)HurtboxType::LEGS] = { 0.482f, 0.497f };
	m_baseHurtboxOffsets[(int)HurtboxType::LEGS] = { -0.007f, 0.159f };

	// Crouch Hurtboxes
	// Head
	m_crouchHurtboxExtents[(int)HurtboxType::HEAD] = { 0.281f, 0.176f };
	m_crouchHurtboxOffsets[(int)HurtboxType::HEAD] = { 0.272f, 1.425f };
	// Body
	m_crouchHurtboxExtents[(int)HurtboxType::BODY] = { 0.6f, 0.401f };
	m_crouchHurtboxOffsets[(int)HurtboxType::BODY] = { 0.112f, 0.846f };
	// Legs
	m_crouchHurtboxExtents[(int)HurtboxType::LEGS] = { 0.544f, 0.364f };
	m_crouchHurtboxOffsets[(int)HurtboxType::LEGS] = { 0.116f, 0.308f };

	// --- Light Punch ---
	{
		AttackParams& p = m_lightPunchParams;
		p.totalDuration = 0.166667f;
		p.hitboxStart = 0.0666667f;
		p.hitboxEnd = 0.15f;
	
		p.hitboxes.clear();
		p.hitboxes.push_back({ {0.76f, 1.7f}, {0.454f, 0.157f} });

		p.damage = 100; p.hitFrame = 2; p.blockFrame = -2; p.hitStop = 0.0666667f; p.knockback = 0.223f;
		p.isDown = false;
		p.headOffsetVal = { 0.024f, -0.08f }; p.headSizeVal = { 0.0f, -0.113f };
		p.bodyOffsetVal = { 0.304f, -0.054f }; p.bodySizeVal = { 0.208f, 0.08f };
		p.legsOffsetVal = { 0.096f, 0.0f }; p.legsSizeVal = { 0.096f, -0.024f };
		p.cancelEnabled = true; p.cancelStart = 0.0666667f; p.cancelEnd = 0.166667f;
		p.cancelToLight = true; p.cancelToMedium = true; p.cancelToHeavyPunch = false; p.cancelToMediumKick = false; p.cancelToHeavy = false;
		p.speedModifiers.clear();
	}

	// --- Medium Punch ---
	{
		AttackParams& p = m_mediumPunchParams;
		p.totalDuration = 0.333333f;
		p.hitboxStart = 0.1f;
		p.hitboxEnd = 0.333333f;
		p.hitboxes.clear();
		p.hitboxes.push_back({ {0.735f, 1.642f}, {0.509f, 0.174f} });

		p.damage = 400; p.hitFrame = 5; p.blockFrame = -2; p.hitStop = 0.05f; p.knockback = 0.0f;
		p.isDown = false;
		p.headOffsetVal = { 0.221f, -0.029f }; p.headSizeVal = { -0.118f, -0.162f };
		p.bodyOffsetVal = { 0.486f, 0.0f }; p.bodySizeVal = { 0.177f, 0.0f };
		p.legsOffsetVal = { 0.103f, 0.0f }; p.legsSizeVal = { 0.0f, 0.0f };
		p.cancelEnabled = true; p.cancelStart = 0.1f; p.cancelEnd = 0.333333f;
		p.cancelToLight = false; p.cancelToMedium = false; p.cancelToHeavyPunch = true; p.cancelToMediumKick = false; p.cancelToHeavy = true;

		p.speedModifiers.clear();
		p.speedModifiers.push_back({ 0.0f, 6.0f, 0.369f });
		p.speedModifiers.push_back({ 7.0f, 12.0f, 2.533f });
	}

	// --- Heavy Punch ---
	{
		AttackParams& p = m_heavyPunchParams;
		p.totalDuration = 0.5f;
		p.hitboxStart = 0.133333f;
		p.hitboxEnd = 0.5f;
		p.hitboxes.clear();
		p.hitboxes.push_back({ {0.852f, 1.435f}, {0.687f, 0.202f} });

		p.damage = 700; p.hitFrame = 5; p.blockFrame = -2; p.hitStop = 0.0666667f; p.knockback = 0.0f;
		p.isDown = false;
		p.headOffsetVal = { 0.206f, 0.0f }; p.headSizeVal = { -0.014f, -0.088f };
		p.bodyOffsetVal = { 0.515f, -0.059f }; p.bodySizeVal = { 0.294f, 0.0f };
		p.legsOffsetVal = { 0.206f, 0.0f }; p.legsSizeVal = { 0.059f, 0.0f };
		p.cancelEnabled = true; p.cancelStart = 0.133333f; p.cancelEnd = 0.416667f;
		p.cancelToLight = false; p.cancelToMedium = false; p.cancelToHeavyPunch = false; p.cancelToMediumKick = false; p.cancelToHeavy = true;

		p.speedModifiers.clear();
		p.speedModifiers.push_back({ 0.0f, 10.0f, 0.344f });
		p.speedModifiers.push_back({ 11.0f, 20.0f, 2.057f });
		p.speedModifiers.push_back({ 22.0f, 30.0f, 0.486f });
	}

	// --- Medium Kick ---
	{
		AttackParams& p = m_mediumKickParams;
		p.totalDuration = 0.416667f;
		p.hitboxStart = 0.1f;
		p.hitboxEnd = 0.416667f;
		p.hitboxes.clear();
		p.hitboxes.push_back({ {1.5f, 1.56f}, {0.351f, 0.24f} });

		p.damage = 400; p.hitFrame = 5; p.blockFrame = -2; p.hitStop = 0.05f; p.knockback = 0.0f;
		p.isDown = false;
		p.headOffsetVal = { 0.088f, 0.0f }; p.headSizeVal = { -0.044f, 0.0f };
		p.bodyOffsetVal = { 0.794f, 0.0f }; p.bodySizeVal = { 0.382f, 0.0f };
		p.legsOffsetVal = { 0.53f, 0.0f }; p.legsSizeVal = { -0.088f, 0.0f };
		p.cancelEnabled = false;

		p.speedModifiers.clear();
		p.speedModifiers.push_back({ 0.0f, 4.0f, 0.362f });
		p.speedModifiers.push_back({ 5.0f, 9.0f, 0.503f });
		p.speedModifiers.push_back({ 10.0f, 20.0f, 0.295f });
	}

	// --- Heavy Kick ---
	{
		AttackParams& p = m_heavyKickParams;
		p.totalDuration = 0.5f;
		p.hitboxStart = 0.133333f;
		p.hitboxEnd = 0.5f;
		p.hitboxes.clear();
		p.hitboxes.push_back({ {1.324f, 1.759f}, {0.551f, 0.23f} });

		p.damage = 700; p.hitFrame = 5; p.blockFrame = -2; p.hitStop = 0.0666667f; p.knockback = 0.0f;
		p.isDown = false;
		p.headOffsetVal = { 0.088f, 0.0f }; p.headSizeVal = { 0.029f, 0.0f };
		p.bodyOffsetVal = { 0.735f, 0.25f }; p.bodySizeVal = { 0.485f, -0.147f };
		p.legsOffsetVal = { 0.221f, 0.308f }; p.legsSizeVal = { -0.088f, 0.088f };
		p.cancelEnabled = false;

		p.speedModifiers.clear();
		p.speedModifiers.push_back({ 0.0f, 6.0f, 0.463f });
		p.speedModifiers.push_back({ 7.0f, 16.0f, 1.006f });
		p.speedModifiers.push_back({ 17.0f, 30.0f, 0.345f });
	}
}

void Player::Update(float tick)
{
	PollInputs();

	if (m_currentState) {
		m_isCrouching = m_currentState->IsCrouch();
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
			if (IsKeyPress('A')) m_inputs.moveLeft = true;
			else if (IsKeyPress('D')) m_inputs.moveRight = true;
			if (IsKeyPress('S')) m_inputs.moveDown = true;
			if (IsKeyTrigger('W')) m_inputs.jump = true;
		}
		if (IsKeyTrigger('U')) m_inputs.LightPunch = true;
		if (IsKeyTrigger('I')) m_inputs.MediumPunch = true;
		if (IsKeyTrigger('O')) m_inputs.HeavyPunch = true;
		if (IsKeyTrigger('K')) m_inputs.MediumKick = true;
		if (IsKeyTrigger('L')) m_inputs.HeavyKick = true;
		break;

	case PlayerInputType::PLAYER_2:
		if (!IsKeyPress(VK_RBUTTON))
		{
			if (IsKeyPress(VK_LEFT)) m_inputs.moveLeft = true;
			else if (IsKeyPress(VK_RIGHT)) m_inputs.moveRight = true;
			if (IsKeyPress(VK_DOWN)) m_inputs.moveDown = true;
			if (IsKeyTrigger(VK_UP)) m_inputs.jump = true;
		}
		if (IsKeyTrigger(VK_NUMPAD1)) m_inputs.LightPunch = true;
		if (IsKeyTrigger(VK_NUMPAD2)) m_inputs.MediumPunch = true;
		if (IsKeyTrigger(VK_NUMPAD4)) m_inputs.HeavyPunch = true;
		if (IsKeyTrigger(VK_NUMPAD5)) m_inputs.MediumKick = true;
		if (IsKeyTrigger(VK_NUMPAD3)) m_inputs.HeavyKick = true;
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
		float currentSpeed = m_animSpeed;

		if (m_pActiveAttackParams)
		{
			bool isControlledAnim = (
				strcmp(m_currentAnim.name, "LightPunch") == 0 ||
				strcmp(m_currentAnim.name, "MediumPunch") == 0 ||
				strcmp(m_currentAnim.name, "HeavyPunch") == 0 ||
				strcmp(m_currentAnim.name, "MediumKick") == 0 ||
				strcmp(m_currentAnim.name, "HeavyKick") == 0 ||
				strcmp(m_currentAnim.name, "Down") == 0 ||
				strcmp(m_currentAnim.name, "WakeUp") == 0
				);

			if (!isControlledAnim)
			{
				m_pActiveAttackParams = nullptr;
			}
			else
			{
				for (const auto& mod : m_pActiveAttackParams->speedModifiers)
				{
					if (m_currentAnim.frame >= mod.startFrame && m_currentAnim.frame < mod.endFrame)
					{
						currentSpeed *= mod.speed;
						break;
					}
				}
			}
		}

		m_currentAnim.frame += currentSpeed * tick * 60.0f;
	}
}

void Player::UpdateModelBlend()
{
	m_model->UpdateWithBlend(
		m_currentAnim.name, (int)m_currentAnim.frame,
		m_previousAnim.name, (int)m_previousAnim.frame,
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

bool Player::IsInvincible() const
{
	if (m_currentState)
	{
		return m_currentState->IsInvincible();
	}
	return false;
}

void Player::PlayAnimation(const char* name, bool forceRestart)
{
	m_isAnimPaused = false;
	m_hasHit = false;
	m_animSpeed = 1.0f;

	if (!forceRestart && strcmp(m_currentAnim.name, name) == 0)
	{
		return;
	}

	m_previousAnim = m_currentAnim;
	m_currentAnim.name = name;
	m_currentAnim.frame = 0;
	m_blendFactor = 0.0f;
}

void Player::SetAnimationSpeed(float speed)
{
	m_animSpeed = speed;
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

void Player::ForceJumpState(bool isJumping)
{
	m_isJumping = isJumping;
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

void Player::SetHurtboxCrouch(HurtboxType type, const DirectX::XMFLOAT2& offset, const DirectX::XMFLOAT2& extents)
{
	if (type >= HurtboxType::COUNT) return;
	int idx = (int)type;
	m_crouchHurtboxOffsets[idx] = offset;
	m_crouchHurtboxExtents[idx] = extents;
}
DirectX::XMFLOAT2 Player::GetHurtboxCrouchOffset(HurtboxType type) const
{
	if (type >= HurtboxType::COUNT) return { 0.0f, 0.0f };
	return m_crouchHurtboxOffsets[(int)type];
}
DirectX::XMFLOAT2 Player::GetHurtboxCrouchExtents(HurtboxType type) const
{
	if (type >= HurtboxType::COUNT) return { 0.0f, 0.0f };
	return m_crouchHurtboxExtents[(int)type];
}

void Player::SetIsCrouching(bool isCrouching)
{
	m_isCrouching = isCrouching;
}
bool Player::GetIsCrouching() const
{
	return m_isCrouching;
}

DirectX::BoundingBox Player::GetHurtbox(HurtboxType type) const
{
	if (type >= HurtboxType::COUNT) return DirectX::BoundingBox();

	int idx = (int)type;
	float direction = (m_rotation.y < 0.0f) ? 1.0f : -1.0f;

	float offsetX = m_isCrouching ? m_crouchHurtboxOffsets[idx].x : m_baseHurtboxOffsets[idx].x;
	float offsetY = m_isCrouching ? m_crouchHurtboxOffsets[idx].y : m_baseHurtboxOffsets[idx].y;
	float extentX = m_isCrouching ? m_crouchHurtboxExtents[idx].x : m_baseHurtboxExtents[idx].x;
	float extentY = m_isCrouching ? m_crouchHurtboxExtents[idx].y : m_baseHurtboxExtents[idx].y;

	if (m_isAttacking && m_pActiveAttackParams != nullptr)
	{
		const AttackParams& params = *m_pActiveAttackParams;

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
	// 非アクティブになったらリストをクリア
	if (!isActive)
	{
		m_activeHitboxes.clear();
	}
}

bool Player::IsAttacking() const
{
	return m_isAttacking;
}

void Player::SetCurrentAttackParams(AttackParams* params)
{
	m_pActiveAttackParams = params;
}
AttackParams* Player::GetCurrentAttackParams() const
{
	return m_pActiveAttackParams;
}

void Player::UpdateAttackBoxes()
{
	if (!m_pActiveAttackParams) return;

	float direction = (m_rotation.y < 0.0f) ? 1.0f : -1.0f;
	m_activeHitboxes.clear();

	// 攻撃判定(Hitbox)の更新
	for (const auto& boxData : m_pActiveAttackParams->hitboxes)
	{
		DirectX::XMFLOAT3 center = {
			m_position.x + (boxData.offset.x * direction),
			m_position.y + boxData.offset.y,
			m_position.z
		};
		DirectX::XMFLOAT3 boxExtents = {
			boxData.extents.x,
			boxData.extents.y,
			0.1f
		};
		m_activeHitboxes.push_back(DirectX::BoundingBox(center, boxExtents));
	}
}

const std::vector<DirectX::BoundingBox>& Player::GetActiveHitboxes() const
{
	return m_activeHitboxes;
}

void Player::DrawHitbox()
{
	if (!m_isAttacking) return;

	using namespace DirectX;
	Geometory::SetColor(XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f));

	for (const auto& box : m_activeHitboxes)
	{
		XMFLOAT3 corners[8];
		box.GetCorners(corners);

		static const int edge[4][2] = { {0,1},{1,2},{2,3},{3,0} };
		for (int i = 0; i < 4; ++i) {
			Geometory::AddLine(corners[edge[i][0]], corners[edge[i][1]]);
		}
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

void Player::Reset()
{
	m_hp = m_maxHp;
	m_isJumping = false;
	m_isAttacking = false;
	m_hasHit = false;
	m_velocity = { 0.0f, 0.0f, 0.0f };
	m_activeHitboxes.clear();
	SetState(new PlayerStateIdle());
}