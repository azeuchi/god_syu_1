#include "SceneKeyConfig.h"
#include "Input.h"
#include "Player.h"
#include "CameraBase.h"
#include "LightBase.h"
#include "Shader.h"
#include "Model.h"
#include "SkyDome.h"
#include "PlayerParameterLoader.h"
#include "PlayerAssetLoader.h"
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

// �E�E�E�E�E�E�E�J�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�̈ړ��E�E�E�E�E�E�E�𐧌��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�͈�
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

	// �E�E�E�E�E�E�E�t�E�E�E�E�E�E�E�H�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�g�E�E�E�E�E�E�E�V�E�E�E�E�E�E�E�X�E�E�E�E�E�E�E�e�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�̏��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E� (DirectWrite�E�E�E�E�E�E�E��E�E�E�E�E�E�E�)
	SimpleFont::Init(GetDevice(), GetContext());

	// �E�E�E�E�E�E�E�g�E�E�E�E�E�E�E�b�E�E�E�E�E�E�E�v�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�j�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�[�E�E�E�E�E�E�E�̍��E�E�E�E�E�E�E�ڂ�ݒ�
	m_topItems = {
		{ L"Player 1 Config" },
		{ L"Player 2 Config" },
		{ L"Training Mode" },
		{ L"Game Start" }
	};

	// �E�E�E�E�E�E�E�I�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�Ă��E�E�E�E�E�E�E��E�E�E�E�E�E�E�f�E�E�E�E�E�E�E�o�E�E�E�E�E�E�E�C�E�E�E�E�E�E�E�X�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�ڑ��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�Ă��E�E�E�E�E�E�E�Ȃ��E�E�E�E�E�E�E�ꍁE�E�E�E��E�E�E�̓L�E�E�E�E�E�E�E�[�E�E�E�E�E�E�E�{�E�E�E�E�E�E�E�[�E�E�E�E�E�E�E�h�E�E�E�E�E�E�E�ɖ߂�
	if (g_inputDeviceP1 != InputDeviceType::KEYBOARD && !IsPadConnected((int)g_inputDeviceP1 - 1))
	{
		g_inputDeviceP1 = InputDeviceType::KEYBOARD;
	}
	if (g_inputDeviceP2 != InputDeviceType::KEYBOARD && !IsPadConnected((int)g_inputDeviceP2 - 1))
	{
		g_inputDeviceP2 = InputDeviceType::KEYBOARD;
	}

	// �E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�̃f�E�E�E�E�E�E�E�o�E�E�E�E�E�E�E�C�E�E�E�E�E�E�E�X�E�E�E�E�E�E�E��E�E�E�E�E�E�E�Ԃɍ��E�E�E�E�E�E�E��E�E�E��E�E�E��E�E�E��E�E�E�E�E�E�E�āE�E�E�E��E�E�E��E�E�E�E�E�E�E�j�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�[�E�E�E�E�E�E�E�̃|�E�E�E�E�E�E�E�C�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�^�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�\�E�E�E�E�E�E�E�z
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

	// �v���C���[�`��w���p�[�i�A�E�g���C���E�J�����O���Q�[���Ƌ��ʂŗp�Ӂj
	m_playerRenderer.Setup(this);

	// �E�w�E�i�E�i�E�X�E�J�E�C�E�h�E�[�E��E��E�j�E�ꎮ�E��E�p�E��E�
	m_skyDome = new SkyDome();
	m_skyDome->Setup(this);


	// �E�E�E�E�E�E�E�v�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�C�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�[1�E�E�E�E�E�E�E�̐��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�Ɛݒ�
	CreateObj<Player>("Player");
	Player* player = GetObj<Player>("Player");
	player->SetInputType(PlayerInputType::AI); // �E�E�E�E�E�E�E�L�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�Ȃ��E�E�E�E�E�E�E�悤AI�E�E�E�E�E�E�E�i�E�E�E�E�E�E�E�ҋ@�E�E�E�E�E�E�E�j�E�E�E�E�E�E�E�ɌŒ�
	PlayerParameterLoader::LoadSettings(player);
	if (!player->Load("Assets/Model/knight/Idle.fbx", 0.014f, true, false))
	{
		MessageBox(NULL, "�E�E�E�E�E�E�E�v�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�C�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�[�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�f�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�̓ǂݍ��E�E�E�E�E�E�E�݂Ɏ��E�E�E�E�E�E�E�s�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�܂��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�B", "Error", MB_OK);
	}
	PlayerAssetLoader::LoadCommonAnimations(player);
	player->SetPosition({ -2.0f, 0.0f, 0.0f });
	player->SetRotation({ 0.0f, DirectX::XM_PI / -2.0f, 0.0f });
	player->Reset();

	// �E�E�E�E�E�E�E�v�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�C�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�[2�E�E�E�E�E�E�E�̐��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�Ɛݒ�
	CreateObj<Player>("Player2");
	Player* player2 = GetObj<Player>("Player2");
	player2->SetInputType(PlayerInputType::AI); // �E�E�E�E�E�E�E�v�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�C�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�[�E�E�E�E�E�E�E�̑��E�E�E�E�E�E�E��E�E�E�E�E�E�E�𖳌��E�E�E�E�E�E�E��E�E�E�E�E�E�E�
	player2->SetMoveSpeed(player->GetMoveSpeed());
	player2->SetScale(player->GetScale());
	PlayerParameterLoader::CopyParameters(player, player2);
	if (!player2->Load("Assets/Model/knight/Idle.fbx", 0.014f, true, false))
	{
		MessageBox(NULL, "�E�E�E�E�E�E�E�v�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�C�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�[2�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�f�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�̓ǂݍ��E�E�E�E�E�E�E�݂Ɏ��E�E�E�E�E�E�E�s�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�܂��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�B", "Error", MB_OK);
	}
	PlayerAssetLoader::LoadCommonAnimations(player2);
	player2->SetPosition({ 2.0f, 0.0f, 0.0f });
	player2->SetRotation({ 0.0f, DirectX::XM_PI / 2.0f, 0.0f }); // �E�E�E�E�E�E�E��E�E�E�E�E�E�E�]�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�Ō��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�𒲐��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�
	player2->Reset();

	// �E�E�E�E�E�E�E�G�E�E�E�E�E�E�E�t�E�E�E�E�E�E�E�F�E�E�E�E�E�E�E�N�E�E�E�E�E�E�E�g�E�E�E�E�E�E�E�̏��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�
	m_hitEffects.Init(10);

	// �E�E�E�E�E�E�E�J�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�̏��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�ݒ�
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

	if (m_skyDome) { delete m_skyDome; m_skyDome = nullptr; }
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

	// �E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�ꂩ�E�E�E�E�E�E�E�̃L�E�E�E�E�E�E�E�[�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�蓖�Ă��E�E�E�E�E�E�E�I�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�A�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�͂�҂��E�E�E�E�E�E�E�Ă��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�
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
		// �E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�j�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�[�E�E�E�E�E�E�E�̊K�E�E�E�E�E�E�E�w�E�E�E�E�E�E�E�ɉ��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E��E�E�E�E�E�E�E�
		if (m_menuState == MenuState::TopMenu)
		{
			// �E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�E�E�E�E�E�E�E�E�L�E�E�E�E�E�E�E�[�E�E�E�E�E�E�E�Ńg�E�E�E�E�E�E�E�b�E�E�E�E�E�E�E�v�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�j�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�[�E�E�E�E�E�E�E�̃J�E�E�E�E�E�E�E�[�E�E�E�E�E�E�E�\�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�ړ�
			if (isMenuRight())
			{
				m_topSelectedIndex = (m_topSelectedIndex + 1) % m_topItems.size();
			}
			if (isMenuLeft())
			{
				m_topSelectedIndex = (m_topSelectedIndex - 1 + m_topItems.size()) % m_topItems.size();
			}

			// �E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�L�E�E�E�E�E�E�E�[�E�E�E�E�E�E�E�őJ�E�E�E�E�E�E�E��E�E�E�E�E�E�E�
			if (isMenuConfirm())
			{
				if (m_topSelectedIndex == 0)
				{
					// 1P�E�E�E�E�E�E�E�ݒ��E�E�E�E�E�E�E��E�E�E�E�E�E�E�J�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�B�E�E�E�E�E�E�E�W�E�E�E�E�E�E�E�J�E�E�E�E�E�E�E�A�E�E�E�E�E�E�E�j�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�[�E�E�E�E�E�E�E�V�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�̂��E�E�E�E�E�E�E�߂ɃX�E�E�E�E�E�E�E�P�E�E�E�E�E�E�E�[�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�0�E�E�E�E�E�E�E�ɁE�E�E�E��E�E�E��E�E�E�E�E�E�E�Z�E�E�E�E�E�E�E�b�E�E�E�E�E�E�E�g�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�
					m_menuState = MenuState::ConfigP1;
					m_configSelectedIndex = 0;
					m_windowScaleY = 0.0f;
				}
				else if (m_topSelectedIndex == 1)
				{
					// 2P�E�E�E�E�E�E�E�ݒ��E�E�E�E�E�E�E��E�E�E�E�E�E�E�J�E�E�E�E�E�E�E��E�E�E�E�E�E�E�
					m_menuState = MenuState::ConfigP2;
					m_configSelectedIndex = 0;
					m_windowScaleY = 0.0f;
				}
				else if (m_topSelectedIndex == 2)
				{
					// �E�E�E�E�E�E�E�g�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�[�E�E�E�E�E�E�E�j�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�O�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�[�E�E�E�E�E�E�E�h�E�E�E�E�E�E�E�Ɉڍs�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�A�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�v�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�C�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�[�E�E�E�E�E�E�E�̑��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�^�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�
					m_menuState = MenuState::TrainingMode;
					if (Player* p1 = GetObj<Player>("Player")) p1->SetInputType(PlayerInputType::PLAYER_1);
					if (Player* p2 = GetObj<Player>("Player2")) p2->SetInputType(PlayerInputType::PLAYER_2);
				}
				else if (m_topSelectedIndex == 3)
				{
					// �E�E�E�E�E�E�E�Q�E�E�E�E�E�E�E�[�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�X�E�E�E�E�E�E�E�^�E�E�E�E�E�E�E�[�E�E�E�E�E�E�E�g
					s_isConfigSet = true;
				}
			}
		}
		else if (m_menuState == MenuState::ConfigP1 || m_menuState == MenuState::ConfigP2)
		{
			// �E�E�E�E�E�E�E�R�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�t�E�E�E�E�E�E�E�B�E�E�E�E�E�E�E�O�E�E�E�E�E�E�E��E�E�E�E�E�E�E�ʂ��E�E�E�E�E�E�E�J�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�ۂ̃A�E�E�E�E�E�E�E�j�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�[�E�E�E�E�E�E�E�V�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�i�E�E�E�E�E�E�E�c�E�E�E�E�E�E�E�ɐL�E�E�E�E�E�E�E�тēW�E�E�E�E�E�E�E�J�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�j
			if (m_windowScaleY < 1.0f)
			{
				m_windowScaleY += tick * 15.0f;
				if (m_windowScaleY > 1.0f) m_windowScaleY = 1.0f;
			}
			else
			{
				// �E�E�E�E�E�E�E�W�E�E�E�E�E�E�E�J�E�E�E�E�E�E�E�A�E�E�E�E�E�E�E�j�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�[�E�E�E�E�E�E�E�V�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�I�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�Ă��E�E�E�E�E�E�E��E�E�E�E�E�E�E�ꍁE�E�E�E��E�E�E�̂ݑ��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�󂯕t�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�
				std::vector<ConfigItem>& currentItems = (m_menuState == MenuState::ConfigP1) ? m_p1Items : m_p2Items;

				// �E�E�E�E�E�E�E�㉺�E�E�E�E�E�E�E�L�E�E�E�E�E�E�E�[�E�E�E�E�E�E�E�Őݒ荀�E�E�E�E�E�E�E�ڂ̃J�E�E�E�E�E�E�E�[�E�E�E�E�E�E�E�\�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�ړ�
				if (isMenuDown())
				{
					m_configSelectedIndex = (m_configSelectedIndex + 1) % currentItems.size();
				}
				if (isMenuUp())
				{
					m_configSelectedIndex = (m_configSelectedIndex - 1 + currentItems.size()) % currentItems.size();
				}

				// �E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�L�E�E�E�E�E�E�E�[�E�E�E�E�E�E�E�̏��E�E�E�E�E�E�E��E�E�E�E�E�E�E�
				if (isMenuConfirm())
				{
					if (currentItems[m_configSelectedIndex].isDeviceSelect)
					{
						// �E�E�E�E�E�E�E�f�E�E�E�E�E�E�E�o�E�E�E�E�E�E�E�C�E�E�E�E�E�E�E�X�E�E�E�E�E�E�E�؂�ւ�
						InputDeviceType currentDevice = (m_menuState == MenuState::ConfigP1) ? g_inputDeviceP1 : g_inputDeviceP2;
						InputDeviceType nextDevice = currentDevice;

						// �E�E�E�E�E�E�E�L�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�ȃf�E�E�E�E�E�E�E�o�E�E�E�E�E�E�E�C�E�E�E�E�E�E�E�X�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E���E�E�E�E�E�E�E��E�E�E�E�E�E�E�܂ŁE�E�E�E��E�E�E��E�E�E�E�E�E�E�[�E�E�E�E�E�E�E�v�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�i�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�ڑ��E�E�E�E�E�E�E�̃p�E�E�E�E�E�E�E�b�E�E�E�E�E�E�E�h�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�X�E�E�E�E�E�E�E�L�E�E�E�E�E�E�E�b�E�E�E�E�E�E�E�v�E�E�E�E�E�E�E�j
						do {
							nextDevice = (InputDeviceType)(((int)nextDevice + 1) % 5);
							if (nextDevice == InputDeviceType::KEYBOARD) break; // �E�E�E�E�E�E�E�L�E�E�E�E�E�E�E�[�E�E�E�E�E�E�E�{�E�E�E�E�E�E�E�[�E�E�E�E�E�E�E�h�E�E�E�E�E�E�E�͏�ɗL�E�E�E�E�E�E�E��E�E�E�E�E�E�E�
							if (IsPadConnected((int)nextDevice - 1)) break;     // �E�E�E�E�E�E�E�ڑ��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�Ă��E�E�E�E�E�E�E��E�E�E�E�E�E�E�R�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�g�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�[�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�[�E�E�E�E�E�E�E�Ȃ�L�E�E�E�E�E�E�E��E�E�E�E�E�E�E�
						} while (nextDevice != currentDevice);

						if (m_menuState == MenuState::ConfigP1) g_inputDeviceP1 = nextDevice;
						else g_inputDeviceP2 = nextDevice;

						RefreshConfigPointers(); // �E�E�E�E�E�E�E�\�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�ƃ|�E�E�E�E�E�E�E�C�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�^�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�X�E�E�E�E�E�E�E�V
					}
					else if (currentItems[m_configSelectedIndex].keyPtr == nullptr)
					{
						// Back�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�I�E�E�E�E�E�E�E�΂ꂽ�E�E�E�E�E�E�E�ꍁE�E�E�E��E�E�E�̓g�E�E�E�E�E�E�E�b�E�E�E�E�E�E�E�v�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�j�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�[�E�E�E�E�E�E�E�ɖ߂�
						m_menuState = MenuState::TopMenu;
					}
					else
					{
						// �E�E�E�E�E�E�E�L�E�E�E�E�E�E�E�[�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�蓖�đҋ@�E�E�E�E�E�E�E��E�E�E�E�E�E�E�ԂɈڍs
						m_waitBindKeyPtr = currentItems[m_configSelectedIndex].keyPtr;
					}
				}

				// �E�E�E�E�E�E�E�L�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�Z�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�L�E�E�E�E�E�E�E�[�E�E�E�E�E�E�E�ł��E�E�E�E�E�E�E�g�E�E�E�E�E�E�E�b�E�E�E�E�E�E�E�v�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�j�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�[�E�E�E�E�E�E�E�ɖ߂�
				if (isMenuCancel())
				{
					m_menuState = MenuState::TopMenu;
				}
			}
		}
		else if (m_menuState == MenuState::TrainingMode)
		{
			// �E�E�E�E�E�E�E�g�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�[�E�E�E�E�E�E�E�j�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�O�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�[�E�E�E�E�E�E�E�h�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�AESC�E�E�E�E�E�E�E�L�E�E�E�E�E�E�E�[�E�E�E�E�E�E�E�܂��E�E�E�E�E�E�E��E�E�E�E�E�E�E�BACK�E�E�E�E�E�E�E�{�E�E�E�E�E�E�E�^�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�Ŗ߂�
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

	// �E�E�E�E�E�E�E�v�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�C�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�[�E�E�E�E�E�E�E�̍X�E�E�E�E�E�E�E�V
	if (player) player->Update(tick);
	if (player2) player2->Update(tick);

	// �E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�蔻�E�E�E�E�E�E�E�菈��E�E�E�E�E�E�E��E�E�E�E�E�E�E�i�E�E�E�E�E�E�E�U�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�̃q�E�E�E�E�E�E�E�b�E�E�E�E�E�E�E�g�E�E�E�E�E�E�E��E�E�E�E�E�E�E�v�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�C�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�[�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�m�E�E�E�E�E�E�E�̉��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�j
	BattleCollision::UpdateInteractions(player, player2, tick, m_hitEffects.Raw(), 6.0f);

	m_hitEffects.Update(tick);

	// �E�E�E�E�E�E�E�v�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�C�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�[�E�E�E�E�E�E�E�̈ʒu�E�E�E�E�E�E�E�ɍ��E�E�E�E�E�E�E��E�E�E��E�E�E��E�E�E��E�E�E�E�E�E�E�ăJ�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�Ǐ]�E�E�E�E�E�E�E�E�E�E�E�E�E�E�E�Y�E�E�E�E�E�E�E�[�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�鐧�E�E�E�E�E�E�E��E�E�E�E�E�E�E�
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

		// �E�E�E�E�E�E�E�L�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�N�E�E�E�E�E�E�E�^�E�E�E�E�E�E�E�[�E�E�E�E�E�E�E�Ԃ̋��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�Ɋ�Â��E�E�E�E�E�E�E�ăJ�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�̈��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�E�E�E�E��E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�v�E�E�E�E�E�E�E�Z�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�
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

		// �E�E�E�X�E�E�E�J�E�E�E�C�E�E�E�h�E�E�E�[�E�E�E��E�E�E��E�E�E��E�E�E��E�E�E�J�E�E�E��E�E�E��E�E�E��E�E�E��E�E�E�ʒu�E�E�E�ɒǏ]�E�E�E��E�E�E��E�E�E��E�E�E��E�E�E��E�E�E�
		if (m_skyDome) m_skyDome->Update(pCamera->GetPos());
	}
}

void SceneKeyConfig::DrawRectPixel(float px, float py, float pw, float ph, DirectX::XMFLOAT4 color)
{
	// �E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�Sprite�E�E�E�E�E�E�E�N�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�X�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�u�E�E�E�E�E�E�E�w�E�E�E�E�E�E�E�肵�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�W�E�E�E�E�E�E�E��E�E�E�ES�E�E�E�E�E�E�E�Ƃ��E�E�E�E�E�E�E��E�E�E�E�E�E�E�v�E�E�E�E�E�E�E�d�E�E�E�E�E�E�E�l�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�߁A
	// �E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�(px, py)�E�E�E�E�E�E�E�ɕ`�E�E�E�E�E�E�E�悳�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�悤�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�S�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�W�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�v�E�E�E�E�E�E�E�Z�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�Ă��E�E�E�E�E�E�E��E�E�E�E�E�E�E�NDC�E�E�E�E�E�E�E�ɕϊ��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�
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
	// �E�E�E�E�E�E�E�A�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�t�E�E�E�E�E�E�E�@�E�E�E�E�E�E�E�x�E�E�E�E�E�E�E�b�E�E�E�E�E�E�E�g�E�E�E�E�E�E�E�Ɛ��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�͂��E�E�E�E�E�E�E�̂܂܁E�E�E�E��E�E�E��E�E�E�E�E�E�E�C�E�E�E�E�E�E�E�h�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�Ƃ��E�E�E�E�E�E�E�ĕԂ�
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

	// �E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�L�E�E�E�E�E�E�E�[�E�E�E�E�E�E�E�̖��E�E�E�E�E�E�E�̑Ή�
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

	// �E�E�E�E�E�E�E��E�E�E�E�E�E�E�L�E�E�E�E�E�E�E�ȊO�E�E�E�E�E�E�E�͔ԍ��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�o�E�E�E�E�E�E�E�͂��E�E�E�E�E�E�E��E�E�E�E�E�E�E�
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

	// �E�w�E�i�E�i�E�X�E�J�E�C�E�h�E�[�E��E��E�j�E�̕`�E��E�B�E�v�E��E��E�C�E��E��E�[�E��E��E��E�ɕ`�E��E�
	if (m_skyDome && pCamera) m_skyDome->DrawWithState(pCamera->GetView(), pCamera->GetProj());

	// �E�v�E��E��E�C�E��E��E�[�E�`�E��E�i�E�Q�E�[�E��E��E�Ɠ��E��E��E��E��E�A�E��E�ɑS�E��E��E�̃A�E�E�E�g�E��E��E�C�E��E��E��E��E��E��E�ɖ{�E�́j
	m_playerRenderer.DrawOutline(this, player);
	m_playerRenderer.DrawOutline(this, player2);
	m_playerRenderer.DrawBody(this, player);
	m_playerRenderer.DrawBody(this, player2);

	// �E��E�ѓ��E��E�͒ʏ�̃J�E��E��E��E��E�O�E�i�E��E��E�ʁj�E�ŕ`�E��E��E��E��E�ߊ��E��E�֖߂�
	GetContext()->RSSetState(nullptr);

	// �E�E�E�E�E�E�E��E�E�E�E�E�E�E�ѓ��E�E�E�E�E�E�E��E�E�E�E�E�E�E�̕`�E�E�E�E�E�E�E��E�E�E�E�E�E�E�
	if (player && player->GetProjectile())
	{
		player->GetProjectile()->Draw(pCamera->GetView(), pCamera->GetProj());
	}
	if (player2 && player2->GetProjectile())
	{
		player2->GetProjectile()->Draw(pCamera->GetView(), pCamera->GetProj());
	}

	// �E�E�E�E�E�E�E�q�E�E�E�E�E�E�E�b�E�E�E�E�E�E�E�g�E�E�E�E�E�E�E�G�E�E�E�E�E�E�E�t�E�E�E�E�E�E�E�F�E�E�E�E�E�E�E�N�E�E�E�E�E�E�E�g�E�E�E�E�E�E�E�̕`�E�E�E�E�E�E�E��E�E�E�E�E�E�E�
	DirectX::XMFLOAT4X4 view = pCamera->GetView();
	DirectX::XMFLOAT4X4 proj = pCamera->GetProj();
	m_hitEffects.Draw(view, proj);

	// �E�E�E�E�E�E�E�g�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�[�E�E�E�E�E�E�E�j�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�O�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�[�E�E�E�E�E�E�E�h�E�E�E�E�E�E�E�ȊO�E�E�E�E�E�E�E�̏ꍇ�E�E�E�E�E�E�E�̂݁AUI�E�E�E�E�E�E�E��E�E�E�E�E�E�E�`�E�E�E�E�E�E�E�悷�E�E�E�E�E�E�E��E�E�E�E�E�E�E�
	if (m_menuState != MenuState::TrainingMode)
	{
		SimpleUI::Clear();

		// �E�E�E�E�E�E�E��E�E�E�E�E�E�E�ʏ㕔�E�E�E�E�E�E�E�Ƀg�E�E�E�E�E�E�E�b�E�E�E�E�E�E�E�v�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�j�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�[�E�E�E�E�E�E�E�p�E�E�E�E�E�E�E�̏��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�߂̃l�E�E�E�E�E�E�E�C�E�E�E�E�E�E�E�r�E�E�E�E�E�E�E�[�E�E�E�E�E�E�E�u�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�[�E�E�E�E�E�E�E�w�E�E�E�E�E�E�E�i�E�E�E�E�E�E�E��E�E�E�E�E�E�E�`�E�E�E�E�E�E�E��E�E�E�E�E�E�E�
		DrawRectPixel(0, 0, 1280, 150, { 0.0f, 0.15f, 0.3f, 0.8f });

		if (m_menuState == MenuState::TopMenu)
		{
			// �E�E�E�E�E�E�E�g�E�E�E�E�E�E�E�b�E�E�E�E�E�E�E�v�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�j�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�[�E�E�E�E�E�E�E�̍��E�E�E�E�E�E�E�ژg�E�E�E�E�E�E�E��E�E�E�E�E�E�E�`�E�E�E�E�E�E�E��E�E�E�E�E�E�E�
			for (int i = 0; i < m_topItems.size(); ++i)
			{
				DirectX::XMFLOAT4 color = (i == m_topSelectedIndex) ? XMFLOAT4(0.0f, 0.8f, 1.0f, 0.9f) : XMFLOAT4(0.0f, 0.4f, 0.6f, 0.7f);
				DrawRectPixel(50 + i * 250, 50, 200, 50, color);
			}
		}
		else if (m_menuState == MenuState::ConfigP1 || m_menuState == MenuState::ConfigP2)
		{
			// �E�E�E�E�E�E�E�R�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�t�E�E�E�E�E�E�E�B�E�E�E�E�E�E�E�O�E�E�E�E�E�E�E�p�E�E�E�E�E�E�E�̃p�E�E�E�E�E�E�E�l�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�`�E�E�E�E�E�E�E��E�E�E�E�E�E�E�i�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�㉺�E�E�E�E�E�E�E�ɐL�E�E�E�E�E�E�E�т鉉�E�E�E�E�E�E�E�o�E�E�E�E�E�E�E�v�E�E�E�E�E�E�E�Z�E�E�E�E�E�E�E�j
			float panelHeight = 540.0f * m_windowScaleY; // �E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�ڂ��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�̂Řg�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�L�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�
			float panelY = 150.0f + (540.0f - panelHeight) / 2.0f;

			DrawRectPixel(50, panelY, 400, panelHeight, { 0.0f, 0.15f, 0.3f, 0.8f });

			// �E�E�E�E�E�E�E�g�E�E�E�E�E�E�E�̓W�E�E�E�E�E�E�E�J�E�E�E�E�E�E�E�A�E�E�E�E�E�E�E�j�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�[�E�E�E�E�E�E�E�V�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�I�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�Ă��E�E�E�E�E�E�E�璁E�E�E�E��E�E�E�g�E�E�E�E�E�E�E�̍��E�E�E�E�E�E�E�ژg�E�E�E�E�E�E�E��E�E�E�E�E�E�E�`�E�E�E�E�E�E�E�悷�E�E�E�E�E�E�E��E�E�E�E�E�E�E�
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

		// �E�E�E�E�E�E�E�o�E�E�E�E�E�E�E�^�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�l�E�E�E�E�E�E�E�p�E�E�E�E�E�E�E�`UI�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�ꊁE�E�E�E��E�E�E�`�E�E�E�E�E�E�E��E�E�E�E�E�E�E�
		SimpleUI::DrawAll();

		// �E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�DirectWrite�E�E�E�E�E�E�E�ɂ��E�E�E�E�E�E�E�t�E�E�E�E�E�E�E�H�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�g�E�E�E�E�E�E�E�̕`�E�E�E�E�E�E�E�揈��E�E�E�E�E�E�E�
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

			// �E�E�E�E�E�E�E�g�E�E�E�E�E�E�E�̃A�E�E�E�E�E�E�E�j�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�[�E�E�E�E�E�E�E�V�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�I�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�Ɋe�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�ڂ̃e�E�E�E�E�E�E�E�L�E�E�E�E�E�E�E�X�E�E�E�E�E�E�E�g�E�E�E�E�E�E�E��E�E�E�E�E�E�E�`�E�E�E�E�E�E�E��E�E�E�E�E�E�E�
			if (m_windowScaleY >= 1.0f)
			{
				std::vector<ConfigItem>& currentItems = isP1 ? m_p1Items : m_p2Items;
				InputDeviceType currentDevice = isP1 ? g_inputDeviceP1 : g_inputDeviceP2;

				for (int i = 0; i < currentItems.size(); ++i)
				{
					DirectX::XMFLOAT4 textColor = (i == m_configSelectedIndex) ? XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f) : XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);

					// �E�E�E�E�E�E�E�A�E�E�E�E�E�E�E�N�E�E�E�E�E�E�E�V�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�`�E�E�E�E�E�E�E��E�E�E�E�E�E�E� (�E�E�E�E�E�E�E��E�E�E�E�E�E�E�: Light Punch)
					SimpleFont::Draw(currentItems[i].label, 80, 175 + i * 45, 20.0f, textColor);

					// �E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�݂̊��E�E�E�E�E�E�E�蓖�ăL�E�E�E�E�E�E�E�[�E�E�E�E�E�E�E�A�E�E�E�E�E�E�E�܂��E�E�E�E�E�E�E�͑ҋ@�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�́E�E�E�E��E�E�E��E�E�E�E�E�E�E�b�E�E�E�E�E�E�E�Z�E�E�E�E�E�E�E�[�E�E�E�E�E�E�E�W�E�E�E�E�E�E�E��E�E�E�E�E�E�E�`�E�E�E�E�E�E�E��E�E�E�E�E�E�E�
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
