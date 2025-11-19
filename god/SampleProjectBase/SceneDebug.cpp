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
#include <cmath> // ★ round (四捨五入) のために追加

using namespace DirectX;
using namespace DirectX::SimpleMath;

// SceneBlank と同じグローバル変数・定数
Texture* g_uiTex_debug = nullptr;
const char* SETTINGS_FILE_DEBUG = "player_settings.ini";

// ★ 60FPSを基準とした1フレームの時間
const float FRAME_TIME_60FPS = 1.0f / 60.0f;

void SceneDebug::SavePlayerSettings()
{
	Player* player = GetObj<Player>("Player");
	if (player)
	{
		std::ofstream ofs(SETTINGS_FILE_DEBUG);
		if (ofs.is_open())
		{
			// 既存の保存 (くらい判定など)
			ofs << player->GetMoveSpeed() << std::endl;
			DirectX::XMFLOAT3 scale = player->GetScale();
			ofs << scale.x << " " << scale.y << " " << scale.z << std::endl;
			DirectX::XMFLOAT2 boxExtents = player->GetBoundingBoxExtents();
			ofs << boxExtents.x << " " << boxExtents.y << std::endl;
			DirectX::XMFLOAT2 boxOffset = player->GetBoundingBoxOffset();
			ofs << boxOffset.x << " " << boxOffset.y << std::endl;

			// AttackParams (攻撃判定) の保存
			AttackParams& params = player->GetLightPunchParams();
			ofs << params.totalDuration << std::endl;
			ofs << params.hitboxStart << std::endl;
			ofs << params.hitboxEnd << std::endl;
			ofs << params.hitboxOffset.x << " " << params.hitboxOffset.y << std::endl;
			ofs << params.hitboxExtents.x << " " << params.hitboxExtents.y << std::endl;

			ofs.close();
		}
	}
}

void SceneDebug::Init()
{
	// --- シェーダー読み込み (SceneBlank と同様) ---
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

	// --- プレイヤーを1体だけ生成 ---
	CreateObj<Player>("Player");
	Player* player = GetObj<Player>("Player");

	// ★FSMを無効化するため、AIに設定
	player->SetInputType(PlayerInputType::AI);

	// --- 設定ファイルの読み込み (SceneBlank と同様) ---
	float moveSpeed = 2.0f;
	DirectX::XMFLOAT3 scale = { 1.0f, 1.0f, 1.0f };
	DirectX::XMFLOAT2 boxExtents = { 0.5f, 1.0f };
	DirectX::XMFLOAT2 boxOffset = { 0.0f, 1.0f };
	AttackParams params = player->GetLightPunchParams();

	std::ifstream ifs(SETTINGS_FILE_DEBUG);
	if (ifs.is_open())
	{
		// 既存の読み込み
		ifs >> moveSpeed;
		ifs >> scale.x >> scale.y >> scale.z;
		ifs >> boxExtents.x >> boxExtents.y;
		ifs >> boxOffset.x >> boxOffset.y;

		// AttackParams の読み込み
		if (!ifs.eof()) ifs >> params.totalDuration;
		if (!ifs.eof()) ifs >> params.hitboxStart;
		if (!ifs.eof()) ifs >> params.hitboxEnd;
		if (!ifs.eof()) ifs >> params.hitboxOffset.x >> params.hitboxOffset.y;
		if (!ifs.eof()) ifs >> params.hitboxExtents.x >> params.hitboxExtents.y;

		ifs.close();
	}

	// 読み込んだ値をプレイヤーに設定
	player->SetMoveSpeed(moveSpeed);
	player->SetScale(scale);
	player->SetBoundingBoxExtents(boxExtents);
	player->SetBoundingBoxOffset(boxOffset);
	player->GetLightPunchParams() = params;


	// --- モデル・アニメーション読み込み ---
	if (!player->Load("Assets/Model/knight/Idle.fbx", 0.02f, true, false))
	{
		MessageBox(NULL, "プレイヤーモデルの読み込みに失敗しました。", "Model Load Error", MB_OK);
	}
	player->GetModel()->LoadAnimation("Assets/Model/knight/Walking.fbx", "Walk", true);
	player->GetModel()->LoadAnimation("Assets/Model/knight/WalkBack.fbx", "WalkBack", true);
	player->GetModel()->LoadAnimation("Assets/Model/knight/LightPunch.fbx", "LightPunch", true);

	// アニメーションを初期化
	player->Debug_SetAnimation(m_animNames[m_selectedAnimIndex]);

	// プレイヤーを中央に配置
	player->SetPosition({ 0.0f, 0.0f, 0.0f });
	player->SetRotation({ 0.0f, DirectX::XM_PI / -2.0f, 0.0f });

	m_showImGui = true;
	g_uiTex_debug = new Texture();

	// デバッグ用の初期化
	m_isPaused = true;
	m_selectedAnimIndex = 0;
	m_currentFrame = 0;
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

	// ImGui の状態に応じてアニメーションを手動制御する

	if (m_isPaused)
	{
		// 一時停止中
		player->Debug_SetFrame(m_currentFrame);
		player->UpdateModelBlend(); // ボーンを更新
	}
	else
	{
		// 再生中
		player->UpdateAnimation(tick); // アニメーションのフレームを進める
		player->UpdateModelBlend();    // ボーンを更新
		m_currentFrame = player->Debug_GetFrame(); // ImGui表示用に現在フレームを同期

		// "LightPunch" (index 3) は、デバッグシーンでは非ループとして扱う
		if (m_selectedAnimIndex == 3)
		{
			// ---  (FPSの計算) ---
			// ImGuiの表示と合わせるため、60FPS固定で計算する
			float totalDuration = player->GetLightPunchParams().totalDuration;
			// 秒を60FPS基準のフレームに変換 (四捨五入)
			int animLengthInFrames = static_cast<int>(std::round(totalDuration / FRAME_TIME_60FPS));
			if (animLengthInFrames <= 0) animLengthInFrames = 1;

			// 終了フレームに達したら（または超えたら）停止
			if (m_currentFrame >= (animLengthInFrames - 1))
			{
				m_isPaused = true; // 再生停止
				m_currentFrame = animLengthInFrames - 1; // 最終フレームで固定
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


	// --- プレイヤーの描画 (Player2 は削除) ---
	Player* player = GetObj<Player>("Player");
	if (player) {
		XMFLOAT3 pos = player->GetPosition();
		XMFLOAT3 rot = player->GetRotation();
		XMFLOAT3 pScale = player->GetScale();
		Matrix playerScaleMat = Matrix::CreateScale(pScale.x, pScale.y, pScale.z);
		Matrix modelBaseScaleMat = player->GetModel()->GetScaleBaseMatrix();
		Matrix rotMat = DirectX::XMMatrixRotationRollPitchYaw(rot.x, rot.y, rot.z);
		Matrix transMat = Matrix::CreateTranslation(pos.x, pos.y, 0.0f); // Zを0に固定
		Matrix world = modelBaseScaleMat * playerScaleMat * rotMat * transMat;

		XMStoreFloat4x4(&mat[0], XMMatrixTranspose(world));

		shader[0]->WriteBuffer(0, mat);
		shader[1]->WriteBuffer(0, light);
		shader[1]->WriteBuffer(1, camera);

		player->SetVertexShader(shader[0]);
		player->SetPixelShader(shader[1]);
		player->Draw();
		player->DrawBoundingBox(); // くらい判定（緑箱）

		// "LightPunch" (インデックス3) が選択されている時だけ
		// 攻撃判定（赤箱）のデフォルト値を描画する
		if (m_selectedAnimIndex == 3)
		{
			AttackParams& params = player->GetLightPunchParams();
			player->UpdateHitbox(params.hitboxOffset, params.hitboxExtents);
			player->SetActiveHitbox(true); // 常に表示
			player->DrawHitbox();
			player->SetActiveHitbox(false); // 描画後にリセット
		}
	}

	// --- ImGuiの描画 ---
	if (m_showImGui)
	{
		DrawImGui();
	}

}

// デバッグ機能の核心
void SceneDebug::DrawImGui()
{
	Player* player = GetObj<Player>("Player");

	ImGui::Begin("Debug Settings");
	ImGui::Text("FPS: %.1f", m_fps);
	ImGui::Separator();
	ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Camera: Use Mouse/Keys to move");
	ImGui::Text("Player is at (0, 0, 0)");


	if (player)
	{
		// --- 当たり判定とスケール (SceneBlank からコピー) ---
		ImGui::Separator();
		ImGui::Text("Collision & Scale");

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

		ImGui::Separator();
		ImGui::Text("Hurtbox (Green Box)"); // くらい判定
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

		// --- アニメーション制御 ---
		ImGui::Separator();
		ImGui::Text("Animation Control");

		// アニメーション選択
		if (ImGui::Combo("Animation", &m_selectedAnimIndex, m_animNames, IM_ARRAYSIZE(m_animNames)))
		{
			// アニメーションを切り替えて再生開始
			player->Debug_SetAnimation(m_animNames[m_selectedAnimIndex]);
			m_currentFrame = 0; // フレームをリセット
			m_isPaused = true;  // 切り替え時は一時停止
		}

		// "LightPunch" (index 3) かどうかでUIを変更
		if (m_selectedAnimIndex == 3)
		{
			// "LightPunch" の場合は「再生」ボタン
			if (ImGui::Button("Play LightPunch"))
			{
				player->Debug_SetAnimation("LightPunch", true);
				m_currentFrame = 0;
				m_isPaused = false; // 再生開始
			}
			ImGui::SameLine();
			ImGui::Checkbox("Pause", &m_isPaused); // 一時停止もできるように
		}
		else
		{
			// "LightPunch" 以外は 再生/一時停止 のチェックボックスのみ
			ImGui::Checkbox("Pause (Loop)", &m_isPaused);
		}


		// フレーム制御 (一時停止中のみ)
		if (m_isPaused)
		{
			// コマ送り
			if (ImGui::Button("Frame Step"))
			{
				m_currentFrame++; // m_currentFrame を進める
			}
			ImGui::SameLine();
			// フレーム指定
			ImGui::InputInt("Frame", &m_currentFrame);
		}
		else
		{
			ImGui::Text("Frame: %d", m_currentFrame);
		}

		// "LightPunch" (index 3) が選択されている時だけ攻撃判定スライダーを表示
		if (m_selectedAnimIndex == 3)
		{
			ImGui::Separator();
			ImGui::Text("Light Punch Params (Red Box) - 60FPS base");
			AttackParams& params = player->GetLightPunchParams();

			// (フレーム単位のUI) ---

			// (A) float(秒) -> int(フレーム) に変換 (四捨五入)
			int totalFrames = static_cast<int>(std::round(params.totalDuration / FRAME_TIME_60FPS));
			int startFrames = static_cast<int>(std::round(params.hitboxStart / FRAME_TIME_60FPS));
			int endFrames = static_cast<int>(std::round(params.hitboxEnd / FRAME_TIME_60FPS));

			// ImGui::InputInt でフレーム数を編集
			if (ImGui::InputInt("Total Frames", &totalFrames))
			{
				if (totalFrames < 1) totalFrames = 1; // 最小1フレーム
				// int(フレーム) -> float(秒) に逆変換して保存
				params.totalDuration = totalFrames * FRAME_TIME_60FPS;
			}
			if (ImGui::InputInt("Hitbox Start Frame", &startFrames))
			{
				if (startFrames < 0) startFrames = 0;
				params.hitboxStart = startFrames * FRAME_TIME_60FPS;
			}
			if (ImGui::InputInt("Hitbox End Frame", &endFrames))
			{
				if (endFrames < 0) endFrames = 0;
				params.hitboxEnd = endFrames * FRAME_TIME_60FPS;
			}

			//  攻撃判定の位置とサイズはそのまま 
			ImGui::SliderFloat2("Hitbox Offset", &params.hitboxOffset.x, -2.0f, 2.0f);
			ImGui::SliderFloat2("Hitbox Extents", &params.hitboxExtents.x, 0.1f, 2.0f);
		}


		// --- 保存ボタン ---
		ImGui::Separator();
		if (ImGui::Button("SAVE Settings"))
		{
			SavePlayerSettings(); // AttackParams も保存されます
		}
	}

	ImGui::End();
}