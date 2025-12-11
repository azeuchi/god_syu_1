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

// UI用テクスチャ管理
Texture* g_uiTex_debug = nullptr;

// 設定ファイルのパス
const char* SETTINGS_FILE_DEBUG = "player_settings.ini";

// 1フレームあたりの時間 (60FPS = 1/60秒 = 約0.0166秒)
const float FRAME_TIME_60FPS = 1.0f / 60.0f;

// 現在編集中の技を選択する変数
// 0: 弱パンチ (LightPunch)
// 1: 中パンチ (MediumPunch)
static int s_currentAttackType = 0;

/**
 * @brief プレイヤー設定の保存
 */
void SceneDebug::SavePlayerSettings()
{
	Player* player = GetObj<Player>("Player");
	if (player)
	{
		std::ofstream ofs(SETTINGS_FILE_DEBUG);
		if (ofs.is_open())
		{
			// 1. 基本設定
			ofs << player->GetMoveSpeed() << std::endl;
			DirectX::XMFLOAT3 scale = player->GetScale();
			ofs << scale.x << " " << scale.y << " " << scale.z << std::endl;

			// 2. 立ち判定
			for (int i = 0; i < (int)HurtboxType::COUNT; ++i) {
				DirectX::XMFLOAT2 ext = player->GetHurtboxBaseExtents((HurtboxType)i);
				DirectX::XMFLOAT2 off = player->GetHurtboxBaseOffset((HurtboxType)i);
				ofs << ext.x << " " << ext.y << " " << off.x << " " << off.y << std::endl;
			}

			// 3. しゃがみ判定
			for (int i = 0; i < (int)HurtboxType::COUNT; ++i) {
				DirectX::XMFLOAT2 ext = player->GetHurtboxCrouchExtents((HurtboxType)i);
				DirectX::XMFLOAT2 off = player->GetHurtboxCrouchOffset((HurtboxType)i);
				ofs << ext.x << " " << ext.y << " " << off.x << " " << off.y << std::endl;
			}

			// 4. 弱パンチ
			AttackParams& lParams = player->GetLightPunchParams();
			ofs << lParams.totalDuration << std::endl;
			ofs << lParams.hitboxStart << std::endl;
			ofs << lParams.hitboxEnd << std::endl;
			ofs << lParams.hitboxOffset.x << " " << lParams.hitboxOffset.y << std::endl;
			ofs << lParams.hitboxExtents.x << " " << lParams.hitboxExtents.y << std::endl;
			ofs << lParams.damage << std::endl;
			ofs << lParams.hitFrame << std::endl;
			ofs << lParams.blockFrame << std::endl;
			ofs << lParams.headOffsetVal.x << " " << lParams.headOffsetVal.y << std::endl;
			ofs << lParams.headSizeVal.x << " " << lParams.headSizeVal.y << std::endl;
			ofs << lParams.bodyOffsetVal.x << " " << lParams.bodyOffsetVal.y << std::endl;
			ofs << lParams.bodySizeVal.x << " " << lParams.bodySizeVal.y << std::endl;
			ofs << lParams.legsOffsetVal.x << " " << lParams.legsOffsetVal.y << std::endl;
			ofs << lParams.legsSizeVal.x << " " << lParams.legsSizeVal.y << std::endl;

			// 5. 中パンチ
			AttackParams& mParams = player->GetMediumPunchParams();
			ofs << mParams.totalDuration << std::endl;
			ofs << mParams.hitboxStart << std::endl;
			ofs << mParams.hitboxEnd << std::endl;
			ofs << mParams.hitboxOffset.x << " " << mParams.hitboxOffset.y << std::endl;
			ofs << mParams.hitboxExtents.x << " " << mParams.hitboxExtents.y << std::endl;
			ofs << mParams.damage << std::endl;
			ofs << mParams.hitFrame << std::endl;
			ofs << mParams.blockFrame << std::endl;
			ofs << mParams.headOffsetVal.x << " " << mParams.headOffsetVal.y << std::endl;
			ofs << mParams.headSizeVal.x << " " << mParams.headSizeVal.y << std::endl;
			ofs << mParams.bodyOffsetVal.x << " " << mParams.bodyOffsetVal.y << std::endl;
			ofs << mParams.bodySizeVal.x << " " << mParams.bodySizeVal.y << std::endl;
			ofs << mParams.legsOffsetVal.x << " " << mParams.legsOffsetVal.y << std::endl;
			ofs << mParams.legsSizeVal.x << " " << mParams.legsSizeVal.y << std::endl;

			ofs.close();
		}
	}
}

/**
 * @brief 初期化処理
 */
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
	for (int i = 0; i < 2; ++i)
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
	AttackParams lParams = player->GetLightPunchParams();
	AttackParams mParams = player->GetMediumPunchParams();

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
		for (int i = 0; i < (int)HurtboxType::COUNT; ++i) {
			DirectX::XMFLOAT2 ext, off;
			if (!ifs.eof()) ifs >> ext.x >> ext.y >> off.x >> off.y;
			player->SetHurtboxCrouch((HurtboxType)i, off, ext);
		}

		if (!ifs.eof()) ifs >> lParams.totalDuration;
		if (!ifs.eof()) ifs >> lParams.hitboxStart;
		if (!ifs.eof()) ifs >> lParams.hitboxEnd;
		if (!ifs.eof()) ifs >> lParams.hitboxOffset.x >> lParams.hitboxOffset.y;
		if (!ifs.eof()) ifs >> lParams.hitboxExtents.x >> lParams.hitboxExtents.y;
		if (!ifs.eof()) ifs >> lParams.damage;
		if (!ifs.eof()) ifs >> lParams.hitFrame;
		if (!ifs.eof()) ifs >> lParams.blockFrame;
		if (!ifs.eof()) ifs >> lParams.headOffsetVal.x >> lParams.headOffsetVal.y;
		if (!ifs.eof()) ifs >> lParams.headSizeVal.x >> lParams.headSizeVal.y;
		if (!ifs.eof()) ifs >> lParams.bodyOffsetVal.x >> lParams.bodyOffsetVal.y;
		if (!ifs.eof()) ifs >> lParams.bodySizeVal.x >> lParams.bodySizeVal.y;
		if (!ifs.eof()) ifs >> lParams.legsOffsetVal.x >> lParams.legsOffsetVal.y;
		if (!ifs.eof()) ifs >> lParams.legsSizeVal.x >> lParams.legsSizeVal.y;

		if (!ifs.eof()) ifs >> mParams.totalDuration;
		if (!ifs.eof()) ifs >> mParams.hitboxStart;
		if (!ifs.eof()) ifs >> mParams.hitboxEnd;
		if (!ifs.eof()) ifs >> mParams.hitboxOffset.x >> mParams.hitboxOffset.y;
		if (!ifs.eof()) ifs >> mParams.hitboxExtents.x >> mParams.hitboxExtents.y;
		if (!ifs.eof()) ifs >> mParams.damage;
		if (!ifs.eof()) ifs >> mParams.hitFrame;
		if (!ifs.eof()) ifs >> mParams.blockFrame;
		if (!ifs.eof()) ifs >> mParams.headOffsetVal.x >> mParams.headOffsetVal.y;
		if (!ifs.eof()) ifs >> mParams.headSizeVal.x >> mParams.headSizeVal.y;
		if (!ifs.eof()) ifs >> mParams.bodyOffsetVal.x >> mParams.bodyOffsetVal.y;
		if (!ifs.eof()) ifs >> mParams.bodySizeVal.x >> mParams.bodySizeVal.y;
		if (!ifs.eof()) ifs >> mParams.legsOffsetVal.x >> mParams.legsOffsetVal.y;
		if (!ifs.eof()) ifs >> mParams.legsSizeVal.x >> mParams.legsSizeVal.y;

		ifs.close();
	}

	player->SetMoveSpeed(moveSpeed);
	player->SetScale(scale);
	player->GetLightPunchParams() = lParams;
	player->GetMediumPunchParams() = mParams;

	player->Load("Assets/Model/knight/Idle.fbx", 0.02f, true, false);
	player->GetModel()->LoadAnimation("Assets/Model/knight/LightPunch.fbx", "LightPunch", true);
	player->GetModel()->LoadAnimation("Assets/Model/knight/MediumPunch.fbx", "MediumPunch", true);
	player->GetModel()->LoadAnimation("Assets/Model/knight/CrouchIdle.fbx", "CrouchIdle", true);

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
	if (g_uiTex_debug) { delete g_uiTex_debug; g_uiTex_debug = nullptr; }
}

void SceneDebug::Update(float tick)
{
	if (tick > 0.0f) { m_fps = 1.0f / tick; }
	else { m_fps = 0.0f; }

	if (IsKeyTrigger(VK_TAB)) { m_showImGui = !m_showImGui; }

	Player* player = GetObj<Player>("Player");
	if (!player) return;

	player->SetActiveHitbox(m_isAttacking);
	if (s_currentAttackType == 0) player->SetCurrentAttackParams(&player->GetLightPunchParams());
	else player->SetCurrentAttackParams(&player->GetMediumPunchParams());

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
		AttackParams* params = player->GetCurrentAttackParams();
		int animLengthInFrames = static_cast<int>(std::round(params->totalDuration / FRAME_TIME_60FPS));
		if (animLengthInFrames <= 0) animLengthInFrames = 1;

		if (m_currentFrame >= (animLengthInFrames - 1))
		{
			player->Debug_SetAnimation("Idle", true);
			m_isAttacking = false;
			m_isPaused = false;
			m_currentFrame = 0;
			player->SetCurrentAttackParams(nullptr);
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
	DirectX::XMFLOAT4 light[] = { pLight->GetDiffuse(), pLight->GetAmbient(), {lightDir.x, lightDir.y, lightDir.z, 0.0f} };
	DirectX::XMFLOAT3 camPos = pCamera->GetPos();
	DirectX::XMFLOAT4 camera[] = { {camPos.x, camPos.y, camPos.z, 0.0f} };
	Shader* shader[] = { GetObj<Shader>("VS_SkinMeshAnimation"), GetObj<Shader>("PS_TexColor") };

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
			AttackParams* params = player->GetCurrentAttackParams();
			int startFrame = static_cast<int>(std::round(params->hitboxStart / FRAME_TIME_60FPS));
			int endFrame = static_cast<int>(std::round(params->hitboxEnd / FRAME_TIME_60FPS));
			if (m_currentFrame >= startFrame && m_currentFrame < endFrame)
			{
				player->UpdateHitbox(params->hitboxOffset, params->hitboxExtents);
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

	ImGui::Begin("Debug Settings");
	ImGui::Text("FPS: %.1f", m_fps);

	if (!player) { ImGui::End(); return; }

	// --- Animation Control ---
	if (ImGui::CollapsingHeader("Animation Control", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::Text("Select Attack:");
		ImGui::RadioButton("Light Punch", &s_currentAttackType, 0);
		ImGui::SameLine();
		ImGui::RadioButton("Medium Punch", &s_currentAttackType, 1);

		if (!m_isAttacking)
		{
			if (ImGui::Button("Test Play")) {
				const char* animName = (s_currentAttackType == 0) ? "LightPunch" : "MediumPunch";
				player->Debug_SetAnimation(animName, true);
				m_isAttacking = true;
				m_isPaused = false;
				m_currentFrame = 0;
				m_animTimer = 0.0f;
				player->SetIsCrouching(false);
			}
			ImGui::SameLine();
			if (ImGui::Button("Test Crouch")) {
				player->Debug_SetAnimation("CrouchIdle", true);
				player->SetIsCrouching(true);
				m_isAttacking = false;
				m_isPaused = false;
			}
			ImGui::SameLine();
			if (ImGui::Button("Test Stand")) {
				player->Debug_SetAnimation("Idle", true);
				player->SetIsCrouching(false);
				m_isAttacking = false;
				m_isPaused = false;
			}
			ImGui::SameLine();
			if (ImGui::Button("Step Play")) {
				const char* animName = (s_currentAttackType == 0) ? "LightPunch" : "MediumPunch";
				player->Debug_SetAnimation(animName, true);
				m_isAttacking = true;
				m_isPaused = true;
				m_currentFrame = 0;
				m_animTimer = 0.0f;
				player->SetIsCrouching(false);
			}
		}
		else
		{
			ImGui::TextColored(ImVec4(1, 0, 0, 1), ">> ATTACKING <<");
		}

		ImGui::SameLine();
		ImGui::Checkbox("Pause", &m_isPaused);

		if (m_isPaused) {
			ImGui::SameLine();
			if (ImGui::Button("+1 Frame")) m_currentFrame++;
			ImGui::InputInt("Frame", &m_currentFrame);
		}
		else {
			ImGui::Text("Frame: %d", m_currentFrame);
		}
	}

	// --- Attack Parameters ---
	if (ImGui::CollapsingHeader("Attack Parameters", ImGuiTreeNodeFlags_DefaultOpen))
	{
		AttackParams& params = (s_currentAttackType == 0) ? player->GetLightPunchParams() : player->GetMediumPunchParams();
		ImGui::Text(s_currentAttackType == 0 ? "Light Punch" : "Medium Punch");

		int totalFrames = static_cast<int>(std::round(params.totalDuration / FRAME_TIME_60FPS));
		int startFrames = static_cast<int>(std::round(params.hitboxStart / FRAME_TIME_60FPS));
		int endFrames = static_cast<int>(std::round(params.hitboxEnd / FRAME_TIME_60FPS));

		if (ImGui::InputInt("Total Frames", &totalFrames)) {
			if (totalFrames < 1) totalFrames = 1;
			params.totalDuration = totalFrames * FRAME_TIME_60FPS;
		}
		if (ImGui::InputInt("Start Frame", &startFrames)) {
			if (startFrames < 0) startFrames = 0;
			params.hitboxStart = startFrames * FRAME_TIME_60FPS;
		}
		if (ImGui::InputInt("End Frame", &endFrames)) {
			if (endFrames < 0) endFrames = 0;
			params.hitboxEnd = endFrames * FRAME_TIME_60FPS;
		}

		ImGui::Text("Hitbox (Red Box)");
		ImGui::SliderFloat2("Offset", &params.hitboxOffset.x, -2.0f, 2.0f);
		ImGui::SliderFloat2("Extents", &params.hitboxExtents.x, 0.1f, 2.0f);

		ImGui::Text("Game Data");
		ImGui::InputInt("Damage", &params.damage);
		ImGui::InputInt("Hit Adv", &params.hitFrame);
		ImGui::InputInt("Block Adv", &params.blockFrame);

		if (ImGui::TreeNode("Hurtbox Modifiers (Attack)")) {
			ImGui::Text("Head");
			ImGui::SliderFloat2("Offset##H", &params.headOffsetVal.x, -1.0f, 1.0f);
			ImGui::SliderFloat2("Size+##H", &params.headSizeVal.x, -1.0f, 1.0f);
			ImGui::Text("Body");
			ImGui::SliderFloat2("Offset##B", &params.bodyOffsetVal.x, -1.0f, 1.0f);
			ImGui::SliderFloat2("Size+##B", &params.bodySizeVal.x, -1.0f, 1.0f);
			ImGui::Text("Legs");
			ImGui::SliderFloat2("Offset##L", &params.legsOffsetVal.x, -1.0f, 1.0f);
			ImGui::SliderFloat2("Size+##L", &params.legsSizeVal.x, -1.0f, 1.0f);
			ImGui::TreePop();
		}
	}

	// --- Hurtbox Settings ---
	if (ImGui::CollapsingHeader("Base Hurtbox Settings"))
	{
		const char* hurtboxNames[] = { "Head", "Body", "Legs" };

		if (ImGui::TreeNode("Standing (Base)"))
		{
			for (int i = 0; i < (int)HurtboxType::COUNT; ++i) {
				ImGui::PushID(i);
				ImGui::Text("%s", hurtboxNames[i]);
				DirectX::XMFLOAT2 ext = player->GetHurtboxBaseExtents((HurtboxType)i);
				DirectX::XMFLOAT2 off = player->GetHurtboxBaseOffset((HurtboxType)i);
				bool changed = false;
				if (ImGui::SliderFloat2("Extents", &ext.x, 0.1f, 5.0f)) changed = true;
				if (ImGui::SliderFloat2("Offset", &off.x, -5.0f, 5.0f)) changed = true;
				if (changed) player->SetHurtboxBase((HurtboxType)i, off, ext);
				ImGui::PopID();
			}
			ImGui::TreePop();
		}

		if (ImGui::TreeNode("Crouching"))
		{
			for (int i = 0; i < (int)HurtboxType::COUNT; ++i) {
				ImGui::PushID(100 + i);
				ImGui::Text("%s", hurtboxNames[i]);
				DirectX::XMFLOAT2 ext = player->GetHurtboxCrouchExtents((HurtboxType)i);
				DirectX::XMFLOAT2 off = player->GetHurtboxCrouchOffset((HurtboxType)i);
				bool changed = false;
				if (ImGui::SliderFloat2("Extents", &ext.x, 0.1f, 5.0f)) changed = true;
				if (ImGui::SliderFloat2("Offset", &off.x, -5.0f, 5.0f)) changed = true;
				if (changed) player->SetHurtboxCrouch((HurtboxType)i, off, ext);
				ImGui::PopID();
			}
			ImGui::TreePop();
		}
	}

	ImGui::Separator();
	if (ImGui::Button("SAVE ALL SETTINGS", ImVec2(-1, 40)))
	{
		SavePlayerSettings();
	}

	ImGui::End();
}