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

using namespace DirectX;
using namespace DirectX::SimpleMath;

Texture* g_uiTex_debug = nullptr;

const char* SETTINGS_FILE_DEBUG = "player_settings.ini";

const float FRAME_TIME_60FPS = 1.0f / 60.0f;

// 0:LP, 1:MP, 2:HP, 3:MK, 4:HK, 5:Down, 6:WakeUp
static int s_currentAttackType = 0;

static const char* GetCurrentAnimName()
{
	if (s_currentAttackType == 0) return "LightPunch";
	if (s_currentAttackType == 1) return "MediumPunch";
	if (s_currentAttackType == 2) return "HeavyPunch";
	if (s_currentAttackType == 3) return "MediumKick";
	if (s_currentAttackType == 4) return "HeavyKick";
	if (s_currentAttackType == 5) return "Down";
	return "WakeUp";
}

void SceneDebug::SavePlayerSettings()
{
	Player* player = GetObj<Player>("Player");
	if (player)
	{
		std::ofstream ofs(SETTINGS_FILE_DEBUG);
		if (ofs.is_open())
		{
			// ヘルパーラムダ: AttackParamsを1つ書き出す関数
			auto WriteParams = [&](const AttackParams& params) {
				// タイミング・判定
				ofs << params.totalDuration << std::endl;
				ofs << params.hitboxStart << std::endl;
				ofs << params.hitboxEnd << std::endl;

				// 判定ボックス保存(AnimatedBox対応)
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

				// Hitboxes
				WriteAnimatedBoxes(params.hitboxes);
				// Hurtboxes
				WriteAnimatedBoxes(params.hurtboxes);

				// ゲームバランス
				ofs << params.damage << std::endl;
				ofs << params.hitFrame << std::endl;
				ofs << params.blockFrame << std::endl;
				ofs << params.hitStop << std::endl;
				ofs << params.knockback << std::endl;

				// ダウン設定
				ofs << params.isDown << std::endl;

				// キャンセル設定
				ofs << params.cancelEnabled << std::endl;
				ofs << params.cancelStart << std::endl;
				ofs << params.cancelEnd << std::endl;
				ofs << params.cancelToLight << " " << params.cancelToMedium << " " << params.cancelToHeavyPunch << " " << params.cancelToMediumKick << " " << params.cancelToHeavy << std::endl;

				// 速度変化リスト (Speed Modifiers)
				size_t speedModCount = params.speedModifiers.size();
				ofs << speedModCount << std::endl;
				for (const auto& mod : params.speedModifiers) {
					ofs << mod.startFrame << " " << mod.endFrame << " " << mod.speed << std::endl;
				}
				};

			// ==================================================
			// 基本設定の保存
			// ==================================================
			ofs << player->GetMoveSpeed() << std::endl;
			DirectX::XMFLOAT3 scale = player->GetScale();
			ofs << scale.x << " " << scale.y << " " << scale.z << std::endl;

			// ==================================================
			// 立ち(Base)のくらい判定保存
			// ==================================================
			for (int i = 0; i < (int)HurtboxType::COUNT; ++i) {
				DirectX::XMFLOAT2 ext = player->GetHurtboxBaseExtents((HurtboxType)i);
				DirectX::XMFLOAT2 off = player->GetHurtboxBaseOffset((HurtboxType)i);
				ofs << ext.x << " " << ext.y << " " << off.x << " " << off.y << std::endl;
			}

			// ==================================================
			// しゃがみ(Crouch)のくらい判定保存
			// ==================================================
			for (int i = 0; i < (int)HurtboxType::COUNT; ++i) {
				DirectX::XMFLOAT2 ext = player->GetHurtboxCrouchExtents((HurtboxType)i);
				DirectX::XMFLOAT2 off = player->GetHurtboxCrouchOffset((HurtboxType)i);
				ofs << ext.x << " " << ext.y << " " << off.x << " " << off.y << std::endl;
			}

			// ==================================================
			// 各技パラメータの保存
			// ==================================================
			WriteParams(player->GetLightPunchParams());
			WriteParams(player->GetMediumPunchParams());
			WriteParams(player->GetHeavyPunchParams());
			WriteParams(player->GetMediumKickParams());
			WriteParams(player->GetHeavyKickParams());

			ofs.close();
		}
	}
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
	float moveSpeed = 2.0f;
	DirectX::XMFLOAT3 scale = { 1.0f, 1.0f, 1.0f };

	std::ifstream ifs(SETTINGS_FILE_DEBUG);
	if (ifs.is_open())
	{
		//  基本設定
		ifs >> moveSpeed;
		ifs >> scale.x >> scale.y >> scale.z;

		// 立ち(Base)くらい判定
		for (int i = 0; i < (int)HurtboxType::COUNT; ++i) {
			DirectX::XMFLOAT2 ext, off;
			if (!ifs.eof()) ifs >> ext.x >> ext.y >> off.x >> off.y;
			player->SetHurtboxBase((HurtboxType)i, off, ext);
		}

		// しゃがみ(Crouch)くらい判定
		for (int i = 0; i < (int)HurtboxType::COUNT; ++i) {
			DirectX::XMFLOAT2 ext, off;
			if (!ifs.eof()) ifs >> ext.x >> ext.y >> off.x >> off.y;
			player->SetHurtboxCrouch((HurtboxType)i, off, ext);
		}

		// パラメータ読み込み用ラムダ
		auto LoadOneParam = [&](AttackParams& p) {
			if (ifs.eof()) return;
			ifs >> p.totalDuration;
			ifs >> p.hitboxStart >> p.hitboxEnd;

			// 判定ボックス読み込み (AnimatedBox対応)
			auto LoadAnimatedBoxes = [&](std::vector<AnimatedBox>& targetList) {
				size_t boxCount = 0;
				ifs >> boxCount;
				targetList.clear();
				for (size_t i = 0; i < boxCount; ++i)
				{
					AnimatedBox abox;
					size_t keyframeCount = 0;
					ifs >> keyframeCount;
					for (size_t k = 0; k < keyframeCount; ++k)
					{
						BoxKeyframe key;
						ifs >> key.frame;
						ifs >> key.data.offset.x >> key.data.offset.y >> key.data.extents.x >> key.data.extents.y;
						abox.keyframes.push_back(key);
					}
					targetList.push_back(abox);
				}
				};

			LoadAnimatedBoxes(p.hitboxes);
			LoadAnimatedBoxes(p.hurtboxes);


			ifs >> p.damage >> p.hitFrame >> p.blockFrame >> p.hitStop >> p.knockback;
			ifs >> p.isDown;

			// 不要になった古いHurtboxパラメータを読み飛ばし

			ifs >> p.cancelEnabled >> p.cancelStart >> p.cancelEnd;
			ifs >> p.cancelToLight >> p.cancelToMedium >> p.cancelToHeavyPunch >> p.cancelToMediumKick >> p.cancelToHeavy;

			// 速度変化リスト読み込み
			size_t count = 0;
			if (!ifs.eof()) ifs >> count;
			p.speedModifiers.clear();
			for (size_t k = 0; k < count; ++k) {
				AnimSpeedModifier mod;
				ifs >> mod.startFrame >> mod.endFrame >> mod.speed;
				p.speedModifiers.push_back(mod);
			}
			};

		// 各技読み込み
		LoadOneParam(player->GetLightPunchParams());
		LoadOneParam(player->GetMediumPunchParams());
		LoadOneParam(player->GetHeavyPunchParams());
		LoadOneParam(player->GetMediumKickParams());
		LoadOneParam(player->GetHeavyKickParams());

		ifs.close();
		// 読み込み成功時は上書き
		player->SetMoveSpeed(moveSpeed);
		player->SetScale(scale);
	}
	else
	{
		// 失敗時は何もしない
		DebugLog::log(DebugLog::WARNING_LOG, "設定ファイルなし。初期値を使用。");
	}

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

/**
 * @brief 更新処理
 */
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
	// 常に現在の技のパラメータをセットしておく（編集中も反映させるため）
	AttackParams* params = nullptr;
	if (s_currentAttackType == 0) params = &player->GetLightPunchParams();
	else if (s_currentAttackType == 1) params = &player->GetMediumPunchParams();
	else if (s_currentAttackType == 2) params = &player->GetHeavyPunchParams();
	else if (s_currentAttackType == 3) params = &player->GetMediumKickParams();
	else if (s_currentAttackType == 4) params = &player->GetHeavyKickParams();

	player->SetCurrentAttackParams(params);

	// --- 速度計算用ロジック ---
	float speed = 1.0f;
	const char* animName = GetCurrentAnimName();

	if (m_isAttacking || s_currentAttackType <= 4) { // 攻撃技選択中
		int totalAnimFrames = player->GetModel()->GetAnimationTotalFrame(animName);

		// パラメータがある場合はそれに基づいて速度計算
		if (params) {
			float targetFrames = params->totalDuration * 60.0f;
			if (targetFrames <= 1.0f) targetFrames = 1.0f;
			speed = (float)totalAnimFrames / targetFrames;
		}
	}

	// ★ここで計算した速度をプレイヤーに適用する！ (これが必要でした)
	player->SetAnimationSpeed(speed);

	// --- アニメーション制御 ---
	if (!m_isPaused && m_isAttacking)
	{
		// 再生中のみ時間を進める
		m_animTimer += tick;
		if (m_animTimer >= FRAME_TIME_60FPS)
		{
			while (m_animTimer >= FRAME_TIME_60FPS)
			{
				player->UpdateAnimation(FRAME_TIME_60FPS);
				m_animTimer -= FRAME_TIME_60FPS;
			}
		}

		// 現在のモデルフレームからデバッグ用フレーム値を逆算
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
		// 停止中または非再生中
		// スライダーで設定された m_currentFrame を強制適用する
		// これにより、一時停止中にスライダーを動かしてもアニメーションが動く
		int modelFrame = static_cast<int>(m_currentFrame * speed);

		// 技名が変わっているかもしれないのでセット
		player->Debug_SetAnimation(animName, false);
		player->Debug_SetFrame(modelFrame);
	}

	// ブレンドとボックスの更新（常に呼ぶ）
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
			// ループしないように戻す
			player->Debug_SetAnimation("Idle", true);
			m_isAttacking = false;
			m_isPaused = false;
			m_currentFrame = 0;
			// paramsはnullにしない（編集継続のため）
		}
	}

	// --- カメラ追従処理 ---
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

		// 編集中の技があれば、強制的に攻撃中扱いにしてボックスを描画する
		bool isEditingAttack = (s_currentAttackType <= 4); // 0~4は攻撃

		if (isEditingAttack) {
			player->SetActiveHitbox(true);
			// Updateで計算済みだが描画直前にも確実に
			player->UpdateAttackBoxes();
		}

		// Hurtboxなどの描画
		player->DrawBoundingBox();

		// Hitboxの描画
		if (isEditingAttack)
		{
			player->DrawHitbox();
		}

		// フラグを戻す（再生中でなければ）
		if (!m_isAttacking) {
			player->SetActiveHitbox(false);
		}
	}

	if (m_showImGui)
	{
		DrawImGui();
	}
}

/**
 * @brief デバッグGUIの描画
 */
void SceneDebug::DrawImGui()
{
	Player* player = GetObj<Player>("Player");

	ImGui::Begin("Debug Settings");
	ImGui::Text("FPS: %.1f", m_fps);

	if (!player) {
		ImGui::End();
		return;
	}

	// ==================================================
	// アニメーション再生・確認
	// ==================================================
	if (ImGui::CollapsingHeader("Animation Control", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::Text("Select Action:");

		// Punch
		ImGui::Text("Punch:"); ImGui::SameLine();
		ImGui::RadioButton("L##P", &s_currentAttackType, 0); ImGui::SameLine();
		ImGui::RadioButton("M##P", &s_currentAttackType, 1); ImGui::SameLine();
		ImGui::RadioButton("H##P", &s_currentAttackType, 2);

		// Kick
		ImGui::Text("Kick :"); ImGui::SameLine();
		ImGui::RadioButton("M##K", &s_currentAttackType, 3); ImGui::SameLine();
		ImGui::RadioButton("H##K", &s_currentAttackType, 4);

		// Other
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
	}

	// ==================================================
	// 攻撃パラメータ調整
	// ==================================================
	AttackParams* pParams = nullptr;
	if (s_currentAttackType == 0) pParams = &player->GetLightPunchParams();
	else if (s_currentAttackType == 1) pParams = &player->GetMediumPunchParams();
	else if (s_currentAttackType == 2) pParams = &player->GetHeavyPunchParams();
	else if (s_currentAttackType == 3) pParams = &player->GetMediumKickParams();
	else if (s_currentAttackType == 4) pParams = &player->GetHeavyKickParams();

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

		ImGui::Text("Hit Active Range (Ref):");
		if (ImGui::InputInt("Start Frame", &startFrames)) {
			if (startFrames < 0) startFrames = 0;
			params.hitboxStart = startFrames * FRAME_TIME_60FPS;
		}
		if (ImGui::InputInt("End Frame", &endFrames)) {
			if (endFrames < 0) endFrames = 0;
			params.hitboxEnd = endFrames * FRAME_TIME_60FPS;
		}

		// ----------------------------------------------------
		// ボックスリスト編集用ヘルパー
		// ----------------------------------------------------
		auto EditAnimatedBoxList = [&](const char* label, std::vector<AnimatedBox>& boxList, bool isHurtbox) {
			if (ImGui::TreeNode(label))
			{
				for (int i = 0; i < (int)boxList.size(); ++i)
				{
					ImGui::PushID(i);
					if (isHurtbox) {
						// 最初の3つは名前を付ける (Head, Body, Legs)
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
							newKey.frame = (float)m_currentFrame; // 現在のフレームで追加
							// 直前のキーがあればコピー、なければデフォルト
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
					// 新規追加時のデフォルトキーフレーム
					newBox.keyframes.push_back({ 0.0f, {{0.5f, 1.5f}, {0.3f, 0.3f}} });
					boxList.push_back(newBox);
				}

				ImGui::TreePop();
			}
			};

		// Hitboxes (Red)
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.5f, 0.5f, 1.0f));
		EditAnimatedBoxList("Hitboxes (Red)", params.hitboxes, false);
		ImGui::PopStyleColor();

		// Hurtboxes (Green/Blue)
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 1.0f, 0.5f, 1.0f));
		EditAnimatedBoxList("Hurtboxes (Green)", params.hurtboxes, true);
		ImGui::PopStyleColor();


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
	else if (!pParams)
	{
		// Down / WakeUp 用の説明
		ImGui::Text("Selected action does not have AttackParams.");
	}

	// ==================================================
	// くらい判定調整 (基本設定)
	// ==================================================
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