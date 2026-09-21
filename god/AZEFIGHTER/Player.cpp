#include "Player.h"
#include "Input.h"
#include "Sound.h"
#include <DirectXCollision.h>
#include "Geometory.h" 
#include <fstream>
#include "Shader.h"
#include "Model.h"
#include <Xinput.h>

// ステートパターン用
#include "PlayerState.h" 
#include "PlayerStateIdle.h" 
#include "PlayerStateJump.h" 
#include "PlayerStateDamage.h"
#include "PlayerStateCrouch.h"
#include "Hadouken.h"
#include "PlayerStateDeath.h"

#include "Projectile.h"

#include <DirectXMath.h>
#include <algorithm>

KeyConfig g_keyConfigP1 = { 'W', 'S', 'A', 'D', 'U', 'I', 'O', 'J', 'K', 'L' };
KeyConfig g_keyConfigP2 = { VK_UP, VK_DOWN, VK_LEFT, VK_RIGHT, VK_NUMPAD1, VK_NUMPAD2, VK_NUMPAD4, VK_NUMPAD6, VK_NUMPAD5, VK_NUMPAD3 };

PadConfig g_padConfigP1 = { XINPUT_GAMEPAD_DPAD_UP, XINPUT_GAMEPAD_DPAD_DOWN, XINPUT_GAMEPAD_DPAD_LEFT, XINPUT_GAMEPAD_DPAD_RIGHT, XINPUT_GAMEPAD_X, XINPUT_GAMEPAD_Y, XINPUT_GAMEPAD_RIGHT_SHOULDER, XINPUT_GAMEPAD_LEFT_SHOULDER, XINPUT_GAMEPAD_A, XINPUT_GAMEPAD_B };
PadConfig g_padConfigP2 = { XINPUT_GAMEPAD_DPAD_UP, XINPUT_GAMEPAD_DPAD_DOWN, XINPUT_GAMEPAD_DPAD_LEFT, XINPUT_GAMEPAD_DPAD_RIGHT, XINPUT_GAMEPAD_X, XINPUT_GAMEPAD_Y, XINPUT_GAMEPAD_RIGHT_SHOULDER, XINPUT_GAMEPAD_LEFT_SHOULDER, XINPUT_GAMEPAD_A, XINPUT_GAMEPAD_B };

InputDeviceType g_inputDeviceP1 = InputDeviceType::KEYBOARD;
InputDeviceType g_inputDeviceP2 = InputDeviceType::PAD_0;
bool Player::s_variableSpeed = true;

namespace
{
	// 攻撃パラメータに従って再生しているアニメーションか。
	// ここから外れたら（のけぞり等に切り替わったら）攻撃中の情報を消す
	bool IsAttackAnimation(const char* name)
	{
		for (int i = 0; i < NORMAL_MOVE_COUNT; ++i)
		{
			if (strcmp(name, GetNormalMoveInfo((NormalMove)i).animName) == 0) return true;
		}
		const char* others[] = { "Hadouken", "Shoryuken", "Down", "WakeUp" };
		for (const char* other : others)
		{
			if (strcmp(name, other) == 0) return true;
		}
		return false;
	}
}

namespace {
	// アニメーション中の指定フレームにおける当たり判定の位置とサイズを計算するヘルパー関数
	BoxData CalculateInterpolatedBox(const AnimatedBox& animBox, float currentFrame)
	{
		// キーフレームが存在しない場合は初期値を返す
		if (animBox.keyframes.empty())
		{
			return BoxData();
		}

		// キーフレームが1つだけなら、常にその値を使う
		if (animBox.keyframes.size() == 1)
		{
			return animBox.keyframes[0].data;
		}

		// 現在フレームが最初のキーフレームより前なら、最初の値を使う
		if (currentFrame <= animBox.keyframes.front().frame)
		{
			return animBox.keyframes.front().data;
		}
		// 現在フレームが最後のキーフレームより後なら、最後の値を使う
		if (currentFrame >= animBox.keyframes.back().frame)
		{
			return animBox.keyframes.back().data;
		}

		// キーフレーム間の補間処理
		for (size_t i = 0; i < animBox.keyframes.size() - 1; ++i)
		{
			const auto& k1 = animBox.keyframes[i];
			const auto& k2 = animBox.keyframes[i + 1];

			// 現在のフレームがどのキーフレーム間に存在するかチェック
			if (currentFrame >= k1.frame && currentFrame < k2.frame)
			{
				float range = k2.frame - k1.frame;
				float t = 0.0f;
				// ゼロ除算防止
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

				// 位置とサイズを線形補間 (Lerp)
				XMVECTOR vOff = XMVectorLerp(vOff1, vOff2, t);
				XMVECTOR vExt = XMVectorLerp(vExt1, vExt2, t);

				XMStoreFloat2(&result.offset, vOff);
				XMStoreFloat2(&result.extents, vExt);

				return result;
			}
		}

		// 安全のためのフォールバック
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
	, m_projectile(std::make_unique<Projectile>())
	, m_attackTimer(0.0f)
{
	// 初期アニメーションとステートの設定
	m_currentAnim = { "Idle", 0 };
	m_previousAnim = { "Idle", 0 };
	SetState(std::make_unique<PlayerStateIdle>());

	InitDefaultParameters();
}

Player::~Player()
{
	// ステートと飛び道具は unique_ptr が解放する
}

// プレイヤーの移動速度や、各種攻撃技の初期パラメータを設定する
void Player::InitDefaultParameters()
{
	m_moveSpeed = 2.102f;
	m_jumpSpeed = 2.5f;
	m_scale = { 1.0f, 1.0f, 1.0f };

	// 立ち状態の基本くらい判定 (Head, Body, Legs)
	m_baseHurtboxExtents[(int)HurtboxType::HEAD] = { 0.33f, 0.33f };
	m_baseHurtboxOffsets[(int)HurtboxType::HEAD] = { 0.024f, 1.923f };
	m_baseHurtboxExtents[(int)HurtboxType::BODY] = { 0.598f, 0.522f };
	m_baseHurtboxOffsets[(int)HurtboxType::BODY] = { 0.011f, 1.198f };
	m_baseHurtboxExtents[(int)HurtboxType::LEGS] = { 0.482f, 0.497f };
	m_baseHurtboxOffsets[(int)HurtboxType::LEGS] = { -0.007f, 0.159f };

	// しゃがみ状態の基本くらい判定
	m_crouchHurtboxExtents[(int)HurtboxType::HEAD] = { 0.281f, 0.176f };
	m_crouchHurtboxOffsets[(int)HurtboxType::HEAD] = { 0.272f, 1.425f };
	m_crouchHurtboxExtents[(int)HurtboxType::BODY] = { 0.6f, 0.401f };
	m_crouchHurtboxOffsets[(int)HurtboxType::BODY] = { 0.112f, 0.846f };
	m_crouchHurtboxExtents[(int)HurtboxType::LEGS] = { 0.544f, 0.364f };
	m_crouchHurtboxOffsets[(int)HurtboxType::LEGS] = { 0.116f, 0.308f };


	// 追加くらい判定の初期化ヘルパー。基本のくらい判定(頭・体・足)は常時有効なので、
	// 技固有の追加くらい判定はデフォルトでは無し(空)にしておく
	auto InitDefaultHurtboxes = [&](std::vector<WindowedHurtbox>& boxes) {
		boxes.clear();
		};


	// --- 弱パンチの設定 ---
	{
		AttackParams& p = GetNormalParams(NormalMove::LP);
		p.totalDuration = 0.166667f;
		p.hitboxStart = 0.0666667f; // 発生
		p.hitboxEnd = 0.15f;        // 持続終了

		// 攻撃判定（Hitbox）の作成
		p.hitboxes.clear();
		AnimatedBox ab;
		ab.keyframes.push_back({ 0.0f, { {0.76f, 1.7f}, {0.454f, 0.157f} } });
		p.hitboxes.push_back(ab);

		p.damage = 100; p.hitFrame = 2; p.blockFrame = -2; p.hitStop = 0.0333333f; p.knockback = 0.223f;
		p.isDown = false;
		p.attackLevel = AttackLevel::HIGH; // 上段判定

		InitDefaultHurtboxes(p.moveHurtboxes);

		// キャンセル可能タイミングの設定
		p.cancelEnabled = true; p.cancelStart = 0.0666667f; p.cancelEnd = 0.166667f;
		p.cancelToLight = true; p.cancelToMedium = true; p.cancelToHeavyPunch = false; p.cancelToMediumKick = false; p.cancelToHeavy = false;
		p.speedModifiers.clear();
	}

	// --- 中パンチの設定 ---
	{
		AttackParams& p = GetNormalParams(NormalMove::MP);
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

		// 発生や硬直をごまかすためのアニメーション再生速度調整
		p.speedModifiers.clear();
		p.speedModifiers.push_back({ 0.0f, 6.0f, 0.369f });
		p.speedModifiers.push_back({ 7.0f, 12.0f, 2.533f });
	}

	// --- 強パンチの設定 ---
	{
		AttackParams& p = GetNormalParams(NormalMove::HP);
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

	// --- 中キックの設定 ---
	{
		AttackParams& p = GetNormalParams(NormalMove::MK);
		p.totalDuration = 0.416667f;
		p.hitboxStart = 0.1f;
		p.hitboxEnd = 0.416667f;
		p.hitboxes.clear();
		AnimatedBox ab;
		ab.keyframes.push_back({ 0.0f, { {1.5f, 1.56f}, {0.351f, 0.24f} } });
		p.hitboxes.push_back(ab);

		p.damage = 400; p.hitFrame = 5; p.blockFrame = -2; p.hitStop = 0.05f; p.knockback = 0.0f;
		p.isDown = false;
		p.attackLevel = AttackLevel::MID; // 中段判定（しゃがみガード不可）

		InitDefaultHurtboxes(p.moveHurtboxes);

		p.cancelEnabled = false;

		p.speedModifiers.clear();
		p.speedModifiers.push_back({ 0.0f, 4.0f, 0.362f });
		p.speedModifiers.push_back({ 5.0f, 9.0f, 0.503f });
		p.speedModifiers.push_back({ 10.0f, 20.0f, 0.295f });
	}

	// --- 強キックの設定 ---
	{
		AttackParams& p = GetNormalParams(NormalMove::HK);
		p.totalDuration = 0.5f;
		p.hitboxStart = 0.133333f;
		p.hitboxEnd = 0.5f;
		p.hitboxes.clear();
		AnimatedBox ab;
		ab.keyframes.push_back({ 0.0f, { {1.324f, 1.759f}, {0.551f, 0.23f} } });
		p.hitboxes.push_back(ab);

		p.damage = 700; p.hitFrame = 5; p.blockFrame = -2; p.hitStop = 0.0666667f; p.knockback = 0.0f;
		p.isDown = false;
		p.attackLevel = AttackLevel::LOW; // 下段判定（立ちガード不可）

		InitDefaultHurtboxes(p.moveHurtboxes);

		p.cancelEnabled = false;

		p.speedModifiers.clear();
		p.speedModifiers.push_back({ 0.0f, 6.0f, 0.463f });
		p.speedModifiers.push_back({ 7.0f, 16.0f, 1.006f });
		p.speedModifiers.push_back(
			{ 17.0f, 30.0f, 0.345f });
	}

	// --- 立ち弱K・しゃがみ攻撃の設定 ---
	// ini が読めなかったときの初期値。フレームは 60FPS 換算。判定の位置は Maya で測った手足の到達点に合わせている
	{
		struct NormalDefault
		{
			NormalMove move;
			int startup, activeEnd, total;
			DirectX::XMFLOAT2 offset, extents;
			int damage, hitFrame, blockFrame;
			float hitStop, knockback;
			bool isDown;
			AttackLevel level;
			bool cancelTo[ATTACK_BUTTON_COUNT]; // LP, MP, HP, LK, MK, HK
			bool legHurtbox;                    // 伸ばした足にも、くらい判定を出す
		};
		const NormalDefault defaults[] =
		{
			// 技                     発生 持続 全体  判定の中心        判定の半径       ダメ  有利 ガード  ヒットストップ ノックバック ダウン  段                 キャンセル先          足のくらい
			{ NormalMove::LK,        4,  7, 14, { 0.80f, 0.42f }, { 0.30f, 0.18f }, 200, 2,  -1, 0.0333f, 0.2f, false, AttackLevel::HIGH, { 1, 1, 0, 1, 0, 0 }, false },
			{ NormalMove::CROUCH_LP, 4,  6, 12, { 0.95f, 1.24f }, { 0.30f, 0.14f }, 100, 2,  -1, 0.0333f, 0.2f, false, AttackLevel::HIGH, { 1, 1, 0, 1, 0, 0 }, false },
			{ NormalMove::CROUCH_MP, 5,  8, 18, { 1.07f, 1.30f }, { 0.33f, 0.14f }, 400, 4,  -2, 0.05f,   0.2f, false, AttackLevel::HIGH, { 0, 0, 1, 0, 0, 1 }, false },
			{ NormalMove::CROUCH_HP, 6, 12, 32, { 0.50f, 1.80f }, { 0.35f, 0.55f }, 700, 3,  -8, 0.0667f, 0.3f, false, AttackLevel::HIGH, { 0, 0, 0, 0, 0, 0 }, false },
			{ NormalMove::CROUCH_LK, 4,  6, 13, { 1.05f, 0.20f }, { 0.30f, 0.15f }, 150, 2,  -1, 0.0333f, 0.2f, false, AttackLevel::LOW,  { 1, 0, 0, 1, 0, 0 }, false },
			{ NormalMove::CROUCH_MK, 7, 10, 24, { 1.22f, 0.20f }, { 0.33f, 0.15f }, 400, 3,  -4, 0.05f,   0.3f, false, AttackLevel::LOW,  { 0, 0, 0, 0, 0, 0 }, true  },
			{ NormalMove::CROUCH_HK, 8, 11, 34, { 1.15f, 0.18f }, { 0.35f, 0.16f }, 800, 0, -14, 0.0667f, 0.5f, true,  AttackLevel::LOW,  { 0, 0, 0, 0, 0, 0 }, true  },
		};
		for (const NormalDefault& d : defaults)
		{
			AttackParams& p = GetNormalParams(d.move);
			p.totalDuration = d.total / 60.0f;
			p.hitboxStart = d.startup / 60.0f;
			p.hitboxEnd = d.activeEnd / 60.0f;

			p.hitboxes.clear();
			AnimatedBox ab;
			ab.keyframes.push_back({ 0.0f, { d.offset, d.extents } });
			p.hitboxes.push_back(ab);

			InitDefaultHurtboxes(p.moveHurtboxes);
			if (d.legHurtbox)
			{
				// 足を伸ばしている間は足先も打たれる。空振りを見て差し返せるようにするため
				p.moveHurtboxes.push_back({ { d.offset.x - 0.1f, d.offset.y }, { d.extents.x, d.extents.y + 0.05f }, d.startup, d.activeEnd + 6 });
			}

			p.damage = d.damage; p.hitFrame = d.hitFrame; p.blockFrame = d.blockFrame;
			p.hitStop = d.hitStop; p.knockback = d.knockback; p.isDown = d.isDown;
			p.attackLevel = d.level;

			p.cancelToLight = d.cancelTo[0]; p.cancelToMedium = d.cancelTo[1]; p.cancelToHeavyPunch = d.cancelTo[2];
			p.cancelToLightKick = d.cancelTo[3]; p.cancelToMediumKick = d.cancelTo[4]; p.cancelToHeavy = d.cancelTo[5];
			p.cancelEnabled = false;
			for (bool c : d.cancelTo) p.cancelEnabled = p.cancelEnabled || c;
			p.cancelStart = p.hitboxStart;
			p.cancelEnd = p.totalDuration;
			p.speedModifiers.clear();
		}

		// しゃがみ強Pと足払いは踏み込みながら出す。Maya 側では、この前進の分だけ軸足を引いてある
		GetNormalParams(NormalMove::CROUCH_HP).forwardSpeed = 3.0f;
		GetNormalParams(NormalMove::CROUCH_HK).forwardSpeed = 2.5f;
	}

	// --- 波動拳 (弱/中/強) の設定 ---
	{
		InitDefaultHurtboxes(m_hadoukenLParams.moveHurtboxes);
		m_hadoukenLParams.totalDuration = 0.6f;
		m_hadoukenLParams.projectileSpeed = 4.0f; // 弾速
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

	// --- 昇竜拳 (弱/中/強) の設定 ---
	// 強いほど高く飛び、無敵が長く、外したときの隙も大きい
	{
		AttackParams* shoryuken[3] = { &m_shoryukenLParams, &m_shoryukenMParams, &m_shoryukenHParams };
		const int totalFrames[3] = { 38, 42, 48 };
		const int activeEndFrames[3] = { 10, 12, 14 };
		const int invincibleFrames[3] = { 4, 6, 8 };
		const int damages[3] = { 800, 1000, 1200 };
		for (int i = 0; i < 3; ++i)
		{
			AttackParams& p = *shoryuken[i];
			p.totalDuration = totalFrames[i] / 60.0f;
			p.hitboxStart = 3.0f / 60.0f;
			p.hitboxEnd = activeEndFrames[i] / 60.0f;
			p.invincibleEnd = invincibleFrames[i] / 60.0f;
			p.forwardSpeed = 3.0f + 1.0f * i;

			p.hitboxes.clear();
			AnimatedBox ab;
			ab.keyframes.push_back({ 0.0f, { {0.55f, 2.0f}, {0.45f, 0.75f} } });
			p.hitboxes.push_back(ab);
			InitDefaultHurtboxes(p.moveHurtboxes);

			p.damage = damages[i]; p.hitFrame = 5; p.blockFrame = -30; p.hitStop = 0.1f; p.knockback = 0.5f;
			p.isDown = true;
			p.attackLevel = AttackLevel::HIGH;
			p.cancelEnabled = false;
			p.speedModifiers.clear();
		}
	}
}

// 毎フレーム呼ばれる更新処理の大枠
void Player::Update(float tick)
{
	PollInputs();
	UpdateCommandTimer(tick);
	UpdateInputBuffer(tick);

	if (m_currentState) {
		m_isCrouching = m_currentState->IsCrouch();
		m_currentState->Update(this, tick);
		if (m_pActiveAttackParams) m_attackElapsedSec += tick;
	}

	UpdatePhysics(tick);

	UpdateAnimation(tick);

	UpdateModelBlend();

	UpdateAttackBoxes();

	if (m_projectile) {
		m_projectile->Update(tick);
	}
}

// キーボードやコントローラーの入力を取得し、PlayerInputs構造体にマッピングする
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
			if (IsKeyTrigger(g_keyConfigP1.lightKick)) m_inputs.LightKick = true;
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
			if (IsPadTrigger(padNo, g_padConfigP1.lightKick)) m_inputs.LightKick = true;
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
			if (IsKeyTrigger(g_keyConfigP2.lightKick)) m_inputs.LightKick = true;
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
			if (IsPadTrigger(padNo, g_padConfigP2.lightKick)) m_inputs.LightKick = true;
			if (IsPadTrigger(padNo, g_padConfigP2.mediumKick)) m_inputs.MediumKick = true;
			if (IsPadTrigger(padNo, g_padConfigP2.heavyKick)) m_inputs.HeavyKick = true;
		}
		break;

	case PlayerInputType::AI:
	case PlayerInputType::CPU:
		// 外で組み立てた入力をそのまま使う。このあと入力バッファやコマンド判定を人間と同じように通る
		m_inputs = m_injectedInputs;
		break;
	}
}


// 重力の適用と座標の移動処理
void Player::SetInjectedInputs(const PlayerInputs& in)
{
	m_injectedInputs = in;
}

void Player::UpdateInputBuffer(float tick)
{
	const float BUF = 0.1f; // about 6 frames of attack input buffer
	for (int i = 0; i < ATTACK_BUTTON_COUNT; ++i)
	{
		const AttackButton button = (AttackButton)i;
		if (IsButtonPressed(m_inputs, button)) m_atkBuf[i] = BUF;
		else { m_atkBuf[i] -= tick; if (m_atkBuf[i] < 0.0f) m_atkBuf[i] = 0.0f; }
		SetButtonPressed(m_inputs, button, m_atkBuf[i] > 0.0f);
	}
}

void Player::UpdatePhysics(float tick)
{
	if (m_isJumping) {
		m_velocity.y -= GRAVITY * tick;
	}

	m_position.x += m_velocity.x * tick;
	m_position.y += m_velocity.y * tick;
	m_position.z += m_velocity.z * tick;

	// 地面との当たり判定（Y座標0を床とする）
	if (m_position.y <= 0.0f) {
		m_position.y = 0.0f;
		if (m_isJumping) {
			m_velocity.y = 0.0f;
			m_velocity.x = 0.0f;
			m_isJumping = false;
			Sound::PlaySE("Assets/Sound/SE/Game/ジャンプの着地.mp3");
		}
	}
}

// アニメーションのフレーム進行と再生速度の動的変更を行う
void Player::UpdateAnimation(float tick)
{
	// モーション遷移時のブレンド係数を更新
	if (m_blendFactor < 1.0f)
	{
		m_blendFactor += tick / m_transitionDuration;
		if (m_blendFactor > 1.0f) m_blendFactor = 1.0f;
	}

	if (!m_isAnimPaused)
	{
		float currentSpeed = m_animSpeed;

		// 攻撃モーション中の場合、指定されたフレーム区間で再生速度を変動させる
		if (m_pActiveAttackParams)
		{
			bool isControlledAnim = IsAttackAnimation(m_currentAnim.name);

			if (!isControlledAnim)
			{
				m_pActiveAttackParams = nullptr;
			}
			else if (s_variableSpeed)
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

		// フレームを進める（60FPS基準）
		m_currentAnim.frame += currentSpeed * tick * 60.0f;

		// 攻撃判定生成用のタイマーをフレームと同期
		if (m_pActiveAttackParams)
		{
			// 攻撃判定は実経過時間で測る。アニメ再生速度でズレないようにするため
			m_attackTimer = m_attackElapsedSec * 60.0f;
		}
	}
}

// 変更前のアニメーションと変更後のアニメーションを滑らかに補間する
void Player::UpdateModelBlend()
{
	m_model->UpdateWithBlend(
		m_currentAnim.name, (int)m_currentAnim.frame,
		m_previousAnim.name, (int)m_previousAnim.frame,
		m_blendFactor);
}

// プレイヤーのステート（待機、歩行、ジャンプ、攻撃など）を切り替える
void Player::SetState(std::unique_ptr<PlayerState> newState)
{
	if (newState == nullptr) return;

	// 体力が0以下なら死亡ステート以外への遷移を拒否
	if (m_hp <= 0 && !newState->IsDeathState()) return;

	// すでに死亡状態なら、他の状態による上書きをブロック
	if (m_currentState && m_currentState->IsDeathState() && m_hp <= 0) return;

	m_currentState = std::move(newState);
	m_currentState->OnEnter(this);
}

// 現在のステートが無敵かどうかを判定
bool Player::IsInvincible() const
{
	if (m_currentState)
	{
		return m_currentState->IsInvincible();
	}
	return false;
}

bool Player::IsInHitstun() const
{
	return m_currentState && m_currentState->IsHitstun();
}

bool Player::IsActionable() const
{
	return m_currentState && m_currentState->IsInterruptible() && !m_isJumping && m_hp > 0;
}

// 指定した名前のアニメーションを再生する
void Player::PlayAnimation(const char* name, bool forceRestart)
{
	m_isAnimPaused = false;
	m_hasHit = false;
	m_animSpeed = 1.0f;

	// 同じアニメーションが既に再生中で、かつ強制リスタートフラグが無ければ何もしない
	if (!forceRestart && strcmp(m_currentAnim.name, name) == 0)
	{
		return;
	}

	// ブレンド処理のために現在のアニメーションを「前回」として保存
	m_previousAnim = m_currentAnim;
	m_currentAnim.name = name;
	m_currentAnim.frame = 0;
	m_blendFactor = 0.0f;
}

// アニメーションの再生速度倍率を設定
void Player::SetAnimationSpeed(float speed)
{
	m_animSpeed = speed;
}

// アニメーションの一時停止を設定（ヒットストップなどで使用）
void Player::SetAnimPause(bool pause)
{
	m_isAnimPaused = pause;
}

// 現在のアニメーションが最終フレームまで再生されたかを確認
bool Player::IsAnimEnd() const
{
	int total = m_model->GetAnimationTotalFrame(m_currentAnim.name);
	return (m_currentAnim.frame >= total - 1);
}

// 前進しているか後退しているかを判定するための内積を計算
float Player::GetForwardMoveDot() const
{
	using namespace DirectX::SimpleMath;
	Vector3 velocity = m_velocity;
	Vector3 rotation = m_rotation;

	// 移動していない場合は0を返す
	if (Vector2(velocity.x, velocity.z).LengthSquared() <= 0.01f)
	{
		return 0.0f;
	}

	// 向いている方向のベクトルを計算
	Matrix rotMat = Matrix::CreateRotationY(rotation.y);
	Vector3 forward = Vector3::Transform(Vector3(0.0f, 0.0f, -1.0f), rotMat);

	Vector3 moveDir = velocity;
	moveDir.y = 0.0f;
	moveDir.Normalize();

	// 進行方向と向いている方向の内積（プラスなら前進、マイナスなら後退）
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
		m_velocity.y = JUMP_VELOCITY;
		m_isJumping = true;
		Sound::PlaySE("Assets/Sound/SE/Game/ジャンプ.mp3");
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

// 向きやしゃがみ状態を考慮して、実際のワールド座標系のくらい判定ボックスを取得する
DirectX::BoundingBox Player::GetHurtbox(HurtboxType type) const
{
	if (type >= HurtboxType::COUNT) return DirectX::BoundingBox();

	int idx = (int)type;
	// 向いている方向によってX座標のオフセットを反転させる
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
		extentX, extentY, 0.1f // Z軸の厚みは固定
	};

	return DirectX::BoundingBox(center, extents);
}

// くらい判定（Hurtbox）のデバッグ描画
void Player::DrawBoundingBox()
{
	using namespace DirectX;

	if (m_isColliding) Geometory::SetColor(XMFLOAT4(1.0f, 1.0f, 0.0f, 1.0f));
	else Geometory::SetColor(XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f));

	// 基本のくらい判定(頭・体・足)は常時有効なので、常にこれを描画する。
	// 技固有の追加くらい判定は別途 m_activeHurtboxes として描画される
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

// 相手プレイヤーとの押し合い判定（単純なBODY同士の衝突）
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
	// 非アクティブになったら現在の判定リストをクリア
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

// 攻撃技のパラメータをセットし、攻撃フレームの計測を開始する
void Player::SetCurrentAttackParams(AttackParams* params)
{
	m_pActiveAttackParams = params;
	for (float& buffered : m_atkBuf) buffered = 0.0f;
	m_attackTimer = 0.0f;
	m_attackElapsedSec = 0.0f;
}

AttackParams* Player::GetCurrentAttackParams() const
{
	return m_pActiveAttackParams;
}

// 攻撃モーション中の現在フレームに合わせた攻撃判定・くらい判定の更新
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

	// 発生から持続終了までの間かどうかをチェック
	float startFrame = m_pActiveAttackParams->hitboxStart * 60.0f;
	float endFrame = m_pActiveAttackParams->hitboxEnd * 60.0f;
	// 発生判定も実経過時間基準。アニメフレームだと再生速度ぶんズレるため
	float judgeFrame = m_attackTimer;
	bool isHitActive = (judgeFrame >= startFrame && judgeFrame < endFrame);

	// 攻撃判定 (Hitbox) の生成
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

	// 攻撃中の追加くらい判定 (Hurtbox) の生成
	// m_attackTimer は60FPS基準のゲームフレーム。区間内のものだけを固定サイズで生成する
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

// 攻撃判定 (Hitbox) のデバッグ描画
void Player::DrawHitbox()
{
	if (!m_isAttacking) return;

	using namespace DirectX;
	// 攻撃判定は赤色で描画
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

// ダメージを受ける処理。ガードが成功していれば削りダメージ（10分の1）になる
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
		// HPが0になったら死亡ステートへ遷移
		SetState(std::make_unique<PlayerStateDeath>());
	}
}

float Player::GetHpRatio() const
{
	return (float)m_hp / (float)m_maxHp;
}

// ラウンド開始時などにプレイヤーの状態をリセット
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
	SetState(std::make_unique<PlayerStateIdle>());
}

bool Player::CanFireProjectile() const
{
	if (!m_projectile) return false;
	return !m_projectile->IsActive();
}

// コマンド入力のタイマーを管理（波動拳コマンドなどの受付時間処理）
void Player::UpdateCommandTimer(float tick)
{
	if (m_cmdTimerDown > 0) m_cmdTimerDown -= tick;
	if (m_cmdTimerDownForward > 0) m_cmdTimerDownForward -= tick;
	if (m_cmdTimerDownTap > 0) m_cmdTimerDownTap -= tick;
	if (m_cmdTimerDownTwice > 0) m_cmdTimerDownTwice -= tick;

	const PlayerInputs& in = GetInputs();

	// 向いている方向によって前入力か後ろ入力かを判定
	bool forward = (m_rotation.y < 0.0f) ? in.moveRight : in.moveLeft;

	// 下入力がされたらタイマーをスタート
	if (in.moveDown && !forward) {
		m_cmdTimerDown = CMD_WINDOW;
	}
	// 下入力かつ前入力（斜め前）がされた場合
	else if (in.moveDown && forward) {
		if (m_cmdTimerDown > 0) {
			m_cmdTimerDownForward = CMD_WINDOW;
		}
	}

	// 昇竜拳は下を2回押す。押しっぱなしでは出ないよう、押した瞬間だけを数える
	const bool downPressed = in.moveDown && !m_prevMoveDown;
	m_prevMoveDown = in.moveDown;
	if (downPressed) {
		if (m_cmdTimerDownTap > 0) {
			m_cmdTimerDownTwice = CMD_WINDOW;
			m_cmdTimerDownTap = 0.0f;
		}
		else {
			m_cmdTimerDownTap = CMD_WINDOW;
		}
	}
}

// 波動拳のコマンド（下 -> 斜め前 -> 前＋パンチ）が成立しているかチェック
bool Player::CheckHadoukenCommand() const
{
	const PlayerInputs& in = GetInputs();
	bool forward = (m_rotation.y < 0.0f) ? in.moveRight : in.moveLeft;

	return (m_cmdTimerDownForward > 0 && forward && (in.LightPunch || in.MediumPunch || in.HeavyPunch));
}

// 昇竜拳のコマンド（下、下＋パンチ）が成立しているかチェック
bool Player::CheckShoryukenCommand() const
{
	const PlayerInputs& in = GetInputs();
	return (m_cmdTimerDownTwice > 0 && in.moveDown && (in.LightPunch || in.MediumPunch || in.HeavyPunch));
}

// 現在ガード可能な状態か、後ろ入力がされているかを判定
bool Player::IsGuarding() const
{
	// ジャンプ中、攻撃中などはガード不可
	if (m_isJumping || m_isAttacking || m_pActiveAttackParams != nullptr) return false;

	// ダメージやダウン中もガード不可
	if (strcmp(m_currentAnim.name, "Damage") == 0 ||
		strcmp(m_currentAnim.name, "Down") == 0 ||
		strcmp(m_currentAnim.name, "WakeUp") == 0 ||
		strcmp(m_currentAnim.name, "Death") == 0)
	{
		return false;
	}

	const PlayerInputs& in = GetInputs();
	bool isFacingRight = (m_rotation.y < 0.0f);
	// 相手の反対方向（後ろ）キーが押されているか
	bool isPressingBack = isFacingRight ? in.moveLeft : in.moveRight;

	return isPressingBack;
}

// 立ちガード（上段ガード）が成立しているか判定
bool Player::IsGuardingHigh() const
{
	if (!IsGuarding()) return false;
	const PlayerInputs& in = GetInputs();
	// 後ろ入力かつ下に入力していない場合
	return !in.moveDown;
}

// しゃがみガード（下段ガード）が成立しているか判定
bool Player::IsGuardingLow() const
{
	if (!IsGuarding()) return false;
	const PlayerInputs& in = GetInputs();
	// 後ろ入力かつ下にも入力している場合
	return in.moveDown;
}

// 相手の攻撃判定レベルに対してガードが成功するかを検証
bool Player::TryGuard(AttackLevel atkLevel) const
{
	if (!IsGuarding()) return false;

	if (m_guardAllLevels) return true;
	if (atkLevel == AttackLevel::MID)
	{
		// 中段攻撃は立ちガードのみ可能
		return IsGuardingHigh();
	}
	else if (atkLevel == AttackLevel::LOW)
	{
		// 下段攻撃はしゃがみガードのみ可能
		return IsGuardingLow();
	}

	// 上段攻撃は立ちガードでもしゃがみガードでも防げる
	return true;
}