// SceneDebug.cpp
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
#include <string>
#include <cstdio>
#include "PlayerParameterLoader.h"

using namespace DirectX;
using namespace DirectX::SimpleMath;

Texture* g_uiTex_debug = nullptr;

const float FRAME_TIME_60FPS = 1.0f / 60.0f;

// 0:LP, 1:MP, 2:HP, 3:MK, 4:HK, 5:Down, 6:WakeUp, 7:HadouL, 8:HadouM, 9:HadouH
static int s_currentAttackType = 0;

static const char* GetCurrentAnimName()
{
	if (s_currentAttackType == 0) return "LightPunch";
	if (s_currentAttackType == 1) return "MediumPunch";
	if (s_currentAttackType == 2) return "HeavyPunch";
	if (s_currentAttackType == 3) return "MediumKick";
	if (s_currentAttackType == 4) return "HeavyKick";
	if (s_currentAttackType == 5) return "Down";
	if (s_currentAttackType == 6) return "WakeUp";
	return "Hadouken";
}

// 判定ボックス(BoundingBox)のリストを、指定色のワイヤーフレームで描画する
// 既存の DrawHitbox / DrawBoundingBox と同じく前面4辺だけを線で描く
static void DrawBoxListWire(const std::vector<DirectX::BoundingBox>& boxes, const DirectX::XMFLOAT4& color)
{
	Geometory::SetColor(color);
	static const int edge[4][2] = { {0,1},{1,2},{2,3},{3,0} };
	for (const auto& box : boxes)
	{
		DirectX::XMFLOAT3 corners[8];
		box.GetCorners(corners);
		for (int e = 0; e < 4; ++e)
		{
			Geometory::AddLine(corners[edge[e][0]], corners[edge[e][1]]);
		}
	}
}

void SceneDebug::SavePlayerSettings()
{
	Player* player = GetObj<Player>("Player");
	if (!player) return;

	// 1. 共通設定の保存
	{
		std::ofstream ofs("Param_Common.ini");
		if (ofs.is_open())
		{
			ofs << player->GetMoveSpeed() << std::endl;
			DirectX::XMFLOAT3 scale = player->GetScale();
			ofs << scale.x << " " << scale.y << " " << scale.z << std::endl;

			// 立ち(Base)のくらい判定
			for (int i = 0; i < (int)HurtboxType::COUNT; ++i) {
				DirectX::XMFLOAT2 ext = player->GetHurtboxBaseExtents((HurtboxType)i);
				DirectX::XMFLOAT2 off = player->GetHurtboxBaseOffset((HurtboxType)i);
				ofs << ext.x << " " << ext.y << " " << off.x << " " << off.y << std::endl;
			}

			// しゃがみ(Crouch)のくらい判定
			for (int i = 0; i < (int)HurtboxType::COUNT; ++i) {
				DirectX::XMFLOAT2 ext = player->GetHurtboxCrouchExtents((HurtboxType)i);
				DirectX::XMFLOAT2 off = player->GetHurtboxCrouchOffset((HurtboxType)i);
				ofs << ext.x << " " << ext.y << " " << off.x << " " << off.y << std::endl;
			}
			ofs.close();
		}
	}

	// 2. 各攻撃パラメータの保存用ヘルパー関数
	auto SaveOneAttack = [&](const char* filename, const AttackParams& params) {
		std::ofstream ofs(filename);
		if (!ofs.is_open()) return;

		// 形式バージョン(新形式の目印)
		ofs << "FGPARAM 2" << std::endl;

		// タイミング・判定
		ofs << params.totalDuration << std::endl;
		ofs << params.hitboxStart << std::endl;
		ofs << params.hitboxEnd << std::endl;

		// 攻撃判定(Hitbox)はキーフレーム式のまま保存
		auto WriteAnimatedBoxes = [&](const std::vector<AnimatedBox>& boxes) {
			ofs << boxes.size() << std::endl;
			for (const auto& abox : boxes) {
				ofs << abox.keyframes.size() << std::endl;
				for (const auto& key : abox.keyframes) {
					ofs << key.frame << std::endl;
					ofs << key.data.offset.x << " " << key.data.offset.y << std::endl;
					ofs << key.data.extents.x << " " << key.data.extents.y << std::endl;
				}
			}
			};
		WriteAnimatedBoxes(params.hitboxes);

		// くらい判定(Hurtbox)は区間付き固定ボックスとして保存
		ofs << params.moveHurtboxes.size() << std::endl;
		for (const auto& wh : params.moveHurtboxes) {
			ofs << wh.offset.x << " " << wh.offset.y << " "
				<< wh.extents.x << " " << wh.extents.y << " "
				<< wh.startFrame << " " << wh.endFrame << std::endl;
		}

		// ゲームバランス
		ofs << params.damage << std::endl;
		ofs << params.hitFrame << std::endl;
		ofs << params.blockFrame << std::endl;
		ofs << params.hitStop << std::endl;
		ofs << params.knockback << std::endl;
		ofs << params.isDown << std::endl;

		// キャンセル設定
		ofs << params.cancelEnabled << std::endl;
		ofs << params.cancelStart << std::endl;
		ofs << params.cancelEnd << std::endl;
		ofs << params.cancelToLight << " " << params.cancelToMedium << " " << params.cancelToHeavyPunch << " " << params.cancelToMediumKick << " " << params.cancelToHeavy << std::endl;

		// 速度変化リスト
		size_t speedModCount = params.speedModifiers.size();
		ofs << speedModCount << std::endl;
		for (const auto& mod : params.speedModifiers) {
			ofs << mod.startFrame << " " << mod.endFrame << " " << mod.speed << std::endl;
		}

		// 飛び道具設定
		ofs << params.projectileSpeed << std::endl;
		ofs << params.projectileSize << std::endl;

		ofs.close();
		};

	// 3. 各ファイルの保存実行
	SaveOneAttack("Param_LightPunch.ini", player->GetLightPunchParams());
	SaveOneAttack("Param_MediumPunch.ini", player->GetMediumPunchParams());
	SaveOneAttack("Param_HeavyPunch.ini", player->GetHeavyPunchParams());
	SaveOneAttack("Param_MediumKick.ini", player->GetMediumKickParams());
	SaveOneAttack("Param_HeavyKick.ini", player->GetHeavyKickParams());

	SaveOneAttack("Param_HadoukenL.ini", player->GetHadoukenLParams());
	SaveOneAttack("Param_HadoukenM.ini", player->GetHadoukenMParams());
	SaveOneAttack("Param_HadoukenH.ini", player->GetHadoukenHParams());

	DebugLog::log(DebugLog::INFO_LOG, "設定を分割ファイルとして保存しました。");
}

void SceneDebug::Init()
{
	// --- シェーダー読み込み ---
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

	// --- プレイヤー生成 ---
	CreateObj<Player>("Player");
	Player* player = GetObj<Player>("Player");

	player->SetInputType(PlayerInputType::AI);

	// --- 設定ファイルからパラメータ読み込み ---
	PlayerParameterLoader::LoadSettings(player);

	// --- モデル・アニメーションロード ---
	player->Load("Assets/Model/knight/Idle.fbx", 0.014f, true, false);
	player->GetModel()->LoadAnimation("Assets/Model/knight/LightPunch.fbx", "LightPunch", true);
	player->GetModel()->LoadAnimation("Assets/Model/knight/MediumPunch.fbx", "MediumPunch", true);
	player->GetModel()->LoadAnimation("Assets/Model/knight/HeavyPunch.fbx", "HeavyPunch", true);
	player->GetModel()->LoadAnimation("Assets/Model/knight/MediumKick.fbx", "MediumKick", true);
	player->GetModel()->LoadAnimation("Assets/Model/knight/HeavyKick.fbx", "HeavyKick", true);
	player->GetModel()->LoadAnimation("Assets/Model/knight/CrouchIdle.fbx", "CrouchIdle", true);
	player->GetModel()->LoadAnimation("Assets/Model/knight/Down.fbx", "Down", true);
	player->GetModel()->LoadAnimation("Assets/Model/knight/WakeUp.fbx", "WakeUp", true);
	player->GetModel()->LoadAnimation("Assets/Model/knight/Hadouken.fbx", "Hadouken", true);

	// 初期状態は Idle (待機)
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

	// --- パラメータの同期 ---
	AttackParams* params = GetSelectedParams(player);

	player->SetCurrentAttackParams(params);

	// --- 速度計算用ロジック ---
	float speed = 1.0f;
	const char* animName = GetCurrentAnimName();

	if (m_isAttacking || s_currentAttackType <= 4 || s_currentAttackType >= 7) {
		int totalAnimFrames = player->GetModel()->GetAnimationTotalFrame(animName);

		if (params) {
			float targetFrames = params->totalDuration * 60.0f;
			if (targetFrames <= 1.0f) targetFrames = 1.0f;
			speed = (float)totalAnimFrames / targetFrames;
		}
	}

	player->SetAnimationSpeed(speed);

	// --- アニメーション制御 ---
	if (!m_isPaused && m_isAttacking)
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

		float currentModelFrame = (float)player->Debug_GetFrame();
		if (speed > 0.0f) {
			m_currentFrame = static_cast<int>(currentModelFrame / speed);
		}
		else {
			m_currentFrame = (int)currentModelFrame;
		}
	}
	else
	{
		int modelFrame = static_cast<int>(m_currentFrame * speed);
		player->Debug_SetAnimation(animName, false);
		player->Debug_SetFrame(modelFrame);
	}

	player->SetAttackTimer((float)m_currentFrame);

	player->UpdateModelBlend();
	player->UpdateAttackBoxes();

	// --- 攻撃終了判定 ---
	if (m_isAttacking)
	{
		int totalGameFrames = 0;
		if (params) {
			totalGameFrames = static_cast<int>(std::round(params->totalDuration * 60.0f));
		}
		else {
			totalGameFrames = player->GetModel()->GetAnimationTotalFrame(GetCurrentAnimName());
		}

		if (m_currentFrame >= totalGameFrames)
		{
			player->Debug_SetAnimation("Idle", true);
			m_isAttacking = false;
			m_isPaused = false;
			m_currentFrame = 0;
		}
	}

	CameraBase* camera = GetObj<CameraBase>("Camera");
	if (camera)
	{
		DirectX::XMFLOAT3 playerPos = player->GetPosition();
		float targetZ = -3.5f;
		float heightOffset = 1.2f;
		DirectX::XMFLOAT3 targetPos = { playerPos.x, playerPos.y + heightOffset, targetZ };
		DirectX::XMFLOAT3 targetLook = { playerPos.x, playerPos.y + 1.0f, 0.0f };
		camera->SetPos(targetPos);
		camera->SetLook(targetLook);
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

		// ----------------------------------------------------
		// 判定ボックスの表示制御
		// ストリートファイターのトレーニングモードのように、現在フレームの
		// くらい判定(緑)・攻撃判定(赤)を常に重ねて表示する。
		// ----------------------------------------------------
		bool isEditingAttack = (s_currentAttackType <= 4 || s_currentAttackType >= 7);

		if (isEditingAttack) {
			// 攻撃判定は発生期間内だけ「出ている」ものとして扱う
			bool showHitbox = false;
			AttackParams* pParams = player->GetCurrentAttackParams();
			if (pParams) {
				float currentTime = (float)m_currentFrame * FRAME_TIME_60FPS;
				showHitbox = (currentTime >= pParams->hitboxStart && currentTime < pParams->hitboxEnd);
			}
			player->SetActiveHitbox(showHitbox);

			// 表示用の判定を毎フレーム作り直す (区間外でも追加くらいを正しく反映するため)
			player->UpdateAttackBoxes();

			// 基本のくらい判定は常時表示。技固有の追加くらい判定は区間中だけ重ねて描く
			player->DrawBoundingBox();
			DrawBoxListWire(player->GetActiveHurtboxes(), XMFLOAT4(0.1f, 0.9f, 0.2f, 1.0f));

			// 攻撃判定は赤で描く (発生期間外は中身が空なので何も出ない)
			DrawBoxListWire(player->GetActiveHitboxes(), XMFLOAT4(1.0f, 0.15f, 0.1f, 1.0f));
		}
		else {
			// Down / WakeUp などは基本のくらい判定のみ表示する
			player->SetActiveHitbox(false);
			player->DrawBoundingBox();
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

	if (!player) {
		ImGui::End();
		return;
	}

	if (ImGui::CollapsingHeader("Animation Control", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::Text("Select Action:");

		ImGui::Text("Punch:"); ImGui::SameLine();
		ImGui::RadioButton("L##P", &s_currentAttackType, 0); ImGui::SameLine();
		ImGui::RadioButton("M##P", &s_currentAttackType, 1); ImGui::SameLine();
		ImGui::RadioButton("H##P", &s_currentAttackType, 2);

		ImGui::Text("Kick :"); ImGui::SameLine();
		ImGui::RadioButton("M##K", &s_currentAttackType, 3); ImGui::SameLine();
		ImGui::RadioButton("H##K", &s_currentAttackType, 4);

		ImGui::Text("Special:"); ImGui::SameLine();
		ImGui::RadioButton("Hadou L", &s_currentAttackType, 7); ImGui::SameLine();
		ImGui::RadioButton("Hadou M", &s_currentAttackType, 8); ImGui::SameLine();
		ImGui::RadioButton("Hadou H", &s_currentAttackType, 9);

		ImGui::Text("Other:"); ImGui::SameLine();
		ImGui::RadioButton("Down", &s_currentAttackType, 5); ImGui::SameLine();
		ImGui::RadioButton("WakeUp", &s_currentAttackType, 6);

		if (!m_isAttacking)
		{
			auto StartAttack = [&](bool startPaused) {
				const char* animName = GetCurrentAnimName();
				player->Debug_SetAnimation(animName, true);

				m_isAttacking = true;
				m_isPaused = startPaused;
				m_currentFrame = 0;
				m_animTimer = 0.0f;
				player->SetIsCrouching(false);
				};

			if (ImGui::Button("Test Play")) StartAttack(false);
			ImGui::SameLine();
			if (ImGui::Button("Step Play")) StartAttack(true);
		}
		else
		{
			ImGui::TextColored(ImVec4(1, 0, 0, 1), ">> PLAYING <<");
			ImGui::SameLine();
			if (ImGui::Button("Stop")) {
				m_isAttacking = false;
				player->Debug_SetAnimation("Idle", true);
			}
		}

		ImGui::SameLine();
		ImGui::Checkbox("Pause", &m_isPaused);

		if (m_isPaused || !m_isAttacking)
		{
			ImGui::SameLine();
			if (ImGui::Button("+1")) m_currentFrame++;
			ImGui::SameLine();
			if (ImGui::Button("-1")) m_currentFrame--;

			ImGui::InputInt("Frame", &m_currentFrame);
			if (m_currentFrame < 0) m_currentFrame = 0;
		}
		else
		{
			ImGui::Text("Frame: %d", m_currentFrame);
		}

		// タイムライン (横軸でタイミングを確認し、クリック／ドラッグで頭出し)
		ImGui::Separator();
		DrawTimeline(GetSelectedParams(player));
	}

	AttackParams* pParams = GetSelectedParams(player);

	if (pParams && ImGui::CollapsingHeader("Attack Parameters", ImGuiTreeNodeFlags_DefaultOpen))
	{
		AttackParams& params = *pParams;

		int totalFrames = static_cast<int>(std::round(params.totalDuration / FRAME_TIME_60FPS));
		int startFrames = static_cast<int>(std::round(params.hitboxStart / FRAME_TIME_60FPS));
		int endFrames = static_cast<int>(std::round(params.hitboxEnd / FRAME_TIME_60FPS));
		int hitStopFrames = static_cast<int>(std::round(params.hitStop / FRAME_TIME_60FPS));

		if (ImGui::InputInt("Total Frames", &totalFrames)) {
			if (totalFrames < 1) totalFrames = 1;
			params.totalDuration = totalFrames * FRAME_TIME_60FPS;
		}

		if (s_currentAttackType <= 4) {
			ImGui::Text("Hit Active Range (Ref):");
			if (ImGui::InputInt("Start Frame", &startFrames)) {
				if (startFrames < 0) startFrames = 0;
				params.hitboxStart = startFrames * FRAME_TIME_60FPS;
			}
			if (ImGui::InputInt("End Frame", &endFrames)) {
				if (endFrames < 0) endFrames = 0;
				params.hitboxEnd = endFrames * FRAME_TIME_60FPS;
			}
		}
		else {
			ImGui::Text("Projectile Settings:");
			ImGui::SliderFloat("Proj Speed", &params.projectileSpeed, 1.0f, 30.0f);
			ImGui::SliderFloat("Proj Size", &params.projectileSize, 0.1f, 5.0f);
		}

		auto EditAnimatedBoxList = [&](const char* label, std::vector<AnimatedBox>& boxList, bool isHurtbox) {
			if (ImGui::TreeNode(label))
			{
				for (int i = 0; i < (int)boxList.size(); ++i)
				{
					ImGui::PushID(i);
					if (isHurtbox) {
						const char* names[] = { "Head", "Body", "Legs" };
						const char* name = (i < 3) ? names[i] : "Extra";
						ImGui::Text("Box #%d (%s)", i, name);
					}
					else {
						ImGui::Text("Hitbox #%d", i);
					}

					ImGui::SameLine();
					if (ImGui::Button("Del Group")) {
						boxList.erase(boxList.begin() + i);
						ImGui::PopID();
						break;
					}

					AnimatedBox& abox = boxList[i];
					if (ImGui::TreeNode("Keyframes"))
					{
						for (int k = 0; k < (int)abox.keyframes.size(); ++k)
						{
							ImGui::PushID(k);
							BoxKeyframe& key = abox.keyframes[k];

							ImGui::Text("Key #%d", k);
							ImGui::SameLine();
							if (ImGui::Button("Del Key")) {
								abox.keyframes.erase(abox.keyframes.begin() + k);
								ImGui::PopID();
								break;
							}

							int frame = (int)key.frame;
							if (ImGui::InputInt("Frame", &frame)) key.frame = (float)frame;

							ImGui::SliderFloat2("Offset", &key.data.offset.x, -2.0f, 2.0f);
							ImGui::SliderFloat2("Extents", &key.data.extents.x, 0.1f, 2.0f);

							ImGui::Separator();
							ImGui::PopID();
						}

						if (ImGui::Button("Add Keyframe"))
						{
							BoxKeyframe newKey;
							newKey.frame = (float)m_currentFrame;
							if (!abox.keyframes.empty()) newKey.data = abox.keyframes.back().data;
							else {
								newKey.data.offset = { 0.8f, 1.5f };
								newKey.data.extents = { 0.3f, 0.3f };
							}
							abox.keyframes.push_back(newKey);
						}
						ImGui::TreePop();
					}
					ImGui::Separator();
					ImGui::PopID();
				}

				if (ImGui::Button(isHurtbox ? "Add Hurtbox" : "Add Hitbox"))
				{
					AnimatedBox newBox;
					newBox.keyframes.push_back({ 0.0f, {{0.5f, 1.5f}, {0.3f, 0.3f}} });
					boxList.push_back(newBox);
				}

				ImGui::TreePop();
			}
			};

		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.5f, 0.5f, 1.0f));
		EditAnimatedBoxList("Hitboxes (Red)", params.hitboxes, false);
		ImGui::PopStyleColor();

		// くらい判定(Hurtbox): 基本判定は常時有効。ここでは区間限定の追加判定を編集する
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 1.0f, 0.5f, 1.0f));
		bool hurtOpen = ImGui::TreeNode("Move Hurtboxes (Green, additive)");
		ImGui::PopStyleColor();
		if (hurtOpen)
		{
			ImGui::TextDisabled("基本のくらい判定は常時有効。ここは技固有の追加分だけ");

			for (int i = 0; i < (int)params.moveHurtboxes.size(); ++i)
			{
				ImGui::PushID(i);
				WindowedHurtbox& wh = params.moveHurtboxes[i];

				ImGui::Text("Box #%d", i);
				ImGui::SameLine();
				if (ImGui::Button("Delete")) {
					params.moveHurtboxes.erase(params.moveHurtboxes.begin() + i);
					ImGui::PopID();
					break;
				}

				// 出現〜消滅の区間 (フレーム)
				if (ImGui::SliderInt("Start F", &wh.startFrame, 0, totalFrames)) {
					if (wh.startFrame > wh.endFrame) wh.endFrame = wh.startFrame;
				}
				if (ImGui::SliderInt("End F", &wh.endFrame, 0, totalFrames)) {
					if (wh.endFrame < wh.startFrame) wh.startFrame = wh.endFrame;
				}
				ImGui::SliderFloat2("Offset", &wh.offset.x, -2.0f, 2.0f);
				ImGui::SliderFloat2("Extents", &wh.extents.x, 0.1f, 2.0f);
				ImGui::Separator();
				ImGui::PopID();
			}

			if (ImGui::Button("Add Hurtbox"))
			{
				// 現在フレームを開始に、少しだけ持続する区間で追加する
				WindowedHurtbox wh;
				wh.startFrame = m_currentFrame;
				wh.endFrame = m_currentFrame + 3;
				if (wh.endFrame > totalFrames) wh.endFrame = totalFrames;
				wh.offset = { 0.0f, 1.0f };
				wh.extents = { 0.3f, 0.3f };
				params.moveHurtboxes.push_back(wh);
			}
			ImGui::TreePop();
		}


		ImGui::Separator();
		ImGui::Text("Game Data");
		ImGui::InputInt("Damage", &params.damage);
		ImGui::InputInt("Hit Adv", &params.hitFrame);
		ImGui::InputInt("Block Adv", &params.blockFrame);

		if (ImGui::InputInt("Hit Stop (Frames)", &hitStopFrames)) {
			if (hitStopFrames < 0) hitStopFrames = 0;
			params.hitStop = hitStopFrames * FRAME_TIME_60FPS;
		}

		ImGui::SliderFloat("Knockback", &params.knockback, 0.0f, 5.0f);
		ImGui::Checkbox("Cause Down", &params.isDown);

		if (ImGui::TreeNode("Cancel Settings"))
		{
			ImGui::Checkbox("Enable Cancel", &params.cancelEnabled);
			if (params.cancelEnabled) {
				int cancelStartF = static_cast<int>(std::round(params.cancelStart / FRAME_TIME_60FPS));
				int cancelEndF = static_cast<int>(std::round(params.cancelEnd / FRAME_TIME_60FPS));
				if (ImGui::SliderInt("Start Frame", &cancelStartF, 0, totalFrames)) params.cancelStart = cancelStartF * FRAME_TIME_60FPS;
				if (ImGui::SliderInt("End Frame", &cancelEndF, 0, totalFrames)) params.cancelEnd = cancelEndF * FRAME_TIME_60FPS;

				ImGui::Checkbox("-> Light P", &params.cancelToLight);
				ImGui::SameLine(); ImGui::Checkbox("-> Medium P", &params.cancelToMedium);
				ImGui::SameLine(); ImGui::Checkbox("-> Heavy P", &params.cancelToHeavyPunch);
				ImGui::Checkbox("-> Medium K", &params.cancelToMediumKick);
				ImGui::SameLine(); ImGui::Checkbox("-> Heavy K", &params.cancelToHeavy);
			}
			ImGui::TreePop();
		}

		if (ImGui::TreeNode("Speed Modifiers"))
		{
			for (int i = 0; i < (int)params.speedModifiers.size(); ++i)
			{
				ImGui::PushID(i);
				AnimSpeedModifier& mod = params.speedModifiers[i];
				ImGui::Text("Modifier #%d", i);
				ImGui::SameLine();
				if (ImGui::Button("Delete")) {
					params.speedModifiers.erase(params.speedModifiers.begin() + i);
					ImGui::PopID();
					break;
				}
				int sFrame = (int)mod.startFrame;
				int eFrame = (int)mod.endFrame;
				if (ImGui::SliderInt("Start F", &sFrame, 0, totalFrames)) mod.startFrame = (float)sFrame;
				if (ImGui::SliderInt("End F", &eFrame, 0, totalFrames)) mod.endFrame = (float)eFrame;
				ImGui::SliderFloat("Speed x", &mod.speed, 0.1f, 5.0f);
				ImGui::Separator();
				ImGui::PopID();
			}
			if (ImGui::Button("Add Modifier"))
			{
				params.speedModifiers.push_back({ 0.0f, 5.0f, 1.0f });
			}
			ImGui::TreePop();
		}
	}

	if (ImGui::CollapsingHeader("Base Hurtbox Settings"))
	{
		const char* hurtboxNames[] = { "Head", "Body", "Legs" };

		if (ImGui::TreeNode("Standing (Base)"))
		{
			for (int i = 0; i < (int)HurtboxType::COUNT; ++i)
			{
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
			for (int i = 0; i < (int)HurtboxType::COUNT; ++i)
			{
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
// 現在選択中の技に対応する AttackParams を返す
// Update() と DrawImGui() の両方から呼ぶことで、技選択の対応表を一箇所にまとめる
AttackParams* SceneDebug::GetSelectedParams(Player* player)
{
	if (!player) return nullptr;

	switch (s_currentAttackType)
	{
	case 0: return &player->GetLightPunchParams();
	case 1: return &player->GetMediumPunchParams();
	case 2: return &player->GetHeavyPunchParams();
	case 3: return &player->GetMediumKickParams();
	case 4: return &player->GetHeavyKickParams();
	case 7: return &player->GetHadoukenLParams();
	case 8: return &player->GetHadoukenMParams();
	case 9: return &player->GetHadoukenHParams();
	default: return nullptr; // Down / WakeUp などフレームデータを持たない動作
	}
}

void SceneDebug::DrawTimeline(AttackParams* params)
{
	// フレームデータを持たない動作 (Down/WakeUp) では何も出さない
	if (!params)
	{
		ImGui::TextDisabled("(この動作にはフレームデータがありません)");
		return;
	}

	// 内部パラメータは「秒」で保持しているので、60FPS基準でフレームへ換算する
	const float toFrame = 1.0f / FRAME_TIME_60FPS; // = 60
	int totalF = static_cast<int>(std::round(params->totalDuration * toFrame));
	if (totalF < 1) totalF = 1;
	int hitStartF = static_cast<int>(std::round(params->hitboxStart * toFrame));
	int hitEndF = static_cast<int>(std::round(params->hitboxEnd * toFrame));

	ImDrawList* dl = ImGui::GetWindowDrawList();

	// 描画開始位置とトラックの寸法
	const ImVec2 origin = ImGui::GetCursorScreenPos();
	const float labelW = 64.0f;
	const float laneH = 16.0f;
	const float laneGap = 4.0f;
	const float rulerH = 16.0f;
	float fullW = ImGui::GetContentRegionAvail().x;
	float trackW = fullW - labelW;
	if (trackW < 60.0f) trackW = 60.0f;
	const float pxPerFrame = trackW / static_cast<float>(totalF);
	const float trackX0 = origin.x + labelW;

	auto FrameToX = [&](float f) { return trackX0 + f * pxPerFrame; };

	enum Lane { LANE_PHASE, LANE_HIT, LANE_HURT, LANE_CANCEL, LANE_SPEED, LANE_COUNT };
	auto LaneTop = [&](int lane) { return origin.y + rulerH + lane * (laneH + laneGap); };
	const float bottomY = LaneTop(LANE_COUNT - 1) + laneH;
	const float totalH = bottomY - origin.y;

	const ImU32 colText = ImGui::GetColorU32(ImGuiCol_Text);
	const ImU32 colTick = IM_COL32(150, 150, 150, 120);
	const ImU32 colTrack = IM_COL32(255, 255, 255, 18);
	const ImU32 colTrackEdge = IM_COL32(255, 255, 255, 40);
	const ImU32 colStartup = IM_COL32(140, 140, 130, 200);
	const ImU32 colActive = IM_COL32(226, 75, 74, 235);
	const ImU32 colRecover = IM_COL32(240, 165, 60, 210);
	const ImU32 colHit = IM_COL32(226, 75, 74, 235);
	const ImU32 colHurt = IM_COL32(120, 190, 90, 150);
	const ImU32 colCancel = IM_COL32(55, 138, 221, 200);
	const ImU32 colSpeed = IM_COL32(140, 120, 230, 200);
	const ImU32 colPlayhead = IM_COL32(80, 190, 255, 255);

	ImGui::InvisibleButton("##timeline", ImVec2(fullW, totalH));
	if (ImGui::IsItemActive())
	{
		float mx = ImGui::GetIO().MousePos.x;
		int f = static_cast<int>(std::round((mx - trackX0) / pxPerFrame));
		if (f < 0) f = 0;
		if (f > totalF) f = totalF;
		m_currentFrame = f;
		m_isPaused = true;
	}

	auto DrawLaneBase = [&](int lane) {
		float y0 = LaneTop(lane);
		dl->AddRectFilled(ImVec2(trackX0, y0), ImVec2(trackX0 + trackW, y0 + laneH), colTrack, 3.0f);
		dl->AddRect(ImVec2(trackX0, y0), ImVec2(trackX0 + trackW, y0 + laneH), colTrackEdge, 3.0f);
		};
	auto DrawBand = [&](int lane, float fStart, float fEnd, ImU32 col) {
		if (fEnd <= fStart) return;
		float y0 = LaneTop(lane);
		dl->AddRectFilled(ImVec2(FrameToX(fStart), y0 + 1.0f),
			ImVec2(FrameToX(fEnd), y0 + laneH - 1.0f), col, 3.0f);
		};
	auto DrawLaneLabel = [&](int lane, const char* name) {
		dl->AddText(ImVec2(origin.x, LaneTop(lane) + 1.0f), colText, name);
		};

	for (int i = 0; i < LANE_COUNT; ++i) DrawLaneBase(i);

	int step = (totalF <= 20) ? 4 : (totalF <= 40 ? 5 : 10);
	for (int f = 0; f <= totalF; f += step)
	{
		float x = FrameToX((float)f);
		dl->AddLine(ImVec2(x, origin.y + rulerH - 4.0f), ImVec2(x, bottomY), colTick);
		char buf[16];
		snprintf(buf, sizeof(buf), "%d", f);
		dl->AddText(ImVec2(x + 2.0f, origin.y), colText, buf);
	}

	// Phase: 発生 / 持続 / 硬直
	DrawLaneLabel(LANE_PHASE, "Phase");
	DrawBand(LANE_PHASE, 0.0f, (float)hitStartF, colStartup);
	DrawBand(LANE_PHASE, (float)hitStartF, (float)hitEndF, colActive);
	DrawBand(LANE_PHASE, (float)hitEndF, (float)totalF, colRecover);

	// Hitbox: 攻撃判定が出ている区間
	DrawLaneLabel(LANE_HIT, "Hitbox");
	DrawBand(LANE_HIT, (float)hitStartF, (float)hitEndF, colHit);

	// Hurtbox: 基本判定は常時(薄く全域)、技固有の追加判定は区間だけ濃く描く
	DrawLaneLabel(LANE_HURT, "Hurtbox");
	DrawBand(LANE_HURT, 0.0f, (float)totalF, colHurt);
	for (const auto& wh : params->moveHurtboxes)
	{
		DrawBand(LANE_HURT, (float)wh.startFrame, (float)wh.endFrame, IM_COL32(99, 153, 34, 235));
	}

	// Cancel: 次の技へ移行できる窓（有効時のみ）
	DrawLaneLabel(LANE_CANCEL, "Cancel");
	if (params->cancelEnabled)
	{
		int cs = static_cast<int>(std::round(params->cancelStart * toFrame));
		int ce = static_cast<int>(std::round(params->cancelEnd * toFrame));
		DrawBand(LANE_CANCEL, (float)cs, (float)ce, colCancel);
	}

	// Speed: 再生速度変化の区間
	DrawLaneLabel(LANE_SPEED, "Speed");
	for (const auto& mod : params->speedModifiers)
	{
		DrawBand(LANE_SPEED, mod.startFrame, mod.endFrame, colSpeed);
	}

	// 再生位置（プレイヘッド）を全レーンを貫く縦線で描く
	float px = FrameToX((float)m_currentFrame);
	dl->AddLine(ImVec2(px, origin.y + rulerH - 6.0f), ImVec2(px, bottomY), colPlayhead, 2.0f);
	dl->AddTriangleFilled(
		ImVec2(px - 4.0f, origin.y + rulerH - 6.0f),
		ImVec2(px + 4.0f, origin.y + rulerH - 6.0f),
		ImVec2(px, origin.y + rulerH), colPlayhead);

	char fbuf[24];
	snprintf(fbuf, sizeof(fbuf), "%d / %dF", m_currentFrame, totalF);
	dl->AddText(ImVec2(trackX0, bottomY + 2.0f), colText, fbuf);

	ImGui::Dummy(ImVec2(fullW, ImGui::GetTextLineHeight() + 2.0f));
}