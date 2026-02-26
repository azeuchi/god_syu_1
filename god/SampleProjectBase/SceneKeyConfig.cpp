#include "SceneKeyConfig.h"
#include <system/imgui/imgui.h>
#include "Input.h"
#include "Player.h"
#include <stdio.h>
#include "CameraBase.h"
#include "LightBase.h"
#include "Shader.h"
#include "Model.h"
#include "PlayerParameterLoader.h"
#include "BattleCollision.h"
#include "HitEffect.h"
#include "Projectile.h"
#include "Geometory.h"
#include <algorithm>

using namespace DirectX;
using namespace DirectX::SimpleMath;

// カメラの移動制限範囲
const float CAMERA_LIMIT_X = 4.0f;

bool SceneKeyConfig::s_isConfigSet = false;

void SceneKeyConfig::Init()
{
	s_isConfigSet = false;
	m_waitBindKeyPtr = nullptr;

	// ==================================================
	// シェーダーの読み込み
	// ==================================================
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

	// 共通のアニメーション読み込みラムダ
	auto LoadAnims = [](Player* p) {
		p->GetModel()->LoadAnimation("Assets/Model/knight/Walking.fbx", "Walk", true);
		p->GetModel()->LoadAnimation("Assets/Model/knight/WalkBack.fbx", "WalkBack", true);
		p->GetModel()->LoadAnimation("Assets/Model/knight/CrouchIdle.fbx", "CrouchIdle", true);
		p->GetModel()->LoadAnimation("Assets/Model/knight/LightPunch.fbx", "LightPunch", true);
		p->GetModel()->LoadAnimation("Assets/Model/knight/MediumPunch.fbx", "MediumPunch", true);
		p->GetModel()->LoadAnimation("Assets/Model/knight/HeavyPunch.fbx", "HeavyPunch", true);
		p->GetModel()->LoadAnimation("Assets/Model/knight/MediumKick.fbx", "MediumKick", true);
		p->GetModel()->LoadAnimation("Assets/Model/knight/HeavyKick.fbx", "HeavyKick", true);
		p->GetModel()->LoadAnimation("Assets/Model/knight/Jump.fbx", "Jump", true);
		p->GetModel()->LoadAnimation("Assets/Model/knight/Damage.fbx", "Damage", true);
		p->GetModel()->LoadAnimation("Assets/Model/knight/Down.fbx", "Down", true);
		p->GetModel()->LoadAnimation("Assets/Model/knight/WakeUp.fbx", "WakeUp", true);
		p->GetModel()->LoadAnimation("Assets/Model/knight/Hadouken.fbx", "Hadouken", true);
		p->GetModel()->LoadAnimation("Assets/Model/knight/Death.fbx", "Death", true);
		};

	// ==================================================
	// プレイヤー1の生成と設定ロード
	// ==================================================
	CreateObj<Player>("Player");
	Player* player = GetObj<Player>("Player");
	player->SetInputType(PlayerInputType::PLAYER_1);
	PlayerParameterLoader::LoadSettings(player);
	if (!player->Load("Assets/Model/knight/Idle.fbx", 0.014f, true, false))
	{
		MessageBox(NULL, "プレイヤーモデルの読み込みに失敗しました。", "Error", MB_OK);
	}
	LoadAnims(player);
	player->SetPosition({ -2.0f, 0.0f, 0.0f });
	player->SetRotation({ 0.0f, DirectX::XM_PI / -2.0f, 0.0f });
	player->Reset();

	// ==================================================
	// プレイヤー2の生成
	// ==================================================
	CreateObj<Player>("Player2");
	Player* player2 = GetObj<Player>("Player2");
	player2->SetInputType(PlayerInputType::PLAYER_2);
	player2->SetMoveSpeed(player->GetMoveSpeed());
	DirectX::XMFLOAT3 scaleP2 = player->GetScale();
	scaleP2.x *= -1.0f; // X軸反転
	player2->SetScale(scaleP2);
	PlayerParameterLoader::CopyParameters(player, player2);
	if (!player2->Load("Assets/Model/knight/Idle.fbx", 0.014f, true, false))
	{
		MessageBox(NULL, "プレイヤー2モデルの読み込みに失敗しました。", "Error", MB_OK);
	}
	LoadAnims(player2);
	player2->SetPosition({ 2.0f, 0.0f, 0.0f });
	player2->SetRotation({ 0.0f, DirectX::XM_PI / 2.0f, 0.0f });
	player2->Reset();

	// ==================================================
	// エフェクト・カメラ初期化
	// ==================================================
	for (int i = 0; i < 10; i++)
	{
		m_hitEffects.push_back(new HitEffect());
	}

	CameraBase* pCamera = GetObj<CameraBase>("Camera");
	if (pCamera)
	{
		pCamera->SetPos({ 0.0f, 1.2f, -5.0f });
		pCamera->SetLook({ 0.0f, 1.0f, 4.0f });
	}
}

void SceneKeyConfig::Uninit()
{
	for (auto effect : m_hitEffects)
	{
		delete effect;
	}
	m_hitEffects.clear();
}

void SceneKeyConfig::Update(float tick)
{
	// キーバインド設定中の場合は入力を取得して割り当て
	if (m_waitBindKeyPtr != nullptr)
	{
		for (int i = 8; i < 256; ++i)
		{
			if (IsKeyTrigger(i))
			{
				*m_waitBindKeyPtr = i;
				m_waitBindKeyPtr = nullptr;
				break;
			}
		}
		// 設定中はキャラが変に動かないようフレームを進めないことも可能ですが、
		// 動きが見えたほうが良いため以降の更新処理も継続します
	}

	Player* player = GetObj<Player>("Player");
	Player* player2 = GetObj<Player>("Player2");

	// プレイヤー更新
	if (player) player->Update(tick);
	if (player2) player2->Update(tick);

	// 当たり判定処理（ゲーム本編と同様に押し合いや攻撃ヒットを処理）
	BattleCollision::UpdateInteractions(player, player2, tick, m_hitEffects, 6.0f);

	// ヒットエフェクト更新
	for (auto effect : m_hitEffects)
	{
		effect->Update(tick);
	}

	// カメラ制御（ゲーム本編のロジック）
	CameraBase* pCamera = GetObj<CameraBase>("Camera");
	if (pCamera && player && player2)
	{
		XMFLOAT3 p1Pos = player->GetPosition();
		XMFLOAT3 p2Pos = player2->GetPosition();

		float centerX = (p1Pos.x + p2Pos.x) * 0.5f;
		centerX = std::clamp(centerX, -CAMERA_LIMIT_X, CAMERA_LIMIT_X);

		float maxY = (p1Pos.y > p2Pos.y) ? p1Pos.y : p2Pos.y;
		float targetLookY = 1.4f + (maxY * 0.2f);
		float targetPosY = 1.5f + (maxY * 0.1f);

		float distX = fabsf(p1Pos.x - p2Pos.x);
		float zoomFactorX = 0.45f;
		float zoomFactorY = 0.8f;
		float zoomAmount = 0.0f;
		float reqZoomX = (distX - 1.5f) * zoomFactorX;
		float reqZoomY = maxY * zoomFactorY;
		if (reqZoomX > reqZoomY) zoomAmount = reqZoomX;
		else zoomAmount = reqZoomY;
		if (zoomAmount < 0.0f) zoomAmount = 0.0f;

		float baseZ = -3.5f;
		float targetZ = baseZ - zoomAmount;
		if (targetZ < -9.0f) targetZ = -9.0f;

		XMFLOAT3 targetPos = { centerX, targetPosY, targetZ };
		XMFLOAT3 targetLook = { centerX, targetLookY, 0.0f };

		float smoothSpeed = 4.0f * tick;

		XMFLOAT3 currentPos = pCamera->GetPos();
		XMFLOAT3 currentLook = pCamera->GetLook();

		XMVECTOR vCurPos = XMLoadFloat3(&currentPos);
		XMVECTOR vTarPos = XMLoadFloat3(&targetPos);
		XMVECTOR vNewPos = XMVectorLerp(vCurPos, vTarPos, smoothSpeed);
		XMVECTOR vCurLook = XMLoadFloat3(&currentLook);
		XMVECTOR vTarLook = XMLoadFloat3(&targetLook);
		XMVECTOR vNewLook = XMVectorLerp(vCurLook, vTarLook, smoothSpeed);

		XMFLOAT3 newPos, newLook;
		XMStoreFloat3(&newPos, vNewPos);
		XMStoreFloat3(&newLook, vNewLook);

		pCamera->SetPos(newPos);
		pCamera->SetLook(newLook);
	}
}

void SceneKeyConfig::Draw()
{
	Player* player = GetObj<Player>("Player");
	Player* player2 = GetObj<Player>("Player2");
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

	// プレイヤー1 描画
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
	}

	// プレイヤー2 描画
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
	}

	// 飛び道具 描画
	if (player && player->GetProjectile())
	{
		player->GetProjectile()->Draw(pCamera->GetView(), pCamera->GetProj());
	}
	if (player2 && player2->GetProjectile())
	{
		player2->GetProjectile()->Draw(pCamera->GetView(), pCamera->GetProj());
	}

	// エフェクト 描画
	DirectX::XMFLOAT4X4 view = pCamera->GetView();
	DirectX::XMFLOAT4X4 proj = pCamera->GetProj();
	for (auto effect : m_hitEffects)
	{
		effect->Draw(view, proj);
	}

	DrawImGui();
}

const char* SceneKeyConfig::GetKeyName(int vk)
{
	if (vk >= 'A' && vk <= 'Z')
	{
		static char buf[2] = { 0 };
		buf[0] = (char)vk;
		buf[1] = '\0';
		return buf;
	}
	if (vk >= '0' && vk <= '9')
	{
		static char buf[2] = { 0 };
		buf[0] = (char)vk;
		buf[1] = '\0';
		return buf;
	}
	switch (vk)
	{
	case VK_UP: return "UP";
	case VK_DOWN: return "DOWN";
	case VK_LEFT: return "LEFT";
	case VK_RIGHT: return "RIGHT";
	case VK_SPACE: return "SPACE";
	case VK_RETURN: return "ENTER";
	case VK_NUMPAD1: return "NUM 1";
	case VK_NUMPAD2: return "NUM 2";
	case VK_NUMPAD3: return "NUM 3";
	case VK_NUMPAD4: return "NUM 4";
	case VK_NUMPAD5: return "NUM 5";
	case VK_NUMPAD6: return "NUM 6";
	case VK_NUMPAD7: return "NUM 7";
	case VK_NUMPAD8: return "NUM 8";
	case VK_NUMPAD9: return "NUM 9";
	}
	static char buf[16];
	sprintf_s(buf, "Key %d", vk);
	return buf;
}

void SceneKeyConfig::DrawKeyBindButton(const char* label, int* key)
{
	ImGui::Text("%s", label);
	ImGui::SameLine(100);

	char btnLabel[32];
	if (m_waitBindKeyPtr == key)
	{
		sprintf_s(btnLabel, "Press Any Key...");
	}
	else
	{
		sprintf_s(btnLabel, "%s", GetKeyName(*key));
	}

	ImGui::PushID(label);
	if (ImGui::Button(btnLabel, ImVec2(150, 0)))
	{
		m_waitBindKeyPtr = key;
	}
	ImGui::PopID();
}

void SceneKeyConfig::DrawImGui()
{
	ImGui::Begin("Key Config", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize);

	ImGui::Text("Player 1 Config");
	ImGui::Separator();
	DrawKeyBindButton("Up", &g_keyConfigP1.up);
	DrawKeyBindButton("Down", &g_keyConfigP1.down);
	DrawKeyBindButton("Left", &g_keyConfigP1.left);
	DrawKeyBindButton("Right", &g_keyConfigP1.right);
	DrawKeyBindButton("L Punch", &g_keyConfigP1.lightPunch);
	DrawKeyBindButton("M Punch", &g_keyConfigP1.mediumPunch);
	DrawKeyBindButton("H Punch", &g_keyConfigP1.heavyPunch);
	DrawKeyBindButton("M Kick", &g_keyConfigP1.mediumKick);
	DrawKeyBindButton("H Kick", &g_keyConfigP1.heavyKick);

	ImGui::Spacing();
	ImGui::Text("Player 2 Config");
	ImGui::Separator();
	DrawKeyBindButton("Up 2", &g_keyConfigP2.up);
	DrawKeyBindButton("Down 2", &g_keyConfigP2.down);
	DrawKeyBindButton("Left 2", &g_keyConfigP2.left);
	DrawKeyBindButton("Right 2", &g_keyConfigP2.right);
	DrawKeyBindButton("L Punch 2", &g_keyConfigP2.lightPunch);
	DrawKeyBindButton("M Punch 2", &g_keyConfigP2.mediumPunch);
	DrawKeyBindButton("H Punch 2", &g_keyConfigP2.heavyPunch);
	DrawKeyBindButton("M Kick 2", &g_keyConfigP2.mediumKick);
	DrawKeyBindButton("H Kick 2", &g_keyConfigP2.heavyKick);

	ImGui::Spacing();
	ImGui::Separator();
	if (ImGui::Button("Game Start", ImVec2(-1, 40)))
	{
		s_isConfigSet = true;
	}

	ImGui::End();
}