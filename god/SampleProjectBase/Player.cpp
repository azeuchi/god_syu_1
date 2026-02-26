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
#include "Hadouken.h"
#include "PlayerStateDeath.h"

#include "Projectile.h"

#include <DirectXMath.h>
#include <algorithm>

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
	, m_projectile(new Projectile())
	, m_attackTimer(0.0f)
{
	// 初期アニメーションとステートの設定
	m_currentAnim = { "Idle", 0 };
	m_previousAnim = { "Idle", 0 };
	SetState(new PlayerStateIdle());

	InitDefaultParameters();
}

Player::~Player()
{
	// ステートと飛び道具のメモリ解放
	if (m_currentState) {
		delete m_currentState;
		m_currentState = nullptr;
	}
	if (m_projectile) {
		delete m_projectile;
		m_projectile = nullptr;
	}
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


	// 攻撃モーション中もデフォルトで頭・体・足のくらい判定が存在するように初期化するヘルパー
	auto InitDefaultHurtboxes = [&](std::vector<AnimatedBox>& boxes) {
		boxes.clear();
		boxes.resize(3);
		boxes[0].keyframes.push_back({ 0.0f, {m_baseHurtboxOffsets[0], m_baseHurtboxExtents[0]} });
		boxes[1].keyframes.push_back({ 0.0f, {m_baseHurtboxOffsets[1], m_baseHurtboxExtents[1]} });
		boxes[2].keyframes.push_back({ 0.0f, {m_baseHurtboxOffsets[2], m_baseHurtboxExtents[2]} });
		};


	// --- 弱パンチの設定 ---
	{
		AttackParams& p = m_lightPunchParams;
		p.totalDuration = 0.166667f;
		p.hitboxStart = 0.0666667f; // 発生
		p.hitboxEnd = 0.15f;        // 持続終了

		// 攻撃判定（Hitbox）の作成
		p.hitboxes.clear();
		AnimatedBox ab;
		ab.keyframes.push_back({ 0.0f, { {0.76f, 1.7f}, {0.454f, 0.157f} } });
		p.hitboxes.push_back(ab);

		p.damage = 100; p.hitFrame = 2; p.blockFrame = -2; p.hitStop = 0.0666667f; p.knockback = 0.223f;
		p.isDown = false;
		p.attackLevel = AttackLevel::HIGH; // 上段判定

		InitDefaultHurtboxes(p.hurtboxes);

		// キャンセル可能タイミングの設定
		p.cancelEnabled = true; p.cancelStart = 0.0666667f; p.cancelEnd = 0.166667f;
		p.cancelToLight = true; p.cancelToMedium = true; p.cancelToHeavyPunch = false; p.cancelToMediumKick = false; p.cancelToHeavy = false;
		p.speedModifiers.clear();
	}

	// --- 中パンチの設定 ---
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

		InitDefaultHurtboxes(p.hurtboxes);

		p.cancelEnabled = true; p.cancelStart = 0.1f; p.cancelEnd = 0.333333f;
		p.cancelToLight = false; p.cancelToMedium = false; p.cancelToHeavyPunch = true; p.cancelToMediumKick = false; p.cancelToHeavy = true;

		// 発生や硬直をごまかすためのアニメーション再生速度調整
		p.speedModifiers.clear();
		p.speedModifiers.push_back({ 0.0f, 6.0f, 0.369f });
		p.speedModifiers.push_back({ 7.0f, 12.0f, 2.533f });
	}

	// --- 強パンチの設定 ---
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

		InitDefaultHurtboxes(p.hurtboxes);

		p.cancelEnabled = true; p.cancelStart = 0.133333f; p.cancelEnd = 0.416667f;
		p.cancelToLight = false; p.cancelToMedium = false; p.cancelToHeavyPunch = false; p.cancelToMediumKick = false; p.cancelToHeavy = true;

		p.speedModifiers.clear();
		p.speedModifiers.push_back({ 0.0f, 10.0f, 0.344f });
		p.speedModifiers.push_back({ 11.0f, 20.0f, 2.057f });
		p.speedModifiers.push_back({ 22.0f, 30.0f, 0.486f });
	}

	// --- 中キックの設定 ---
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
		p.attackLevel = AttackLevel::MID; // 中段判定（しゃがみガード不可）

		InitDefaultHurtboxes(p.hurtboxes);

		p.cancelEnabled = false;

		p.speedModifiers.clear();
		p.speedModifiers.push_back({ 0.0f, 4.0f, 0.362f });
		p.speedModifiers.push_back({ 5.0f, 9.0f, 0.503f });
		p.speedModifiers.push_back({ 10.0f, 20.0f, 0.295f });
	}

	// --- 強キックの設定 ---
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
		p.attackLevel = AttackLevel::LOW; // 下段判定（立ちガード不可）

		InitDefaultHurtboxes(p.hurtboxes);

		p.cancelEnabled = false;

		p.speedModifiers.clear();
		p.speedModifiers.push_back({ 0.0f, 6.0f, 0.463f });
		p.speedModifiers.push_back({ 7.0f, 16.0f, 1.006f });
		p.speedModifiers.push_back(
			{ 17.0f, 30.0f, 0.345f });
	}

	// --- 波動拳 (弱/中/強) の設定 ---
	{
		InitDefaultHurtboxes(m_hadoukenLParams.hurtboxes);
		m_hadoukenLParams.totalDuration = 0.6f;
		m_hadoukenLParams.projectileSpeed = 4.0f; // 弾速
		m_hadoukenLParams.damage = 500;
		m_hadoukenLParams.attackLevel = AttackLevel::HIGH;

		InitDefaultHurtboxes(m_hadoukenMParams.hurtboxes);
		m_hadoukenMParams.totalDuration = 0.6f;
		m_hadoukenMParams.projectileSpeed = 6.5f;
		m_hadoukenMParams.damage = 500;
		m_hadoukenMParams.attackLevel = AttackLevel::HIGH;

		InitDefaultHurtboxes(m_hadoukenHParams.hurtboxes);
		m_hadoukenHParams.totalDuration = 0.6f;
		m_hadoukenHParams.projectileSpeed = 9.0f;
		m_hadoukenHParams.damage = 500;
		m_hadoukenHParams.attackLevel = AttackLevel::HIGH;
	}
}

// 毎フレーム呼ばれる更新処理の大枠
void Player::Update(float tick)
{
	// 1. 入力の取得
	PollInputs();
	UpdateCommandTimer(tick);

	// 2. 現在のステート（状態）の更新
	if (m_currentState) {
		m_isCrouching = m_currentState->IsCrouch();
		m_currentState->Update(this, tick);
	}

	// 3. 物理・座標の更新
	UpdatePhysics(tick);

	// 4. アニメーションフレームの進行
	UpdateAnimation(tick);

	// 5. モデルのアニメーションブレンド更新
	UpdateModelBlend();

	// 6. 現在のフレームに応じた攻撃判定・くらい判定の更新
	UpdateAttackBoxes();

	// 7. 発射済みの飛び道具の更新
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
		// AIの入力ロジックは別途実装
		break;
	}
}


// 重力の適用と座標の移動処理
void Player::UpdatePhysics(float tick)
{
	if (m_isJumping) {
		m_velocity.y -= 55.0f * tick; // 重力
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

		// フレームを進める（60FPS基準）
		m_currentAnim.frame += currentSpeed * tick * 60.0f;

		// 攻撃判定生成用のタイマーをフレームと同期
		if (m_pActiveAttackParams)
		{
			m_attackTimer = m_currentAnim.frame;
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
void Player::SetState(PlayerState* newState)
{
	if (newState != nullptr)
	{

		// 体力が0以下なら死亡ステート以外への遷移を拒否
		if (m_hp <= 0 && !newState->IsDeathState())
		{
			delete newState;
			return;
		}

		if (m_currentState) {
			// すでに死亡状態なら、他の状態による上書きをブロック
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

// 現在のステートが無敵かどうかを判定
bool Player::IsInvincible() const
{
	if (m_currentState)
	{
		return m_currentState->IsInvincible();
	}
	return false;
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
		m_velocity.y = 19.0f; // ジャンプ初速
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

	if (m_isAttacking && !m_activeHurtboxes.empty())
	{
		// 攻撃中の専用くらい判定が存在する場合は、色を変えて描画
		Geometory::SetColor(XMFLOAT4(0.0f, 1.0f, 0.5f, 1.0f));
		for (const auto& box : m_activeHurtboxes)
		{
			XMFLOAT3 corners[8];
			box.GetCorners(corners);
			static const int edge[4][2] = { {0,1},{1,2},{2,3},{3,0} };
			for (int e = 0; e < 4; ++e) {
				Geometory::AddLine(corners[edge[e][0]], corners[edge[e][1]]);
			}
		}
	}
	else
	{
		// 通常の部位ごとのくらい判定を描画
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
}

// 相手プレイヤーとの押し合い判定（単純なBODY同士の衝突）
bool Player::CheckCollision(const Player* other) const
{
	if (!other) return false;

	BoundingBox myBox = GetHurtbox(HurtboxType::BODY);
	BoundingBox otherBox = other->GetHurtbox(HurtboxType::BODY);
	return myBox.Intersects(otherBox);
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
	m_attackTimer = 0.0f;
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
	bool isHitActive = (currentFrame >= startFrame && currentFrame < endFrame);

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

	// 攻撃中の専用くらい判定 (Hurtbox) の生成
	for (const auto& animBox : m_pActiveAttackParams->hurtboxes)
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
		SetState(new PlayerStateDeath());
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
	SetState(new PlayerStateIdle());
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
}

// 波動拳のコマンド（下 -> 斜め前 -> 前＋パンチ）が成立しているかチェック
bool Player::CheckHadoukenCommand() const
{
	const PlayerInputs& in = GetInputs();
	bool forward = (m_rotation.y < 0.0f) ? in.moveRight : in.moveLeft;

	return (m_cmdTimerDownForward > 0 && forward && (in.LightPunch || in.MediumPunch || in.HeavyPunch));
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