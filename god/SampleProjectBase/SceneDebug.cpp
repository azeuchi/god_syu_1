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
// staticにすることで、シーン関数を抜けても値を保持する
static int s_currentAttackType = 0;

/**
 * @brief プレイヤー設定の保存
 * 現在のパラメータを ini ファイルに書き出す
 */
void SceneDebug::SavePlayerSettings()
{
	Player* player = GetObj<Player>("Player");
	if (player)
	{
		std::ofstream ofs(SETTINGS_FILE_DEBUG);
		if (ofs.is_open())
		{
			// ==================================================
			// 1. 基本設定の保存
			// ==================================================
			ofs << player->GetMoveSpeed() << std::endl; // 移動速度

			DirectX::XMFLOAT3 scale = player->GetScale(); // スケール
			ofs << scale.x << " " << scale.y << " " << scale.z << std::endl;

			// ==================================================
			// 2. 3部位の基本くらい判定 (Base Hurtbox) の保存
			// ==================================================
			// 頭、体、足 それぞれの「サイズ(Extents)」と「位置(Offset)」を保存
			for (int i = 0; i < (int)HurtboxType::COUNT; ++i) {
				DirectX::XMFLOAT2 ext = player->GetHurtboxBaseExtents((HurtboxType)i);
				DirectX::XMFLOAT2 off = player->GetHurtboxBaseOffset((HurtboxType)i);
				ofs << ext.x << " " << ext.y << " " << off.x << " " << off.y << std::endl;
			}

			// ==================================================
			// 3. 弱パンチ (LightPunch) パラメータの保存
			// ==================================================
			AttackParams& lParams = player->GetLightPunchParams();

			// タイミング・判定
			ofs << lParams.totalDuration << std::endl; // 全体フレーム(秒)
			ofs << lParams.hitboxStart << std::endl;   // 発生(秒)
			ofs << lParams.hitboxEnd << std::endl;     // 持続終了(秒)
			ofs << lParams.hitboxOffset.x << " " << lParams.hitboxOffset.y << std::endl; // 判定位置
			ofs << lParams.hitboxExtents.x << " " << lParams.hitboxExtents.y << std::endl; // 判定サイズ

			// ゲームバランス
			ofs << lParams.damage << std::endl;      // ダメージ
			ofs << lParams.hitFrame << std::endl;    // ヒット有利F
			ofs << lParams.blockFrame << std::endl;  // ガード有利F

			// 攻撃中のくらい判定補正 (体が前に出る等の調整値)
			ofs << lParams.headOffsetVal.x << " " << lParams.headOffsetVal.y << std::endl;
			ofs << lParams.headSizeVal.x << " " << lParams.headSizeVal.y << std::endl;
			ofs << lParams.bodyOffsetVal.x << " " << lParams.bodyOffsetVal.y << std::endl;
			ofs << lParams.bodySizeVal.x << " " << lParams.bodySizeVal.y << std::endl;
			ofs << lParams.legsOffsetVal.x << " " << lParams.legsOffsetVal.y << std::endl;
			ofs << lParams.legsSizeVal.x << " " << lParams.legsSizeVal.y << std::endl;

			// ==================================================
			// 4. 中パンチ (MediumPunch) パラメータの保存
			// ==================================================
			AttackParams& mParams = player->GetMediumPunchParams();

			// タイミング・判定
			ofs << mParams.totalDuration << std::endl;
			ofs << mParams.hitboxStart << std::endl;
			ofs << mParams.hitboxEnd << std::endl;
			ofs << mParams.hitboxOffset.x << " " << mParams.hitboxOffset.y << std::endl;
			ofs << mParams.hitboxExtents.x << " " << mParams.hitboxExtents.y << std::endl;

			// ゲームバランス
			ofs << mParams.damage << std::endl;
			ofs << mParams.hitFrame << std::endl;
			ofs << mParams.blockFrame << std::endl;

			// 攻撃中のくらい判定補正
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

	// 読み込み用の一時参照
	AttackParams lParams = player->GetLightPunchParams();
	AttackParams mParams = player->GetMediumPunchParams();

	std::ifstream ifs(SETTINGS_FILE_DEBUG);
	if (ifs.is_open())
	{
		// 1. 基本設定
		ifs >> moveSpeed;
		ifs >> scale.x >> scale.y >> scale.z;

		// 2. くらい判定基本設定 (3部位)
		for (int i = 0; i < (int)HurtboxType::COUNT; ++i) {
			DirectX::XMFLOAT2 ext, off;
			if (!ifs.eof()) ifs >> ext.x >> ext.y >> off.x >> off.y;
			player->SetHurtboxBase((HurtboxType)i, off, ext);
		}

		// 3. 弱パンチ読み込み
		if (!ifs.eof()) ifs >> lParams.totalDuration;
		if (!ifs.eof()) ifs >> lParams.hitboxStart;
		if (!ifs.eof()) ifs >> lParams.hitboxEnd;
		if (!ifs.eof()) ifs >> lParams.hitboxOffset.x >> lParams.hitboxOffset.y;
		if (!ifs.eof()) ifs >> lParams.hitboxExtents.x >> lParams.hitboxExtents.y;
		if (!ifs.eof()) ifs >> lParams.damage;
		if (!ifs.eof()) ifs >> lParams.hitFrame;
		if (!ifs.eof()) ifs >> lParams.blockFrame;
		// 補正値
		if (!ifs.eof()) ifs >> lParams.headOffsetVal.x >> lParams.headOffsetVal.y;
		if (!ifs.eof()) ifs >> lParams.headSizeVal.x >> lParams.headSizeVal.y;
		if (!ifs.eof()) ifs >> lParams.bodyOffsetVal.x >> lParams.bodyOffsetVal.y;
		if (!ifs.eof()) ifs >> lParams.bodySizeVal.x >> lParams.bodySizeVal.y;
		if (!ifs.eof()) ifs >> lParams.legsOffsetVal.x >> lParams.legsOffsetVal.y;
		if (!ifs.eof()) ifs >> lParams.legsSizeVal.x >> lParams.legsSizeVal.y;

		// 4. 中パンチ読み込み
		if (!ifs.eof()) ifs >> mParams.totalDuration;
		if (!ifs.eof()) ifs >> mParams.hitboxStart;
		if (!ifs.eof()) ifs >> mParams.hitboxEnd;
		if (!ifs.eof()) ifs >> mParams.hitboxOffset.x >> mParams.hitboxOffset.y;
		if (!ifs.eof()) ifs >> mParams.hitboxExtents.x >> mParams.hitboxExtents.y;
		if (!ifs.eof()) ifs >> mParams.damage;
		if (!ifs.eof()) ifs >> mParams.hitFrame;
		if (!ifs.eof()) ifs >> mParams.blockFrame;
		// 補正値
		if (!ifs.eof()) ifs >> mParams.headOffsetVal.x >> mParams.headOffsetVal.y;
		if (!ifs.eof()) ifs >> mParams.headSizeVal.x >> mParams.headSizeVal.y;
		if (!ifs.eof()) ifs >> mParams.bodyOffsetVal.x >> mParams.bodyOffsetVal.y;
		if (!ifs.eof()) ifs >> mParams.bodySizeVal.x >> mParams.bodySizeVal.y;
		if (!ifs.eof()) ifs >> mParams.legsOffsetVal.x >> mParams.legsOffsetVal.y;
		if (!ifs.eof()) ifs >> mParams.legsSizeVal.x >> mParams.legsSizeVal.y;

		ifs.close();
	}

	// 読み込んだ値をプレイヤーに適用
	player->SetMoveSpeed(moveSpeed);
	player->SetScale(scale);
	player->GetLightPunchParams() = lParams;
	player->GetMediumPunchParams() = mParams;

	// --- モデル・アニメーションロード ---
	// 必要なアニメーションだけを読み込む
	player->Load("Assets/Model/knight/Idle.fbx", 0.02f, true, false);
	player->GetModel()->LoadAnimation("Assets/Model/knight/LightPunch.fbx", "LightPunch", true);
	player->GetModel()->LoadAnimation("Assets/Model/knight/MediumPunch.fbx", "MediumPunch", true);

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
	else {
		player->SetCurrentAttackParams(&player->GetMediumPunchParams());
	}

	// --- アニメーション制御 ---
	if (m_isPaused)
	{
		// 一時停止中: GUIで操作された m_currentFrame (整数) を強制適用する
		// これによりスライダーでのシークが可能になる
		player->Debug_SetFrame(m_currentFrame);
		m_animTimer = 0.0f; // タイマーは止めておく
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
				player->UpdateAnimation(FRAME_TIME_60FPS);
				m_animTimer -= FRAME_TIME_60FPS;
			}
		}
	}

	// ボーン行列の更新 (アニメ停止中でも姿勢を反映するために毎フレーム呼ぶ)
	player->UpdateModelBlend();

	// 再生中はプレイヤーから現在のフレーム数を取得して同期
	if (!m_isPaused)
	{
		m_currentFrame = player->Debug_GetFrame();
	}

	// --- 攻撃終了判定 ---
	if (m_isAttacking)
	{
		// 選択中の技のパラメータを取得
		AttackParams* params = player->GetCurrentAttackParams();

		float totalDuration = params->totalDuration;
		// 秒数を60FPS基準のフレーム数に換算
		int animLengthInFrames = static_cast<int>(std::round(totalDuration / FRAME_TIME_60FPS));
		if (animLengthInFrames <= 0) animLengthInFrames = 1;

		// 最終フレームに達したら Idle に戻す
		if (m_currentFrame >= (animLengthInFrames - 1))
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

			// 秒 -> フレーム変換
			int startFrame = static_cast<int>(std::round(params->hitboxStart / FRAME_TIME_60FPS));
			int endFrame = static_cast<int>(std::round(params->hitboxEnd / FRAME_TIME_60FPS));

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

	ImGui::Begin("Attack Settings");
	ImGui::Text("FPS: %.1f", m_fps);
	ImGui::Separator();

	if (player)
	{
		// ==================================================
		// 技の選択
		// ==================================================
		ImGui::Text("Select Attack to Edit:");
		ImGui::RadioButton("Light Punch", &s_currentAttackType, 0);
		ImGui::SameLine();
		ImGui::RadioButton("Medium Punch", &s_currentAttackType, 1);

		ImGui::Separator();

		// ==================================================
		// アクションテスト (再生制御)
		// ==================================================
		ImGui::Text("Action Test");

		if (!m_isAttacking)
		{
			// ボタン1: 通常再生 (動作確認用)
			if (ImGui::Button("Test Play"))
			{
				// 選択中の技名を決定
				const char* animName = (s_currentAttackType == 0) ? "LightPunch" : "MediumPunch";
				player->Debug_SetAnimation(animName, true);

				m_isAttacking = true;
				m_isPaused = false; // 再生
				m_currentFrame = 0;
				m_animTimer = 0.0f; // タイマーリセット 

			}

			ImGui::SameLine();

			// ボタン2: 一時停止で開始 (コマ送り確認用)
			if (ImGui::Button("Test Play (Step)"))
			{
				const char* animName = (s_currentAttackType == 0) ? "LightPunch" : "MediumPunch";
				player->Debug_SetAnimation(animName, true);

				m_isAttacking = true;
				m_isPaused = true;  // ポーズ状態で開始
				m_currentFrame = 0;
				m_animTimer = 0.0f;
			}
		}
		else
		{
			ImGui::TextColored(ImVec4(1, 0, 0, 1), ">> ATTACKING <<");
		}

		ImGui::SameLine();

		// 手動ポーズ切り替え
		ImGui::Checkbox("Pause", &m_isPaused);

		// コマ送りUI (ポーズ中のみ有効)
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

		// ==================================================
		// パラメータ調整 (選択中の技)
		// ==================================================
		// 参照を取得 (書き換えると即座に反映される)
		AttackParams& params = (s_currentAttackType == 0) ? player->GetLightPunchParams() : player->GetMediumPunchParams();

		ImGui::Text(s_currentAttackType == 0 ? "Light Punch Parameters" : "Medium Punch Parameters");

		// 秒(float) を 60FPS基準のフレーム(int) に変換して表示・編集
		int totalFrames = static_cast<int>(std::round(params.totalDuration / FRAME_TIME_60FPS));
		int startFrames = static_cast<int>(std::round(params.hitboxStart / FRAME_TIME_60FPS));
		int endFrames = static_cast<int>(std::round(params.hitboxEnd / FRAME_TIME_60FPS));

		// 全体フレーム
		if (ImGui::InputInt("Total Duration", &totalFrames))
		{
			if (totalFrames < 1) totalFrames = 1;
			params.totalDuration = totalFrames * FRAME_TIME_60FPS; // 秒に戻して反映
		}
		// 攻撃発生フレーム
		if (ImGui::InputInt("Hitbox Start", &startFrames))
		{
			if (startFrames < 0) startFrames = 0;
			params.hitboxStart = startFrames * FRAME_TIME_60FPS;
		}
		// 攻撃終了フレーム
		if (ImGui::InputInt("Hitbox End", &endFrames))
		{
			if (endFrames < 0) endFrames = 0;
			params.hitboxEnd = endFrames * FRAME_TIME_60FPS;
		}

		// 攻撃判定 (Hitbox: 赤) の調整
		ImGui::Text("Red Box (Hitbox)");
		ImGui::SliderFloat2("Hitbox Offset", &params.hitboxOffset.x, -2.0f, 2.0f);
		ImGui::SliderFloat2("Hitbox Extents", &params.hitboxExtents.x, 0.1f, 2.0f);

		// ゲームバランスパラメータ
		ImGui::InputInt("Damage", &params.damage);
		ImGui::InputInt("On-Hit Advantage", &params.hitFrame);
		ImGui::InputInt("On-Block Advantage", &params.blockFrame);

		ImGui::Separator();

		// ==================================================
		// 攻撃中のくらい判定補正 (Green Box Modifiers)
		// ==================================================
		ImGui::Text("Green Box Modifiers (Attack Only)");

		ImGui::Text("Head");
		ImGui::SliderFloat2("Head Offset", &params.headOffsetVal.x, -1.0f, 1.0f);
		ImGui::SliderFloat2("Head Size+", &params.headSizeVal.x, -1.0f, 1.0f);

		ImGui::Text("Body");
		ImGui::SliderFloat2("Body Offset", &params.bodyOffsetVal.x, -1.0f, 1.0f);
		ImGui::SliderFloat2("Body Size+", &params.bodySizeVal.x, -1.0f, 1.0f);

		ImGui::Text("Legs");
		ImGui::SliderFloat2("Legs Offset", &params.legsOffsetVal.x, -1.0f, 1.0f);
		ImGui::SliderFloat2("Legs Size+", &params.legsSizeVal.x, -1.0f, 1.0f);

		ImGui::Separator();

		// ==================================================
		// 基本設定 (Base Settings) - 全技共通
		// ==================================================
		ImGui::Text("Player Base Hurtbox Settings");
		const char* hurtboxNames[] = { "Head", "Body", "Legs" };
		for (int i = 0; i < (int)HurtboxType::COUNT; ++i)
		{
			ImGui::PushID(i); // ID重複防止
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

		// ==================================================
		// 保存ボタン
		// ==================================================
		ImGui::Separator();
		if (ImGui::Button("SAVE All Settings"))
		{
			// 全てのデータ (基本設定 + 弱/中パンチパラメータ) をファイルに保存
			SavePlayerSettings();
		}
	}

	ImGui::End();
}