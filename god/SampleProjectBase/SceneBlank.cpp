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
#include <fstream> // ファイル入出力のために追加

using namespace DirectX;
using namespace DirectX::SimpleMath;

Texture* g_uiTex = nullptr;
const char* SETTINGS_FILE = "player_settings.ini"; // 設定ファイル名

void SceneBlank::SavePlayerSettings()
{
	Player* player = GetObj<Player>("Player");
	if (player)
	{
		std::ofstream ofs(SETTINGS_FILE);
		if (ofs.is_open())
		{
			// 現在の値をファイルに書き込む (単純なテキスト形式)
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

	// --- 設定ファイルの読み込み ---
	std::ifstream ifs(SETTINGS_FILE);
	if (ifs.is_open())
	{
		float moveSpeed;
		DirectX::XMFLOAT3 scale;
		DirectX::XMFLOAT3 boxExtents;
		DirectX::XMFLOAT3 boxOffset;

		// ファイルから値を読み込む
		ifs >> moveSpeed;
		ifs >> scale.x >> scale.y >> scale.z;
		ifs >> boxExtents.x >> boxExtents.y >> boxExtents.z;
		ifs >> boxOffset.x >> boxOffset.y >> boxOffset.z;

		if (!ifs.fail()) // 読み込みが成功したら適用
		{
			player->SetMoveSpeed(moveSpeed);
			player->SetScale(scale);
			player->SetBoundingBoxExtents(boxExtents);
			player->SetBoundingBoxOffset(boxOffset);
		}
		ifs.close();
	}


	// プレイヤー1の各種アニメーションの読み込み
	if (!player->Load("Assets/Model/knight/Idle.fbx", 0.02f, true, false))
	{
		MessageBox(NULL, "プレイヤーモデルの読み込みに失敗しました。", "Model Load Error", MB_OK);
	}
	player->GetModel()->LoadAnimation("Assets/Model/knight/Walking.fbx", "Walk", true);
	player->GetModel()->LoadAnimation("Assets/Model/knight/WalkBack.fbx", "WalkBack", true);


	// プレイヤー1の初期位置・回転
	player->SetPosition({ -2.0f, 0.0f, 0.0f });
	player->SetRotation({ 0.0f, DirectX::XM_PI / -2.0f, 0.0f }); // 右向きからスタート


	// プレイヤー2 (相手) の生成
	CreateObj<Player>("Player2");
	Player* player2 = GetObj<Player>("Player2");

	// プレイヤー1と同じモデル・アニメーションを読み込む
	if (!player2->Load("Assets/Model/knight/Idle.fbx", 0.02f, true, false))
	{
		MessageBox(NULL, "プレイヤー2モデルの読み込みに失敗しました。", "Model Load Error", MB_OK);
	}
	player2->GetModel()->LoadAnimation("Assets/Model/knight/Walking.fbx", "Walk", true);
	player2->GetModel()->LoadAnimation("Assets/Model/knight/WalkBack.fbx", "WalkBack", true);

	// プレイヤー2の初期位置・回転 (プレイヤー1と向き合う)
	player2->SetPosition({ 2.0f, 0.0f, 0.0f }); // 少し右からスタート
	player2->SetRotation({ 0.0f, DirectX::XM_PI / 2.0f, 0.0f }); // 左向き


	// アニメーションの初期状態を設定 (プレイヤー1用)
	m_currentState = { "Idle", 0 };
	m_previousState = { "Idle", 0 };
	m_blendFactor = 1.0f;

	m_showImGui = true;

	g_uiTex = new Texture();
	//g_uiTex->Create("Assets/UI/apple.jpg");
}

void SceneBlank::Uninit()
{
	// Uninitからは保存処理を削除

	if (g_uiTex) {
		delete g_uiTex;
		g_uiTex = nullptr;
	}
}

void SceneBlank::Update(float tick)
{
	// FPSの計算 (tickが0になるのを防ぐ)
	if (tick > 0.0f)
	{
		m_fps = 1.0f / tick;
	}
	else
	{
		m_fps = 0.0f;
	}

	// Tabキーが押されたら、ImGuiの表示/非表示を切り替える
	if (IsKeyTrigger(VK_TAB))
	{
		m_showImGui = !m_showImGui; // true/falseを反転させる
	}

	Player* player = GetObj<Player>("Player");
	Player* player2 = GetObj<Player>("Player2");

	// --- プレイヤー1の入力処理と更新 ---
	if (player) {
		// 最初に、現在のY方向の速度（重力用）を維持しつつ、X/Zをリセット
		DirectX::XMFLOAT3 vel = player->GetVelocity();
		vel.x = 0.0f;
		vel.z = 0.0f;

		// 右クリック中は移動しない
		if (!IsKeyPress(VK_RBUTTON)) {
			// 'A' 'D' キーでX方向の速度を設定
			if (IsKeyPress('A')) {
				vel.x = -player->GetMoveSpeed();
			}
			else if (IsKeyPress('D')) {
				vel.x = player->GetMoveSpeed();
			}

			// ジャンプ入力
			if (IsKeyTrigger('W') || IsKeyTrigger(VK_SPACE)) {
				player->Jump(); // Jump() は m_isJumping をチェックする
			}
		}

		// 計算した速度をプレイヤーにセット
		player->SetVelocity(vel);

		// 物理演算（重力、位置更新、地面判定）を実行
		player->Update(tick);


		// --- プレイヤー1のアニメーション状態遷移 ---
		Vector3 velocity = player->GetVelocity();
		Vector3 rotation = player->GetRotation();
		PlayerState targetState = PlayerState::IDLE;

		// 水平方向の移動があるかチェック
		if (Vector2(velocity.x, velocity.z).LengthSquared() > 0.01f)
		{
			// プレイヤーの前方ベクトルを計算
			Matrix rotMat = Matrix::CreateRotationY(rotation.y);
			// 変更: モデルの向きに合わせて前方ベクトルを-Z軸にする
			Vector3 forward = Vector3::Transform(Vector3(0.0f, 0.0f, -1.0f), rotMat);

			// 移動方向ベクトル（y軸を無視して正規化）
			Vector3 moveDir = velocity;
			moveDir.y = 0.0f;
			moveDir.Normalize();

			// 前方ベクトルと移動方向ベクトルの内積を計算
			float dot = forward.Dot(moveDir);

			if (dot > 0.5f) // 概ね前方に移動している
			{
				targetState = PlayerState::WALKING;
			}
			else if (dot < -0.5f) // 概ね後方に移動している
			{
				targetState = PlayerState::WALKING_BACK;
			}
			else 
			{
				targetState = PlayerState::WALKING;
			}
		}

		// 状態からアニメーション名を決定
		const char* targetAnimName = "Idle";
		switch (targetState)
		{
		case PlayerState::WALKING:      targetAnimName = "Walk"; break;
		case PlayerState::WALKING_BACK: targetAnimName = "WalkBack"; break;
		}

		// 遷移処理
		if (strcmp(m_currentState.name, targetAnimName) != 0)
		{
			m_previousState = m_currentState;
			m_currentState.name = targetAnimName;
			m_currentState.frame = 0;
			m_blendFactor = 0.0f;
		}

		// ブレンド率を更新
		if (m_blendFactor < 1.0f)
		{
			m_blendFactor += tick / m_transitionDuration;
			if (m_blendFactor > 1.0f) m_blendFactor = 1.0f;
		}

		m_currentState.frame++;
	}

	// --- プレイヤー2の入力処理と更新 ---
	if (player2) {
		// プレイヤー2も、入力がない場合は水平速度をリセット
		DirectX::XMFLOAT3 vel2 = player2->GetVelocity();
		vel2.x = 0.0f;
		vel2.z = 0.0f;

		// ここにプレイヤー2用の入力処理

		player2->SetVelocity(vel2);

		// プレイヤー2の物理演算を実行
		player2->Update(tick);
	}


	// --- 当たり判定チェック ---
	if (player && player2)
	{
		// それぞれのBoundingBoxを取得
		DirectX::BoundingBox box1 = player->GetBoundingBox();
		DirectX::BoundingBox box2 = player2->GetBoundingBox();

		// 衝突しているかチェック
		bool isColliding = box1.Intersects(box2);

		// 結果を両方のプレイヤーにセット
		player->SetIsColliding(isColliding);
		player2->SetIsColliding(isColliding);
	}
}

void SceneBlank::Draw()
{
	// --- 1. 3Dオブジェクトの描画 ---
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

		// プレイヤーの動的スケールを取得
		XMFLOAT3 pScale = player->GetScale();
		Matrix playerScaleMat = Matrix::CreateScale(pScale.x, pScale.y, pScale.z);

		// モデルの基本スケールと組み合わせる
		Matrix modelBaseScaleMat = player->GetModel()->GetScaleBaseMatrix();
		Matrix rotMat = DirectX::XMMatrixRotationRollPitchYaw(rot.x, rot.y, rot.z);
		Matrix transMat = Matrix::CreateTranslation(pos.x, pos.y, pos.z);
		Matrix world = modelBaseScaleMat * playerScaleMat * rotMat * transMat; // Playerスケールを適用

		XMStoreFloat4x4(&mat[0], XMMatrixTranspose(world));

		shader[0]->WriteBuffer(0, mat);
		shader[1]->WriteBuffer(0, light);
		shader[1]->WriteBuffer(1, camera);

		player->GetModel()->UpdateWithBlend(
			m_currentState.name, m_currentState.frame,
			m_previousState.name, m_previousState.frame,
			m_blendFactor);

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
		XMFLOAT3 pScale = player2->GetScale(); // Player2のスケール

		Matrix playerScaleMat = Matrix::CreateScale(pScale.x, pScale.y, pScale.z);
		Matrix modelBaseScaleMat = player2->GetModel()->GetScaleBaseMatrix();
		Matrix rotMat = DirectX::XMMatrixRotationRollPitchYaw(rot.x, rot.y, rot.z);
		Matrix transMat = Matrix::CreateTranslation(pos.x, pos.y, pos.z);
		Matrix world = modelBaseScaleMat * playerScaleMat * rotMat * transMat;

		XMStoreFloat4x4(&mat[0], XMMatrixTranspose(world));

		// シェーダーバッファ書き込み
		shader[0]->WriteBuffer(0, mat);
		shader[1]->WriteBuffer(0, light);
		shader[1]->WriteBuffer(1, camera);

		// Player2はIdleで描画
		player2->GetModel()->UpdateWithBlend(
			"Idle", 0,
			"Idle", 0,
			1.0f);

		player2->SetVertexShader(shader[0]);
		player2->SetPixelShader(shader[1]);
		player2->Draw();
		player2->DrawBoundingBox(); // プレイヤー2の当たり判定も描画
	}


	// --- 2. ImGuiの描画 ---
	if (m_showImGui)
	{
		DrawImGui();
	}


	// --- 3. 2D UI (SimpleUI) の描画 ----
	// 画面サイズ（例: 1280x720）に合わせて右上に配置
	constexpr float screenWidth = 1280.0f;
	constexpr float screenHeight = 720.0f;
	float uiWidth = 200.0f;
	float uiHeight = 50.0f;
	float x = screenWidth - uiWidth - 20.0f;
	float y = 20.0f;

	// ピクセル→NDC変換
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

	// ImGuiを使用してFPSを表示する
	ImGui::Begin("Information"); // "Information" という名前のウィンドウを作成
	ImGui::Text("FPS: %.1f", m_fps); // m_fpsの値を小数点以下1桁で表示

	if (player)
	{
		ImGui::Separator(); // 区切り線

		// 移動速度
		float moveSpeed = player->GetMoveSpeed();
		if (ImGui::SliderFloat("Move Speed", &moveSpeed, 0.0f, 10.0f))
		{
			player->SetMoveSpeed(moveSpeed);
			SavePlayerSettings(); // 値が変更されたら即保存
		}

		// サイズ（スケール）
		XMFLOAT3 scale = player->GetScale();
		if (ImGui::SliderFloat3("Scale", &scale.x, 0.1f, 5.0f))
		{
			player->SetScale(scale);
			SavePlayerSettings(); // 値が変更されたら即保存
		}

		ImGui::Separator(); // 区切り線
		ImGui::Text("Bounding Box"); // 当たり判定セクション

		// 当たり判定の大きさ (Extents)
		XMFLOAT3 boxExtents = player->GetBoundingBoxExtents();
		if (ImGui::SliderFloat3("Box Extents", &boxExtents.x, 0.1f, 5.0f))
		{
			player->SetBoundingBoxExtents(boxExtents);
			SavePlayerSettings(); // 値が変更されたら即保存
		}

		// 当たり判定の位置 (Offset)
		XMFLOAT3 boxOffset = player->GetBoundingBoxOffset();
		if (ImGui::SliderFloat3("Box Offset", &boxOffset.x, -5.0f, 5.0f))
		{
			player->SetBoundingBoxOffset(boxOffset);
			SavePlayerSettings(); // 値が変更されたら即保存
		}
	}

	ImGui::End();
}