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
#include <algorithm> // sort用

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
			// ヘルパーラムダ: キーフレームリストを書き出す
			auto WriteKeys = [&](const std::vector<HurtboxKey>& keys) {
				ofs << keys.size() << std::endl;
				for (const auto& key : keys) {
					ofs << key.frame << " " << key.offsetVal.x << " " << key.offsetVal.y << " " << key.sizeVal.x << " " << key.sizeVal.y << std::endl;
				}
				};

			// ヘルパーラムダ: AttackParamsを1つ書き出す関数
			auto WriteParams = [&](const AttackParams& params) {
				// タイミング・判定
				ofs << params.totalDuration << std::endl;
				ofs << params.hitboxStart << std::endl;
				ofs << params.hitboxEnd << std::endl;
				ofs << params.hitboxOffset.x << " " << params.hitboxOffset.y << std::endl;
				ofs << params.hitboxExtents.x << " " << params.hitboxExtents.y << std::endl;

				// ゲームバランス
				ofs << params.damage << std::endl;
				ofs << params.hitFrame << std::endl;
				ofs << params.blockFrame << std::endl;
				ofs << params.hitStop << std::endl;
				ofs << params.knockback << std::endl;

				// ダウン設定
				ofs << params.isDown << std::endl;

				// くらい判定補正 (キーフレーム方式)
				WriteKeys(params.headKeys);
				WriteKeys(params.bodyKeys);
				WriteKeys(params.legsKeys);

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
			// 1. 基本設定の保存
			// ==================================================
			ofs << player->GetMoveSpeed() << std::endl;
			DirectX::XMFLOAT3 scale = player->GetScale();
			ofs << scale.x << " " << scale.y << " " << scale.z << std::endl;

			// ==================================================
			// 2. 立ち(Base)のくらい判定保存
			// ==================================================
			for (int i = 0; i < (int)HurtboxType::COUNT; ++i) {
				DirectX::XMFLOAT2 ext = player->GetHurtboxBaseExtents((HurtboxType)i);
				DirectX::XMFLOAT2 off = player->GetHurtboxBaseOffset((HurtboxType)i);
				ofs << ext.x << " " << ext.y << " " << off.x << " " << off.y << std::endl;
			}

			// ==================================================
			// 3. しゃがみ(Crouch)のくらい判定保存
			// ==================================================
			for (int i = 0; i < (int)HurtboxType::COUNT; ++i) {
				DirectX::XMFLOAT2 ext = player->GetHurtboxCrouchExtents((HurtboxType)i);
				DirectX::XMFLOAT2 off = player->GetHurtboxCrouchOffset((HurtboxType)i);
				ofs << ext.x << " " << ext.y << " " << off.x << " " << off.y << std::endl;
			}

			// ==================================================
			// 4. 各技パラメータの保存
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
	// デバッグ中はAIモード(入力を受け付けない)にして、ImGuiから制御する
	player->SetInputType(PlayerInputType::AI);

	// --- 設定ファイルからパラメータ読み込み ---
	float moveSpeed = 2.0f;
	DirectX::XMFLOAT3 scale = { 1.0f, 1.0f, 1.0f };

	std::ifstream ifs(SETTINGS_FILE_DEBUG);
	if (ifs.is_open())
	{
		// 1. 基本設定
		ifs >> moveSpeed;
		ifs >> scale.x >> scale.y >> scale.z;

		// 2. 立ち(Base)くらい判定
		for (int i = 0; i < (int)HurtboxType::COUNT; ++i) {
			DirectX::XMFLOAT2 ext, off;
			if (!ifs.eof()) ifs >> ext.x >> ext.y >> off.x >> off.y;
			player->SetHurtboxBase((HurtboxType)i, off, ext);
		}

		// 3. しゃがみ(Crouch)くらい判定
		for (int i = 0; i < (int)HurtboxType::COUNT; ++i) {
			DirectX::XMFLOAT2 ext, off;
			if (!ifs.eof()) ifs >> ext.x >> ext.y >> off.x >> off.y;
			player->SetHurtboxCrouch((HurtboxType)i, off, ext);
		}

		// キーフレーム読み込み用ヘルパー
		auto LoadKeys = [&](std::vector<HurtboxKey>& keys) {
			size_t count = 0;
			if (!ifs.eof()) ifs >> count;
			keys.clear();
			for (size_t k = 0; k < count; ++k) {
				HurtboxKey key;
				ifs >> key.frame >> key.offsetVal.x >> key.offsetVal.y >> key.sizeVal.x >> key.sizeVal.y;
				keys.push_back(key);
			}
			};

		// パラメータ読み込み用ラムダ
		auto LoadOneParam = [&](AttackParams& p) {
			if (ifs.eof()) return;
			ifs >> p.totalDuration;
			ifs >> p.hitboxStart >> p.hitboxEnd;
			ifs >> p.hitboxOffset.x >> p.hitboxOffset.y;
			ifs >> p.hitboxExtents.x >> p.hitboxExtents.y;
			ifs >> p.damage >> p.hitFrame >> p.blockFrame >> p.hitStop >> p.knockback;
			ifs >> p.isDown;

			// 旧データ互換性: キーフレームがない場合(古いファイル)は読み飛ばしが発生する可能性があるため注意
			// 今回は構造を変えるため、新しい形式のみ対応
			LoadKeys(p.headKeys);
			LoadKeys(p.bodyKeys);
			LoadKeys(p.legsKeys);

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

		// 4. 各技読み込み
		LoadOneParam(player->GetLightPunchParams());
		LoadOneParam(player->GetMediumPunchParams());
		LoadOneParam(player->GetHeavyPunchParams());
		LoadOneParam(player->GetMediumKickParams());
		LoadOneParam(player->GetHeavyKickParams());

		ifs.close();
	}

	// 読み込んだ値をプレイヤーに適用
	player->SetMoveSpeed(moveSpeed);
	player->SetScale(scale);

	// --- モデル・アニメーションロード ---
	player->Load("Assets/Model/knight/Idle.fbx", 0.02f, true, false);
	player->GetModel()->LoadAnimation("Assets/Model/knight/LightPunch.fbx", "LightPunch", true);
	player->GetModel()->LoadAnimation("Assets/Model/knight/MediumPunch.fbx", "MediumPunch", true);
	player->GetModel()->LoadAnimation("Assets/Model/knight/HeavyPunch.fbx", "HeavyPunch", true);
	player->GetModel()->LoadAnimation("Assets/Model/knight/MediumKick.fbx", "MediumKick", true);
	player->GetModel()->LoadAnimation("Assets/Model/knight/HeavyKick.fbx", "HeavyKick", true);
	player->GetModel()->LoadAnimation("Assets/Model/knight/CrouchIdle.fbx", "CrouchIdle", true);

	// ダウンと起き上がり用アニメーションを追加
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

	player->SetActiveHitbox(m_isAttacking);
	// --- パラメータの同期 ---
	AttackParams* params = nullptr;
	if (s_currentAttackType == 0) {
		params = &player->GetLightPunchParams();
	}
	else if (s_currentAttackType == 1) {
		params = &player->GetMediumPunchParams();
	}
	else if (s_currentAttackType == 2) {
		params = &player->GetHeavyPunchParams();
	}
	else if (s_currentAttackType == 3) {
		params = &player->GetMediumKickParams();
	}
	else if (s_currentAttackType == 4) {
		params = &player->GetHeavyKickParams();
	}
	// 5(Down), 6(WakeUp) は params = nullptr のまま

	player->SetCurrentAttackParams(params);

	// --- 速度計算用ロジック ---
	float speed = 1.0f;
	if (m_isAttacking) {
		const char* animName = GetCurrentAnimName();
		int totalAnimFrames = player->GetModel()->GetAnimationTotalFrame(animName);

		// パラメータがある場合はそれに基づいて速度計算
		if (params) {
			float targetFrames = params->totalDuration * 60.0f;
			if (targetFrames <= 1.0f) targetFrames = 1.0f;
			speed = (float)totalAnimFrames / targetFrames;
		}
		else {
			// ダウンや起き上がりは等倍速（または手動指定）で再生
			speed = 1.0f;
		}
	}

	// --- アニメーション制御 ---
	if (m_isPaused)
	{
		int modelFrame = static_cast<int>(m_currentFrame * speed);
		player->Debug_SetFrame(modelFrame);
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

		if (m_isAttacking)
		{
			float currentModelFrame = (float)player->Debug_GetFrame();
			if (speed > 0.0f) {
				m_currentFrame = static_cast<int>(currentModelFrame / speed);
			}
			else {
				m_currentFrame = (int)currentModelFrame;
			}
		}
	}

	player->UpdateModelBlend();

	// --- 攻撃終了判定 ---
	if (m_isAttacking)
	{
		int totalGameFrames = 0;
		if (params) {
			totalGameFrames = static_cast<int>(std::round(params->totalDuration * 60.0f));
		}
		else {
			// パラメータがない場合（ダウンなど）はモデルのアニメーション長さを基準にする
			totalGameFrames = player->GetModel()->GetAnimationTotalFrame(GetCurrentAnimName());
		}

		if (m_currentFrame >= totalGameFrames)
		{
			player->Debug_SetAnimation("Idle", true);
			m_isAttacking = false;
			m_isPaused = false;
			m_currentFrame = 0;
			player->SetCurrentAttackParams(nullptr);
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
		player->DrawBoundingBox();

		if (m_isAttacking)
		{
			AttackParams* params = player->GetCurrentAttackParams();
			if (params) {
				int startFrame = static_cast<int>(std::round(params->hitboxStart * 60.0f));
				int endFrame = static_cast<int>(std::round(params->hitboxEnd * 60.0f));

				if (m_currentFrame >= startFrame && m_currentFrame < endFrame)
				{
					player->UpdateHitbox(params->hitboxOffset, params->hitboxExtents);
					player->SetActiveHitbox(true);
					player->DrawHitbox();
					player->SetActiveHitbox(false);
				}
			}
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

				AttackParams* pParams = nullptr;
				if (s_currentAttackType == 0) pParams = &player->GetLightPunchParams();
				else if (s_currentAttackType == 1) pParams = &player->GetMediumPunchParams();
				else if (s_currentAttackType == 2) pParams = &player->GetHeavyPunchParams();
				else if (s_currentAttackType == 3) pParams = &player->GetMediumKickParams();
				else if (s_currentAttackType == 4) pParams = &player->GetHeavyKickParams();
				// 5, 6 は nullptr

				if (pParams) {
					int originalFrames = player->GetModel()->GetAnimationTotalFrame(animName);
					float targetFrames = pParams->totalDuration * 60.0f;
					if (targetFrames <= 1.0f) targetFrames = 1.0f;
					float speed = (float)originalFrames / targetFrames;
					player->SetAnimationSpeed(speed);
				}
				else {
					// Down, WakeUp はとりあえず等倍速
					player->SetAnimationSpeed(1.0f);
				}

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
		}

		ImGui::SameLine();
		ImGui::Checkbox("Pause", &m_isPaused);

		if (m_isPaused)
		{
			ImGui::SameLine();
			if (ImGui::Button("+1 Frame")) m_currentFrame++;
			ImGui::InputInt("Frame", &m_currentFrame);
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

		// グラフ表示（タイムライン）
		ImDrawList* draw_list = ImGui::GetWindowDrawList();
		ImVec2 p = ImGui::GetCursorScreenPos();
		float width = ImGui::GetContentRegionAvail().x;
		float height = 30.0f;
		float frameWidth = width / (float)totalFrames;

		// 背景（グレー）
		draw_list->AddRectFilled(p, ImVec2(p.x + width, p.y + height), IM_COL32(100, 100, 100, 255));

		// 攻撃発生区間（赤）
		float startX = p.x + startFrames * frameWidth;
		float endX = p.x + endFrames * frameWidth;
		draw_list->AddRectFilled(ImVec2(startX, p.y), ImVec2(endX, p.y + height), IM_COL32(200, 50, 50, 255));

		// 現在再生中のフレーム位置（白線）
		float currX = p.x + m_currentFrame * frameWidth;
		draw_list->AddLine(ImVec2(currX, p.y), ImVec2(currX, p.y + height), IM_COL32(255, 255, 255, 255), 2.0f);

		// キャンセル可能区間（緑ライン 下部）
		if (params.cancelEnabled) {
			int cancelStartF = static_cast<int>(std::round(params.cancelStart / FRAME_TIME_60FPS));
			int cancelEndF = static_cast<int>(std::round(params.cancelEnd / FRAME_TIME_60FPS));
			float cStartX = p.x + cancelStartF * frameWidth;
			float cEndX = p.x + cancelEndF * frameWidth;
			draw_list->AddRectFilled(ImVec2(cStartX, p.y + height - 5), ImVec2(cEndX, p.y + height), IM_COL32(50, 200, 50, 255));
		}

		// 枠線
		draw_list->AddRect(p, ImVec2(p.x + width, p.y + height), IM_COL32(255, 255, 255, 255));

		// ダミーのスペースを入れて次のコントロールへ
		ImGui::Dummy(ImVec2(width, height + 5));


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

		if (ImGui::InputInt("Hit Stop (Frames)", &hitStopFrames)) {
			if (hitStopFrames < 0) hitStopFrames = 0;
			params.hitStop = hitStopFrames * FRAME_TIME_60FPS;
		}

		ImGui::SliderFloat("Knockback", &params.knockback, 0.0f, 5.0f);

		ImGui::Checkbox("Cause Down (Knockback)", &params.isDown);

		if (ImGui::TreeNode("Hurtbox Modifiers (Keyframes)"))
		{
			auto DrawKeyEditor = [&](const char* label, std::vector<HurtboxKey>& keys) {
				if (ImGui::TreeNode(label)) {
					// キーの追加
					if (ImGui::Button("Add Keyframe")) {
						HurtboxKey k;
						k.frame = (float)m_currentFrame; // 現在の位置に追加
						if (!keys.empty()) {
							k.offsetVal = keys.back().offsetVal;
							k.sizeVal = keys.back().sizeVal;
						}
						keys.push_back(k);
						// フレーム順にソート
						std::sort(keys.begin(), keys.end(), [](const HurtboxKey& a, const HurtboxKey& b) {
							return a.frame < b.frame;
							});
					}

					// 各キーの編集
					for (int i = 0; i < (int)keys.size(); ++i) {
						ImGui::PushID(i);
						HurtboxKey& k = keys[i];
						ImGui::Text("Key #%d", i);
						ImGui::SameLine();
						if (ImGui::Button("Delete")) {
							keys.erase(keys.begin() + i);
							ImGui::PopID();
							break;
						}

						int kFrame = (int)k.frame;
						if (ImGui::SliderInt("Frame", &kFrame, 0, totalFrames)) k.frame = (float)kFrame;
						ImGui::SliderFloat2("Offset", &k.offsetVal.x, -1.0f, 1.0f);
						ImGui::SliderFloat2("Size+", &k.sizeVal.x, -1.0f, 1.0f);
						ImGui::Separator();
						ImGui::PopID();
					}
					// ソートしておく
					std::sort(keys.begin(), keys.end(), [](const HurtboxKey& a, const HurtboxKey& b) {
						return a.frame < b.frame;
						});

					ImGui::TreePop();
				}
				};

			DrawKeyEditor("Head Keys", params.headKeys);
			DrawKeyEditor("Body Keys", params.bodyKeys);
			DrawKeyEditor("Legs Keys", params.legsKeys);

			ImGui::TreePop();
		}

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