#include "SceneKeyConfig.h"
#include "Input.h"
#include "Player.h"
#include "CameraBase.h"
#include "LightBase.h"
#include "Shader.h"
#include "Model.h"
#include "SkyDome.h"
#include "PlayerParameterLoader.h"
#include "BattleCollision.h"
#include "HitEffect.h"
#include "Projectile.h"
#include "Geometory.h"
#include "SimpleUI.h"
#include "SimpleFont.h"
#include <stdio.h>
#include <algorithm>
#include <Xinput.h>

using namespace DirectX;
using namespace DirectX::SimpleMath;

// ・ｽE・ｽJ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽﾌ移難ｿｽ・ｽE・ｽｧ鯉ｿｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽﾍ茨ｿｽ
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

	// ・ｽE・ｽt・ｽE・ｽH・ｽE・ｽ・ｽE・ｽ・ｽE・ｽg・ｽE・ｽV・ｽE・ｽX・ｽE・ｽe・ｽE・ｽ・ｽE・ｽ・ｽE・ｽﾌ擾ｿｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ (DirectWrite・ｽE・ｽ・ｽE・ｽ)
	SimpleFont::Init(GetDevice(), GetContext());

	// ・ｽE・ｽg・ｽE・ｽb・ｽE・ｽv・ｽE・ｽ・ｽE・ｽ・ｽE・ｽj・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ[・ｽE・ｽﾌ搾ｿｽ・ｽE・ｽﾚゑｿｽﾝ抵ｿｽ
	m_topItems = {
		{ L"Player 1 Config" },
		{ L"Player 2 Config" },
		{ L"Training Mode" },
		{ L"Game Start" }
	};

	// ・ｽE・ｽI・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽﾄゑｿｽ・ｽE・ｽ・ｽE・ｽf・ｽE・ｽo・ｽE・ｽC・ｽE・ｽX・ｽE・ｽ・ｽE・ｽ・ｽE・ｽﾚ托ｿｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽﾄゑｿｽ・ｽE・ｽﾈゑｿｽ・ｽE・ｽ鼾・・ｽ・ｽﾍキ・ｽE・ｽ[・ｽE・ｽ{・ｽE・ｽ[・ｽE・ｽh・ｽE・ｽﾉ戻ゑｿｽ
	if (g_inputDeviceP1 != InputDeviceType::KEYBOARD && !IsPadConnected((int)g_inputDeviceP1 - 1))
	{
		g_inputDeviceP1 = InputDeviceType::KEYBOARD;
	}
	if (g_inputDeviceP2 != InputDeviceType::KEYBOARD && !IsPadConnected((int)g_inputDeviceP2 - 1))
	{
		g_inputDeviceP2 = InputDeviceType::KEYBOARD;
	}

	// ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽﾌデ・ｽE・ｽo・ｽE・ｽC・ｽE・ｽX・ｽE・ｽ・ｽE・ｽﾔに搾ｿｽ・ｽE・ｽ・ｽ・ｽ・ｽ・ｽE・ｽﾄ・・ｽ・ｽ・ｽE・ｽj・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ[・ｽE・ｽﾌポ・ｽE・ｽC・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ^・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ\・ｽE・ｽz
	RefreshConfigPointers();

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

	// ・ｽX・ｽJ・ｽC・ｽh・ｽ[・ｽ・ｽ・ｽp・ｽV・ｽF・ｽ[・ｽ_・ｽ[・ｽiVS_Object・ｽj・ｽﾌ用・ｽ・ｽ
	Shader* vsObj = GetObj<Shader>("VS_Object");
	if (!vsObj)
	{
		vsObj = CreateObj<VertexShader>("VS_Object");
		vsObj->Load("Assets/Shader/VS_Object.cso");
	}

	// ・ｽw・ｽi・ｽi・ｽX・ｽJ・ｽC・ｽh・ｽ[・ｽ・ｽ・ｽj・ｽﾌ読み搾ｿｽ・ｽ・ｽ
	CreateObj<Model>("SkyModel");
	Model* skyModel = GetObj<Model>("SkyModel");
	skyModel->Load("Assets/Model/SkyDome/SkyDome.fbx", 1.0f, true, true);
	skyModel->SetTexture("Assets/Model/SkyDome/SkyDome.png");
	skyModel->SetPixelShader((PixelShader*)GetObj<Shader>("PS_TexColor"));
	m_skyDome = new SkyDome();
	m_skyDome->Init(skyModel);

	// ・ｽX・ｽJ・ｽC・ｽh・ｽ[・ｽ・ｽ・ｽ`・ｽ・ｽp・ｽF・ｽJ・ｽ・ｽ・ｽ・ｽ・ｽO・ｽﾈゑｿｽ・ｽ・ｽ・ｽX・ｽ^・ｽ・ｽ・ｽC・ｽU・ｽ[・ｽX・ｽe・ｽ[・ｽg
	{
		D3D11_RASTERIZER_DESC skyRs = {};
		skyRs.FillMode = D3D11_FILL_SOLID;
		skyRs.CullMode = D3D11_CULL_NONE;
		skyRs.FrontCounterClockwise = FALSE;
		skyRs.DepthClipEnable = FALSE;
		GetDevice()->CreateRasterizerState(&skyRs, &m_pCullNone);
	}

	// スカイドーム(最奥)描画用：LESS_EQUAL の深度ステート
	{
		D3D11_DEPTH_STENCIL_DESC depthDesc3D = {};
		depthDesc3D.DepthEnable = TRUE;
		depthDesc3D.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
		depthDesc3D.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
		depthDesc3D.StencilEnable = FALSE;
		GetDevice()->CreateDepthStencilState(&depthDesc3D, &m_pDepthState3D);
	}

	// ・ｽE・ｽv・ｽE・ｽ・ｽE・ｽ・ｽE・ｽC・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ[・ｽE・ｽﾌア・ｽE・ｽj・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ[・ｽE・ｽV・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽﾇみ搾ｿｽ・ｽE・ｽﾞ具ｿｽ・ｽE・ｽﾊ・・ｽ・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ_・ｽE・ｽ・ｽE・ｽ
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

	// ・ｽE・ｽv・ｽE・ｽ・ｽE・ｽ・ｽE・ｽC・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ[1・ｽE・ｽﾌ撰ｿｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽﾆ設抵ｿｽ
	CreateObj<Player>("Player");
	Player* player = GetObj<Player>("Player");
	player->SetInputType(PlayerInputType::AI); // ・ｽE・ｽL・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽﾈゑｿｽ・ｽE・ｽ謔､AI・ｽE・ｽi・ｽE・ｽﾒ機・ｽE・ｽj・ｽE・ｽﾉ固抵ｿｽ
	PlayerParameterLoader::LoadSettings(player);
	if (!player->Load("Assets/Model/knight/Idle.fbx", 0.014f, true, false))
	{
		MessageBox(NULL, "・ｽE・ｽv・ｽE・ｽ・ｽE・ｽ・ｽE・ｽC・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ[・ｽE・ｽ・ｽE・ｽ・ｽE・ｽf・ｽE・ｽ・ｽE・ｽ・ｽE・ｽﾌ読み搾ｿｽ・ｽE・ｽﾝに趣ｿｽ・ｽE・ｽs・ｽE・ｽ・ｽE・ｽ・ｽE・ｽﾜゑｿｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽB", "Error", MB_OK);
	}
	LoadAnims(player);
	player->SetPosition({ -2.0f, 0.0f, 0.0f });
	player->SetRotation({ 0.0f, DirectX::XM_PI / -2.0f, 0.0f });
	player->Reset();

	// ・ｽE・ｽv・ｽE・ｽ・ｽE・ｽ・ｽE・ｽC・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ[2・ｽE・ｽﾌ撰ｿｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽﾆ設抵ｿｽ
	CreateObj<Player>("Player2");
	Player* player2 = GetObj<Player>("Player2");
	player2->SetInputType(PlayerInputType::AI); // ・ｽE・ｽv・ｽE・ｽ・ｽE・ｽ・ｽE・ｽC・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ[・ｽE・ｽﾌ托ｿｽ・ｽE・ｽ・ｽE・ｽｳ鯉ｿｽ・ｽE・ｽ・ｽE・ｽ
	player2->SetMoveSpeed(player->GetMoveSpeed());
	player2->SetScale(player->GetScale());
	PlayerParameterLoader::CopyParameters(player, player2);
	if (!player2->Load("Assets/Model/knight/Idle.fbx", 0.014f, true, false))
	{
		MessageBox(NULL, "・ｽE・ｽv・ｽE・ｽ・ｽE・ｽ・ｽE・ｽC・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ[2・ｽE・ｽ・ｽE・ｽ・ｽE・ｽf・ｽE・ｽ・ｽE・ｽ・ｽE・ｽﾌ読み搾ｿｽ・ｽE・ｽﾝに趣ｿｽ・ｽE・ｽs・ｽE・ｽ・ｽE・ｽ・ｽE・ｽﾜゑｿｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽB", "Error", MB_OK);
	}
	LoadAnims(player2);
	player2->SetPosition({ 2.0f, 0.0f, 0.0f });
	player2->SetRotation({ 0.0f, DirectX::XM_PI / 2.0f, 0.0f }); // ・ｽE・ｽ・ｽE・ｽ]・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽﾅ鯉ｿｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽｲ撰ｿｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ
	player2->Reset();

	// ・ｽE・ｽG・ｽE・ｽt・ｽE・ｽF・ｽE・ｽN・ｽE・ｽg・ｽE・ｽﾌ擾ｿｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ
	for (int i = 0; i < 10; i++)
	{
		m_hitEffects.push_back(new HitEffect());
	}

	// ・ｽE・ｽJ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽﾌ擾ｿｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽﾝ抵ｿｽ
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
	if (m_skyDome) { delete m_skyDome; m_skyDome = nullptr; }
	if (m_pCullNone) { m_pCullNone->Release(); m_pCullNone = nullptr; }
	if (m_pDepthState3D) { m_pDepthState3D->Release(); m_pDepthState3D = nullptr; }
}

void SceneKeyConfig::RefreshConfigPointers()
{
	m_p1Items = {
		{ L"Device", nullptr, true },
		{ L"Up", (g_inputDeviceP1 == InputDeviceType::KEYBOARD) ? &g_keyConfigP1.up : &g_padConfigP1.up, false },
		{ L"Down", (g_inputDeviceP1 == InputDeviceType::KEYBOARD) ? &g_keyConfigP1.down : &g_padConfigP1.down, false },
		{ L"Left", (g_inputDeviceP1 == InputDeviceType::KEYBOARD) ? &g_keyConfigP1.left : &g_padConfigP1.left, false },
		{ L"Right", (g_inputDeviceP1 == InputDeviceType::KEYBOARD) ? &g_keyConfigP1.right : &g_padConfigP1.right, false },
		{ L"L Punch", (g_inputDeviceP1 == InputDeviceType::KEYBOARD) ? &g_keyConfigP1.lightPunch : &g_padConfigP1.lightPunch, false },
		{ L"M Punch", (g_inputDeviceP1 == InputDeviceType::KEYBOARD) ? &g_keyConfigP1.mediumPunch : &g_padConfigP1.mediumPunch, false },
		{ L"H Punch", (g_inputDeviceP1 == InputDeviceType::KEYBOARD) ? &g_keyConfigP1.heavyPunch : &g_padConfigP1.heavyPunch, false },
		{ L"M Kick", (g_inputDeviceP1 == InputDeviceType::KEYBOARD) ? &g_keyConfigP1.mediumKick : &g_padConfigP1.mediumKick, false },
		{ L"H Kick", (g_inputDeviceP1 == InputDeviceType::KEYBOARD) ? &g_keyConfigP1.heavyKick : &g_padConfigP1.heavyKick, false },
		{ L"Back", nullptr, false }
	};

	m_p2Items = {
		{ L"Device", nullptr, true },
		{ L"Up", (g_inputDeviceP2 == InputDeviceType::KEYBOARD) ? &g_keyConfigP2.up : &g_padConfigP2.up, false },
		{ L"Down", (g_inputDeviceP2 == InputDeviceType::KEYBOARD) ? &g_keyConfigP2.down : &g_padConfigP2.down, false },
		{ L"Left", (g_inputDeviceP2 == InputDeviceType::KEYBOARD) ? &g_keyConfigP2.left : &g_padConfigP2.left, false },
		{ L"Right", (g_inputDeviceP2 == InputDeviceType::KEYBOARD) ? &g_keyConfigP2.right : &g_padConfigP2.right, false },
		{ L"L Punch", (g_inputDeviceP2 == InputDeviceType::KEYBOARD) ? &g_keyConfigP2.lightPunch : &g_padConfigP2.lightPunch, false },
		{ L"M Punch", (g_inputDeviceP2 == InputDeviceType::KEYBOARD) ? &g_keyConfigP2.mediumPunch : &g_padConfigP2.mediumPunch, false },
		{ L"H Punch", (g_inputDeviceP2 == InputDeviceType::KEYBOARD) ? &g_keyConfigP2.heavyPunch : &g_padConfigP2.heavyPunch, false },
		{ L"M Kick", (g_inputDeviceP2 == InputDeviceType::KEYBOARD) ? &g_keyConfigP2.mediumKick : &g_padConfigP2.mediumKick, false },
		{ L"H Kick", (g_inputDeviceP2 == InputDeviceType::KEYBOARD) ? &g_keyConfigP2.heavyKick : &g_padConfigP2.heavyKick, false },
		{ L"Back", nullptr, false }
	};
}


void SceneKeyConfig::Update(float tick)
{
	auto isMenuUp = [&]() {
		if (IsKeyTrigger(VK_UP)) return true;
		for (int i = 0; i < 4; ++i) if (IsPadTrigger(i, XINPUT_GAMEPAD_DPAD_UP)) return true;
		return false;
		};
	auto isMenuDown = [&]() {
		if (IsKeyTrigger(VK_DOWN)) return true;
		for (int i = 0; i < 4; ++i) if (IsPadTrigger(i, XINPUT_GAMEPAD_DPAD_DOWN)) return true;
		return false;
		};
	auto isMenuLeft = [&]() {
		if (IsKeyTrigger(VK_LEFT)) return true;
		for (int i = 0; i < 4; ++i) if (IsPadTrigger(i, XINPUT_GAMEPAD_DPAD_LEFT)) return true;
		return false;
		};
	auto isMenuRight = [&]() {
		if (IsKeyTrigger(VK_RIGHT)) return true;
		for (int i = 0; i < 4; ++i) if (IsPadTrigger(i, XINPUT_GAMEPAD_DPAD_RIGHT)) return true;
		return false;
		};
	auto isMenuConfirm = [&]() {
		if (IsKeyTrigger(VK_RETURN)) return true;
		for (int i = 0; i < 4; ++i) if (IsPadTrigger(i, XINPUT_GAMEPAD_A)) return true;
		return false;
		};
	auto isMenuCancel = [&]() {
		if (IsKeyTrigger(VK_BACK) || IsKeyTrigger(VK_ESCAPE)) return true;
		for (int i = 0; i < 4; ++i) if (IsPadTrigger(i, XINPUT_GAMEPAD_B)) return true;
		return false;
		};

	// ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ黷ｩ・ｽE・ｽﾌキ・ｽE・ｽ[・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ闢厄ｿｽﾄゑｿｽ・ｽE・ｽI・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽA・ｽE・ｽ・ｽE・ｽ・ｽE・ｽﾍゑｿｽﾒゑｿｽ・ｽE・ｽﾄゑｿｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ
	if (m_waitBindKeyPtr != nullptr)
	{
		bool isP1 = (m_menuState == MenuState::ConfigP1);
		InputDeviceType currentDevice = isP1 ? g_inputDeviceP1 : g_inputDeviceP2;

		if (currentDevice == InputDeviceType::KEYBOARD)
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
		}
		else
		{
			int padNo = (int)currentDevice - 1;
			const int buttons[] = {
				XINPUT_GAMEPAD_DPAD_UP, XINPUT_GAMEPAD_DPAD_DOWN, XINPUT_GAMEPAD_DPAD_LEFT, XINPUT_GAMEPAD_DPAD_RIGHT,
				XINPUT_GAMEPAD_START, XINPUT_GAMEPAD_BACK, XINPUT_GAMEPAD_LEFT_THUMB, XINPUT_GAMEPAD_RIGHT_THUMB,
				XINPUT_GAMEPAD_LEFT_SHOULDER, XINPUT_GAMEPAD_RIGHT_SHOULDER,
				XINPUT_GAMEPAD_A, XINPUT_GAMEPAD_B, XINPUT_GAMEPAD_X, XINPUT_GAMEPAD_Y
			};
			for (int b : buttons)
			{
				if (IsPadTrigger(padNo, b))
				{
					*m_waitBindKeyPtr = b;
					m_waitBindKeyPtr = nullptr;
					break;
				}
			}
		}
	}
	else
	{
		// ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽj・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ[・ｽE・ｽﾌ階・ｽE・ｽw・ｽE・ｽﾉ会ｿｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽ・ｽE・ｽ
		if (m_menuState == MenuState::TopMenu)
		{
			// ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽE・ｽE・ｽL・ｽE・ｽ[・ｽE・ｽﾅト・ｽE・ｽb・ｽE・ｽv・ｽE・ｽ・ｽE・ｽ・ｽE・ｽj・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ[・ｽE・ｽﾌカ・ｽE・ｽ[・ｽE・ｽ\・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽﾚ難ｿｽ
			if (isMenuRight())
			{
				m_topSelectedIndex = (m_topSelectedIndex + 1) % m_topItems.size();
			}
			if (isMenuLeft())
			{
				m_topSelectedIndex = (m_topSelectedIndex - 1 + m_topItems.size()) % m_topItems.size();
			}

			// ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽL・ｽE・ｽ[・ｽE・ｽﾅ遷・ｽE・ｽ・ｽE・ｽ
			if (isMenuConfirm())
			{
				if (m_topSelectedIndex == 0)
				{
					// 1P・ｽE・ｽﾝ抵ｿｽ・ｽE・ｽ・ｽE・ｽJ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽB・ｽE・ｽW・ｽE・ｽJ・ｽE・ｽA・ｽE・ｽj・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ[・ｽE・ｽV・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽﾌゑｿｽ・ｽE・ｽﾟにス・ｽE・ｽP・ｽE・ｽ[・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ0・ｽE・ｽﾉ・・ｽ・ｽ・ｽE・ｽZ・ｽE・ｽb・ｽE・ｽg・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ
					m_menuState = MenuState::ConfigP1;
					m_configSelectedIndex = 0;
					m_windowScaleY = 0.0f;
				}
				else if (m_topSelectedIndex == 1)
				{
					// 2P・ｽE・ｽﾝ抵ｿｽ・ｽE・ｽ・ｽE・ｽJ・ｽE・ｽ・ｽE・ｽ
					m_menuState = MenuState::ConfigP2;
					m_configSelectedIndex = 0;
					m_windowScaleY = 0.0f;
				}
				else if (m_topSelectedIndex == 2)
				{
					// ・ｽE・ｽg・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ[・ｽE・ｽj・ｽE・ｽ・ｽE・ｽ・ｽE・ｽO・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ[・ｽE・ｽh・ｽE・ｽﾉ移行・ｽE・ｽ・ｽE・ｽ・ｽE・ｽA・ｽE・ｽ・ｽE・ｽ・ｽE・ｽv・ｽE・ｽ・ｽE・ｽ・ｽE・ｽC・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ[・ｽE・ｽﾌ托ｿｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ^・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ
					m_menuState = MenuState::TrainingMode;
					if (Player* p1 = GetObj<Player>("Player")) p1->SetInputType(PlayerInputType::PLAYER_1);
					if (Player* p2 = GetObj<Player>("Player2")) p2->SetInputType(PlayerInputType::PLAYER_2);
				}
				else if (m_topSelectedIndex == 3)
				{
					// ・ｽE・ｽQ・ｽE・ｽ[・ｽE・ｽ・ｽE・ｽ・ｽE・ｽX・ｽE・ｽ^・ｽE・ｽ[・ｽE・ｽg
					s_isConfigSet = true;
				}
			}
		}
		else if (m_menuState == MenuState::ConfigP1 || m_menuState == MenuState::ConfigP2)
		{
			// ・ｽE・ｽR・ｽE・ｽ・ｽE・ｽ・ｽE・ｽt・ｽE・ｽB・ｽE・ｽO・ｽE・ｽ・ｽE・ｽﾊゑｿｽ・ｽE・ｽJ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽﾛのア・ｽE・ｽj・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ[・ｽE・ｽV・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽi・ｽE・ｽc・ｽE・ｽﾉ伸・ｽE・ｽﾑて展・ｽE・ｽJ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽj
			if (m_windowScaleY < 1.0f)
			{
				m_windowScaleY += tick * 15.0f;
				if (m_windowScaleY > 1.0f) m_windowScaleY = 1.0f;
			}
			else
			{
				// ・ｽE・ｽW・ｽE・ｽJ・ｽE・ｽA・ｽE・ｽj・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ[・ｽE・ｽV・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽI・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽﾄゑｿｽ・ｽE・ｽ・ｽE・ｽ鼾・・ｽ・ｽﾌみ托ｿｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽｯ付・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ
				std::vector<ConfigItem>& currentItems = (m_menuState == MenuState::ConfigP1) ? m_p1Items : m_p2Items;

				// ・ｽE・ｽ繪ｺ・ｽE・ｽL・ｽE・ｽ[・ｽE・ｽﾅ設定項・ｽE・ｽﾚのカ・ｽE・ｽ[・ｽE・ｽ\・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽﾚ難ｿｽ
				if (isMenuDown())
				{
					m_configSelectedIndex = (m_configSelectedIndex + 1) % currentItems.size();
				}
				if (isMenuUp())
				{
					m_configSelectedIndex = (m_configSelectedIndex - 1 + currentItems.size()) % currentItems.size();
				}

				// ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽL・ｽE・ｽ[・ｽE・ｽﾌ擾ｿｽ・ｽE・ｽ・ｽE・ｽ
				if (isMenuConfirm())
				{
					if (currentItems[m_configSelectedIndex].isDeviceSelect)
					{
						// ・ｽE・ｽf・ｽE・ｽo・ｽE・ｽC・ｽE・ｽX・ｽE・ｽﾘゑｿｽﾖゑｿｽ
						InputDeviceType currentDevice = (m_menuState == MenuState::ConfigP1) ? g_inputDeviceP1 : g_inputDeviceP2;
						InputDeviceType nextDevice = currentDevice;

						// ・ｽE・ｽL・ｽE・ｽ・ｽE・ｽ・ｽE・ｽﾈデ・ｽE・ｽo・ｽE・ｽC・ｽE・ｽX・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽﾂゑｿｽ・ｽE・ｽ・ｽE・ｽﾜで・・ｽ・ｽ・ｽE・ｽ[・ｽE・ｽv・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽi・ｽE・ｽ・ｽE・ｽ・ｽE・ｽﾚ托ｿｽ・ｽE・ｽﾌパ・ｽE・ｽb・ｽE・ｽh・ｽE・ｽ・ｽE・ｽ・ｽE・ｽX・ｽE・ｽL・ｽE・ｽb・ｽE・ｽv・ｽE・ｽj
						do {
							nextDevice = (InputDeviceType)(((int)nextDevice + 1) % 5);
							if (nextDevice == InputDeviceType::KEYBOARD) break; // ・ｽE・ｽL・ｽE・ｽ[・ｽE・ｽ{・ｽE・ｽ[・ｽE・ｽh・ｽE・ｽﾍ擾ｿｽﾉ有・ｽE・ｽ・ｽE・ｽ
							if (IsPadConnected((int)nextDevice - 1)) break;     // ・ｽE・ｽﾚ托ｿｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽﾄゑｿｽ・ｽE・ｽ・ｽE・ｽR・ｽE・ｽ・ｽE・ｽ・ｽE・ｽg・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ[・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ[・ｽE・ｽﾈゑｿｽL・ｽE・ｽ・ｽE・ｽ
						} while (nextDevice != currentDevice);

						if (m_menuState == MenuState::ConfigP1) g_inputDeviceP1 = nextDevice;
						else g_inputDeviceP2 = nextDevice;

						RefreshConfigPointers(); // ・ｽE・ｽ\・ｽE・ｽ・ｽE・ｽ・ｽE・ｽﾆポ・ｽE・ｽC・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ^・ｽE・ｽ・ｽE・ｽ・ｽE・ｽX・ｽE・ｽV
					}
					else if (currentItems[m_configSelectedIndex].keyPtr == nullptr)
					{
						// Back・ｽE・ｽ・ｽE・ｽ・ｽE・ｽI・ｽE・ｽﾎれた・ｽE・ｽ鼾・・ｽ・ｽﾍト・ｽE・ｽb・ｽE・ｽv・ｽE・ｽ・ｽE・ｽ・ｽE・ｽj・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ[・ｽE・ｽﾉ戻ゑｿｽ
						m_menuState = MenuState::TopMenu;
					}
					else
					{
						// ・ｽE・ｽL・ｽE・ｽ[・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ闢厄ｿｽﾄ待機・ｽE・ｽ・ｽE・ｽﾔに移行
						m_waitBindKeyPtr = currentItems[m_configSelectedIndex].keyPtr;
					}
				}

				// ・ｽE・ｽL・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽZ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽL・ｽE・ｽ[・ｽE・ｽﾅゑｿｽ・ｽE・ｽg・ｽE・ｽb・ｽE・ｽv・ｽE・ｽ・ｽE・ｽ・ｽE・ｽj・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ[・ｽE・ｽﾉ戻ゑｿｽ
				if (isMenuCancel())
				{
					m_menuState = MenuState::TopMenu;
				}
			}
		}
		else if (m_menuState == MenuState::TrainingMode)
		{
			// ・ｽE・ｽg・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ[・ｽE・ｽj・ｽE・ｽ・ｽE・ｽ・ｽE・ｽO・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ[・ｽE・ｽh・ｽE・ｽ・ｽE・ｽ・ｽE・ｽAESC・ｽE・ｽL・ｽE・ｽ[・ｽE・ｽﾜゑｿｽ・ｽE・ｽ・ｽE・ｽBACK・ｽE・ｽ{・ｽE・ｽ^・ｽE・ｽ・ｽE・ｽ・ｽE・ｽﾅ戻ゑｿｽ
			bool exitTraining = false;
			if (IsKeyTrigger(VK_ESCAPE)) exitTraining = true;
			for (int i = 0; i < 4; ++i) if (IsPadTrigger(i, XINPUT_GAMEPAD_BACK)) exitTraining = true;

			if (exitTraining)
			{
				m_menuState = MenuState::TopMenu;
				if (Player* p1 = GetObj<Player>("Player")) p1->SetInputType(PlayerInputType::AI);
				if (Player* p2 = GetObj<Player>("Player2")) p2->SetInputType(PlayerInputType::AI);
			}
		}
	}

	Player* player = GetObj<Player>("Player");
	Player* player2 = GetObj<Player>("Player2");

	// ・ｽE・ｽv・ｽE・ｽ・ｽE・ｽ・ｽE・ｽC・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ[・ｽE・ｽﾌ更・ｽE・ｽV
	if (player) player->Update(tick);
	if (player2) player2->Update(tick);

	// ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ阡ｻ・ｽE・ｽ闖茨ｿｽ・ｽE・ｽ・ｽE・ｽi・ｽE・ｽU・ｽE・ｽ・ｽE・ｽ・ｽE・ｽﾌヒ・ｽE・ｽb・ｽE・ｽg・ｽE・ｽ・ｽE・ｽv・ｽE・ｽ・ｽE・ｽ・ｽE・ｽC・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ[・ｽE・ｽ・ｽE・ｽ・ｽE・ｽm・ｽE・ｽﾌ会ｿｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽj
	BattleCollision::UpdateInteractions(player, player2, tick, m_hitEffects, 6.0f);

	for (auto effect : m_hitEffects)
	{
		effect->Update(tick);
	}

	// ・ｽE・ｽv・ｽE・ｽ・ｽE・ｽ・ｽE・ｽC・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ[・ｽE・ｽﾌ位置・ｽE・ｽﾉ搾ｿｽ・ｽE・ｽ・ｽ・ｽ・ｽ・ｽE・ｽﾄカ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽﾇ従・ｽE・ｽE・ｽE・ｽY・ｽE・ｽ[・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ髏ｧ・ｽE・ｽ・ｽE・ｽ
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

		// ・ｽE・ｽL・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽN・ｽE・ｽ^・ｽE・ｽ[・ｽE・ｽﾔの具ｿｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽﾉ奇ｿｽﾃゑｿｽ・ｽE・ｽﾄカ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽﾌ茨ｿｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽE・ｽ・ｽ・ｽE・ｽ・ｽE・ｽv・ｽE・ｽZ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ
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

		// ・ｽX・ｽJ・ｽC・ｽh・ｽ[・ｽ・ｽ・ｽ・ｽ・ｽJ・ｽ・ｽ・ｽ・ｽ・ｽﾊ置・ｽﾉ追従・ｽ・ｽ・ｽ・ｽ・ｽ・ｽ
		if (m_skyDome) m_skyDome->Update(pCamera->GetPos());
	}
}

void SceneKeyConfig::DrawRectPixel(float px, float py, float pw, float ph, DirectX::XMFLOAT4 color)
{
	// ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽSprite・ｽE・ｽN・ｽE・ｽ・ｽE・ｽ・ｽE・ｽX・ｽE・ｽ・ｽE・ｽ・ｽE・ｽu・ｽE・ｽw・ｽE・ｽ閧ｵ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽW・ｽE・ｽ・ｽES・ｽE・ｽﾆゑｿｽ・ｽE・ｽ・ｽE・ｽv・ｽE・ｽd・ｽE・ｽl・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽﾟ、
	// ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ(px, py)・ｽE・ｽﾉ描・ｽE・ｽ謔ｳ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ謔､・ｽE・ｽ・ｽE・ｽ・ｽE・ｽS・ｽE・ｽ・ｽE・ｽ・ｽE・ｽW・ｽE・ｽ・ｽE・ｽ・ｽE・ｽv・ｽE・ｽZ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽﾄゑｿｽ・ｽE・ｽ・ｽE・ｽNDC・ｽE・ｽﾉ変奇ｿｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ
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
	// ・ｽE・ｽA・ｽE・ｽ・ｽE・ｽ・ｽE・ｽt・ｽE・ｽ@・ｽE・ｽx・ｽE・ｽb・ｽE・ｽg・ｽE・ｽﾆ撰ｿｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽﾍゑｿｽ・ｽE・ｽﾌまま・・ｽ・ｽ・ｽE・ｽC・ｽE・ｽh・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽﾆゑｿｽ・ｽE・ｽﾄ返ゑｿｽ
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

	// ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽL・ｽE・ｽ[・ｽE・ｽﾌ厄ｿｽ・ｽE・ｽﾌ対会ｿｽ
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

	// ・ｽE・ｽ・ｽE・ｽL・ｽE・ｽﾈ外・ｽE・ｽﾍ番搾ｿｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽo・ｽE・ｽﾍゑｿｽ・ｽE・ｽ・ｽE・ｽ
	static wchar_t buf[16];
	swprintf_s(buf, L"Key %d", vk);
	return buf;
}

const wchar_t* SceneKeyConfig::GetPadButtonName(int button)
{
	switch (button)
	{
	case XINPUT_GAMEPAD_DPAD_UP: return L"D-PAD UP";
	case XINPUT_GAMEPAD_DPAD_DOWN: return L"D-PAD DOWN";
	case XINPUT_GAMEPAD_DPAD_LEFT: return L"D-PAD LEFT";
	case XINPUT_GAMEPAD_DPAD_RIGHT: return L"D-PAD RIGHT";
	case XINPUT_GAMEPAD_START: return L"START";
	case XINPUT_GAMEPAD_BACK: return L"BACK";
	case XINPUT_GAMEPAD_LEFT_THUMB: return L"L3";
	case XINPUT_GAMEPAD_RIGHT_THUMB: return L"R3";
	case XINPUT_GAMEPAD_LEFT_SHOULDER: return L"LB";
	case XINPUT_GAMEPAD_RIGHT_SHOULDER: return L"RB";
	case XINPUT_GAMEPAD_A: return L"A";
	case XINPUT_GAMEPAD_B: return L"B";
	case XINPUT_GAMEPAD_X: return L"X";
	case XINPUT_GAMEPAD_Y: return L"Y";
	}
	return L"Unknown";
}

const wchar_t* SceneKeyConfig::GetDeviceName(InputDeviceType type)
{
	if (type == InputDeviceType::KEYBOARD) return L"Keyboard";

	static wchar_t buf[64];
	int padNo = (int)type - 1;

	if (IsPadXInput(padNo))
	{
		swprintf_s(buf, L"Controller %d (Xbox)", padNo + 1);
	}
	else
	{
		swprintf_s(buf, L"Controller %d (PS)", padNo + 1);
	}

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

	// ・ｽw・ｽi・ｽi・ｽX・ｽJ・ｽC・ｽh・ｽ[・ｽ・ｽ・ｽj・ｽﾌ描・ｽ・ｽB・ｽv・ｽ・ｽ・ｽC・ｽ・ｽ・ｽ[・ｽ・ｽ・ｽ・ｽﾉ描・ｽ・ｽ
	if (m_skyDome && pCamera)
	{
		// スカイドーム(最奥)を描画するため LESS_EQUAL をセット
		if (m_pDepthState3D) GetContext()->OMSetDepthStencilState(m_pDepthState3D, 0);

		if (m_pCullNone) GetContext()->RSSetState(m_pCullNone);
		m_skyDome->Draw(pCamera->GetView(), pCamera->GetProj(), GetObj<Shader>("VS_Object"));
		GetContext()->RSSetState(nullptr);
	}

	// ・ｽE・ｽA・ｽE・ｽE・ｽE・ｽg・ｽE・ｽ・ｽE・ｽ・ｽE・ｽC・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ`・ｽE・ｽ・ｽE・ｽp・ｽE・ｽX
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

	// ・ｽE・ｽﾊ常モ・ｽE・ｽf・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ`・ｽE・ｽ・ｽE・ｽp・ｽE・ｽX
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

	// ・ｽE・ｽ・ｽE・ｽﾑ難ｿｽ・ｽE・ｽ・ｽE・ｽﾌ描・ｽE・ｽ・ｽE・ｽ
	if (player && player->GetProjectile())
	{
		player->GetProjectile()->Draw(pCamera->GetView(), pCamera->GetProj());
	}
	if (player2 && player2->GetProjectile())
	{
		player2->GetProjectile()->Draw(pCamera->GetView(), pCamera->GetProj());
	}

	// ・ｽE・ｽq・ｽE・ｽb・ｽE・ｽg・ｽE・ｽG・ｽE・ｽt・ｽE・ｽF・ｽE・ｽN・ｽE・ｽg・ｽE・ｽﾌ描・ｽE・ｽ・ｽE・ｽ
	DirectX::XMFLOAT4X4 view = pCamera->GetView();
	DirectX::XMFLOAT4X4 proj = pCamera->GetProj();
	for (auto effect : m_hitEffects)
	{
		effect->Draw(view, proj);
	}

	// ・ｽE・ｽg・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ[・ｽE・ｽj・ｽE・ｽ・ｽE・ｽ・ｽE・ｽO・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ[・ｽE・ｽh・ｽE・ｽﾈ外・ｽE・ｽﾌ場合・ｽE・ｽﾌみ、UI・ｽE・ｽ・ｽE・ｽ`・ｽE・ｽ謔ｷ・ｽE・ｽ・ｽE・ｽ
	if (m_menuState != MenuState::TrainingMode)
	{
		SimpleUI::Clear();

		// ・ｽE・ｽ・ｽE・ｽﾊ上部・ｽE・ｽﾉト・ｽE・ｽb・ｽE・ｽv・ｽE・ｽ・ｽE・ｽ・ｽE・ｽj・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ[・ｽE・ｽp・ｽE・ｽﾌ擾ｿｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽﾟのネ・ｽE・ｽC・ｽE・ｽr・ｽE・ｽ[・ｽE・ｽu・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ[・ｽE・ｽw・ｽE・ｽi・ｽE・ｽ・ｽE・ｽ`・ｽE・ｽ・ｽE・ｽ
		DrawRectPixel(0, 0, 1280, 150, { 0.0f, 0.15f, 0.3f, 0.8f });

		if (m_menuState == MenuState::TopMenu)
		{
			// ・ｽE・ｽg・ｽE・ｽb・ｽE・ｽv・ｽE・ｽ・ｽE・ｽ・ｽE・ｽj・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ[・ｽE・ｽﾌ搾ｿｽ・ｽE・ｽﾚ枠・ｽE・ｽ・ｽE・ｽ`・ｽE・ｽ・ｽE・ｽ
			for (int i = 0; i < m_topItems.size(); ++i)
			{
				DirectX::XMFLOAT4 color = (i == m_topSelectedIndex) ? XMFLOAT4(0.0f, 0.8f, 1.0f, 0.9f) : XMFLOAT4(0.0f, 0.4f, 0.6f, 0.7f);
				DrawRectPixel(50 + i * 250, 50, 200, 50, color);
			}
		}
		else if (m_menuState == MenuState::ConfigP1 || m_menuState == MenuState::ConfigP2)
		{
			// ・ｽE・ｽR・ｽE・ｽ・ｽE・ｽ・ｽE・ｽt・ｽE・ｽB・ｽE・ｽO・ｽE・ｽp・ｽE・ｽﾌパ・ｽE・ｽl・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ`・ｽE・ｽ・ｽE・ｽi・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ繪ｺ・ｽE・ｽﾉ伸・ｽE・ｽﾑる演・ｽE・ｽo・ｽE・ｽv・ｽE・ｽZ・ｽE・ｽj
			float panelHeight = 540.0f * m_windowScaleY; // ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽﾚゑｿｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽﾌで枠・ｽE・ｽ・ｽE・ｽ・ｽE・ｽL・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ
			float panelY = 150.0f + (540.0f - panelHeight) / 2.0f;

			DrawRectPixel(50, panelY, 400, panelHeight, { 0.0f, 0.15f, 0.3f, 0.8f });

			// ・ｽE・ｽg・ｽE・ｽﾌ展・ｽE・ｽJ・ｽE・ｽA・ｽE・ｽj・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ[・ｽE・ｽV・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽI・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽﾄゑｿｽ・ｽE・ｽ迺・・ｽ・ｽg・ｽE・ｽﾌ搾ｿｽ・ｽE・ｽﾚ枠・ｽE・ｽ・ｽE・ｽ`・ｽE・ｽ謔ｷ・ｽE・ｽ・ｽE・ｽ
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

		// ・ｽE・ｽo・ｽE・ｽ^・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽl・ｽE・ｽp・ｽE・ｽ`UI・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ鼕・・ｽ・ｽ`・ｽE・ｽ・ｽE・ｽ
		SimpleUI::DrawAll();

		// ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽDirectWrite・ｽE・ｽﾉゑｿｽ・ｽE・ｽt・ｽE・ｽH・ｽE・ｽ・ｽE・ｽ・ｽE・ｽg・ｽE・ｽﾌ描・ｽE・ｽ謠茨ｿｽ・ｽE・ｽ
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
			bool isP1 = (m_menuState == MenuState::ConfigP1);
			SimpleFont::Draw(isP1 ? L"Player 1 Config" : L"Player 2 Config", 50, 50, 32.0f, { 1.0f, 0.9f, 0.2f, 1.0f });

			// ・ｽE・ｽg・ｽE・ｽﾌア・ｽE・ｽj・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ[・ｽE・ｽV・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽI・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽﾉ各・ｽE・ｽ・ｽE・ｽ・ｽE・ｽﾚのテ・ｽE・ｽL・ｽE・ｽX・ｽE・ｽg・ｽE・ｽ・ｽE・ｽ`・ｽE・ｽ・ｽE・ｽ
			if (m_windowScaleY >= 1.0f)
			{
				std::vector<ConfigItem>& currentItems = isP1 ? m_p1Items : m_p2Items;
				InputDeviceType currentDevice = isP1 ? g_inputDeviceP1 : g_inputDeviceP2;

				for (int i = 0; i < currentItems.size(); ++i)
				{
					DirectX::XMFLOAT4 textColor = (i == m_configSelectedIndex) ? XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f) : XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);

					// ・ｽE・ｽA・ｽE・ｽN・ｽE・ｽV・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ`・ｽE・ｽ・ｽE・ｽ (・ｽE・ｽ・ｽE・ｽ: Light Punch)
					SimpleFont::Draw(currentItems[i].label, 80, 175 + i * 45, 20.0f, textColor);

					// ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽﾝの奇ｿｽ・ｽE・ｽ闢厄ｿｽﾄキ・ｽE・ｽ[・ｽE・ｽA・ｽE・ｽﾜゑｿｽ・ｽE・ｽﾍ待機・ｽE・ｽ・ｽE・ｽ・ｽE・ｽﾌ・・ｽ・ｽ・ｽE・ｽb・ｽE・ｽZ・ｽE・ｽ[・ｽE・ｽW・ｽE・ｽ・ｽE・ｽ`・ｽE・ｽ・ｽE・ｽ
					if (currentItems[i].isDeviceSelect)
					{
						SimpleFont::Draw(GetDeviceName(currentDevice), 250, 175 + i * 45, 20.0f, { 0.0f, 1.0f, 0.5f, 1.0f });
					}
					else if (currentItems[i].keyPtr != nullptr)
					{
						if (m_waitBindKeyPtr == currentItems[i].keyPtr)
						{
							SimpleFont::Draw(L"Press Any Button...", 250, 175 + i * 45, 20.0f, { 1.0f, 0.3f, 0.3f, 1.0f });
						}
						else
						{
							const wchar_t* btnName = (currentDevice == InputDeviceType::KEYBOARD) ? GetKeyName(*currentItems[i].keyPtr) : GetPadButtonName(*currentItems[i].keyPtr);
							SimpleFont::Draw(btnName, 250, 175 + i * 45, 20.0f, textColor);
						}
					}
				}
			}
		}

		SimpleFont::End();
	}

	GetContext()->RSSetState(nullptr);
}
