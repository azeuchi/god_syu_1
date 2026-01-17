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

	// --- 当たり判定(Hurtbox)の初期設定 ---

	// 1. 立ち状態 (Base) のくらい判定
	m_baseHurtboxOffsets[(int)HurtboxType::HEAD] = { 0.0f, 1.6f };
	m_baseHurtboxExtents[(int)HurtboxType::HEAD] = { 0.2f, 0.2f };
	m_baseHurtboxOffsets[(int)HurtboxType::BODY] = { 0.0f, 1.0f };
	m_baseHurtboxExtents[(int)HurtboxType::BODY] = { 0.3f, 0.4f };
	m_baseHurtboxOffsets[(int)HurtboxType::LEGS] = { 0.0f, 0.4f };
	m_baseHurtboxExtents[(int)HurtboxType::LEGS] = { 0.3f, 0.4f };

	// 2. しゃがみ状態 (Crouch) のくらい判定
	m_crouchHurtboxOffsets[(int)HurtboxType::HEAD] = { 0.0f, 1.1f };
	m_crouchHurtboxExtents[(int)HurtboxType::HEAD] = { 0.2f, 0.2f };
	m_crouchHurtboxOffsets[(int)HurtboxType::BODY] = { 0.0f, 0.7f };
	m_crouchHurtboxExtents[(int)HurtboxType::BODY] = { 0.35f, 0.3f };
	m_crouchHurtboxOffsets[(int)HurtboxType::LEGS] = { 0.0f, 0.2f };
	m_crouchHurtboxExtents[(int)HurtboxType::LEGS] = { 0.35f, 0.2f };
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

	// 2. FSM（状態）の一元的な遷移チェック
	if (m_currentState) {
		m_isCrouching = m_currentState->IsCrouch();
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
	// 重力処理
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

// アニメーション速度を加味して更新 (tickを反映)
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
			//  ダウンや起き上がりも「速度調整可能な状態」として扱う
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
				// 設定されている速度モディファイアを走査
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

// --- 基本(立ち)設定 ---
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

// --- しゃがみ設定 ---
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

// --- 判定取得 (キーフレーム補間対応) ---
DirectX::BoundingBox Player::GetHurtbox(HurtboxType type) const
{
	if (type >= HurtboxType::COUNT) return DirectX::BoundingBox();

	int idx = (int)type;
	float direction = (m_rotation.y < 0.0f) ? 1.0f : -1.0f;

	float offsetX = m_isCrouching ? m_crouchHurtboxOffsets[idx].x : m_baseHurtboxOffsets[idx].x;
	float offsetY = m_isCrouching ? m_crouchHurtboxOffsets[idx].y : m_baseHurtboxOffsets[idx].y;
	float extentX = m_isCrouching ? m_crouchHurtboxExtents[idx].x : m_baseHurtboxExtents[idx].x;
	float extentY = m_isCrouching ? m_crouchHurtboxExtents[idx].y : m_baseHurtboxExtents[idx].y;

	// 攻撃中の補正をキーフレームから計算
	if (m_isAttacking && m_pActiveAttackParams != nullptr)
	{
		const AttackParams& params = *m_pActiveAttackParams;
		const std::vector<HurtboxKey>* keys = nullptr;

		if (type == HurtboxType::HEAD) keys = &params.headKeys;
		else if (type == HurtboxType::BODY) keys = &params.bodyKeys;
		else if (type == HurtboxType::LEGS) keys = &params.legsKeys;

		if (keys && !keys->empty())
		{
			// 現在のフレーム
			float currentFrame = m_currentAnim.frame;

			// 補間計算用の変数
			DirectX::XMFLOAT2 lerpOffset = { 0.0f, 0.0f };
			DirectX::XMFLOAT2 lerpSize = { 0.0f, 0.0f };

			// キーフレームが1つだけなら固定値
			if (keys->size() == 1)
			{
				lerpOffset = (*keys)[0].offsetVal;
				lerpSize = (*keys)[0].sizeVal;
			}
			else
			{
				// 前後のキーフレームを探す
				const HurtboxKey* pPrev = &(*keys)[0];
				const HurtboxKey* pNext = &(*keys)[keys->size() - 1];

				// もし現在のフレームが最初のキーより前なら、最初のキーを使う
				if (currentFrame <= pPrev->frame) {
					lerpOffset = pPrev->offsetVal;
					lerpSize = pPrev->sizeVal;
				}
				// もし現在のフレームが最後のキーより後なら、最後のキーを使う
				else if (currentFrame >= pNext->frame) {
					lerpOffset = pNext->offsetVal;
					lerpSize = pNext->sizeVal;
				}
				else
				{
					// 間にある場合、区間を探して線形補間
					for (size_t i = 0; i < keys->size() - 1; ++i)
					{
						if (currentFrame >= (*keys)[i].frame && currentFrame < (*keys)[i + 1].frame)
						{
							pPrev = &(*keys)[i];
							pNext = &(*keys)[i + 1];
							break;
						}
					}

					// 進行度 t (0.0 ~ 1.0) を計算
					float range = pNext->frame - pPrev->frame;
					float t = 0.0f;
					if (range > 0.0001f) {
						t = (currentFrame - pPrev->frame) / range;
					}

					// Lerp
					lerpOffset.x = pPrev->offsetVal.x + (pNext->offsetVal.x - pPrev->offsetVal.x) * t;
					lerpOffset.y = pPrev->offsetVal.y + (pNext->offsetVal.y - pPrev->offsetVal.y) * t;
					lerpSize.x = pPrev->sizeVal.x + (pNext->sizeVal.x - pPrev->sizeVal.x) * t;
					lerpSize.y = pPrev->sizeVal.y + (pNext->sizeVal.y - pPrev->sizeVal.y) * t;
				}
			}

			// 計算した補正値を加算
			offsetX += lerpOffset.x;
			offsetY += lerpOffset.y;
			extentX += lerpSize.x;
			extentY += lerpSize.y;
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

void Player::Reset()
{
	m_hp = m_maxHp;
	m_isJumping = false;
	m_isAttacking = false;
	m_hasHit = false;
	m_velocity = { 0.0f, 0.0f, 0.0f };
	SetState(new PlayerStateIdle());
}