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

// 0:LP, 1:MP, 2:HP, 3:HK
static int s_currentAttackType = 0;

static const char* GetCurrentAnimName()
{
	if (s_currentAttackType == 0) return "LightPunch";
	if (s_currentAttackType == 1) return "MediumPunch";
	if (s_currentAttackType == 2) return "HeavyPunch";
	return "HeavyKick";
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
				ofs << params.hitboxOffset.x << " " << params.hitboxOffset.y << std::endl;
				ofs << params.hitboxExtents.x << " " << params.hitboxExtents.y << std::endl;

				// ゲームバランス
				ofs << params.damage << std::endl;
				ofs << params.hitFrame << std::endl;
				ofs << params.blockFrame << std::endl;
				ofs << params.hitStop << std::endl;

				// くらい判定補正
				ofs << params.headOffsetVal.x << " " << params.headOffsetVal.y << std::endl;
				ofs << params.headSizeVal.x << " " << params.headSizeVal.y << std::endl;
				ofs << params.bodyOffsetVal.x << " " << params.bodyOffsetVal.y << std::endl;
				ofs << params.bodySizeVal.x << " " << params.bodySizeVal.y << std::endl;
				ofs << params.legsOffsetVal.x << " " << params.legsOffsetVal.y << std::endl;
				ofs << params.legsSizeVal.x << " " << params.legsSizeVal.y << std::endl;

				// キャンセル設定
				ofs << params.cancelEnabled << std::endl;
				ofs << params.cancelStart << std::endl;
				ofs << params.cancelEnd << std::endl;
				ofs << params.cancelToLight << " " << params.cancelToMedium << " " << params.cancelToHeavy << std::endl;

				// 速度変化リスト (Speed Modifiers)
				// まず要素数を書き込み、その後に中身を書き込む
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

	// 読み込み用ヘルパー
	auto ReadParams = [&](AttackParams& params) {
		std::ifstream& ifs = *(std::ifstream*)nullptr; // ダミー宣言 (実際にはキャプチャできないためロジック内に直書きするか関数化する)
		};

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

		// パラメータ読み込み用ラムダ
		auto LoadOneParam = [&](AttackParams& p) {
			if (ifs.eof()) return;
			ifs >> p.totalDuration;
			ifs >> p.hitboxStart >> p.hitboxEnd;
			ifs >> p.hitboxOffset.x >> p.hitboxOffset.y;
			ifs >> p.hitboxExtents.x >> p.hitboxExtents.y;
			ifs >> p.damage >> p.hitFrame >> p.blockFrame >> p.hitStop;
			ifs >> p.headOffsetVal.x >> p.headOffsetVal.y;
			ifs >> p.headSizeVal.x >> p.headSizeVal.y;
			ifs >> p.bodyOffsetVal.x >> p.bodyOffsetVal.y;
			ifs >> p.bodySizeVal.x >> p.bodySizeVal.y;
			ifs >> p.legsOffsetVal.x >> p.legsOffsetVal.y;
			ifs >> p.legsSizeVal.x >> p.legsSizeVal.y;
			ifs >> p.cancelEnabled >> p.cancelStart >> p.cancelEnd;
			ifs >> p.cancelToLight >> p.cancelToMedium >> p.cancelToHeavy;

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
	player->GetModel()->LoadAnimation("Assets/Model/knight/HeavyKick.fbx", "HeavyKick", true);
	player->GetModel()->LoadAnimation("Assets/Model/knight/CrouchIdle.fbx", "CrouchIdle", true);

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
 * GUI操作に応じたアニメーションのコマ送り制御などを行う
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
	if (s_currentAttackType == 0) {
		player->SetCurrentAttackParams(&player->GetLightPunchParams());
	}
	else if (s_currentAttackType == 1) {
		player->SetCurrentAttackParams(&player->GetMediumPunchParams());
	}
	else if (s_currentAttackType == 2) {
		player->SetCurrentAttackParams(&player->GetHeavyPunchParams());
	}
	else {
		// 大キック
		player->SetCurrentAttackParams(&player->GetHeavyKickParams());
	}

	// --- 速度計算用ロジック (Update/Draw共通で使用するため変数化) ---
	float speed = 1.0f;
	AttackParams* params = player->GetCurrentAttackParams();
	if (m_isAttacking && params) {
		const char* animName = GetCurrentAnimName();
		int totalAnimFrames = player->GetModel()->GetAnimationTotalFrame(animName);
		float targetFrames = params->totalDuration * 60.0f;
		if (targetFrames <= 1.0f) targetFrames = 1.0f;
		speed = (float)totalAnimFrames / targetFrames;
	}

	// --- アニメーション制御 ---
	if (m_isPaused)
	{
		// m_currentFrame は「ゲームフレーム」として扱う
		// 一時停止中: 
		// 1. ゲームフレーム(m_currentFrame) に 速度倍率(speed) を掛けて、モデルフレームを算出する
		int modelFrame = static_cast<int>(m_currentFrame * speed);

		// 2. モデルに適用
		player->Debug_SetFrame(modelFrame);

		m_animTimer = 0.0f;
	}
	else
	{
		// 再生中: 60FPS間隔でフレームを進める
		// tick (経過秒数) を積算し、1/60秒経つごとに1フレーム進める
		m_animTimer += tick;
		if (m_animTimer >= FRAME_TIME_60FPS)
		{
			// 処理落ちしても動きがスローにならないよう、経過時間分だけフレームを進める
			while (m_animTimer >= FRAME_TIME_60FPS)
			{
				// Player::UpdateAnimation内で m_animSpeed が考慮される
				player->UpdateAnimation(FRAME_TIME_60FPS);
				m_animTimer -= FRAME_TIME_60FPS;
			}
		}

		// プレイヤーから現在のモデルフレームを取得し、ゲームフレームに逆変換して同期
		if (m_isAttacking)
		{
			// GameFrame = ModelFrame / speed
			float currentModelFrame = (float)player->Debug_GetFrame();
			if (speed > 0.0f) {
				m_currentFrame = static_cast<int>(currentModelFrame / speed);
			}
			else {
				m_currentFrame = (int)currentModelFrame;
			}
		}
	}

	// ボーン行列の更新 (アニメ停止中でも姿勢を反映するために毎フレーム呼ぶ)
	player->UpdateModelBlend();

	// --- 攻撃終了判定 ---
	if (m_isAttacking)
	{
		// 判定も「ゲームフレーム」で行うことで、GUI設定と完全に同期させる
		if (params)
		{
			// 全体フレーム(ゲーム内時間)を超えたら終了
			int totalGameFrames = static_cast<int>(std::round(params->totalDuration * 60.0f));

			if (m_currentFrame >= totalGameFrames)
			{
				player->Debug_SetAnimation("Idle", true);
				m_isAttacking = false; // 攻撃終了
				m_isPaused = false;    // 再生状態に戻す
				m_currentFrame = 0;

				// パラメータリンクを解除 (安全のため)
				player->SetCurrentAttackParams(nullptr);
			}
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

		// 描画
		player->Draw();

		// くらい判定 (緑箱) の描画 (3部位)
		// m_isAttacking = true の間は、攻撃用補正値が反映された状態で描画される
		player->DrawBoundingBox();

		// --- 攻撃判定 (Hitbox: 赤箱) の描画 ---
		if (m_isAttacking)
		{
			// 現在アクティブなパラメータを使用
			AttackParams* params = player->GetCurrentAttackParams();

			// m_currentFrame を「ゲームフレーム」としたため、
			// hitboxStart (秒) を単純に 60倍してフレームに直すだけで比較可能になる
			int startFrame = static_cast<int>(std::round(params->hitboxStart * 60.0f));
			int endFrame = static_cast<int>(std::round(params->hitboxEnd * 60.0f));

			// 現在フレームが発生期間内なら描画
			if (m_currentFrame >= startFrame && m_currentFrame < endFrame)
			{
				player->UpdateHitbox(params->hitboxOffset, params->hitboxExtents);
				player->SetActiveHitbox(true);
				player->DrawHitbox();
				player->SetActiveHitbox(false); // 描画後はいったん無効化
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
	// アニメーション再生・確認 (CollapsingHeaderで開閉可能)
	// ==================================================
	if (ImGui::CollapsingHeader("Animation Control", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::Text("Select Attack:");
		ImGui::RadioButton("Light Punch", &s_currentAttackType, 0);
		ImGui::SameLine();
		ImGui::RadioButton("Medium Punch", &s_currentAttackType, 1);
		ImGui::SameLine();
		ImGui::RadioButton("Heavy Punch", &s_currentAttackType, 2);
		ImGui::SameLine();
		ImGui::RadioButton("Heavy Kick", &s_currentAttackType, 3);

		if (!m_isAttacking)
		{
			auto StartAttack = [&](bool startPaused) {
				const char* animName = GetCurrentAnimName();
				player->Debug_SetAnimation(animName, true);

				AttackParams* pParams = nullptr;
				if (s_currentAttackType == 0) pParams = &player->GetLightPunchParams();
				else if (s_currentAttackType == 1) pParams = &player->GetMediumPunchParams();
				else if (s_currentAttackType == 2) pParams = &player->GetHeavyPunchParams();
				else pParams = &player->GetHeavyKickParams();

				if (pParams) {
					int originalFrames = player->GetModel()->GetAnimationTotalFrame(animName);
					float targetFrames = pParams->totalDuration * 60.0f;
					if (targetFrames <= 1.0f) targetFrames = 1.0f;

					float speed = (float)originalFrames / targetFrames;
					player->SetAnimationSpeed(speed);
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
			ImGui::TextColored(ImVec4(1, 0, 0, 1), ">> ATTACKING <<");
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
	if (ImGui::CollapsingHeader("Attack Parameters", ImGuiTreeNodeFlags_DefaultOpen))
	{
		AttackParams* pParams = nullptr;
		if (s_currentAttackType == 0) {
			pParams = &player->GetLightPunchParams();
			ImGui::Text("Light Punch");
		}
		else if (s_currentAttackType == 1) {
			pParams = &player->GetMediumPunchParams();
			ImGui::Text("Medium Punch");
		}
		else if (s_currentAttackType == 2) {
			pParams = &player->GetHeavyPunchParams();
			ImGui::Text("Heavy Punch");
		}
		else {
			pParams = &player->GetHeavyKickParams();
			ImGui::Text("Heavy Kick");
		}

		AttackParams& params = *pParams;

		int totalFrames = static_cast<int>(std::round(params.totalDuration / FRAME_TIME_60FPS));
		int startFrames = static_cast<int>(std::round(params.hitboxStart / FRAME_TIME_60FPS));
		int endFrames = static_cast<int>(std::round(params.hitboxEnd / FRAME_TIME_60FPS));
		int hitStopFrames = static_cast<int>(std::round(params.hitStop / FRAME_TIME_60FPS));


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

		if (ImGui::TreeNode("Hurtbox Modifiers (Attack)"))
		{
			ImGui::SliderFloat2("Head Off", &params.headOffsetVal.x, -1.0f, 1.0f);
			ImGui::SliderFloat2("Head Sz+", &params.headSizeVal.x, -1.0f, 1.0f);
			ImGui::SliderFloat2("Body Off", &params.bodyOffsetVal.x, -1.0f, 1.0f);
			ImGui::SliderFloat2("Body Sz+", &params.bodySizeVal.x, -1.0f, 1.0f);
			ImGui::SliderFloat2("Legs Off", &params.legsOffsetVal.x, -1.0f, 1.0f);
			ImGui::SliderFloat2("Legs Sz+", &params.legsSizeVal.x, -1.0f, 1.0f);
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
				ImGui::Checkbox("-> Light", &params.cancelToLight);
				ImGui::SameLine(); ImGui::Checkbox("-> Medium", &params.cancelToMedium);
				ImGui::SameLine(); ImGui::Checkbox("-> Heavy", &params.cancelToHeavy);
			}
			ImGui::TreePop();
		}

		// --- アニメーション速度調整 (Speed Modifiers) ---
		if (ImGui::TreeNode("Speed Modifiers (Variable Speed)"))
		{
			ImGui::Text("Define speed changes per frame range");
			ImGui::Text("Example: Start=4, End=8, Speed=2.0 (Fast)");

			// リスト表示
			for (int i = 0; i < (int)params.speedModifiers.size(); ++i)
			{
				ImGui::PushID(i);
				AnimSpeedModifier& mod = params.speedModifiers[i];

				ImGui::Text("Modifier #%d", i);
				ImGui::SameLine();
				if (ImGui::Button("Delete")) {
					params.speedModifiers.erase(params.speedModifiers.begin() + i);
					ImGui::PopID();
					break; // 削除したらループを抜ける
				}

				// 編集用スライダー (フレーム単位)
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
				// デフォルトで最初の5フレームを等倍にする設定を追加
				params.speedModifiers.push_back({ 0.0f, 5.0f, 1.0f });
			}

			ImGui::TreePop();
		}
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