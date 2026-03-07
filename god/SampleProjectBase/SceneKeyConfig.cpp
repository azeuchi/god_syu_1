#include "SceneKeyConfig.h"
#include "Input.h"
#include "Player.h"
#include "CameraBase.h"
#include "LightBase.h"
#include "Shader.h"
#include "Model.h"
#include "PlayerParameterLoader.h"
#include "BattleCollision.h"
#include "HitEffect.h"
#include "Projectile.h"
#include "Geometory.h"
#include "SimpleUI.h"
#include "SimpleFont.h"
#include <stdio.h>
#include <algorithm>

using namespace DirectX;
using namespace DirectX::SimpleMath;

// カメラの移動を制限する範囲
const float CAMERA_LIMIT_X = 4.0f;

bool SceneKeyConfig::s_isConfigSet = false;

void SceneKeyConfig::Init()
{
	s_isConfigSet = false;
	m_waitBindKeyPtr = nullptr;
	m_menuState = MenuState::TopMenu;
	m_topSelectedIndex = 0;
	m_configSelectedIndex = 0;
	m_windowScaleY = 0.0f;

	// フォントシステムの初期化 (DirectWrite版)
	SimpleFont::Init(GetDevice(), GetContext());

	// トップメニューの項目を設定
	m_topItems = {
		{ L"Player 1 Config" },
		{ L"Player 2 Config" },
		{ L"Training Mode" },
		{ L"Game Start" }
	};

	// 1P用の設定項目と変更対象となる変数のポインタを紐付ける
	m_p1Items = {
		{ L"Up", &g_keyConfigP1.up },
		{ L"Down", &g_keyConfigP1.down },
		{ L"Left", &g_keyConfigP1.left },
		{ L"Right", &g_keyConfigP1.right },
		{ L"L Punch", &g_keyConfigP1.lightPunch },
		{ L"M Punch", &g_keyConfigP1.mediumPunch },
		{ L"H Punch", &g_keyConfigP1.heavyPunch },
		{ L"M Kick", &g_keyConfigP1.mediumKick },
		{ L"H Kick", &g_keyConfigP1.heavyKick },
		{ L"Back", nullptr } // 戻るボタンはポインタにnullptrを割り当てる
	};

	// 2P用の設定項目
	m_p2Items = {
		{ L"Up", &g_keyConfigP2.up },
		{ L"Down", &g_keyConfigP2.down },
		{ L"Left", &g_keyConfigP2.left },
		{ L"Right", &g_keyConfigP2.right },
		{ L"L Punch", &g_keyConfigP2.lightPunch },
		{ L"M Punch", &g_keyConfigP2.mediumPunch },
		{ L"H Punch", &g_keyConfigP2.heavyPunch },
		{ L"M Kick", &g_keyConfigP2.mediumKick },
		{ L"H Kick", &g_keyConfigP2.heavyKick },
		{ L"Back", nullptr }
	};

	Shader* vsSkin = GetObj<Shader>("VS_SkinMeshAnimation");
	if (!vsSkin)
	{
		vsSkin = CreateObj<VertexShader>("VS_SkinMeshAnimation");
		if (FAILED(vsSkin->Load("Assets/Shader/VS_SkinMeshAnimation.cso")))
		{
			MessageBox(NULL, "Assets/Shader/VS_SkinMeshAnimation.cso", "Shader Error", MB_OK);
		}
	}

	Shader* psColor = GetObj<Shader>("PS_TexColor");
	if (!psColor)
	{
		psColor = CreateObj<PixelShader>("PS_TexColor");
		if (FAILED(psColor->Load("Assets/Shader/PS_TexColor.cso")))
		{
			MessageBox(NULL, "Assets/Shader/PS_TexColor.cso", "Shader Error", MB_OK);
		}
	}

	Shader* vsOutline = GetObj<Shader>("VS_SkinMeshOutline");
	if (!vsOutline)
	{
		vsOutline = CreateObj<VertexShader>("VS_SkinMeshOutline");
		if (FAILED(vsOutline->Load("Assets/Shader/VS_SkinMeshOutline.cso")))
		{
			MessageBox(NULL, "Assets/Shader/VS_SkinMeshOutline.cso", "Shader Error", MB_OK);
		}
	}

	Shader* psOutline = GetObj<Shader>("PS_Outline");
	if (!psOutline)
	{
		psOutline = CreateObj<PixelShader>("PS_Outline");
		if (FAILED(psOutline->Load("Assets/Shader/PS_Outline.cso")))
		{
			MessageBox(NULL, "Assets/Shader/PS_Outline.cso", "Shader Error", MB_OK);
		}
	}

	D3D11_RASTERIZER_DESC rsDesc = {};
	rsDesc.FillMode = D3D11_FILL_SOLID;
	rsDesc.FrontCounterClockwise = FALSE;
	rsDesc.DepthBias = 0;
	rsDesc.SlopeScaledDepthBias = 0.0f;
	rsDesc.DepthBiasClamp = 0.0f;
	rsDesc.DepthClipEnable = TRUE;
	rsDesc.ScissorEnable = FALSE;
	rsDesc.MultisampleEnable = FALSE;
	rsDesc.AntialiasedLineEnable = FALSE;

	rsDesc.CullMode = D3D11_CULL_FRONT;
	GetDevice()->CreateRasterizerState(&rsDesc, &m_pCullFront);

	rsDesc.CullMode = D3D11_CULL_BACK;
	GetDevice()->CreateRasterizerState(&rsDesc, &m_pCullBack);

	// プレイヤーのアニメーションを読み込む共通ラムダ式
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

	// プレイヤー1の生成と設定
	CreateObj<Player>("Player");
	Player* player = GetObj<Player>("Player");
	player->SetInputType(PlayerInputType::AI); // キャラが動かないようAI（待機）に固定
	PlayerParameterLoader::LoadSettings(player);
	if (!player->Load("Assets/Model/knight/Idle.fbx", 0.014f, true, false))
	{
		MessageBox(NULL, "プレイヤーモデルの読み込みに失敗しました。", "Error", MB_OK);
	}
	LoadAnims(player);
	player->SetPosition({ -2.0f, 0.0f, 0.0f });
	player->SetRotation({ 0.0f, DirectX::XM_PI / -2.0f, 0.0f });
	player->Reset();

	// プレイヤー2の生成と設定
	CreateObj<Player>("Player2");
	Player* player2 = GetObj<Player>("Player2");
	player2->SetInputType(PlayerInputType::AI); // プレイヤーの操作を無効化
	player2->SetMoveSpeed(player->GetMoveSpeed());
	player2->SetScale(player->GetScale()); // スケールのマイナス反転を削除（モデルが壊れる原因）
	PlayerParameterLoader::CopyParameters(player, player2);
	if (!player2->Load("Assets/Model/knight/Idle.fbx", 0.014f, true, false))
	{
		MessageBox(NULL, "プレイヤー2モデルの読み込みに失敗しました。", "Error", MB_OK);
	}
	LoadAnims(player2);
	player2->SetPosition({ 2.0f, 0.0f, 0.0f });
	player2->SetRotation({ 0.0f, DirectX::XM_PI / 2.0f, 0.0f }); // 回転だけで向きを調整する
	player2->Reset();

	// エフェクトの初期化
	for (int i = 0; i < 10; i++)
	{
		m_hitEffects.push_back(new HitEffect());
	}

	// カメラの初期設定
	CameraBase* pCamera = GetObj<CameraBase>("Camera");
	if (pCamera)
	{
		pCamera->SetPos({ 0.0f, 1.2f, -5.0f });
		pCamera->SetLook({ 0.0f, 1.0f, 4.0f });
	}
}

void SceneKeyConfig::Uninit()
{
	SimpleFont::Uninit();

	for (auto effect : m_hitEffects)
	{
		delete effect;
	}
	m_hitEffects.clear();

	if (m_pCullFront) { m_pCullFront->Release(); m_pCullFront = nullptr; }
	if (m_pCullBack) { m_pCullBack->Release(); m_pCullBack = nullptr; }
}

void SceneKeyConfig::Update(float tick)
{
	// いずれかのキー割り当てが選択され、入力を待っている状態
	if (m_waitBindKeyPtr != nullptr)
	{
		for (int i = 8; i < 256; ++i)
		{
			// 何らかのキーが押されたら、そのキーコードをポインタ先の変数に保存する
			if (IsKeyTrigger(i))
			{
				*m_waitBindKeyPtr = i;
				m_waitBindKeyPtr = nullptr; // 待機状態を解除
				break;
			}
		}
	}
	else
	{
		// メニューの階層に応じた操作処理
		if (m_menuState == MenuState::TopMenu)
		{
			// 上下キーでトップメニューのカーソルを移動
			if (IsKeyTrigger(VK_DOWN))
			{
				m_topSelectedIndex = (m_topSelectedIndex + 1) % m_topItems.size();
			}
			if (IsKeyTrigger(VK_UP))
			{
				m_topSelectedIndex = (m_topSelectedIndex - 1 + m_topItems.size()) % m_topItems.size();
			}

			// 決定キーで遷移
			if (IsKeyTrigger(VK_RETURN))
			{
				if (m_topSelectedIndex == 0)
				{
					// 1P設定を開く。展開アニメーションのためにスケールを0にリセットする
					m_menuState = MenuState::ConfigP1;
					m_configSelectedIndex = 0;
					m_windowScaleY = 0.0f;
				}
				else if (m_topSelectedIndex == 1)
				{
					// 2P設定を開く
					m_menuState = MenuState::ConfigP2;
					m_configSelectedIndex = 0;
					m_windowScaleY = 0.0f;
				}
				else if (m_topSelectedIndex == 2)
				{
					// トレーニングモードに移行し、両プレイヤーの操作権限を与える
					m_menuState = MenuState::TrainingMode;
					if (Player* p1 = GetObj<Player>("Player")) p1->SetInputType(PlayerInputType::PLAYER_1);
					if (Player* p2 = GetObj<Player>("Player2")) p2->SetInputType(PlayerInputType::PLAYER_2);
				}
				else if (m_topSelectedIndex == 3)
				{
					// ゲームスタート
					s_isConfigSet = true;
				}
			}
		}
		else if (m_menuState == MenuState::ConfigP1 || m_menuState == MenuState::ConfigP2)
		{
			// コンフィグ画面が開く際のアニメーション処理（縦に伸びて展開する）
			if (m_windowScaleY < 1.0f)
			{
				m_windowScaleY += tick * 15.0f;
				if (m_windowScaleY > 1.0f) m_windowScaleY = 1.0f;
			}
			else
			{
				// 展開アニメーションが終わっている場合のみ操作を受け付ける
				std::vector<ConfigItem>& currentItems = (m_menuState == MenuState::ConfigP1) ? m_p1Items : m_p2Items;

				// 上下キーで設定項目のカーソルを移動
				if (IsKeyTrigger(VK_DOWN))
				{
					m_configSelectedIndex = (m_configSelectedIndex + 1) % currentItems.size();
				}
				if (IsKeyTrigger(VK_UP))
				{
					m_configSelectedIndex = (m_configSelectedIndex - 1 + currentItems.size()) % currentItems.size();
				}

				// 決定キーで項目の変更待機状態へ
				if (IsKeyTrigger(VK_RETURN))
				{
					if (currentItems[m_configSelectedIndex].keyPtr == nullptr)
					{
						// Backが選ばれた場合はトップメニューに戻る
						m_menuState = MenuState::TopMenu;
					}
					else
					{
						// キー割り当て待機状態に移行
						m_waitBindKeyPtr = currentItems[m_configSelectedIndex].keyPtr;
					}
				}

				// キャンセルキーでもトップメニューに戻る
				if (IsKeyTrigger(VK_BACK) || IsKeyTrigger(VK_ESCAPE))
				{
					m_menuState = MenuState::TopMenu;
				}
			}
		}
		else if (m_menuState == MenuState::TrainingMode)
		{
			// トレーニングモード中、ESCキーが押されたらメニューに戻して操作を無効化する
			if (IsKeyTrigger(VK_ESCAPE))
			{
				m_menuState = MenuState::TopMenu;
				if (Player* p1 = GetObj<Player>("Player")) p1->SetInputType(PlayerInputType::AI);
				if (Player* p2 = GetObj<Player>("Player2")) p2->SetInputType(PlayerInputType::AI);
			}
		}
	}

	Player* player = GetObj<Player>("Player");
	Player* player2 = GetObj<Player>("Player2");

	// プレイヤーの更新
	if (player) player->Update(tick);
	if (player2) player2->Update(tick);

	// 当たり判定処理（攻撃のヒットやプレイヤー同士の押し合いを処理する）
	BattleCollision::UpdateInteractions(player, player2, tick, m_hitEffects, 6.0f);

	for (auto effect : m_hitEffects)
	{
		effect->Update(tick);
	}

	// プレイヤーの位置に合わせてカメラが追従・ズームする制御
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

		// キャラクター間の距離に基づいてカメラの引き具合を計算する
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

void SceneKeyConfig::DrawRectPixel(float px, float py, float pw, float ph, DirectX::XMFLOAT4 color)
{
	// 元のSpriteクラスが「指定した座標を中心とする」仕様だったため、
	// 左上(px, py)に描画されるよう中心座標を計算してからNDCに変換する
	float centerX = px + pw * 0.5f;
	float centerY = py + ph * 0.5f;

	float ndcX = (centerX / 1280.0f) * 2.0f - 1.0f;
	float ndcY = 1.0f - (centerY / 720.0f) * 2.0f;
	float ndcW = (pw / 1280.0f) * 2.0f;
	float ndcH = (ph / 720.0f) * 2.0f;

	SimpleUI::AddRect(ndcX, ndcY, ndcW, ndcH, color, nullptr);
}

const wchar_t* SceneKeyConfig::GetKeyName(int vk)
{
	// アルファベットと数字はそのままワイド文字として返す
	if (vk >= 'A' && vk <= 'Z')
	{
		static wchar_t buf[2] = { 0 };
		buf[0] = (wchar_t)vk;
		buf[1] = L'\0';
		return buf;
	}
	if (vk >= '0' && vk <= '9')
	{
		static wchar_t buf[2] = { 0 };
		buf[0] = (wchar_t)vk;
		buf[1] = L'\0';
		return buf;
	}

	// 特殊キーの名称対応
	switch (vk)
	{
	case VK_UP: return L"UP";
	case VK_DOWN: return L"DOWN";
	case VK_LEFT: return L"LEFT";
	case VK_RIGHT: return L"RIGHT";
	case VK_SPACE: return L"SPACE";
	case VK_RETURN: return L"ENTER";
	case VK_BACK: return L"BACKSPACE";
	case VK_ESCAPE: return L"ESCAPE";
	case VK_NUMPAD1: return L"NUM 1";
	case VK_NUMPAD2: return L"NUM 2";
	case VK_NUMPAD3: return L"NUM 3";
	case VK_NUMPAD4: return L"NUM 4";
	case VK_NUMPAD5: return L"NUM 5";
	case VK_NUMPAD6: return L"NUM 6";
	case VK_NUMPAD7: return L"NUM 7";
	case VK_NUMPAD8: return L"NUM 8";
	case VK_NUMPAD9: return L"NUM 9";
	}

	// 上記以外は番号を出力する
	static wchar_t buf[16];
	swprintf_s(buf, L"Key %d", vk);
	return buf;
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
		GetObj<Shader>("VS_SkinMeshOutline"),
		GetObj<Shader>("PS_Outline")
	};

	// アウトライン描画パス
	if (player) {
		bool isFlipped = (player->GetScale().x < 0.0f);
		if (isFlipped) {
			if (m_pCullBack) GetContext()->RSSetState(m_pCullBack);
		}
		else {
			if (m_pCullFront) GetContext()->RSSetState(m_pCullFront);
		}

		XMFLOAT3 pos = player->GetPosition();
		XMFLOAT3 rot = player->GetRotation();
		XMFLOAT3 pScale = player->GetScale();
		Matrix playerScaleMat = Matrix::CreateScale(pScale.x, pScale.y, pScale.z);
		Matrix modelBaseScaleMat = player->GetModel()->GetScaleBaseMatrix();
		Matrix rotMat = DirectX::XMMatrixRotationRollPitchYaw(rot.x, rot.y, rot.z);
		Matrix transMat = Matrix::CreateTranslation(pos.x, pos.y, pos.z);
		Matrix world = modelBaseScaleMat * playerScaleMat * rotMat * transMat;

		XMStoreFloat4x4(&mat[0], XMMatrixTranspose(world));
		shader[2]->WriteBuffer(0, mat);
		player->SetVertexShader(shader[2]);
		player->SetPixelShader(shader[3]);
		player->Draw();
	}

	if (player2) {
		bool isFlipped = (player2->GetScale().x < 0.0f);
		if (isFlipped) {
			if (m_pCullBack) GetContext()->RSSetState(m_pCullBack);
		}
		else {
			if (m_pCullFront) GetContext()->RSSetState(m_pCullFront);
		}

		XMFLOAT3 pos = player2->GetPosition();
		XMFLOAT3 rot = player2->GetRotation();
		XMFLOAT3 pScale = player2->GetScale();
		Matrix playerScaleMat = Matrix::CreateScale(pScale.x, pScale.y, pScale.z);
		Matrix modelBaseScaleMat = player2->GetModel()->GetScaleBaseMatrix();
		Matrix rotMat = DirectX::XMMatrixRotationRollPitchYaw(rot.x, rot.y, rot.z);
		Matrix transMat = Matrix::CreateTranslation(pos.x, pos.y, pos.z);
		Matrix world = modelBaseScaleMat * playerScaleMat * rotMat * transMat;

		XMStoreFloat4x4(&mat[0], XMMatrixTranspose(world));
		shader[2]->WriteBuffer(0, mat);
		player2->SetVertexShader(shader[2]);
		player2->SetPixelShader(shader[3]);
		player2->Draw();
	}

	// 通常モデル描画パス
	if (player) {
		bool isFlipped = (player->GetScale().x < 0.0f);
		if (isFlipped) {
			if (m_pCullFront) GetContext()->RSSetState(m_pCullFront);
		}
		else {
			if (m_pCullBack) GetContext()->RSSetState(m_pCullBack);
		}

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

	if (player2) {
		bool isFlipped = (player2->GetScale().x < 0.0f);
		if (isFlipped) {
			if (m_pCullFront) GetContext()->RSSetState(m_pCullFront);
		}
		else {
			if (m_pCullBack) GetContext()->RSSetState(m_pCullBack);
		}

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

	if (m_pCullBack) GetContext()->RSSetState(m_pCullBack);

	// 飛び道具の描画
	if (player && player->GetProjectile())
	{
		player->GetProjectile()->Draw(pCamera->GetView(), pCamera->GetProj());
	}
	if (player2 && player2->GetProjectile())
	{
		player2->GetProjectile()->Draw(pCamera->GetView(), pCamera->GetProj());
	}

	// ヒットエフェクトの描画
	DirectX::XMFLOAT4X4 view = pCamera->GetView();
	DirectX::XMFLOAT4X4 proj = pCamera->GetProj();
	for (auto effect : m_hitEffects)
	{
		effect->Draw(view, proj);
	}

	// トレーニングモード以外の場合のみ、UIを描画する
	if (m_menuState != MenuState::TrainingMode)
	{
		SimpleUI::Clear();

		// 画面上部にトップメニュー用の少し明るめのネイビーブルー背景を描画
		DrawRectPixel(0, 0, 1280, 150, { 0.0f, 0.15f, 0.3f, 0.8f });

		if (m_menuState == MenuState::TopMenu)
		{
			// トップメニューの項目枠を描画
			for (int i = 0; i < m_topItems.size(); ++i)
			{
				DirectX::XMFLOAT4 color = (i == m_topSelectedIndex) ? XMFLOAT4(0.0f, 0.8f, 1.0f, 0.9f) : XMFLOAT4(0.0f, 0.4f, 0.6f, 0.7f);
				DrawRectPixel(50 + i * 250, 50, 200, 50, color);
			}
		}
		else if (m_menuState == MenuState::ConfigP1 || m_menuState == MenuState::ConfigP2)
		{
			// コンフィグ用のパネルを描画（中央から上下に伸びる演出計算）
			float panelHeight = 500.0f * m_windowScaleY;
			float panelY = 150.0f + (500.0f - panelHeight) / 2.0f;

			DrawRectPixel(50, panelY, 400, panelHeight, { 0.0f, 0.15f, 0.3f, 0.8f });

			// 枠の展開アニメーションが終わってから中身の項目枠を描画する
			if (m_windowScaleY >= 1.0f)
			{
				std::vector<ConfigItem>& currentItems = (m_menuState == MenuState::ConfigP1) ? m_p1Items : m_p2Items;
				for (int i = 0; i < currentItems.size(); ++i)
				{
					DirectX::XMFLOAT4 color = (i == m_configSelectedIndex) ? XMFLOAT4(1.0f, 0.7f, 0.0f, 0.9f) : XMFLOAT4(0.3f, 0.3f, 0.3f, 0.8f);
					DrawRectPixel(70, 170 + i * 45, 360, 40, color);
				}
			}
		}

		// 登録した四角形UIを一括描画
		SimpleUI::DrawAll();

		// ここからDirectWriteによるフォントの描画処理
		SimpleFont::Begin();

		if (m_menuState == MenuState::TopMenu)
		{
			for (int i = 0; i < m_topItems.size(); ++i)
			{
				DirectX::XMFLOAT4 textColor = (i == m_topSelectedIndex) ? XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f) : XMFLOAT4(0.7f, 0.8f, 1.0f, 1.0f);
				SimpleFont::Draw(m_topItems[i].label, 70 + i * 250, 60, 24.0f, textColor);
			}
		}
		else if (m_menuState == MenuState::ConfigP1 || m_menuState == MenuState::ConfigP2)
		{
			SimpleFont::Draw((m_menuState == MenuState::ConfigP1) ? L"Player 1 Config" : L"Player 2 Config", 50, 50, 32.0f, { 1.0f, 0.9f, 0.2f, 1.0f });

			// 枠のアニメーション終了後に各項目のテキストを描画
			if (m_windowScaleY >= 1.0f)
			{
				std::vector<ConfigItem>& currentItems = (m_menuState == MenuState::ConfigP1) ? m_p1Items : m_p2Items;
				for (int i = 0; i < currentItems.size(); ++i)
				{
					DirectX::XMFLOAT4 textColor = (i == m_configSelectedIndex) ? XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f) : XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);

					// アクション名を描画 (例: Light Punch)
					SimpleFont::Draw(currentItems[i].label, 80, 175 + i * 45, 20.0f, textColor);

					// 現在の割り当てキー、または待機中のメッセージを描画
					if (currentItems[i].keyPtr != nullptr)
					{
						if (m_waitBindKeyPtr == currentItems[i].keyPtr)
						{
							SimpleFont::Draw(L"Press Any Key...", 250, 175 + i * 45, 20.0f, { 1.0f, 0.3f, 0.3f, 1.0f });
						}
						else
						{
							SimpleFont::Draw(GetKeyName(*currentItems[i].keyPtr), 250, 175 + i * 45, 20.0f, textColor);
						}
					}
				}
			}
		}

		SimpleFont::End();
	}

	GetContext()->RSSetState(nullptr);
}