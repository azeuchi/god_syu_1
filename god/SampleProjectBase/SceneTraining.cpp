// SceneTraining.cpp
#include "SceneTraining.h"
#include "Geometory.h"
#include "DebugLog.h"
#include "Model.h"
#include "CameraBase.h"
#include "LightBase.h"
#include "Shader.h"
#include "Player.h"
#include "Input.h"
#include "BattleCollision.h"
#include "PlayerParameterLoader.h"
#include "DirectX.h"
#include <system/imgui/imgui.h>
#include <algorithm>

using namespace DirectX;
using namespace DirectX::SimpleMath;

// GUIボタンからのシーン遷移要求フラグ（SceneRootが監視）
bool SceneTraining::s_requestEnter = false;
bool SceneTraining::s_requestGoGame = false;
bool SceneTraining::s_requestGoDebug = false;

namespace
{
	const float STAGE_LIMIT_X = 6.0f;
	const float CAMERA_LIMIT_X = 4.0f;

	// 共通のアニメーション読み込み（戦闘に必要な一式）
	void LoadCommonAnimations(Player* p)
	{
		Model* m = p->GetModel();
		m->LoadAnimation("Assets/Model/knight/Walking.fbx", "Walk", true);
		m->LoadAnimation("Assets/Model/knight/WalkBack.fbx", "WalkBack", true);
		m->LoadAnimation("Assets/Model/knight/CrouchIdle.fbx", "CrouchIdle", true);
		m->LoadAnimation("Assets/Model/knight/LightPunch.fbx", "LightPunch", true);
		m->LoadAnimation("Assets/Model/knight/MediumPunch.fbx", "MediumPunch", true);
		m->LoadAnimation("Assets/Model/knight/HeavyPunch.fbx", "HeavyPunch", true);
		m->LoadAnimation("Assets/Model/knight/MediumKick.fbx", "MediumKick", true);
		m->LoadAnimation("Assets/Model/knight/HeavyKick.fbx", "HeavyKick", true);
		m->LoadAnimation("Assets/Model/knight/Jump.fbx", "Jump", true);
		m->LoadAnimation("Assets/Model/knight/Damage.fbx", "Damage", true);
		m->LoadAnimation("Assets/Model/knight/Down.fbx", "Down", true);
		m->LoadAnimation("Assets/Model/knight/WakeUp.fbx", "WakeUp", true);
		m->LoadAnimation("Assets/Model/knight/Hadouken.fbx", "Hadouken", true);
		m->LoadAnimation("Assets/Model/knight/Death.fbx", "Death", true);
	}
}

void SceneTraining::Init()
{
	// --- シェーダー読み込み（簡易描画用） ---
	Shader* shader[] = {
		CreateObj<VertexShader>("VS_SkinMeshAnimation"),
		CreateObj<PixelShader>("PS_TexColor"),
	};
	const char* file[] = {
		"Assets/Shader/VS_SkinMeshAnimation.cso",
		"Assets/Shader/PS_TexColor.cso",
	};
	for (int i = 0; i < _countof(shader); ++i)
	{
		if (FAILED(shader[i]->Load(file[i])))
		{
			MessageBox(NULL, file[i], "Shader Error", MB_OK);
		}
	}

	// --- 反転モデル用カリングステート（ゲームシーンと同じ見た目にする）---
	{
		D3D11_RASTERIZER_DESC rsDesc = {};
		rsDesc.FillMode = D3D11_FILL_SOLID;
		rsDesc.FrontCounterClockwise = FALSE;
		rsDesc.DepthClipEnable = TRUE;
		rsDesc.CullMode = D3D11_CULL_FRONT;
		GetDevice()->CreateRasterizerState(&rsDesc, &m_pCullFront);
		rsDesc.CullMode = D3D11_CULL_BACK;
		GetDevice()->CreateRasterizerState(&rsDesc, &m_pCullBack);
	}

	// --- 1P（操作キャラ）---
	CreateObj<Player>("Player");
	Player* p1 = GetObj<Player>("Player");
	p1->SetInputType(PlayerInputType::PLAYER_1);
	PlayerParameterLoader::LoadSettings(p1);
	p1->Load("Assets/Model/knight/Idle.fbx", 0.014f, true, false);
	LoadCommonAnimations(p1);
	p1->SetPosition({ -2.0f, 0.0f, 0.0f });
	p1->SetRotation({ 0.0f, DirectX::XM_PI / -2.0f, 0.0f });
	p1->Reset();

	// --- 2P（ダミー：AI入力）---
	CreateObj<Player>("Player2");
	Player* p2 = GetObj<Player>("Player2");
	p2->SetInputType(PlayerInputType::AI);
	p2->SetGuardAllLevels(true); // トレーニングのダミーはガード中どの段でも防ぐ
	p2->SetMoveSpeed(p1->GetMoveSpeed());
	XMFLOAT3 scaleP2 = p1->GetScale();
	scaleP2.x *= -1.0f; // X軸反転で向かい合わせ
	p2->SetScale(scaleP2);
	PlayerParameterLoader::CopyParameters(p1, p2);
	p2->Load("Assets/Model/knight/Idle.fbx", 0.014f, true, false);
	LoadCommonAnimations(p2);
	p2->SetPosition({ 2.0f, 0.0f, 0.0f });
	p2->SetRotation({ 0.0f, DirectX::XM_PI / 2.0f, 0.0f });
	p2->Reset();

	// --- ヒットエフェクトのプール ---
	for (int i = 0; i < 10; ++i)
	{
		m_hitEffects.push_back(new HitEffect());
	}

	// --- カメラ初期位置 ---
	CameraBase* pCamera = GetObj<CameraBase>("Camera");
	if (pCamera)
	{
		pCamera->SetPos({ 0.0f, 1.5f, -5.0f });
		pCamera->SetLook({ 0.0f, 1.2f, 0.0f });
	}

	DebugLog::log(DebugLog::INFO_LOG, "--- Training Mode Start ---");
}

void SceneTraining::Uninit()
{
	for (auto e : m_hitEffects) { delete e; }
	m_hitEffects.clear();
	if (m_pCullFront) { m_pCullFront->Release(); m_pCullFront = nullptr; }
	if (m_pCullBack) { m_pCullBack->Release(); m_pCullBack = nullptr; }
}

PlayerInputs SceneTraining::BuildDummyInputs(Player* dummy, float tick)
{
	PlayerInputs in; // 既定はすべて false

	// スタンス：しゃがみなら下要素を入れっぱなし（下段ガードにもなる）
	if (m_dummyStance == 1) in.moveDown = true;

	// ガードするか
	bool doGuard = false;
	if (m_dummyGuard == 1)      doGuard = true;             // 常時
	else if (m_dummyGuard == 2) doGuard = m_dummyHitLatch;  // 最初の1発以降

	if (doGuard)
	{
		// 相手と逆方向（後ろ）を入れるとガード姿勢になる
		bool facingRight = (dummy->GetRotation().y < 0.0f);
		if (facingRight) in.moveLeft = true; else in.moveRight = true;
	}

	// アクション：一定間隔で攻撃を繰り返す（ガード中は出さない）
	if (m_dummyAction > 0 && !doGuard)
	{
		m_repeatTimer -= tick;
		if (m_repeatTimer <= 0.0f)
		{
			m_repeatTimer = 0.8f; // 0.8秒ごとに1回
			switch (m_dummyAction)
			{
			case 1: in.LightPunch = true; break;
			case 2: in.MediumPunch = true; break;
			case 3: in.HeavyPunch = true; break;
			case 4: in.MediumKick = true; break;
			case 5: in.HeavyKick = true; break;
			}
		}
	}

	return in;
}

void SceneTraining::ResetPositions()
{
	Player* p1 = GetObj<Player>("Player");
	Player* p2 = GetObj<Player>("Player2");
	if (p1)
	{
		p1->SetPosition({ -2.0f, 0.0f, 0.0f });
		p1->SetRotation({ 0.0f, DirectX::XM_PI / -2.0f, 0.0f });
		p1->Reset();
	}
	if (p2)
	{
		p2->SetPosition({ 2.0f, 0.0f, 0.0f });
		p2->SetRotation({ 0.0f, DirectX::XM_PI / 2.0f, 0.0f });
		p2->Reset();
	}
	m_dummyHitLatch = false;
	m_dummyNeutralTimer = 0.0f;
	m_dummyHomeX = 2.0f;
}

void SceneTraining::Update(float tick)
{
	if (IsKeyTrigger(VK_TAB)) m_showImGui = !m_showImGui;

	Player* p1 = GetObj<Player>("Player");
	Player* p2 = GetObj<Player>("Player2");
	if (!p1 || !p2) return;

	// 無限HP：当たり判定の前に満タンへ戻す（撃墜されない）
	if (m_infiniteHp) { p1->RefillHp(); p2->RefillHp(); }

	// ダミー入力を組み立てて注入（AIはこの入力で動く）
	PlayerInputs dummyIn = BuildDummyInputs(p2, tick);
	p2->SetInjectedInputs(dummyIn);

	// プレイヤー更新
	p1->Update(tick);
	p2->Update(tick);

	// 当たり判定
	CollisionResult result = BattleCollision::UpdateInteractions(p1, p2, tick, m_hitEffects, STAGE_LIMIT_X);

	// ダミーに当たった（ヒット/ガード問わず）らラッチ。途切れたら解除
	m_comboTimer += tick;
	if (result.shakeTimerP2 > 0.0f)
	{
		m_dummyHitLatch = true;
		m_dummyNeutralTimer = 0.0f;
		m_hitCount++;
		m_lastStun = result.dbgStun;
		m_lastBlocked = result.wasBlocked;
		if (result.wasBlocked)
		{
			m_blockCount++;
			m_comboCount = 0; // ガードされたらコンボ途切れ
		}
		else
		{
			// 直近ヒットから間が短ければコンボ継続、空けばリセット
			if (m_comboTimer < 0.4f) m_comboCount++;
			else m_comboCount = 1;
			if (m_comboCount > m_maxCombo) m_maxCombo = m_comboCount;
		}
		m_comboTimer = 0.0f;
	}
	else
	{
		m_dummyNeutralTimer += tick;
		if (m_dummyNeutralTimer > 0.7f) m_dummyHitLatch = false;
	}

	// ダミーの位置を固定（コンボ確認用：ノックバックや後ろ歩きで離れないように）
	if (m_lockDummyPos)
	{
		XMFLOAT3 dp = p2->GetPosition();
		dp.x = m_dummyHomeX;
		p2->SetPosition(dp);
	}

	// エフェクト更新
	for (auto e : m_hitEffects) e->Update(tick);

	// カメラ：2体の中央を映す
	CameraBase* pCamera = GetObj<CameraBase>("Camera");
	if (pCamera)
	{
		float centerX = (p1->GetPosition().x + p2->GetPosition().x) * 0.5f;
		centerX = std::clamp(centerX, -CAMERA_LIMIT_X, CAMERA_LIMIT_X);
		pCamera->SetPos({ centerX, 1.5f, -5.0f });
		pCamera->SetLook({ centerX, 1.2f, 0.0f });
	}
}

void SceneTraining::DrawPlayer(Player* p)
{
	if (!p) return;
	CameraBase* pCamera = GetObj<CameraBase>("Camera");
	LightBase* pLight = GetObj<LightBase>("Light");
	Shader* vs = GetObj<Shader>("VS_SkinMeshAnimation");
	Shader* ps = GetObj<Shader>("PS_TexColor");
	if (!pCamera || !pLight || !vs || !ps) return;

	XMFLOAT3 pos = p->GetPosition();
	XMFLOAT3 rot = p->GetRotation();
	XMFLOAT3 sc = p->GetScale();
	Matrix S = Matrix::CreateScale(sc.x, sc.y, sc.z);
	Matrix baseS = p->GetModel()->GetScaleBaseMatrix();
	Matrix R = DirectX::XMMatrixRotationRollPitchYaw(rot.x, rot.y, rot.z);
	Matrix T = Matrix::CreateTranslation(pos.x, pos.y, 0.0f);
	Matrix world = baseS * S * R * T;

	XMFLOAT4X4 mat[3];
	XMStoreFloat4x4(&mat[0], XMMatrixTranspose(world));
	mat[1] = pCamera->GetView();
	mat[2] = pCamera->GetProj();

	XMFLOAT3 lightDir = pLight->GetDirection();
	XMFLOAT4 light[] = {
		pLight->GetDiffuse(),
		pLight->GetAmbient(),
		{ lightDir.x, lightDir.y, lightDir.z, 0.0f }
	};
	XMFLOAT3 camPos = pCamera->GetPos();
	XMFLOAT4 camera[] = { { camPos.x, camPos.y, camPos.z, 0.0f } };

	vs->WriteBuffer(0, mat);
	ps->WriteBuffer(0, light);
	ps->WriteBuffer(1, camera);
	p->SetVertexShader(vs);
	p->SetPixelShader(ps);

	// 反転している2Pは表裏が逆になるためカリングを切り替える
	bool flipped = (sc.x < 0.0f);
	if (flipped) { if (m_pCullFront) GetContext()->RSSetState(m_pCullFront); }
	else { if (m_pCullBack) GetContext()->RSSetState(m_pCullBack); }

	p->Draw();
}

void SceneTraining::Draw()
{
	Player* p1 = GetObj<Player>("Player");
	Player* p2 = GetObj<Player>("Player2");

	DrawPlayer(p1);
	DrawPlayer(p2);

#ifdef _DEBUG
	if (p1) { p1->DrawBoundingBox(); p1->DrawHitbox(); p1->DrawActiveHurtboxes(); }
	if (p2) { p2->DrawBoundingBox(); p2->DrawHitbox(); p2->DrawActiveHurtboxes(); }
#endif

	// ヒットエフェクト
	CameraBase* pCamera = GetObj<CameraBase>("Camera");
	if (pCamera)
	{
		XMFLOAT4X4 view = pCamera->GetView();
		XMFLOAT4X4 proj = pCamera->GetProj();
		for (auto e : m_hitEffects) e->Draw(view, proj);
	}

	if (m_showImGui) DrawImGui();
}

void SceneTraining::DrawImGui()
{
	ImGui::Begin("トレーニング");

	ImGui::TextUnformatted("1P:あなた  2P:ダミー  (TAB:UI表示切替)");
	ImGui::Separator();

	const char* stances[] = { "立ち", "しゃがみ" };
	ImGui::Combo("ダミーの構え", &m_dummyStance, stances, _countof(stances));

	const char* actions[] = { "なし", "弱P連打", "中P連打", "強P連打", "中K連打", "強K連打" };
	ImGui::Combo("ダミーの行動", &m_dummyAction, actions, _countof(actions));

	const char* guards[] = { "ガードしない", "常にガード", "2発目以降ガード" };
	ImGui::Combo("ダミーのガード", &m_dummyGuard, guards, _countof(guards));

	ImGui::Checkbox("無限HP", &m_infiniteHp);
	ImGui::Checkbox("ダミー位置固定", &m_lockDummyPos);

	ImGui::Separator();
	Player* p1 = GetObj<Player>("Player");
	Player* p2 = GetObj<Player>("Player2");
	if (p1) { ImGui::Text("1P HP"); ImGui::ProgressBar(p1->GetHpRatio(), ImVec2(-1.0f, 0.0f)); }
	if (p2) { ImGui::Text("2P(ダミー) HP"); ImGui::ProgressBar(p2->GetHpRatio(), ImVec2(-1.0f, 0.0f)); }

	ImGui::Text("直近: %s", m_lastBlocked ? "ガード" : "ヒット");
	ImGui::Text("コンボ: %d   (最大: %d)", m_comboCount, m_maxCombo);
	ImGui::Text("与えた硬直: %d F", m_lastStun);
	ImGui::Text("当てた数: %d  (ガード: %d)", m_hitCount, m_blockCount);
	ImGui::Text("ガードラッチ: %s", m_dummyHitLatch ? "ON" : "OFF");
	ImGui::TextDisabled("(無限HPをOFFにするとチップダメージが見えます)");

	if (ImGui::Button("位置リセット"))
	{
		ResetPositions();
		m_hitCount = 0;
		m_blockCount = 0;
		m_comboCount = 0;
		m_maxCombo = 0;
	}

	ImGui::Separator();
	if (ImGui::Button("ゲームへ"))  { s_requestGoGame = true; }
	ImGui::SameLine();
	if (ImGui::Button("デバッグへ")) { s_requestGoDebug = true; }

	ImGui::End();
}
