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
#include "PlayerAssetLoader.h"
#include "Projectile.h"
#include "DirectX.h"
#include "CameraShake.h"
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
	const float REVIVE_DELAY = 2.0f; // 倒れてから起き上がるまでの秒数

	// 当たった側を小刻みに揺らす（ゲーム本編と同じ揺れ幅）
	void UpdateHitShake(float& timer, XMFLOAT3& offset, float tick)
	{
		if (timer > 0.0f)
		{
			timer -= tick;
			float offsetX = ((float)(rand() % 100) / 100.0f - 0.5f) * 0.1f;
			float offsetY = ((float)(rand() % 100) / 100.0f - 0.5f) * 0.1f;
			offset = { offsetX, offsetY, 0.0f };
		}
		else
		{
			offset = { 0.0f, 0.0f, 0.0f };
		}
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

	// --- プレイヤー描画ヘルパー ---
	m_playerRenderer.Setup(this);

	// --- スカイドームを用意 ---
	m_skyDome = std::make_unique<SkyDome>();
	m_skyDome->Setup(this);

	// --- 1P（操作キャラ）---
	CreateObj<Player>("Player");
	Player* p1 = GetObj<Player>("Player");
	p1->SetInputType(PlayerInputType::PLAYER_1);
	PlayerParameterLoader::LoadSettings(p1);
	p1->Load("Assets/Model/knight/Idle.fbx", Player::MODEL_SCALE, true, false);
	PlayerAssetLoader::LoadCommonAnimations(p1);
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
	p2->Load("Assets/Model/knight/Idle.fbx", Player::MODEL_SCALE, true, false);
	PlayerAssetLoader::LoadCommonAnimations(p2);
	p2->SetPosition({ 2.0f, 0.0f, 0.0f });
	p2->SetRotation({ 0.0f, DirectX::XM_PI / 2.0f, 0.0f });
	p2->Reset();

	// --- ヒットエフェクトのプール ---
	m_hitEffects.Init(10);

	// --- エフェクト描画用の設定（ゲーム本編と同じ）---
	// カリングなし：直前に描いたキャラの向きで裏面扱いになり、エフェクトが消えるのを防ぐ
	D3D11_RASTERIZER_DESC rsDesc = {};
	rsDesc.FillMode = D3D11_FILL_SOLID;
	rsDesc.CullMode = D3D11_CULL_NONE;
	rsDesc.DepthClipEnable = TRUE;
	GetDevice()->CreateRasterizerState(&rsDesc, &m_pCullNone);

	// 半透明合成：画像の透明部分が黒い四角で見えないようにする
	D3D11_BLEND_DESC blendDesc = {};
	blendDesc.AlphaToCoverageEnable = TRUE;
	blendDesc.RenderTarget[0].BlendEnable = TRUE;
	blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
	blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
	GetDevice()->CreateBlendState(&blendDesc, &m_pBlendState);

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
	if (m_pCullNone) { m_pCullNone->Release(); m_pCullNone = nullptr; }
	if (m_pBlendState) { m_pBlendState->Release(); m_pBlendState = nullptr; }
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

void SceneTraining::Revive(Player* p1, Player* p2)
{
	// 倒れた側は待機に戻し、もう片方もHPを満タンにして仕切り直す。立ち位置はそのまま
	for (Player* p : { p1, p2 })
	{
		if (p->GetHpRatio() <= 0.0f) p->Reset();
		else p->RefillHp();
	}
	m_reviveTimer = 0.0f;
}

void SceneTraining::Update(float tick)
{
	if (IsKeyTrigger(VK_TAB)) m_showImGui = !m_showImGui;

	Player* p1 = GetObj<Player>("Player");
	Player* p2 = GetObj<Player>("Player2");
	if (!p1 || !p2) return;

	// 倒れた側を起こす。無限HPを入れ直したらすぐ、そうでなければ倒れる動きを見せてから
	if (p1->GetHpRatio() <= 0.0f || p2->GetHpRatio() <= 0.0f)
	{
		m_reviveTimer -= tick;
		if (m_infiniteHp || m_reviveTimer <= 0.0f) Revive(p1, p2);
	}

	// 無限HP：当たり判定の前に満タンへ戻す（撃墜されない）
	if (m_infiniteHp) { p1->RefillHp(); p2->RefillHp(); }

	// ダミー入力を組み立てて注入（AIはこの入力で動く）
	PlayerInputs dummyIn = BuildDummyInputs(p2, tick);
	p2->SetInjectedInputs(dummyIn);

	// ヒットストップ中はキャラの時間を止める（ゲーム本編と同じ）。
	// 止めないと、当たった瞬間から押し返しで滑り出して見える
	float playerTick = tick;
	if (m_hitStopTimer > 0.0f)
	{
		m_hitStopTimer -= tick;
		playerTick = 0.0f;
	}

	// プレイヤー更新
	p1->Update(playerTick);
	p2->Update(playerTick);

	// 当たる直前にダミーがのけぞり中だったか。のけぞりが解けていればガードで割り込めたので、コンボに数えない
	const bool dummyWasInHitstun = p2->IsInHitstun();

	// 当たり判定
	CollisionResult result = BattleCollision::UpdateInteractions(p1, p2, playerTick, m_hitEffects.Raw(), STAGE_LIMIT_X);

	// ヒットストップと揺れ（ゲーム本編と同じ）
	if (result.hitStopTimer > 0.0f)
	{
		m_hitStopTimer = result.hitStopTimer;
		// 画面が揺れるのはヒットしたときだけ。のけぞる側へ横に揺らす
		if (!result.wasBlocked)
		{
			CameraShake::AddTrauma(m_cameraTrauma, CameraShake::TraumaFromHitStop(result.hitStopTimer));
			if (result.shakeTimerP2 > 0.0f)      m_cameraShakeKickDir = 1.0f;
			else if (result.shakeTimerP1 > 0.0f) m_cameraShakeKickDir = -1.0f;
		}
	}
	if (result.shakeTimerP1 > 0.0f) m_shakeTimerP1 = result.shakeTimerP1;
	if (result.shakeTimerP2 > 0.0f) m_shakeTimerP2 = result.shakeTimerP2;
	UpdateHitShake(m_shakeTimerP1, m_shakeOffsetP1, tick);
	UpdateHitShake(m_shakeTimerP2, m_shakeOffsetP2, tick);
	CameraShake::Tick(m_cameraTrauma, m_cameraShakeKickDir, tick, m_cameraShakeOffset);

	// 撃破時の処理は対戦用で、両者の操作をAIに切り替えてしまう。トレーニングでは操作を戻す
	if (result.isRoundOver)
	{
		p1->SetInputType(PlayerInputType::PLAYER_1);
		p2->SetInputType(PlayerInputType::AI);
		m_reviveTimer = REVIVE_DELAY;
	}

	// ダミーに当たった（ヒット/ガード問わず）らラッチ。途切れたら解除
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
			// のけぞりが続いている間に当てたときだけコンボ継続。間が空いて割り込めたなら1からやり直す
			if (dummyWasInHitstun) m_comboCount++;
			else m_comboCount = 1;
			if (m_comboCount > m_maxCombo) m_maxCombo = m_comboCount;
		}
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
	m_hitEffects.Update(tick);

	// カメラ：2体の中央を映す
	CameraBase* pCamera = GetObj<CameraBase>("Camera");
	if (pCamera)
	{
		float centerX = (p1->GetPosition().x + p2->GetPosition().x) * 0.5f;
		centerX = std::clamp(centerX, -CAMERA_LIMIT_X, CAMERA_LIMIT_X);
		// 揺れは位置と注視点を同じだけずらす（視線の向きは変えずに画面全体が揺れる）
		const XMFLOAT3& s = m_cameraShakeOffset;
		pCamera->SetPos({ centerX + s.x, 1.5f + s.y, -5.0f });
		pCamera->SetLook({ centerX + s.x, 1.2f + s.y, 0.0f });

		// スカイドームをカメラ位置に追従させる
		if (m_skyDome) m_skyDome->Update(pCamera->GetPos());
	}
}

void SceneTraining::Draw()
{
	Player* p1 = GetObj<Player>("Player");
	Player* p2 = GetObj<Player>("Player2");

	// スカイドームはプレイヤーより先に描く
	CameraBase* pSkyCam = GetObj<CameraBase>("Camera");
	if (m_skyDome && pSkyCam) m_skyDome->DrawWithState(pSkyCam->GetView(), pSkyCam->GetProj());

	// 先に全員のアウトライン、次に本体を描く
	m_playerRenderer.DrawOutline(this, p1, m_shakeOffsetP1);
	m_playerRenderer.DrawOutline(this, p2, m_shakeOffsetP2);
	m_playerRenderer.DrawBody(this, p1, m_shakeOffsetP1);
	m_playerRenderer.DrawBody(this, p2, m_shakeOffsetP2);

#ifdef _DEBUG
	if (p1) { p1->DrawBoundingBox(); p1->DrawHitbox(); p1->DrawActiveHurtboxes(); }
	if (p2) { p2->DrawBoundingBox(); p2->DrawHitbox(); p2->DrawActiveHurtboxes(); }
#endif

	// 飛び道具（波動拳）とヒットエフェクト
	CameraBase* pCamera = GetObj<CameraBase>("Camera");
	if (pCamera)
	{
		XMFLOAT4X4 view = pCamera->GetView();
		XMFLOAT4X4 proj = pCamera->GetProj();
		float blendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
		if (m_pCullNone) GetContext()->RSSetState(m_pCullNone);
		if (m_pBlendState) GetContext()->OMSetBlendState(m_pBlendState, blendFactor, 0xffffffff);

		if (p1 && p1->GetProjectile()) p1->GetProjectile()->Draw(view, proj);
		if (p2 && p2->GetProjectile()) p2->GetProjectile()->Draw(view, proj);
		m_hitEffects.Draw(view, proj);

		GetContext()->OMSetBlendState(nullptr, blendFactor, 0xffffffff);
		GetContext()->RSSetState(nullptr);
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
