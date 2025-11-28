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
const float FRAME_TIME_60FPS = 1.0f / 60.0f;

void SceneDebug::SavePlayerSettings()
{
	Player* player = GetObj<Player>("Player");
	if (player)
	{
		std::ofstream ofs(SETTINGS_FILE_DEBUG);
		if (ofs.is_open())
		{
			// 既存設定
			ofs << player->GetMoveSpeed() << std::endl;
			DirectX::XMFLOAT3 scale = player->GetScale();
			ofs << scale.x << " " << scale.y << " " << scale.z << std::endl;

			// 3部位の基本設定
			for (int i = 0; i < (int)HurtboxType::COUNT; ++i) {
				DirectX::XMFLOAT2 ext = player->GetHurtboxBaseExtents((HurtboxType)i);
				DirectX::XMFLOAT2 off = player->GetHurtboxBaseOffset((HurtboxType)i);
				ofs << ext.x << " " << ext.y << " " << off.x << " " << off.y << std::endl;
			}

			// AttackParams
			AttackParams& params = player->GetLightPunchParams();
			ofs << params.totalDuration << std::endl;
			ofs << params.hitboxStart << std::endl;
			ofs << params.hitboxEnd << std::endl;
			ofs << params.hitboxOffset.x << " " << params.hitboxOffset.y << std::endl;
			ofs << params.hitboxExtents.x << " " << params.hitboxExtents.y << std::endl;
			ofs << params.damage << std::endl;
			ofs << params.hitFrame << std::endl;
			ofs << params.blockFrame << std::endl;

			// 攻撃中のくらい判定補正 (Offset + Size)
			ofs << params.headOffsetVal.x << " " << params.headOffsetVal.y << std::endl;
			ofs << params.headSizeVal.x << " " << params.headSizeVal.y << std::endl; // ★追加

			ofs << params.bodyOffsetVal.x << " " << params.bodyOffsetVal.y << std::endl;
			ofs << params.bodySizeVal.x << " " << params.bodySizeVal.y << std::endl; // ★追加

			ofs << params.legsOffsetVal.x << " " << params.legsOffsetVal.y << std::endl;
			ofs << params.legsSizeVal.x << " " << params.legsSizeVal.y << std::endl; // ★追加

			ofs.close();
		}
	}
}

void SceneDebug::Init()
{
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

	CreateObj<Player>("Player");
	Player* player = GetObj<Player>("Player");
	player->SetInputType(PlayerInputType::AI);

	float moveSpeed = 2.0f;
	DirectX::XMFLOAT3 scale = { 1.0f, 1.0f, 1.0f };
	AttackParams params = player->GetLightPunchParams();

	std::ifstream ifs(SETTINGS_FILE_DEBUG);
	if (ifs.is_open())
	{
		ifs >> moveSpeed;
		ifs >> scale.x >> scale.y >> scale.z;

		for (int i = 0; i < (int)HurtboxType::COUNT; ++i) {
			DirectX::XMFLOAT2 ext, off;
			if (!ifs.eof()) ifs >> ext.x >> ext.y >> off.x >> off.y;
			player->SetHurtboxBase((HurtboxType)i, off, ext);
		}

		if (!ifs.eof()) ifs >> params.totalDuration;
		if (!ifs.eof()) ifs >> params.hitboxStart;
		if (!ifs.eof()) ifs >> params.hitboxEnd;
		if (!ifs.eof()) ifs >> params.hitboxOffset.x >> params.hitboxOffset.y;
		if (!ifs.eof()) ifs >> params.hitboxExtents.x >> params.hitboxExtents.y;
		if (!ifs.eof()) ifs >> params.damage;
		if (!ifs.eof()) ifs >> params.hitFrame;
		if (!ifs.eof()) ifs >> params.blockFrame;

		// 補正値読み込み
		if (!ifs.eof()) ifs >> params.headOffsetVal.x >> params.headOffsetVal.y;
		if (!ifs.eof()) ifs >> params.headSizeVal.x >> params.headSizeVal.y; // ★追加

		if (!ifs.eof()) ifs >> params.bodyOffsetVal.x >> params.bodyOffsetVal.y;
		if (!ifs.eof()) ifs >> params.bodySizeVal.x >> params.bodySizeVal.y; // ★追加

		if (!ifs.eof()) ifs >> params.legsOffsetVal.x >> params.legsOffsetVal.y;
		if (!ifs.eof()) ifs >> params.legsSizeVal.x >> params.legsSizeVal.y; // ★追加

		ifs.close();
	}

	player->SetMoveSpeed(moveSpeed);
	player->SetScale(scale);
	player->GetLightPunchParams() = params;

	player->Load("Assets/Model/knight/Idle.fbx", 0.02f, true, false);
	player->GetModel()->LoadAnimation("Assets/Model/knight/LightPunch.fbx", "LightPunch", true);

	player->Debug_SetAnimation("Idle", true);

	player->SetPosition({ 0.0f, 0.0f, 0.0f });
	player->SetRotation({ 0.0f, DirectX::XM_PI / -2.0f, 0.0f });

	m_showImGui = true;
	g_uiTex_debug = new Texture();

	m_isAttacking = false;
	m_isPaused = false;
	m_currentFrame = 0;
	m_animTimer = 0.0f;
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

	player->SetActiveHitbox(m_isAttacking);

	if (m_isPaused)
	{
		player->Debug_SetFrame(m_currentFrame);
		m_animTimer = 0.0f;
	}
	else
	{
		m_animTimer += tick;
		if (m_animTimer >= FRAME_TIME_60FPS)
		{
			while (m_animTimer >= FRAME_TIME_60FPS)
			{
				player->UpdateAnimation(FRAME_TIME_60FPS);
				m_animTimer -= FRAME_TIME_60FPS;
			}
		}
	}

	player->UpdateModelBlend();

	if (!m_isPaused)
	{
		m_currentFrame = player->Debug_GetFrame();
	}

	if (m_isAttacking)
	{
		float totalDuration = player->GetLightPunchParams().totalDuration;
		int animLengthInFrames = static_cast<int>(std::round(totalDuration / FRAME_TIME_60FPS));
		if (animLengthInFrames <= 0) animLengthInFrames = 1;

		if (m_currentFrame >= (animLengthInFrames - 1))
		{
			player->Debug_SetAnimation("Idle", true);
			m_isAttacking = false;
			m_isPaused = false;
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
		player->DrawBoundingBox();

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
		ImGui::Text("Action Test");

		if (!m_isAttacking)
		{
			if (ImGui::Button("Test Play"))
			{
				player->Debug_SetAnimation("LightPunch", true);
				m_isAttacking = true;
				m_isPaused = false;
				m_currentFrame = 0;
				m_animTimer = 0.0f;
			}

			ImGui::SameLine();

			if (ImGui::Button("Test Play (Step)"))
			{
				player->Debug_SetAnimation("LightPunch", true);
				m_isAttacking = true;
				m_isPaused = true;
				m_currentFrame = 0;
				m_animTimer = 0.0f;
			}
		}
		else
		{
			ImGui::TextColored(ImVec4(1, 0, 0, 1), ">> ATTACKING <<");
		}

		ImGui::SameLine();

		ImGui::Checkbox("Pause", &m_isPaused);

		if (m_isPaused)
		{
			ImGui::SameLine();
			if (ImGui::Button("+1 Frame"))
			{
				m_currentFrame++;
			}
			ImGui::InputInt("Current Frame", &m_currentFrame);
		}
		else
		{
			ImGui::Text("Frame: %d", m_currentFrame);
		}


		ImGui::Separator();

		// --- パラメータ調整 ---
		ImGui::Text("Light Punch Parameters");
		AttackParams& params = player->GetLightPunchParams();

		int totalFrames = static_cast<int>(std::round(params.totalDuration / FRAME_TIME_60FPS));
		int startFrames = static_cast<int>(std::round(params.hitboxStart / FRAME_TIME_60FPS));
		int endFrames = static_cast<int>(std::round(params.hitboxEnd / FRAME_TIME_60FPS));

		if (ImGui::InputInt("Total Duration", &totalFrames))
		{
			if (totalFrames < 1) totalFrames = 1;
			params.totalDuration = totalFrames * FRAME_TIME_60FPS;
		}
		if (ImGui::InputInt("Hitbox Start", &startFrames))
		{
			if (startFrames < 0) startFrames = 0;
			params.hitboxStart = startFrames * FRAME_TIME_60FPS;
		}
		if (ImGui::InputInt("Hitbox End", &endFrames))
		{
			if (endFrames < 0) endFrames = 0;
			params.hitboxEnd = endFrames * FRAME_TIME_60FPS;
		}

		ImGui::Text("Red Box (Hitbox)");
		ImGui::SliderFloat2("Hitbox Offset", &params.hitboxOffset.x, -2.0f, 2.0f);
		ImGui::SliderFloat2("Hitbox Extents", &params.hitboxExtents.x, 0.1f, 2.0f);

		ImGui::InputInt("Damage", &params.damage);
		ImGui::InputInt("On-Hit Advantage", &params.hitFrame);
		ImGui::InputInt("On-Block Advantage", &params.blockFrame);

		ImGui::Separator();

		// ★追加: 攻撃中のくらい判定補正 (Offset & Size)
		ImGui::Text("Green Box Modifiers (Attack Only)");

		ImGui::Text("Head");
		ImGui::SliderFloat2("Head Offset", &params.headOffsetVal.x, -1.0f, 1.0f);
		ImGui::SliderFloat2("Head Size+", &params.headSizeVal.x, -1.0f, 1.0f); // ★追加

		ImGui::Text("Body");
		ImGui::SliderFloat2("Body Offset", &params.bodyOffsetVal.x, -1.0f, 1.0f);
		ImGui::SliderFloat2("Body Size+", &params.bodySizeVal.x, -1.0f, 1.0f); // ★追加

		ImGui::Text("Legs");
		ImGui::SliderFloat2("Legs Offset", &params.legsOffsetVal.x, -1.0f, 1.0f);
		ImGui::SliderFloat2("Legs Size+", &params.legsSizeVal.x, -1.0f, 1.0f); // ★追加

		ImGui::Separator();

		// ★基本設定 (Base Settings)
		ImGui::Text("Player Base Hurtbox Settings");
		const char* hurtboxNames[] = { "Head", "Body", "Legs" };
		for (int i = 0; i < (int)HurtboxType::COUNT; ++i)
		{
			ImGui::PushID(i);
			ImGui::Text("%s", hurtboxNames[i]);

			DirectX::XMFLOAT2 ext = player->GetHurtboxBaseExtents((HurtboxType)i);
			DirectX::XMFLOAT2 off = player->GetHurtboxBaseOffset((HurtboxType)i);

			if (ImGui::SliderFloat2("Base Extents", &ext.x, 0.1f, 5.0f)) {
				player->SetHurtboxBase((HurtboxType)i, off, ext);
			}
			if (ImGui::SliderFloat2("Base Offset", &off.x, -5.0f, 5.0f)) {
				player->SetHurtboxBase((HurtboxType)i, off, ext);
			}
			ImGui::PopID();
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