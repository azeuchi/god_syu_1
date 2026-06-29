#include "Player.h"
#include "Input.h" 
#include <DirectXCollision.h>
#include "Geometory.h" 
#include <fstream>
#include "Shader.h"
#include "Model.h"
#include <Xinput.h>

// ・ｽX・ｽe・ｽ[・ｽg・ｽp・ｽ^・ｽ[・ｽ・ｽ・ｽp
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
#include "Hadouken.h"
#include "PlayerStateDeath.h"

#include "Projectile.h"

#include <DirectXMath.h>
#include <algorithm>

KeyConfig g_keyConfigP1 = { 'W', 'S', 'A', 'D', 'U', 'I', 'O', 'K', 'L' };
KeyConfig g_keyConfigP2 = { VK_UP, VK_DOWN, VK_LEFT, VK_RIGHT, VK_NUMPAD1, VK_NUMPAD2, VK_NUMPAD4, VK_NUMPAD5, VK_NUMPAD3 };

PadConfig g_padConfigP1 = { XINPUT_GAMEPAD_DPAD_UP, XINPUT_GAMEPAD_DPAD_DOWN, XINPUT_GAMEPAD_DPAD_LEFT, XINPUT_GAMEPAD_DPAD_RIGHT, XINPUT_GAMEPAD_X, XINPUT_GAMEPAD_Y, XINPUT_GAMEPAD_RIGHT_SHOULDER, XINPUT_GAMEPAD_A, XINPUT_GAMEPAD_B };
PadConfig g_padConfigP2 = { XINPUT_GAMEPAD_DPAD_UP, XINPUT_GAMEPAD_DPAD_DOWN, XINPUT_GAMEPAD_DPAD_LEFT, XINPUT_GAMEPAD_DPAD_RIGHT, XINPUT_GAMEPAD_X, XINPUT_GAMEPAD_Y, XINPUT_GAMEPAD_RIGHT_SHOULDER, XINPUT_GAMEPAD_A, XINPUT_GAMEPAD_B };

InputDeviceType g_inputDeviceP1 = InputDeviceType::KEYBOARD;
InputDeviceType g_inputDeviceP2 = InputDeviceType::PAD_0;

namespace {
	// ・ｽA・ｽj・ｽ・ｽ・ｽ[・ｽV・ｽ・ｽ・ｽ・ｽ・ｽ・ｽ・ｽﾌ指・ｽ・ｽt・ｽ・ｽ・ｽ[・ｽ・ｽ・ｽﾉゑｿｽ・ｽ・ｽ・ｽ體厄ｿｽ・ｽ・ｽ阡ｻ・ｽ・ｽﾌ位置・ｽﾆサ・ｽC・ｽY・ｽ・ｽ・ｽv・ｽZ・ｽ・ｽ・ｽ・ｽw・ｽ・ｽ・ｽp・ｽ[・ｽﾖ撰ｿｽ
	BoxData CalculateInterpolatedBox(const AnimatedBox& animBox, float currentFrame)
	{
		// ・ｽL・ｽ[・ｽt・ｽ・ｽ・ｽ[・ｽ・ｽ・ｽ・ｽ・ｽ・ｽ・ｽﾝゑｿｽ・ｽﾈゑｿｽ・ｽ鼾・ｿｽﾍ擾ｿｽ・ｽ・ｽ・ｽl・ｽ・ｽﾔゑｿｽ
		if (animBox.keyframes.empty())
		{
			return BoxData();
		}

		// ・ｽL・ｽ[・ｽt・ｽ・ｽ・ｽ[・ｽ・ｽ・ｽ・ｽ1・ｽﾂゑｿｽ・ｽ・ｽ・ｽﾈゑｿｽA・ｽ・ｽﾉゑｿｽ・ｽﾌ値・ｽ・ｽ・ｽg・ｽ・ｽ
		if (animBox.keyframes.size() == 1)
		{
			return animBox.keyframes[0].data;
		}

		// ・ｽ・ｽ・ｽﾝフ・ｽ・ｽ・ｽ[・ｽ・ｽ・ｽ・ｽ・ｽﾅ擾ｿｽ・ｽﾌキ・ｽ[・ｽt・ｽ・ｽ・ｽ[・ｽ・ｽ・ｽ・ｽ・ｽO・ｽﾈゑｿｽA・ｽﾅ擾ｿｽ・ｽﾌ値・ｽ・ｽ・ｽg・ｽ・ｽ
		if (currentFrame <= animBox.keyframes.front().frame)
		{
			return animBox.keyframes.front().data;
		}
		// ・ｽ・ｽ・ｽﾝフ・ｽ・ｽ・ｽ[・ｽ・ｽ・ｽ・ｽ・ｽﾅ鯉ｿｽﾌキ・ｽ[・ｽt・ｽ・ｽ・ｽ[・ｽ・ｽ・ｽ・ｽ・ｽ・ｽﾈゑｿｽA・ｽﾅ鯉ｿｽﾌ値・ｽ・ｽ・ｽg・ｽ・ｽ
		if (currentFrame >= animBox.keyframes.back().frame)
		{
			return animBox.keyframes.back().data;
		}

		// ・ｽL・ｽ[・ｽt・ｽ・ｽ・ｽ[・ｽ・ｽ・ｽﾔの包ｿｽﾔ擾ｿｽ・ｽ・ｽ
		for (size_t i = 0; i < animBox.keyframes.size() - 1; ++i)
		{
			const auto& k1 = animBox.keyframes[i];
			const auto& k2 = animBox.keyframes[i + 1];

			// ・ｽ・ｽ・ｽﾝのフ・ｽ・ｽ・ｽ[・ｽ・ｽ・ｽ・ｽ・ｽﾇのキ・ｽ[・ｽt・ｽ・ｽ・ｽ[・ｽ・ｽ・ｽﾔに托ｿｽ・ｽﾝゑｿｽ・ｽ驍ｩ・ｽ`・ｽF・ｽb・ｽN
			if (currentFrame >= k1.frame && currentFrame < k2.frame)
			{
				float range = k2.frame - k1.frame;
				float t = 0.0f;
				// ・ｽ[・ｽ・ｽ・ｽ・ｽ・ｽZ・ｽh・ｽ~
				if (range > 0.0001f)
				{
					t = (currentFrame - k1.frame) / range;
				}

				BoxData result;
				using namespace DirectX;
				XMVECTOR vOff1 = XMLoadFloat2(&k1.data.offset);
				XMVECTOR vOff2 = XMLoadFloat2(&k2.data.offset);
				XMVECTOR vExt1 = XMLoadFloat2(&k1.data.extents);
				XMVECTOR vExt2 = XMLoadFloat2(&k2.data.extents);

				// ・ｽﾊ置・ｽﾆサ・ｽC・ｽY・ｽ・ｽ・ｽ・ｽ`・ｽ・ｽ・ｽ (Lerp)
				XMVECTOR vOff = XMVectorLerp(vOff1, vOff2, t);
				XMVECTOR vExt = XMVectorLerp(vExt1, vExt2, t);

				XMStoreFloat2(&result.offset, vOff);
				XMStoreFloat2(&result.extents, vExt);

				return result;
			}
		}

		// ・ｽ・ｽ・ｽS・ｽﾌゑｿｽ・ｽﾟのフ・ｽH・ｽ[・ｽ・ｽ・ｽo・ｽb・ｽN
		return animBox.keyframes.back().data;
	}
}


Player::Player()
	: m_model(std::make_shared<Model>())
	, m_position(0.0f, 0.0f, 0.0f)
	, m_rotation(0.0f, 0.0f, 0.0f)
	, m_scale(1.0f, 1.0f, 1.0f)
	, m_velocity(0.0f, 0.0f, 0.0f)
	, m_isJumping(false)
	, m_moveSpeed(2.0f)
	, m_jumpSpeed(2.5f)
	, m_currentState(nullptr)
	, m_inputType(PlayerInputType::AI)
	, m_blendFactor(1.0f)
	, m_isAttacking(false)
	, m_isAnimPaused(false)
	, m_hp(10000)
	, m_hasHit(false)
	, m_pActiveAttackParams(nullptr)
	, m_animSpeed(1.0f)
	, m_projectile(new Projectile())
	, m_attackTimer(0.0f)
{
	// ・ｽ・ｽ・ｽ・ｽ・ｽA・ｽj・ｽ・ｽ・ｽ[・ｽV・ｽ・ｽ・ｽ・ｽ・ｽﾆス・ｽe・ｽ[・ｽg・ｽﾌ設抵ｿｽ
	m_currentAnim = { "Idle", 0 };
	m_previousAnim = { "Idle", 0 };
	SetState(new PlayerStateIdle());

	InitDefaultParameters();
}

Player::~Player()
{
	// ・ｽX・ｽe・ｽ[・ｽg・ｽﾆ費ｿｽﾑ難ｿｽ・ｽ・ｽﾌ・ｿｽ・ｽ・ｽ・ｽ・ｽ・ｽ・ｽ・ｽ
	if (m_currentState) {
		delete m_currentState;
		m_currentState = nullptr;
	}
	if (m_projectile) {
		delete m_projectile;
		m_projectile = nullptr;
	}
}

// ・ｽv・ｽ・ｽ・ｽC・ｽ・ｽ・ｽ[・ｽﾌ移難ｿｽ・ｽ・ｽ・ｽx・ｽ・ｽA・ｽe・ｽ・ｽU・ｽ・ｽ・ｽZ・ｽﾌ擾ｿｽ・ｽ・ｽ・ｽp・ｽ・ｽ・ｽ・ｽ・ｽ[・ｽ^・ｽ・ｽﾝ定す・ｽ・ｽ
void Player::InitDefaultParameters()
{
	m_moveSpeed = 2.102f;
	m_jumpSpeed = 2.5f;
	m_scale = { 1.0f, 1.0f, 1.0f };

	// ・ｽ・ｽ・ｽ・ｽ・ｽ・ｽﾔの奇ｿｽ{・ｽ・ｽ・ｽ轤｢・ｽ・ｽ・ｽ・ｽ (Head, Body, Legs)
	m_baseHurtboxExtents[(int)HurtboxType::HEAD] = { 0.33f, 0.33f };
	m_baseHurtboxOffsets[(int)HurtboxType::HEAD] = { 0.024f, 1.923f };
	m_baseHurtboxExtents[(int)HurtboxType::BODY] = { 0.598f, 0.522f };
	m_baseHurtboxOffsets[(int)HurtboxType::BODY] = { 0.011f, 1.198f };
	m_baseHurtboxExtents[(int)HurtboxType::LEGS] = { 0.482f, 0.497f };
	m_baseHurtboxOffsets[(int)HurtboxType::LEGS] = { -0.007f, 0.159f };

	// ・ｽ・ｽ・ｽ痰ｪ・ｽﾝ擾ｿｽﾔの奇ｿｽ{・ｽ・ｽ・ｽ轤｢・ｽ・ｽ・ｽ・ｽ
	m_crouchHurtboxExtents[(int)HurtboxType::HEAD] = { 0.281f, 0.176f };
	m_crouchHurtboxOffsets[(int)HurtboxType::HEAD] = { 0.272f, 1.425f };
	m_crouchHurtboxExtents[(int)HurtboxType::BODY] = { 0.6f, 0.401f };
	m_crouchHurtboxOffsets[(int)HurtboxType::BODY] = { 0.112f, 0.846f };
	m_crouchHurtboxExtents[(int)HurtboxType::LEGS] = { 0.544f, 0.364f };
	m_crouchHurtboxOffsets[(int)HurtboxType::LEGS] = { 0.116f, 0.308f };


	// ・ｽﾇ会ｿｽ・ｽ・ｽ・ｽ轤｢・ｽ・ｽ・ｽ・ｽﾌ擾ｿｽ・ｽ・ｽ・ｽ・ｽ・ｽw・ｽ・ｽ・ｽp・ｽ[・ｽB・ｽ・ｽ{・ｽﾌゑｿｽ・ｽ轤｢・ｽ・ｽ・ｽ・ｽ(・ｽ・ｽ・ｽE・ｽﾌ・・ｽ・ｽ)・ｽﾍ常時・ｽL・ｽ・ｽ・ｽﾈので、
	// ・ｽZ・ｽﾅ有・ｽﾌ追会ｿｽ・ｽ・ｽ・ｽ轤｢・ｽ・ｽ・ｽ・ｽﾍデ・ｽt・ｽH・ｽ・ｽ・ｽg・ｽﾅは厄ｿｽ・ｽ・ｽ(・ｽ・ｽ)・ｽﾉゑｿｽ・ｽﾄゑｿｽ・ｽ・ｽ
	auto InitDefaultHurtboxes = [&](std::vector<WindowedHurtbox>& boxes) {
		boxes.clear();
		};


	// --- ・ｽ・ｽp・ｽ・ｽ・ｽ`・ｽﾌ設抵ｿｽ ---
	{
		AttackParams& p = m_lightPunchParams;
		p.totalDuration = 0.166667f;
		p.hitboxStart = 0.0666667f; // ・ｽ・ｽ・ｽ・ｽ
		p.hitboxEnd = 0.15f;        // ・ｽ・ｽ・ｽ・ｽ・ｽI・ｽ・ｽ

		// ・ｽU・ｽ・ｽ・ｽ・ｽ・ｽ・ｽiHitbox・ｽj・ｽﾌ作成
		p.hitboxes.clear();
		AnimatedBox ab;
		ab.keyframes.push_back({ 0.0f, { {0.76f, 1.7f}, {0.454f, 0.157f} } });
		p.hitboxes.push_back(ab);

		p.damage = 100; p.hitFrame = 2; p.blockFrame = -2; p.hitStop = 0.0666667f; p.knockback = 0.223f;
		p.isDown = false;
		p.attackLevel = AttackLevel::HIGH; // ・ｽ・ｽi・ｽ・ｽ・ｽ・ｽ

		InitDefaultHurtboxes(p.moveHurtboxes);

		// ・ｽL・ｽ・ｽ・ｽ・ｽ・ｽZ・ｽ・ｽ・ｽﾂ能・ｽ^・ｽC・ｽ~・ｽ・ｽ・ｽO・ｽﾌ設抵ｿｽ
		p.cancelEnabled = true; p.cancelStart = 0.0666667f; p.cancelEnd = 0.166667f;
		p.cancelToLight = true; p.cancelToMedium = true; p.cancelToHeavyPunch = false; p.cancelToMediumKick = false; p.cancelToHeavy = false;
		p.speedModifiers.clear();
	}

	// --- ・ｽ・ｽ・ｽp・ｽ・ｽ・ｽ`・ｽﾌ設抵ｿｽ ---
	{
		AttackParams& p = m_mediumPunchParams;
		p.totalDuration = 0.333333f;
		p.hitboxStart = 0.1f;
		p.hitboxEnd = 0.333333f;
		p.hitboxes.clear();
		AnimatedBox ab;
		ab.keyframes.push_back({ 0.0f, { {0.735f, 1.642f}, {0.509f, 0.174f} } });
		p.hitboxes.push_back(ab);

		p.damage = 400; p.hitFrame = 5; p.blockFrame = -2; p.hitStop = 0.05f; p.knockback = 0.0f;
		p.isDown = false;
		p.attackLevel = AttackLevel::HIGH;

		InitDefaultHurtboxes(p.moveHurtboxes);

		p.cancelEnabled = true; p.cancelStart = 0.1f; p.cancelEnd = 0.333333f;
		p.cancelToLight = false; p.cancelToMedium = false; p.cancelToHeavyPunch = true; p.cancelToMediumKick = false; p.cancelToHeavy = true;

		// ・ｽ・ｽ・ｽ・ｽ・ｽ・ｽd・ｽ・ｽ・ｽ・ｽ・ｽ・ｽ・ｽﾜゑｿｽ・ｽ・ｽ・ｽ・ｽ・ｽﾟのア・ｽj・ｽ・ｽ・ｽ[・ｽV・ｽ・ｽ・ｽ・ｽ・ｽﾄ撰ｿｽ・ｽ・ｽ・ｽx・ｽ・ｽ・ｽ・ｽ
		p.speedModifiers.clear();
		p.speedModifiers.push_back({ 0.0f, 6.0f, 0.369f });
		p.speedModifiers.push_back({ 7.0f, 12.0f, 2.533f });
	}

	// --- ・ｽ・ｽ・ｽp・ｽ・ｽ・ｽ`・ｽﾌ設抵ｿｽ ---
	{
		AttackParams& p = m_heavyPunchParams;
		p.totalDuration = 0.5f;
		p.hitboxStart = 0.133333f;
		p.hitboxEnd = 0.5f;
		p.hitboxes.clear();
		AnimatedBox ab;
		ab.keyframes.push_back({ 0.0f, { {0.852f, 1.435f}, {0.687f, 0.202f} } });
		p.hitboxes.push_back(ab);

		p.damage = 700; p.hitFrame = 5; p.blockFrame = -2; p.hitStop = 0.0666667f; p.knockback = 0.0f;
		p.isDown = false;
		p.attackLevel = AttackLevel::HIGH;

		InitDefaultHurtboxes(p.moveHurtboxes);

		p.cancelEnabled = true; p.cancelStart = 0.133333f; p.cancelEnd = 0.416667f;
		p.cancelToLight = false; p.cancelToMedium = false; p.cancelToHeavyPunch = false; p.cancelToMediumKick = false; p.cancelToHeavy = true;

		p.speedModifiers.clear();
		p.speedModifiers.push_back({ 0.0f, 10.0f, 0.344f });
		p.speedModifiers.push_back({ 11.0f, 20.0f, 2.057f });
		p.speedModifiers.push_back({ 22.0f, 30.0f, 0.486f });
	}

	// --- ・ｽ・ｽ・ｽL・ｽb・ｽN・ｽﾌ設抵ｿｽ ---
	{
		AttackParams& p = m_mediumKickParams;
		p.totalDuration = 0.416667f;
		p.hitboxStart = 0.1f;
		p.hitboxEnd = 0.416667f;
		p.hitboxes.clear();
		AnimatedBox ab;
		ab.keyframes.push_back({ 0.0f, { {1.5f, 1.56f}, {0.351f, 0.24f} } });
		p.hitboxes.push_back(ab);

		p.damage = 400; p.hitFrame = 5; p.blockFrame = -2; p.hitStop = 0.05f; p.knockback = 0.0f;
		p.isDown = false;
		p.attackLevel = AttackLevel::MID; // ・ｽ・ｽ・ｽi・ｽ・ｽ・ｽ・ｽi・ｽ・ｽ・ｽ痰ｪ・ｽﾝガ・ｽ[・ｽh・ｽs・ｽﾂ）

		InitDefaultHurtboxes(p.moveHurtboxes);

		p.cancelEnabled = false;

		p.speedModifiers.clear();
		p.speedModifiers.push_back({ 0.0f, 4.0f, 0.362f });
		p.speedModifiers.push_back({ 5.0f, 9.0f, 0.503f });
		p.speedModifiers.push_back({ 10.0f, 20.0f, 0.295f });
	}

	// --- ・ｽ・ｽ・ｽL・ｽb・ｽN・ｽﾌ設抵ｿｽ ---
	{
		AttackParams& p = m_heavyKickParams;
		p.totalDuration = 0.5f;
		p.hitboxStart = 0.133333f;
		p.hitboxEnd = 0.5f;
		p.hitboxes.clear();
		AnimatedBox ab;
		ab.keyframes.push_back({ 0.0f, { {1.324f, 1.759f}, {0.551f, 0.23f} } });
		p.hitboxes.push_back(ab);

		p.damage = 700; p.hitFrame = 5; p.blockFrame = -2; p.hitStop = 0.0666667f; p.knockback = 0.0f;
		p.isDown = false;
		p.attackLevel = AttackLevel::LOW; // ・ｽ・ｽ・ｽi・ｽ・ｽ・ｽ・ｽi・ｽ・ｽ・ｽ・ｽ・ｽK・ｽ[・ｽh・ｽs・ｽﾂ）

		InitDefaultHurtboxes(p.moveHurtboxes);

		p.cancelEnabled = false;

		p.speedModifiers.clear();
		p.speedModifiers.push_back({ 0.0f, 6.0f, 0.463f });
		p.speedModifiers.push_back({ 7.0f, 16.0f, 1.006f });
		p.speedModifiers.push_back(
			{ 17.0f, 30.0f, 0.345f });
	}

	// --- ・ｽg・ｽ・ｽ・ｽ・ｽ (・ｽ・ｽ/・ｽ・ｽ/・ｽ・ｽ) ・ｽﾌ設抵ｿｽ ---
	{
		InitDefaultHurtboxes(m_hadoukenLParams.moveHurtboxes);
		m_hadoukenLParams.totalDuration = 0.6f;
		m_hadoukenLParams.projectileSpeed = 4.0f; // ・ｽe・ｽ・ｽ
		m_hadoukenLParams.damage = 500;
		m_hadoukenLParams.attackLevel = AttackLevel::HIGH;

		InitDefaultHurtboxes(m_hadoukenMParams.moveHurtboxes);
		m_hadoukenMParams.totalDuration = 0.6f;
		m_hadoukenMParams.projectileSpeed = 6.5f;
		m_hadoukenMParams.damage = 500;
		m_hadoukenMParams.attackLevel = AttackLevel::HIGH;

		InitDefaultHurtboxes(m_hadoukenHParams.moveHurtboxes);
		m_hadoukenHParams.totalDuration = 0.6f;
		m_hadoukenHParams.projectileSpeed = 9.0f;
		m_hadoukenHParams.damage = 500;
		m_hadoukenHParams.attackLevel = AttackLevel::HIGH;
	}
}

// ・ｽ・ｽ・ｽt・ｽ・ｽ・ｽ[・ｽ・ｽ・ｽﾄばゑｿｽ・ｽX・ｽV・ｽ・ｽ・ｽ・ｽ・ｽﾌ托ｿｽg
void Player::Update(float tick)
{
	// 1. ・ｽ・ｽ・ｽﾍの取得
	PollInputs();
	UpdateCommandTimer(tick);
	UpdateInputBuffer(tick);

	// 2. ・ｽ・ｽ・ｽﾝのス・ｽe・ｽ[・ｽg・ｽi・ｽ・ｽﾔ）・ｽﾌ更・ｽV
	if (m_currentState) {
		m_isCrouching = m_currentState->IsCrouch();
		m_currentState->Update(this, tick);
		if (m_pActiveAttackParams) m_attackElapsedSec += tick;
	}

	// 3. ・ｽ・ｽ・ｽ・ｽ・ｽE・ｽ・ｽ・ｽW・ｽﾌ更・ｽV
	UpdatePhysics(tick);

	// 4. ・ｽA・ｽj・ｽ・ｽ・ｽ[・ｽV・ｽ・ｽ・ｽ・ｽ・ｽt・ｽ・ｽ・ｽ[・ｽ・ｽ・ｽﾌ進・ｽs
	UpdateAnimation(tick);

	// 5. ・ｽ・ｽ・ｽf・ｽ・ｽ・ｽﾌア・ｽj・ｽ・ｽ・ｽ[・ｽV・ｽ・ｽ・ｽ・ｽ・ｽu・ｽ・ｽ・ｽ・ｽ・ｽh・ｽX・ｽV
	UpdateModelBlend();

	// 6. ・ｽ・ｽ・ｽﾝのフ・ｽ・ｽ・ｽ[・ｽ・ｽ・ｽﾉ会ｿｽ・ｽ・ｽ・ｽ・ｽ・ｽU・ｽ・ｽ・ｽ・ｽ・ｽ・ｽE・ｽ・ｽ・ｽ轤｢・ｽ・ｽ・ｽ・ｽﾌ更・ｽV
	UpdateAttackBoxes();

	// 7. ・ｽ・ｽ・ｽﾋ済みの費ｿｽﾑ難ｿｽ・ｽ・ｽﾌ更・ｽV
	if (m_projectile) {
		m_projectile->Update(tick);
	}
}

// ・ｽL・ｽ[・ｽ{・ｽ[・ｽh・ｽ・ｽR・ｽ・ｽ・ｽg・ｽ・ｽ・ｽ[・ｽ・ｽ・ｽ[・ｽﾌ難ｿｽ・ｽﾍゑｿｽ・ｽ謫ｾ・ｽ・ｽ・ｽAPlayerInputs・ｽ\・ｽ・ｽ・ｽﾌにマ・ｽb・ｽs・ｽ・ｽ・ｽO・ｽ・ｽ・ｽ・ｽ
void Player::PollInputs()
{
	m_inputs = {};

	switch (m_inputType)
	{
	case PlayerInputType::PLAYER_1:
		if (g_inputDeviceP1 == InputDeviceType::KEYBOARD)
		{
			if (!IsKeyPress(VK_RBUTTON))
			{
				if (IsKeyPress(g_keyConfigP1.left)) m_inputs.moveLeft = true;
				else if (IsKeyPress(g_keyConfigP1.right)) m_inputs.moveRight = true;
				if (IsKeyPress(g_keyConfigP1.down)) m_inputs.moveDown = true;
				if (IsKeyTrigger(g_keyConfigP1.up)) m_inputs.jump = true;
			}
			if (IsKeyTrigger(g_keyConfigP1.lightPunch)) m_inputs.LightPunch = true;
			if (IsKeyTrigger(g_keyConfigP1.mediumPunch)) m_inputs.MediumPunch = true;
			if (IsKeyTrigger(g_keyConfigP1.heavyPunch)) m_inputs.HeavyPunch = true;
			if (IsKeyTrigger(g_keyConfigP1.mediumKick)) m_inputs.MediumKick = true;
			if (IsKeyTrigger(g_keyConfigP1.heavyKick)) m_inputs.HeavyKick = true;
		}
		else
		{
			int padNo = (int)g_inputDeviceP1 - 1;
			if (IsPadPress(padNo, g_padConfigP1.left)) m_inputs.moveLeft = true;
			else if (IsPadPress(padNo, g_padConfigP1.right)) m_inputs.moveRight = true;
			if (IsPadPress(padNo, g_padConfigP1.down)) m_inputs.moveDown = true;
			if (IsPadTrigger(padNo, g_padConfigP1.up)) m_inputs.jump = true;

			if (IsPadTrigger(padNo, g_padConfigP1.lightPunch)) m_inputs.LightPunch = true;
			if (IsPadTrigger(padNo, g_padConfigP1.mediumPunch)) m_inputs.MediumPunch = true;
			if (IsPadTrigger(padNo, g_padConfigP1.heavyPunch)) m_inputs.HeavyPunch = true;
			if (IsPadTrigger(padNo, g_padConfigP1.mediumKick)) m_inputs.MediumKick = true;
			if (IsPadTrigger(padNo, g_padConfigP1.heavyKick)) m_inputs.HeavyKick = true;
		}
		break;

	case PlayerInputType::PLAYER_2:
		if (g_inputDeviceP2 == InputDeviceType::KEYBOARD)
		{
			if (!IsKeyPress(VK_RBUTTON))
			{
				if (IsKeyPress(g_keyConfigP2.left)) m_inputs.moveLeft = true;
				else if (IsKeyPress(g_keyConfigP2.right)) m_inputs.moveRight = true;
				if (IsKeyPress(g_keyConfigP2.down)) m_inputs.moveDown = true;
				if (IsKeyTrigger(g_keyConfigP2.up)) m_inputs.jump = true;
			}
			if (IsKeyTrigger(g_keyConfigP2.lightPunch)) m_inputs.LightPunch = true;
			if (IsKeyTrigger(g_keyConfigP2.mediumPunch)) m_inputs.MediumPunch = true;
			if (IsKeyTrigger(g_keyConfigP2.heavyPunch)) m_inputs.HeavyPunch = true;
			if (IsKeyTrigger(g_keyConfigP2.mediumKick)) m_inputs.MediumKick = true;
			if (IsKeyTrigger(g_keyConfigP2.heavyKick)) m_inputs.HeavyKick = true;
		}
		else
		{
			int padNo = (int)g_inputDeviceP2 - 1;
			if (IsPadPress(padNo, g_padConfigP2.left)) m_inputs.moveLeft = true;
			else if (IsPadPress(padNo, g_padConfigP2.right)) m_inputs.moveRight = true;
			if (IsPadPress(padNo, g_padConfigP2.down)) m_inputs.moveDown = true;
			if (IsPadTrigger(padNo, g_padConfigP2.up)) m_inputs.jump = true;

			if (IsPadTrigger(padNo, g_padConfigP2.lightPunch)) m_inputs.LightPunch = true;
			if (IsPadTrigger(padNo, g_padConfigP2.mediumPunch)) m_inputs.MediumPunch = true;
			if (IsPadTrigger(padNo, g_padConfigP2.heavyPunch)) m_inputs.HeavyPunch = true;
			if (IsPadTrigger(padNo, g_padConfigP2.mediumKick)) m_inputs.MediumKick = true;
			if (IsPadTrigger(padNo, g_padConfigP2.heavyKick)) m_inputs.HeavyKick = true;
		}
		break;

	case PlayerInputType::AI:
		m_inputs = m_injectedInputs;
		// AI・ｽﾌ難ｿｽ・ｽﾍ・ｿｽ・ｽW・ｽb・ｽN・ｽﾍ別途・ｽ・ｽ・ｽ・ｽ
		break;
	}
}


// ・ｽd・ｽﾍの適・ｽp・ｽﾆ搾ｿｽ・ｽW・ｽﾌ移難ｿｽ・ｽ・ｽ・ｽ・ｽ
void Player::SetInjectedInputs(const PlayerInputs& in)
{
	m_injectedInputs = in;
}

void Player::UpdateInputBuffer(float tick)
{
	const float BUF = 0.1f; // about 6 frames of attack input buffer
	bool raw[5] = { m_inputs.LightPunch, m_inputs.MediumPunch, m_inputs.HeavyPunch, m_inputs.MediumKick, m_inputs.HeavyKick };
	for (int i = 0; i < 5; ++i)
	{
		if (raw[i]) m_atkBuf[i] = BUF;
		else { m_atkBuf[i] -= tick; if (m_atkBuf[i] < 0.0f) m_atkBuf[i] = 0.0f; }
	}
	m_inputs.LightPunch  = (m_atkBuf[0] > 0.0f);
	m_inputs.MediumPunch = (m_atkBuf[1] > 0.0f);
	m_inputs.HeavyPunch  = (m_atkBuf[2] > 0.0f);
	m_inputs.MediumKick  = (m_atkBuf[3] > 0.0f);
	m_inputs.HeavyKick   = (m_atkBuf[4] > 0.0f);
}

void Player::UpdatePhysics(float tick)
{
	if (m_isJumping) {
		m_velocity.y -= 55.0f * tick; // ・ｽd・ｽ・ｽ
	}

	m_position.x += m_velocity.x * tick;
	m_position.y += m_velocity.y * tick;
	m_position.z += m_velocity.z * tick;

	// ・ｽn・ｽﾊとの難ｿｽ・ｽ・ｽ・ｽ阡ｻ・ｽ・ｽiY・ｽ・ｽ・ｽW0・ｽ・ｽ・ｽ・ｽ・ｽﾆゑｿｽ・ｽ・ｽj
	if (m_position.y <= 0.0f) {
		m_position.y = 0.0f;
		if (m_isJumping) {
			m_velocity.y = 0.0f;
			m_velocity.x = 0.0f;
			m_isJumping = false;
		}
	}
}

// ・ｽA・ｽj・ｽ・ｽ・ｽ[・ｽV・ｽ・ｽ・ｽ・ｽ・ｽﾌフ・ｽ・ｽ・ｽ[・ｽ・ｽ・ｽi・ｽs・ｽﾆ再撰ｿｽ・ｽ・ｽ・ｽx・ｽﾌ難ｿｽ・ｽI・ｽﾏ更・ｽ・ｽ・ｽs・ｽ・ｽ
void Player::UpdateAnimation(float tick)
{
	// ・ｽ・ｽ・ｽ[・ｽV・ｽ・ｽ・ｽ・ｽ・ｽJ・ｽﾚ趣ｿｽ・ｽﾌブ・ｽ・ｽ・ｽ・ｽ・ｽh・ｽW・ｽ・ｽ・ｽ・ｽ・ｽX・ｽV
	if (m_blendFactor < 1.0f)
	{
		m_blendFactor += tick / m_transitionDuration;
		if (m_blendFactor > 1.0f) m_blendFactor = 1.0f;
	}

	if (!m_isAnimPaused)
	{
		float currentSpeed = m_animSpeed;

		// ・ｽU・ｽ・ｽ・ｽ・ｽ・ｽ[・ｽV・ｽ・ｽ・ｽ・ｽ・ｽ・ｽ・ｽﾌ場合・ｽA・ｽw・ｽ閧ｳ・ｽ黷ｽ・ｽt・ｽ・ｽ・ｽ[・ｽ・ｽ・ｽ・ｽﾔで再撰ｿｽ・ｽ・ｽ・ｽx・ｽ・ｽﾏ難ｿｽ・ｽ・ｽ・ｽ・ｽ・ｽ・ｽ
		if (m_pActiveAttackParams)
		{
			bool isControlledAnim = (
				strcmp(m_currentAnim.name, "LightPunch") == 0 ||
				strcmp(m_currentAnim.name, "MediumPunch") == 0 ||
				strcmp(m_currentAnim.name, "HeavyPunch") == 0 ||
				strcmp(m_currentAnim.name, "MediumKick") == 0 ||
				strcmp(m_currentAnim.name, "HeavyKick") == 0 ||
				strcmp(m_currentAnim.name, "Hadouken") == 0 ||
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

		// ・ｽt・ｽ・ｽ・ｽ[・ｽ・ｽ・ｽ・ｽi・ｽﾟゑｿｽi60FPS・ｽ譓・ｽj
		m_currentAnim.frame += currentSpeed * tick * 60.0f;

		// ・ｽU・ｽ・ｽ・ｽ・ｽ・ｽ關ｶ・ｽ・ｽ・ｽp・ｽﾌタ・ｽC・ｽ}・ｽ[・ｽ・ｽ・ｽt・ｽ・ｽ・ｽ[・ｽ・ｽ・ｽﾆ難ｿｽ・ｽ・ｽ
		if (m_pActiveAttackParams)
		{
			// 攻撃判定の発生タイミングは経過時間（60FPS基準）で測る。
			// アニメ再生速度を変えても判定が前後にズレないようにするため。
			m_attackTimer = m_attackElapsedSec * 60.0f;
		}
	}
}

// ・ｽﾏ更・ｽO・ｽﾌア・ｽj・ｽ・ｽ・ｽ[・ｽV・ｽ・ｽ・ｽ・ｽ・ｽﾆ変更・ｽ・ｽﾌア・ｽj・ｽ・ｽ・ｽ[・ｽV・ｽ・ｽ・ｽ・ｽ・ｽ・ｽ・ｽ・ｽ・ｽ轤ｩ・ｽﾉ包ｿｽﾔゑｿｽ・ｽ・ｽ
void Player::UpdateModelBlend()
{
	m_model->UpdateWithBlend(
		m_currentAnim.name, (int)m_currentAnim.frame,
		m_previousAnim.name, (int)m_previousAnim.frame,
		m_blendFactor);
}

// ・ｽv・ｽ・ｽ・ｽC・ｽ・ｽ・ｽ[・ｽﾌス・ｽe・ｽ[・ｽg・ｽi・ｽﾒ機・ｽA・ｽ・ｽ・ｽs・ｽA・ｽW・ｽ・ｽ・ｽ・ｽ・ｽv・ｽA・ｽU・ｽ・ｽ・ｽﾈど）・ｽ・ｽﾘゑｿｽﾖゑｿｽ・ｽ・ｽ
void Player::SetState(PlayerState* newState)
{
	if (newState != nullptr)
	{

		// ・ｽﾌ力ゑｿｽ0・ｽﾈ会ｿｽ・ｽﾈら死・ｽS・ｽX・ｽe・ｽ[・ｽg・ｽﾈ外・ｽﾖの遷・ｽﾚゑｿｽ・ｽ・ｽ・ｽ・ｽ
		if (m_hp <= 0 && !newState->IsDeathState())
		{
			delete newState;
			return;
		}

		if (m_currentState) {
			// ・ｽ・ｽ・ｽﾅに趣ｿｽ・ｽS・ｽ・ｽﾔなゑｿｽA・ｽ・ｽ・ｽﾌ擾ｿｽﾔにゑｿｽ・ｽ繽托ｿｽ・ｽ・ｽ・ｽ・ｽu・ｽ・ｽ・ｽb・ｽN
			if (m_currentState->IsDeathState() && m_hp <= 0) {
				delete newState;
				return;
			}
			delete m_currentState;
		}

		m_currentState = newState;
		m_currentState->OnEnter(this);
	}
}

// ・ｽ・ｽ・ｽﾝのス・ｽe・ｽ[・ｽg・ｽ・ｽ・ｽ・ｽ・ｽG・ｽ・ｽ・ｽﾇゑｿｽ・ｽ・ｽ・ｽｻ抵ｿｽ
bool Player::IsInvincible() const
{
	if (m_currentState)
	{
		return m_currentState->IsInvincible();
	}
	return false;
}

// ・ｽw・ｽ閧ｵ・ｽ・ｽ・ｽ・ｽ・ｽO・ｽﾌア・ｽj・ｽ・ｽ・ｽ[・ｽV・ｽ・ｽ・ｽ・ｽ・ｽ・ｽ・ｽﾄ撰ｿｽ・ｽ・ｽ・ｽ・ｽ
void Player::PlayAnimation(const char* name, bool forceRestart)
{
	m_isAnimPaused = false;
	m_hasHit = false;
	m_animSpeed = 1.0f;

	// ・ｽ・ｽ・ｽ・ｽ・ｽA・ｽj・ｽ・ｽ・ｽ[・ｽV・ｽ・ｽ・ｽ・ｽ・ｽ・ｽ・ｽ・ｽ・ｽﾉ再撰ｿｽ・ｽ・ｽ・ｽﾅ、・ｽ・ｽ・ｽﾂ具ｿｽ・ｽ・ｽ・ｽ・ｽ・ｽX・ｽ^・ｽ[・ｽg・ｽt・ｽ・ｽ・ｽO・ｽ・ｽ・ｽ・ｽ・ｽ・ｽ・ｽ・ｽﾎ会ｿｽ・ｽ・ｽ・ｽ・ｽ・ｽﾈゑｿｽ
	if (!forceRestart && strcmp(m_currentAnim.name, name) == 0)
	{
		return;
	}

	// ・ｽu・ｽ・ｽ・ｽ・ｽ・ｽh・ｽ・ｽ・ｽ・ｽ・ｽﾌゑｿｽ・ｽﾟに鯉ｿｽ・ｽﾝのア・ｽj・ｽ・ｽ・ｽ[・ｽV・ｽ・ｽ・ｽ・ｽ・ｽ・ｽ・ｽu・ｽO・ｽ・ｽv・ｽﾆゑｿｽ・ｽﾄ保托ｿｽ
	m_previousAnim = m_currentAnim;
	m_currentAnim.name = name;
	m_currentAnim.frame = 0;
	m_blendFactor = 0.0f;
}

// ・ｽA・ｽj・ｽ・ｽ・ｽ[・ｽV・ｽ・ｽ・ｽ・ｽ・ｽﾌ再撰ｿｽ・ｽ・ｽ・ｽx・ｽ{・ｽ・ｽ・ｽ・ｽﾝ抵ｿｽ
void Player::SetAnimationSpeed(float speed)
{
	m_animSpeed = speed;
}

// ・ｽA・ｽj・ｽ・ｽ・ｽ[・ｽV・ｽ・ｽ・ｽ・ｽ・ｽﾌ一時・ｽ・ｽ~・ｽ・ｽﾝ抵ｿｽi・ｽq・ｽb・ｽg・ｽX・ｽg・ｽb・ｽv・ｽﾈどで使・ｽp・ｽj
void Player::SetAnimPause(bool pause)
{
	m_isAnimPaused = pause;
}

// ・ｽ・ｽ・ｽﾝのア・ｽj・ｽ・ｽ・ｽ[・ｽV・ｽ・ｽ・ｽ・ｽ・ｽ・ｽ・ｽﾅ終・ｽt・ｽ・ｽ・ｽ[・ｽ・ｽ・ｽﾜで再撰ｿｽ・ｽ・ｽ・ｽ黷ｽ・ｽ・ｽ・ｽ・ｽ・ｽm・ｽF
bool Player::IsAnimEnd() const
{
	int total = m_model->GetAnimationTotalFrame(m_currentAnim.name);
	return (m_currentAnim.frame >= total - 1);
}

// ・ｽO・ｽi・ｽ・ｽ・ｽﾄゑｿｽ・ｽ驍ｩ・ｽ・ｽﾞゑｿｽ・ｽﾄゑｿｽ・ｽ驍ｩ・ｽｻ定す・ｽ驍ｽ・ｽﾟの難ｿｽ・ｽﾏゑｿｽ・ｽv・ｽZ
float Player::GetForwardMoveDot() const
{
	using namespace DirectX::SimpleMath;
	Vector3 velocity = m_velocity;
	Vector3 rotation = m_rotation;

	// ・ｽﾚ難ｿｽ・ｽ・ｽ・ｽﾄゑｿｽ・ｽﾈゑｿｽ・ｽ鼾・ｿｽ・ｽ0・ｽ・ｽﾔゑｿｽ
	if (Vector2(velocity.x, velocity.z).LengthSquared() <= 0.01f)
	{
		return 0.0f;
	}

	// ・ｽ・ｽ・ｽ・ｽ・ｽﾄゑｿｽ・ｽ・ｽ・ｽ・ｽ・ｽ・ｽﾌベ・ｽN・ｽg・ｽ・ｽ・ｽ・ｽ・ｽv・ｽZ
	Matrix rotMat = Matrix::CreateRotationY(rotation.y);
	Vector3 forward = Vector3::Transform(Vector3(0.0f, 0.0f, -1.0f), rotMat);

	Vector3 moveDir = velocity;
	moveDir.y = 0.0f;
	moveDir.Normalize();

	// ・ｽi・ｽs・ｽ・ｽ・ｽ・ｽ・ｽﾆ鯉ｿｽ・ｽ・ｽ・ｽﾄゑｿｽ・ｽ・ｽ・ｽ・ｽ・ｽ・ｽﾌ難ｿｽ・ｽﾏ（・ｽv・ｽ・ｽ・ｽX・ｽﾈゑｿｽO・ｽi・ｽA・ｽ}・ｽC・ｽi・ｽX・ｽﾈゑｿｽ・ｽﾞ）
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
	if (m_projectile) {
	}
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
		m_velocity.y = 19.0f; // ・ｽW・ｽ・ｽ・ｽ・ｽ・ｽv・ｽ・ｽ・ｽ・ｽ
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

// ・ｽ・ｽ・ｽ・ｽ・ｽ竄ｵ・ｽ痰ｪ・ｽﾝ擾ｿｽﾔゑｿｽ・ｽl・ｽ・ｽ・ｽ・ｽ・ｽﾄ、・ｽ・ｽ・ｽﾛの・ｿｽ・ｽ[・ｽ・ｽ・ｽh・ｽ・ｽ・ｽW・ｽn・ｽﾌゑｿｽ・ｽ轤｢・ｽ・ｽ・ｽ・ｽ{・ｽb・ｽN・ｽX・ｽ・ｽ・ｽ謫ｾ・ｽ・ｽ・ｽ・ｽ
DirectX::BoundingBox Player::GetHurtbox(HurtboxType type) const
{
	if (type >= HurtboxType::COUNT) return DirectX::BoundingBox();

	int idx = (int)type;
	// ・ｽ・ｽ・ｽ・ｽ・ｽﾄゑｿｽ・ｽ・ｽ・ｽ・ｽ・ｽ・ｽﾉゑｿｽ・ｽ・ｽ・ｽX・ｽ・ｽ・ｽW・ｽﾌオ・ｽt・ｽZ・ｽb・ｽg・ｽｽ転・ｽ・ｽ・ｽ・ｽ・ｽ・ｽ
	float direction = (m_rotation.y < 0.0f) ? 1.0f : -1.0f;

	float offsetX = m_isCrouching ? m_crouchHurtboxOffsets[idx].x : m_baseHurtboxOffsets[idx].x;
	float offsetY = m_isCrouching ? m_crouchHurtboxOffsets[idx].y : m_baseHurtboxOffsets[idx].y;
	float extentX = m_isCrouching ? m_crouchHurtboxExtents[idx].x : m_baseHurtboxExtents[idx].x;
	float extentY = m_isCrouching ? m_crouchHurtboxExtents[idx].y : m_baseHurtboxExtents[idx].y;

	DirectX::XMFLOAT3 center = {
		m_position.x + (offsetX * direction),
		m_position.y + offsetY,
		m_position.z
	};
	DirectX::XMFLOAT3 extents = {
		extentX, extentY, 0.1f // Z・ｽ・ｽ・ｽﾌ鯉ｿｽ・ｽﾝは固抵ｿｽ
	};

	return DirectX::BoundingBox(center, extents);
}

// ・ｽ・ｽ・ｽ轤｢・ｽ・ｽ・ｽ・ｽiHurtbox・ｽj・ｽﾌデ・ｽo・ｽb・ｽO・ｽ`・ｽ・ｽ
void Player::DrawBoundingBox()
{
	using namespace DirectX;

	if (m_isColliding) Geometory::SetColor(XMFLOAT4(1.0f, 1.0f, 0.0f, 1.0f));
	else Geometory::SetColor(XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f));

	// ・ｽ・ｽ{・ｽﾌゑｿｽ・ｽ轤｢・ｽ・ｽ・ｽ・ｽ(・ｽ・ｽ・ｽE・ｽﾌ・・ｽ・ｽ)・ｽﾍ常時・ｽL・ｽ・ｽ・ｽﾈので、・ｽ・ｽﾉゑｿｽ・ｽ・ｽ・ｽ`・ｽ謔ｷ・ｽ・ｽB
	// ・ｽZ・ｽﾅ有・ｽﾌ追会ｿｽ・ｽ・ｽ・ｽ轤｢・ｽ・ｽ・ｽ・ｽﾍ別途 m_activeHurtboxes ・ｽﾆゑｿｽ・ｽﾄ描・ｽ謔ｳ・ｽ・ｽ・ｽ
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

// ・ｽ・ｽ・ｽ・ｽv・ｽ・ｽ・ｽC・ｽ・ｽ・ｽ[・ｽﾆの会ｿｽ・ｽ・ｽ・ｽ・ｽ・ｽ・ｽ・ｽ・ｽ・ｽ・ｽi・ｽP・ｽ・ｽ・ｽ・ｽBODY・ｽ・ｽ・ｽm・ｽﾌ衝突）
bool Player::CheckCollision(const Player* other) const
{
	if (!other) return false;

	BoundingBox myBox = GetHurtbox(HurtboxType::BODY);
	BoundingBox otherBox = other->GetHurtbox(HurtboxType::BODY);
	return myBox.Intersects(otherBox);
}

void Player::DrawActiveHurtboxes()
{
	using namespace DirectX;
	// move-specific extra hurtboxes (cyan)
	Geometory::SetColor(XMFLOAT4(0.1f, 0.9f, 0.9f, 1.0f));
	static const int edge[4][2] = { {0,1},{1,2},{2,3},{3,0} };
	for (const auto& box : m_activeHurtboxes)
	{
		XMFLOAT3 corners[8];
		box.GetCorners(corners);
		for (int e = 0; e < 4; ++e) {
			Geometory::AddLine(corners[edge[e][0]], corners[edge[e][1]]);
		}
	}
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
void Player::SetJumpSpeed(float speed) { m_jumpSpeed = speed; }
float Player::GetJumpSpeed() const { return m_jumpSpeed; }
void Player::SetScale(const DirectX::XMFLOAT3& scale) { m_scale = scale; }
DirectX::XMFLOAT3 Player::GetScale() const { return m_scale; }


void Player::SetActiveHitbox(bool isActive)
{
	m_isAttacking = isActive;
	// ・ｽ・ｽA・ｽN・ｽe・ｽB・ｽu・ｽﾉなゑｿｽ・ｽ・ｽ・ｽ迪ｻ・ｽﾝの費ｿｽ・ｽ閭奇ｿｽX・ｽg・ｽ・ｽ・ｽN・ｽ・ｽ・ｽA
	if (!isActive)
	{
		m_activeHitboxes.clear();
		m_activeHurtboxes.clear();
	}
}

bool Player::IsAttacking() const
{
	return m_isAttacking;
}

// ・ｽU・ｽ・ｽ・ｽZ・ｽﾌパ・ｽ・ｽ・ｽ・ｽ・ｽ[・ｽ^・ｽ・ｽ・ｽZ・ｽb・ｽg・ｽ・ｽ・ｽA・ｽU・ｽ・ｽ・ｽt・ｽ・ｽ・ｽ[・ｽ・ｽ・ｽﾌ計・ｽ・ｽ・ｽ・ｽ・ｽJ・ｽn・ｽ・ｽ・ｽ・ｽ
void Player::SetCurrentAttackParams(AttackParams* params)
{
	m_pActiveAttackParams = params;
	for (int i = 0; i < 5; ++i) m_atkBuf[i] = 0.0f;
	m_attackTimer = 0.0f;
	m_attackElapsedSec = 0.0f;
}

AttackParams* Player::GetCurrentAttackParams() const
{
	return m_pActiveAttackParams;
}

// ・ｽU・ｽ・ｽ・ｽ・ｽ・ｽ[・ｽV・ｽ・ｽ・ｽ・ｽ・ｽ・ｽ・ｽﾌ鯉ｿｽ・ｽﾝフ・ｽ・ｽ・ｽ[・ｽ・ｽ・ｽﾉ搾ｿｽ・ｽ墲ｹ・ｽ・ｽ・ｽU・ｽ・ｽ・ｽ・ｽ・ｽ・ｽE・ｽ・ｽ・ｽ轤｢・ｽ・ｽ・ｽ・ｽﾌ更・ｽV
void Player::UpdateAttackBoxes()
{
	if (!m_pActiveAttackParams)
	{
		m_activeHitboxes.clear();
		m_activeHurtboxes.clear();
		return;
	}

	float direction = (m_rotation.y < 0.0f) ? 1.0f : -1.0f;
	m_activeHitboxes.clear();
	m_activeHurtboxes.clear();

	float currentFrame = m_currentAnim.frame;

	// ・ｽ・ｽ・ｽ・ｽ・ｽ・ｽ・ｽ邇晢ｿｽ・ｽ・ｽI・ｽ・ｽ・ｽﾜでの間ゑｿｽ・ｽﾇゑｿｽ・ｽ・ｽ・ｽ・ｽ・ｽ`・ｽF・ｽb・ｽN
	float startFrame = m_pActiveAttackParams->hitboxStart * 60.0f;
	float endFrame = m_pActiveAttackParams->hitboxEnd * 60.0f;
	// 発生判定はゲームフレーム（経過時間）で行う。アニメフレームだと再生速度ぶん前にズレるため。
	float judgeFrame = m_attackTimer;
	bool isHitActive = (judgeFrame >= startFrame && judgeFrame < endFrame);

	// ・ｽU・ｽ・ｽ・ｽ・ｽ・ｽ・ｽ (Hitbox) ・ｽﾌ撰ｿｽ・ｽ・ｽ
	if (isHitActive || m_isAttacking)
	{
		for (const auto& animBox : m_pActiveAttackParams->hitboxes)
		{
			BoxData currentBox = CalculateInterpolatedBox(animBox, currentFrame);

			DirectX::XMFLOAT3 center = {
				m_position.x + (currentBox.offset.x * direction),
				m_position.y + currentBox.offset.y,
				m_position.z
			};
			DirectX::XMFLOAT3 boxExtents = {
				currentBox.extents.x,
				currentBox.extents.y,
				0.1f
			};
			m_activeHitboxes.push_back(DirectX::BoundingBox(center, boxExtents));
		}
	}

	// ・ｽU・ｽ・ｽ・ｽ・ｽ・ｽﾌ追会ｿｽ・ｽ・ｽ・ｽ轤｢・ｽ・ｽ・ｽ・ｽ (Hurtbox) ・ｽﾌ撰ｿｽ・ｽ・ｽ
	// m_attackTimer ・ｽ・ｽ60FPS・ｽ譓・ｽﾌゲ・ｽ[・ｽ・ｽ・ｽt・ｽ・ｽ・ｽ[・ｽ・ｽ・ｽB・ｽ・ｽﾔ難ｿｽ・ｽﾌゑｿｽ・ｽﾌゑｿｽ・ｽ・ｽ・ｽ・ｽ・ｽﾅ抵ｿｽT・ｽC・ｽY・ｽﾅ撰ｿｽ・ｽ・ｽ・ｽ・ｽ・ｽ・ｽ
	float gameFrame = m_attackTimer;
	for (const auto& wh : m_pActiveAttackParams->moveHurtboxes)
	{
		if (gameFrame < (float)wh.startFrame || gameFrame >= (float)wh.endFrame) continue;

		DirectX::XMFLOAT3 center = {
			m_position.x + (wh.offset.x * direction),
			m_position.y + wh.offset.y,
			m_position.z
		};
		DirectX::XMFLOAT3 boxExtents = {
			wh.extents.x,
			wh.extents.y,
			0.1f
		};
		m_activeHurtboxes.push_back(DirectX::BoundingBox(center, boxExtents));
	}
}

const std::vector<DirectX::BoundingBox>& Player::GetActiveHitboxes() const
{
	return m_activeHitboxes;
}

const std::vector<DirectX::BoundingBox>& Player::GetActiveHurtboxes() const
{
	return m_activeHurtboxes;
}

// ・ｽU・ｽ・ｽ・ｽ・ｽ・ｽ・ｽ (Hitbox) ・ｽﾌデ・ｽo・ｽb・ｽO・ｽ`・ｽ・ｽ
void Player::DrawHitbox()
{
	if (!m_isAttacking) return;

	using namespace DirectX;
	// ・ｽU・ｽ・ｽ・ｽ・ｽ・ｽ・ｽﾍ赤色・ｽﾅ描・ｽ・ｽ
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

// ・ｽ_・ｽ・ｽ・ｽ[・ｽW・ｽ・ｽ・ｽｯる処・ｽ・ｽ・ｽB・ｽK・ｽ[・ｽh・ｽ・ｽ・ｽ・ｽ・ｽ・ｽ・ｽ・ｽ・ｽﾄゑｿｽ・ｽ・ｽﾎ搾ｿｽ・ｽ_・ｽ・ｽ・ｽ[・ｽW・ｽi10・ｽ・ｽ・ｽ・ｽ1・ｽj・ｽﾉなゑｿｽ
void Player::ReceiveDamage(int damage, AttackLevel atkLevel)
{
	if (TryGuard(atkLevel))
	{
		damage /= 10;
	}

	m_hp -= damage;
	if (m_hp <= 0)
	{
		m_hp = 0;
		// HP・ｽ・ｽ0・ｽﾉなゑｿｽ・ｽ・ｽ・ｽ邇・ｽS・ｽX・ｽe・ｽ[・ｽg・ｽﾖ遷・ｽ・ｽ
		SetState(new PlayerStateDeath());
	}
}

float Player::GetHpRatio() const
{
	return (float)m_hp / (float)m_maxHp;
}

// ・ｽ・ｽ・ｽE・ｽ・ｽ・ｽh・ｽJ・ｽn・ｽ・ｽ・ｽﾈどにプ・ｽ・ｽ・ｽC・ｽ・ｽ・ｽ[・ｽﾌ擾ｿｽﾔゑｿｽ・ｽ・ｽ・ｽZ・ｽb・ｽg
void Player::Reset()
{
	m_hp = m_maxHp;
	m_isJumping = false;
	m_isAttacking = false;
	m_hasHit = false;
	m_velocity = { 0.0f, 0.0f, 0.0f };
	m_activeHitboxes.clear();
	m_activeHurtboxes.clear();
	if (m_projectile) m_projectile->Deactivate();
	SetState(new PlayerStateIdle());
}

bool Player::CanFireProjectile() const
{
	if (!m_projectile) return false;
	return !m_projectile->IsActive();
}

// ・ｽR・ｽ}・ｽ・ｽ・ｽh・ｽ・ｽ・ｽﾍのタ・ｽC・ｽ}・ｽ[・ｽ・ｽ・ｽﾇ暦ｿｽ・ｽi・ｽg・ｽ・ｽ・ｽ・ｽ・ｽR・ｽ}・ｽ・ｽ・ｽh・ｽﾈどの趣ｿｽt・ｽ・ｽ・ｽﾔ擾ｿｽ・ｽ・ｽ・ｽj
void Player::UpdateCommandTimer(float tick)
{
	if (m_cmdTimerDown > 0) m_cmdTimerDown -= tick;
	if (m_cmdTimerDownForward > 0) m_cmdTimerDownForward -= tick;

	const PlayerInputs& in = GetInputs();

	// ・ｽ・ｽ・ｽ・ｽ・ｽﾄゑｿｽ・ｽ・ｽ・ｽ・ｽ・ｽ・ｽﾉゑｿｽ・ｽ・ｽﾄ前・ｽ・ｽ・ｽﾍゑｿｽ・ｽ・ｽ・ｽ・ｽ・ｽﾍゑｿｽ・ｽｻ抵ｿｽ
	bool forward = (m_rotation.y < 0.0f) ? in.moveRight : in.moveLeft;

	// ・ｽ・ｽ・ｽ・ｽ・ｽﾍゑｿｽ・ｽ・ｽ・ｽ黷ｽ・ｽ・ｽ^・ｽC・ｽ}・ｽ[・ｽ・ｽ・ｽX・ｽ^・ｽ[・ｽg
	if (in.moveDown && !forward) {
		m_cmdTimerDown = CMD_WINDOW;
	}
	// ・ｽ・ｽ・ｽ・ｽ・ｽﾍゑｿｽ・ｽﾂ前・ｽ・ｽ・ｽﾍ（・ｽﾎめ前・ｽj・ｽ・ｽ・ｽ・ｽ・ｽ黷ｽ・ｽ鼾・
	else if (in.moveDown && forward) {
		if (m_cmdTimerDown > 0) {
			m_cmdTimerDownForward = CMD_WINDOW;
		}
	}
}

// ・ｽg・ｽ・ｽ・ｽ・ｽ・ｽﾌコ・ｽ}・ｽ・ｽ・ｽh・ｽi・ｽ・ｽ -> ・ｽﾎめ前 -> ・ｽO・ｽ{・ｽp・ｽ・ｽ・ｽ`・ｽj・ｽ・ｽ・ｽ・ｽ・ｽ・ｽ・ｽ・ｽ・ｽﾄゑｿｽ・ｽ驍ｩ・ｽ`・ｽF・ｽb・ｽN
bool Player::CheckHadoukenCommand() const
{
	const PlayerInputs& in = GetInputs();
	bool forward = (m_rotation.y < 0.0f) ? in.moveRight : in.moveLeft;

	return (m_cmdTimerDownForward > 0 && forward && (in.LightPunch || in.MediumPunch || in.HeavyPunch));
}

// ・ｽ・ｽ・ｽﾝガ・ｽ[・ｽh・ｽﾂ能・ｽﾈ擾ｿｽﾔゑｿｽ・ｽA・ｽ・ｽ・ｽ・ｽ・ｽﾍゑｿｽ・ｽ・ｽ・ｽ・ｽﾄゑｿｽ・ｽ驍ｩ・ｽｻ抵ｿｽ
bool Player::IsGuarding() const
{
	// ・ｽW・ｽ・ｽ・ｽ・ｽ・ｽv・ｽ・ｽ・ｽA・ｽU・ｽ・ｽ・ｽ・ｽ・ｽﾈどはガ・ｽ[・ｽh・ｽs・ｽ・ｽ
	if (m_isJumping || m_isAttacking || m_pActiveAttackParams != nullptr) return false;

	// ・ｽ_・ｽ・ｽ・ｽ[・ｽW・ｽ・ｽ_・ｽE・ｽ・ｽ・ｽ・ｽ・ｽ・ｽ・ｽK・ｽ[・ｽh・ｽs・ｽ・ｽ
	if (strcmp(m_currentAnim.name, "Damage") == 0 ||
		strcmp(m_currentAnim.name, "Down") == 0 ||
		strcmp(m_currentAnim.name, "WakeUp") == 0 ||
		strcmp(m_currentAnim.name, "Death") == 0)
	{
		return false;
	}

	const PlayerInputs& in = GetInputs();
	bool isFacingRight = (m_rotation.y < 0.0f);
	// ・ｽ・ｽ・ｽ・ｽﾌ費ｿｽ・ｽﾎ包ｿｽ・ｽ・ｽ・ｽi・ｽ・ｽ・ｽj・ｽL・ｽ[・ｽ・ｽ・ｽ・ｽ・ｽ・ｽ・ｽ・ｽﾄゑｿｽ・ｽ驍ｩ
	bool isPressingBack = isFacingRight ? in.moveLeft : in.moveRight;

	return isPressingBack;
}

// ・ｽ・ｽ・ｽ・ｽ・ｽK・ｽ[・ｽh・ｽi・ｽ・ｽi・ｽK・ｽ[・ｽh・ｽj・ｽ・ｽ・ｽ・ｽ・ｽ・ｽ・ｽ・ｽ・ｽﾄゑｿｽ・ｽ驍ｩ・ｽ・ｽ・ｽ・ｽ
bool Player::IsGuardingHigh() const
{
	if (!IsGuarding()) return false;
	const PlayerInputs& in = GetInputs();
	// ・ｽ・ｽ・ｽ・ｽ・ｽﾍゑｿｽ・ｽﾂ会ｿｽ・ｽﾉ難ｿｽ・ｽﾍゑｿｽ・ｽﾄゑｿｽ・ｽﾈゑｿｽ・ｽ鼾・
	return !in.moveDown;
}

// ・ｽ・ｽ・ｽ痰ｪ・ｽﾝガ・ｽ[・ｽh・ｽi・ｽ・ｽ・ｽi・ｽK・ｽ[・ｽh・ｽj・ｽ・ｽ・ｽ・ｽ・ｽ・ｽ・ｽ・ｽ・ｽﾄゑｿｽ・ｽ驍ｩ・ｽ・ｽ・ｽ・ｽ
bool Player::IsGuardingLow() const
{
	if (!IsGuarding()) return false;
	const PlayerInputs& in = GetInputs();
	// ・ｽ・ｽ・ｽ・ｽ・ｽﾍゑｿｽ・ｽﾂ会ｿｽ・ｽﾉゑｿｽ・ｽ・ｽ・ｽﾍゑｿｽ・ｽﾄゑｿｽ・ｽ・ｽ鼾・
	return in.moveDown;
}

// ・ｽ・ｽ・ｽ・ｽﾌ攻・ｽ・ｽ・ｽ・ｽ・ｽ閭鯉ｿｽx・ｽ・ｽ・ｽﾉ対ゑｿｽ・ｽﾄガ・ｽ[・ｽh・ｽ・ｽ・ｽ・ｽ・ｽ・ｽ・ｽ・ｽ・ｽ驍ｩ・ｽ・ｽ・ｽ・ｽ・ｽ・ｽ
bool Player::TryGuard(AttackLevel atkLevel) const
{
	if (!IsGuarding()) return false;

	if (m_guardAllLevels) return true;
	if (atkLevel == AttackLevel::MID)
	{
		// ・ｽ・ｽ・ｽi・ｽU・ｽ・ｽ・ｽﾍ暦ｿｽ・ｽ・ｽ・ｽK・ｽ[・ｽh・ｽﾌみ可能
		return IsGuardingHigh();
	}
	else if (atkLevel == AttackLevel::LOW)
	{
		// ・ｽ・ｽ・ｽi・ｽU・ｽ・ｽ・ｽﾍゑｿｽ・ｽ痰ｪ・ｽﾝガ・ｽ[・ｽh・ｽﾌみ可能
		return IsGuardingLow();
	}

	// ・ｽ・ｽi・ｽU・ｽ・ｽ・ｽﾍ暦ｿｽ・ｽ・ｽ・ｽK・ｽ[・ｽh・ｽﾅゑｿｽ・ｽ・ｽ・ｽ痰ｪ・ｽﾝガ・ｽ[・ｽh・ｽﾅゑｿｽ・ｽh・ｽ・ｽ・ｽ・ｽ
	return true;
}