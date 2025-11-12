#include "math.h"
#include "SceneBlank.h"
#include "Geometory.h"
#include "DebugLog.h"
#include "Model.h"
#include "CameraBase.h"
#include "LightBase.h"
#include "Shader.h"
#include "Player.h" 
#include "SimpleUI.h"
#include "Texture.h"
#include "Input.h" 
#include <system/imgui/imgui.h>
#include <fstream> 

using namespace DirectX;
using namespace DirectX::SimpleMath;

Texture* g_uiTex = nullptr;
const char* SETTINGS_FILE = "player_settings.ini";

void SceneBlank::SavePlayerSettings()
{
	Player* player = GetObj<Player>("Player");
	if (player)
	{
		std::ofstream ofs(SETTINGS_FILE);
		if (ofs.is_open())
		{
			ofs << player->GetMoveSpeed() << std::endl;
			DirectX::XMFLOAT3 scale = player->GetScale();
			ofs << scale.x << " " << scale.y << " " << scale.z << std::endl;
			DirectX::XMFLOAT3 boxExtents = player->GetBoundingBoxExtents();
			ofs << boxExtents.x << " " << boxExtents.y << " " << boxExtents.z << std::endl;
			DirectX::XMFLOAT3 boxOffset = player->GetBoundingBoxOffset();
			ofs << boxOffset.x << " " << boxOffset.y << " " << boxOffset.z << std::endl;
			ofs.close();
		}
	}
}

void SceneBlank::Init()
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

	// プレイヤー1生成
	CreateObj<Player>("Player");
	Player* player = GetObj<Player>("Player");

	// プレイヤー1 を PLAYER_1 (A/Dキー) に設定
	player->SetInputType(PlayerInputType::PLAYER_1);

	// デフォルト値を設定
	float moveSpeed = 2.0f;
	DirectX::XMFLOAT3 scale = { 1.0f, 1.0f, 1.0f };
	DirectX::XMFLOAT3 boxExtents = { 0.5f, 1.0f, 0.5f };
	DirectX::XMFLOAT3 boxOffset = { 0.0f, 1.0f, 0.0f };

	std::ifstream ifs(SETTINGS_FILE);
	if (ifs.is_open())
	{
		ifs >> moveSpeed;
		ifs >> scale.x >> scale.y >> scale.z;
		ifs >> boxExtents.x >> boxExtents.y >> boxExtents.z;
		ifs >> boxOffset.x >> boxOffset.y >> boxOffset.z;
		ifs.close();
	}

	player->SetMoveSpeed(moveSpeed);
	player->SetScale(scale);
	player->SetBoundingBoxExtents(boxExtents);
	player->SetBoundingBoxOffset(boxOffset);

	if (!player->Load("Assets/Model/knight/Idle.fbx", 0.02f, true, false))
	{
		MessageBox(NULL, "プレイヤーモデルの読み込みに失敗しました。", "Model Load Error", MB_OK);
	}
	player->GetModel()->LoadAnimation("Assets/Model/knight/Walking.fbx", "Walk", true);
	player->GetModel()->LoadAnimation("Assets/Model/knight/WalkBack.fbx", "WalkBack", true);

	player->SetPosition({ -2.0f, 0.0f, 0.0f });
	player->SetRotation({ 0.0f, DirectX::XM_PI / -2.0f, 0.0f });


	// プレイヤー2 (相手) の生成
	CreateObj<Player>("Player2");
	Player* player2 = GetObj<Player>("Player2");

	// プレイヤー2 を PLAYER_2 (矢印キー) に設定
	player2->SetInputType(PlayerInputType::PLAYER_2);

	player2->SetMoveSpeed(moveSpeed);
	player2->SetScale(scale);
	player2->SetBoundingBoxExtents(boxExtents);
	player2->SetBoundingBoxOffset(boxOffset);
	if (!player2->Load("Assets/Model/knight/Idle.fbx", 0.02f, true, false))
	{
		MessageBox(NULL, "プレイヤー2モデルの読み込みに失敗しました。", "Model Load Error", MB_OK);
	}
	player2->GetModel()->LoadAnimation("Assets/Model/knight/Walking.fbx", "Walk", true);
	player2->GetModel()->LoadAnimation("Assets/Model/knight/WalkBack.fbx", "WalkBack", true);
	player2->SetPosition({ 2.0f, 0.0f, 0.0f });
	player2->SetRotation({ 0.0f, DirectX::XM_PI / 2.0f, 0.0f });

	m_showImGui = true;

	g_uiTex = new Texture();
}

void SceneBlank::Uninit()
{
	if (g_uiTex) {
		delete g_uiTex;
		g_uiTex = nullptr;
	}
}

void SceneBlank::Update(float tick)
{
	if (tick > 0.0f) { m_fps = 1.0f / tick; }
	else { m_fps = 0.0f; }

	if (IsKeyTrigger(VK_TAB)) {
		m_showImGui = !m_showImGui;
	}

	Player* player = GetObj<Player>("Player");
	Player* player2 = GetObj<Player>("Player2");

	// --- プレイヤー1の入力処理と更新 ---
	if (player) {
		player->Update(tick); // PLAYER_1 の入力で FSM が動く
	}

	// --- プレイヤー2の入力処理と更新 ---
	if (player2) {
		player2->Update(tick); // PLAYER_2 の入力で FSM が動く
	}


	// --- 当たり判定チェック ---
	if (player && player2)
	{
		DirectX::BoundingBox box1 = player->GetBoundingBox();
		DirectX::BoundingBox box2 = player2->GetBoundingBox();
		bool isColliding = box1.Intersects(box2);
		player->SetIsColliding(isColliding);
		player2->SetIsColliding(isColliding);
	}
}

void SceneBlank::Draw()
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

	// --- プレイヤー1の描画 ---
	Player* player = GetObj<Player>("Player");
	if (player) {
		XMFLOAT3 pos = player->GetPosition();
		XMFLOAT3 rot = player->GetRotation();
		XMFLOAT3 pScale = player->GetScale();
		Matrix playerScaleMat = Matrix::CreateScale(pScale.x, pScale.y, pScale.z);
		Matrix modelBaseScaleMat = player->GetModel()->GetScaleBaseMatrix();
		Matrix rotMat = DirectX::XMMatrixRotationRollPitchYaw(rot.x, rot.y, rot.z);
		Matrix transMat = Matrix::CreateTranslation(pos.x, pos.y, pos.z);
		Matrix world = modelBaseScaleMat * playerScaleMat * rotMat * transMat;

		XMStoreFloat4x4(&mat[0], XMMatrixTranspose(world));

		shader[0]->WriteBuffer(0, mat);
		shader[1]->WriteBuffer(0, light);
		shader[1]->WriteBuffer(1, camera);

		player->SetVertexShader(shader[0]);
		player->SetPixelShader(shader[1]);
		player->Draw();
		player->DrawBoundingBox();
	}

	// --- プレイヤー2の描画 ---
	Player* player2 = GetObj<Player>("Player2");
	if (player2) {
		XMFLOAT3 pos = player2->GetPosition();
		XMFLOAT3 rot = player2->GetRotation();
		XMFLOAT3 pScale = player2->GetScale();
		Matrix playerScaleMat = Matrix::CreateScale(pScale.x, pScale.y, pScale.z);
		Matrix modelBaseScaleMat = player2->GetModel()->GetScaleBaseMatrix();
		Matrix rotMat = DirectX::XMMatrixRotationRollPitchYaw(rot.x, rot.y, rot.z);
		Matrix transMat = Matrix::CreateTranslation(pos.x, pos.y, pos.z);
		Matrix world = modelBaseScaleMat * playerScaleMat * rotMat * transMat;

		XMStoreFloat4x4(&mat[0], XMMatrixTranspose(world));

		shader[0]->WriteBuffer(0, mat);
		shader[1]->WriteBuffer(0, light);
		shader[1]->WriteBuffer(1, camera);

		player2->SetVertexShader(shader[0]);
		player2->SetPixelShader(shader[1]);
		player2->Draw();
		player2->DrawBoundingBox();
	}


	// --- 2. ImGuiの描画 ---
	if (m_showImGui)
	{
		DrawImGui();
	}


	// --- 3. 2D UI (SimpleUI) の描画 ----
	constexpr float screenWidth = 1280.0f;
	constexpr float screenHeight = 720.0f;
	float uiWidth = 200.0f;
	float uiHeight = 50.0f;
	float x = screenWidth - uiWidth - 20.0f;
	float y = 20.0f;
	float ndcX = (x / screenWidth) * 2.0f - 1.0f;
	float ndcY = 1.0f - (y / screenHeight) * 2.0f;
	float ndcW = (uiWidth / screenWidth) * 2.0f;
	float ndcH = (uiHeight / screenHeight) * 2.0f;
	SimpleUI::Clear();
	SimpleUI::AddRect(ndcX, ndcY, ndcW, ndcH, { 1,1,1,1 }, g_uiTex);
	DirectX::XMFLOAT4X4 identity;
	DirectX::XMStoreFloat4x4(&identity, DirectX::XMMatrixIdentity());
	SimpleUI::SetMatrix(identity, identity, identity);
	SimpleUI::DrawAll();
}


void SceneBlank::DrawImGui()
{
	Player* player = GetObj<Player>("Player");
	Player* player2 = GetObj<Player>("Player2");

	ImGui::Begin("Information");
	ImGui::Text("FPS: %.1f", m_fps);

	if (player)
	{
		ImGui::Separator();

		// --- 値の編集 ---
		// スライダーで値が変更されたら、player と player2 の両方に適用する

		float moveSpeed = player->GetMoveSpeed();
		if (ImGui::SliderFloat("Move Speed", &moveSpeed, 0.0f, 10.0f))
		{
			player->SetMoveSpeed(moveSpeed);
			if (player2) player2->SetMoveSpeed(moveSpeed);
		}

		XMFLOAT3 scale = player->GetScale();
		if (ImGui::SliderFloat3("Scale", &scale.x, 0.1f, 5.0f))
		{
			player->SetScale(scale);
			if (player2) player2->SetScale(scale);
		}

		ImGui::Separator();
		ImGui::Text("Bounding Box");

		XMFLOAT3 boxExtents = player->GetBoundingBoxExtents();
		if (ImGui::SliderFloat3("Box Extents", &boxExtents.x, 0.1f, 5.0f))
		{
			player->SetBoundingBoxExtents(boxExtents);
			if (player2) player2->SetBoundingBoxExtents(boxExtents);
		}

		XMFLOAT3 boxOffset = player->GetBoundingBoxOffset();
		if (ImGui::SliderFloat3("Box Offset", &boxOffset.x, -5.0f, 5.0f))
		{
			player->SetBoundingBoxOffset(boxOffset);
			if (player2) player2->SetBoundingBoxOffset(boxOffset);
		}

		// --- 保存ボタン ---
		ImGui::Separator();
		if (ImGui::Button("SAVE"))
		{
			SavePlayerSettings(); // ボタンが押された時だけ保存
		}
	}

	ImGui::End();
}