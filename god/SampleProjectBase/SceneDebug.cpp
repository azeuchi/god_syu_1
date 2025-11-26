#include "math.h"
#include "SceneDebug.h"
#include "Geometory.h"
#include "DebugLog.h"
#include "Model.h"
#include "CameraBase.h"
#include "LightBase.h"
#include "Shader.h"
#include "Player.h" 
#include "Texture.h"
#include "Input.h" 
#include <system/imgui/imgui.h>
#include <fstream> 
#include <cmath> 

using namespace DirectX;
using namespace DirectX::SimpleMath;

Texture* g_uiTex_debug = nullptr;
const char* SETTINGS_FILE_DEBUG = "player_settings.ini";
// 60FPSを基準とした1フレームの時間
const float FRAME_TIME_60FPS = 1.0f / 60.0f;

void SceneDebug::SavePlayerSettings()
{
	Player* player = GetObj<Player>("Player");
	if (player)
	{
		std::ofstream ofs(SETTINGS_FILE_DEBUG);
		if (ofs.is_open())
		{
			//  くらい判定など
			ofs << player->GetMoveSpeed() << std::endl;
			DirectX::XMFLOAT3 scale = player->GetScale();
			ofs << scale.x << " " << scale.y << " " << scale.z << std::endl;
			DirectX::XMFLOAT2 boxExtents = player->GetBoundingBoxExtents();
			ofs << boxExtents.x << " " << boxExtents.y << std::endl;
			DirectX::XMFLOAT2 boxOffset = player->GetBoundingBoxOffset();
			ofs << boxOffset.x << " " << boxOffset.y << std::endl;

			// AttackParams (攻撃判定)
			AttackParams& params = player->GetLightPunchParams();
			ofs << params.totalDuration << std::endl;
			ofs << params.hitboxStart << std::endl;
			ofs << params.hitboxEnd << std::endl;
			ofs << params.hitboxOffset.x << " " << params.hitboxOffset.y << std::endl;
			ofs << params.hitboxExtents.x << " " << params.hitboxExtents.y << std::endl;
			ofs << params.damage << std::endl;
			ofs << params.hitFrame << std::endl;
			ofs << params.blockFrame << std::endl;

			ofs.close();
		}
	}
}

void SceneDebug::Init()
{
	// シェーダー読み込み
	Shader* shader[] = {
		CreateObj<VertexShader>("VS_SkinMeshAnimation"),
		CreateObj<PixelShader>("PS_TexColor"),
	};
	const char* file[] = {
		"Assets/Shader/VS_SkinMeshAnimation.cso",
		"Assets/Shader/PS_TexColor.cso",
	};
	int shaderNum = _countof(shader);
	for (int i = 0; i < shaderNum; ++i)
	{
		if (FAILED(shader[i]->Load(file[i])))
		{
			MessageBox(NULL, file[i], "Shader Error", MB_OK);
		}
	}

	// プレイヤー生成
	CreateObj<Player>("Player");
	Player* player = GetObj<Player>("Player");
	player->SetInputType(PlayerInputType::AI);

	// 設定読み込み
	float moveSpeed = 2.0f;
	DirectX::XMFLOAT3 scale = { 1.0f, 1.0f, 1.0f };
	DirectX::XMFLOAT2 boxExtents = { 0.5f, 1.0f };
	DirectX::XMFLOAT2 boxOffset = { 0.0f, 1.0f };
	AttackParams params = player->GetLightPunchParams();

	std::ifstream ifs(SETTINGS_FILE_DEBUG);
	if (ifs.is_open())
	{
		ifs >> moveSpeed;
		ifs >> scale.x >> scale.y >> scale.z;
		ifs >> boxExtents.x >> boxExtents.y;
		ifs >> boxOffset.x >> boxOffset.y;

		if (!ifs.eof()) ifs >> params.totalDuration;
		if (!ifs.eof()) ifs >> params.hitboxStart;
		if (!ifs.eof()) ifs >> params.hitboxEnd;
		if (!ifs.eof()) ifs >> params.hitboxOffset.x >> params.hitboxOffset.y;
		if (!ifs.eof()) ifs >> params.hitboxExtents.x >> params.hitboxExtents.y;
		if (!ifs.eof()) ifs >> params.damage;
		if (!ifs.eof()) ifs >> params.hitFrame;
		if (!ifs.eof()) ifs >> params.blockFrame;

		ifs.close();
	}

	player->SetMoveSpeed(moveSpeed);
	player->SetScale(scale);
	player->SetBoundingBoxExtents(boxExtents);
	player->SetBoundingBoxOffset(boxOffset);
	player->GetLightPunchParams() = params;

	// アニメーション読み込み
	player->Load("Assets/Model/knight/Idle.fbx", 0.02f, true, false);
	player->GetModel()->LoadAnimation("Assets/Model/knight/LightPunch.fbx", "LightPunch", true);

	// 初期状態は Idle
	player->Debug_SetAnimation("Idle", true);

	player->SetPosition({ 0.0f, 0.0f, 0.0f });
	player->SetRotation({ 0.0f, DirectX::XM_PI / -2.0f, 0.0f });

	m_showImGui = true;
	g_uiTex_debug = new Texture();

	m_isAttacking = false;
	m_isPaused = false; // 初期状態は再生(Idleループ)
	m_currentFrame = 0;
	m_animTimer = 0.0f; // タイマー初期化
}

void SceneDebug::Uninit()
{
	if (g_uiTex_debug) {
		delete g_uiTex_debug;
		g_uiTex_debug = nullptr;
	}
}

void SceneDebug::Update(float tick)
{
	if (tick > 0.0f) { m_fps = 1.0f / tick; }
	else { m_fps = 0.0f; }

	if (IsKeyTrigger(VK_TAB)) {
		m_showImGui = !m_showImGui;
	}

	Player* player = GetObj<Player>("Player");
	if (!player) return;

	// --- アニメーション制御 ---

	if (m_isPaused)
	{
		// 一時停止中: ImGuiで操作された m_currentFrame を適用する
		player->Debug_SetFrame(m_currentFrame);
		m_animTimer = 0.0f; // ポーズ中はタイマーをリセット
	}
	else
	{
		// 再生中: 60FPS間隔でフレームを進める
		m_animTimer += tick;

		// 1フレーム分の時間が経過したら更新
		if (m_animTimer >= FRAME_TIME_60FPS)
		{
			// 時間が経過した分だけフレームを進める
			while (m_animTimer >= FRAME_TIME_60FPS)
			{
				player->UpdateAnimation(FRAME_TIME_60FPS);
				m_animTimer -= FRAME_TIME_60FPS;
			}
		}
	}

	// ボーン行列の更新 (これは停止中でも姿勢反映のために必要)
	player->UpdateModelBlend();

	// 再生中はプレイヤーから現在のフレーム数を取得して同期
	if (!m_isPaused)
	{
		m_currentFrame = player->Debug_GetFrame();
	}


	// 攻撃再生中の終了判定
	if (m_isAttacking)
	{
		float totalDuration = player->GetLightPunchParams().totalDuration;
		// 秒数を60FPS基準のフレーム数に変換
		int animLengthInFrames = static_cast<int>(std::round(totalDuration / FRAME_TIME_60FPS));
		if (animLengthInFrames <= 0) animLengthInFrames = 1;

		// 終了フレームに達したら Idle に戻る
		if (m_currentFrame >= (animLengthInFrames - 1))
		{
			player->Debug_SetAnimation("Idle", true); // Idleに戻す
			m_isAttacking = false;
			m_isPaused = false; // 自動的に再生再開(Idleループ)
			m_currentFrame = 0;
		}
	}
}

void SceneDebug::Draw()
{
	CameraBase* pCamera = GetObj<CameraBase>("Camera");
	LightBase* pLight = GetObj<LightBase>("Light");

	DirectX::XMFLOAT4X4 mat[3];
	DirectX::XMStoreFloat4x4(&mat[0], DirectX::XMMatrixIdentity());
	mat[1] = pCamera->GetView();
	mat[2] = pCamera->GetProj();
	DirectX::XMFLOAT3 lightDir = pLight->GetDirection();
	DirectX::XMFLOAT4 light[] = {
		pLight->GetDiffuse(),
		pLight->GetAmbient(),
		{lightDir.x, lightDir.y, lightDir.z, 0.0f}
	};
	DirectX::XMFLOAT3 camPos = pCamera->GetPos();
	DirectX::XMFLOAT4 camera[] = {
		{camPos.x, camPos.y, camPos.z, 0.0f}
	};
	Shader* shader[] = {
		GetObj<Shader>("VS_SkinMeshAnimation"),
		GetObj<Shader>("PS_TexColor"),
	};

	Player* player = GetObj<Player>("Player");
	if (player) {
		XMFLOAT3 pos = player->GetPosition();
		XMFLOAT3 rot = player->GetRotation();
		XMFLOAT3 pScale = player->GetScale();
		Matrix playerScaleMat = Matrix::CreateScale(pScale.x, pScale.y, pScale.z);
		Matrix modelBaseScaleMat = player->GetModel()->GetScaleBaseMatrix();
		Matrix rotMat = DirectX::XMMatrixRotationRollPitchYaw(rot.x, rot.y, rot.z);
		Matrix transMat = Matrix::CreateTranslation(pos.x, pos.y, 0.0f);
		Matrix world = modelBaseScaleMat * playerScaleMat * rotMat * transMat;

		XMStoreFloat4x4(&mat[0], XMMatrixTranspose(world));

		shader[0]->WriteBuffer(0, mat);
		shader[1]->WriteBuffer(0, light);
		shader[1]->WriteBuffer(1, camera);

		player->SetVertexShader(shader[0]);
		player->SetPixelShader(shader[1]);
		player->Draw();
		player->DrawBoundingBox(); // くらい判定（緑箱）は常時表示

		// 攻撃中かつ発生期間内なら攻撃判定（赤箱）を表示
		if (m_isAttacking)
		{
			AttackParams& params = player->GetLightPunchParams();
			int startFrame = static_cast<int>(std::round(params.hitboxStart / FRAME_TIME_60FPS));
			int endFrame = static_cast<int>(std::round(params.hitboxEnd / FRAME_TIME_60FPS));

			if (m_currentFrame >= startFrame && m_currentFrame < endFrame)
			{
				player->UpdateHitbox(params.hitboxOffset, params.hitboxExtents);
				player->SetActiveHitbox(true);
				player->DrawHitbox();
				player->SetActiveHitbox(false);
			}
		}
	}

	if (m_showImGui)
	{
		DrawImGui();
	}
}

void SceneDebug::DrawImGui()
{
	Player* player = GetObj<Player>("Player");

	ImGui::Begin("Attack Settings");
	ImGui::Text("FPS: %.1f", m_fps);
	ImGui::Separator();

	if (player)
	{
		// --- 1. アクションテスト ---
		ImGui::Text("Action Test");

		// 攻撃開始ボタン群
		if (!m_isAttacking)
		{
			// ★ ボタン1: 普通に再生 (動作確認用)
			if (ImGui::Button("Test Play"))
			{
				player->Debug_SetAnimation("LightPunch", true);
				m_isAttacking = true;
				m_isPaused = false; // 再生状態で開始
				m_currentFrame = 0;
				m_animTimer = 0.0f; // タイマーリセット 
			}

			ImGui::SameLine();

			// ★ ボタン2: 一時停止で開始 (コマ送り確認用)
			if (ImGui::Button("Test Play (Step)"))
			{
				player->Debug_SetAnimation("LightPunch", true);
				m_isAttacking = true;
				m_isPaused = true;  // 一時停止状態で開始 (0フレーム目で止まる)
				m_currentFrame = 0;
				m_animTimer = 0.0f; // タイマーリセット
			}
		}
		else
		{
			ImGui::TextColored(ImVec4(1, 0, 0, 1), ">> ATTACKING <<");
		}

		ImGui::SameLine();

		// 再生・停止チェックボックス
		ImGui::Checkbox("Pause", &m_isPaused);

		// --- コマ送り & スライダー (攻撃中のみ表示) ---
		if (m_isAttacking)
		{
			AttackParams& params = player->GetLightPunchParams();
			int totalFrames = static_cast<int>(std::round(params.totalDuration / FRAME_TIME_60FPS));

			ImGui::Separator();
			ImGui::Text("Frame Control");

			// 1. コマ送りボタン (-1, +1)
			if (ImGui::Button("-1 Frame")) {
				m_currentFrame--;
				if (m_currentFrame < 0) m_currentFrame = 0;
				m_isPaused = true; // 操作時はポーズ
			}
			ImGui::SameLine();
			if (ImGui::Button("+1 Frame")) {
				m_currentFrame++;
				if (m_currentFrame >= totalFrames) m_currentFrame = totalFrames - 1;
				m_isPaused = true;
			}

			// 2. フレームスライダー (シークバー)
			if (ImGui::SliderInt("Scrub", &m_currentFrame, 0, totalFrames - 1))
			{
				m_isPaused = true; // 操作時はポーズ
			}
		}
		else
		{
			ImGui::Text("Frame: %d", m_currentFrame);
		}


		ImGui::Separator();

		// --- 2. パラメータ調整 (60FPS基準) ---
		ImGui::Text("Light Punch Parameters (60FPS)");
		AttackParams& params = player->GetLightPunchParams();

		int totalFrames = static_cast<int>(std::round(params.totalDuration / FRAME_TIME_60FPS));
		int startFrames = static_cast<int>(std::round(params.hitboxStart / FRAME_TIME_60FPS));
		int endFrames = static_cast<int>(std::round(params.hitboxEnd / FRAME_TIME_60FPS));

		if (ImGui::InputInt("Total Duration (Frames)", &totalFrames))
		{
			if (totalFrames < 1) totalFrames = 1;
			params.totalDuration = totalFrames * FRAME_TIME_60FPS;
		}
		if (ImGui::InputInt("Hitbox Start (Frames)", &startFrames))
		{
			if (startFrames < 0) startFrames = 0;
			params.hitboxStart = startFrames * FRAME_TIME_60FPS;
		}
		if (ImGui::InputInt("Hitbox End (Frames)", &endFrames))
		{
			if (endFrames < 0) endFrames = 0;
			params.hitboxEnd = endFrames * FRAME_TIME_60FPS;
		}

		ImGui::SliderFloat2("Hitbox Offset", &params.hitboxOffset.x, -2.0f, 2.0f);
		ImGui::SliderFloat2("Hitbox Extents", &params.hitboxExtents.x, 0.1f, 2.0f);

		ImGui::InputInt("Damage", &params.damage);
		ImGui::InputInt("On-Hit Advantage", &params.hitFrame);
		ImGui::InputInt("On-Block Advantage", &params.blockFrame);

		ImGui::Separator();

		// --- 3. プレイヤー設定 ---
		ImGui::Text("Player Collision (Hurtbox)");
		XMFLOAT2 boxExtents = player->GetBoundingBoxExtents();
		if (ImGui::SliderFloat2("Hurtbox Extents", &boxExtents.x, 0.1f, 5.0f))
		{
			player->SetBoundingBoxExtents(boxExtents);
		}
		XMFLOAT2 boxOffset = player->GetBoundingBoxOffset();
		if (ImGui::SliderFloat2("Hurtbox Offset", &boxOffset.x, -5.0f, 5.0f))
		{
			player->SetBoundingBoxOffset(boxOffset);
		}
		float moveSpeed = player->GetMoveSpeed();
		if (ImGui::SliderFloat("Move Speed", &moveSpeed, 0.0f, 10.0f))
		{
			player->SetMoveSpeed(moveSpeed);
		}
		XMFLOAT3 scale = player->GetScale();
		if (ImGui::SliderFloat3("Scale", &scale.x, 0.1f, 5.0f))
		{
			player->SetScale(scale);
		}


		// --- 保存ボタン ---
		ImGui::Separator();
		if (ImGui::Button("SAVE All Settings"))
		{
			SavePlayerSettings();
		}
	}

	ImGui::End();
}