#pragma once
#include "SceneBase.hpp"
#include <vector>
#include <DirectXMath.h>
#include <d3d11.h>
#include "Player.h"
#include "PlayerRenderer.h"
#include "HitEffectPool.h"

class HitEffect;
class SkyDome;

// �E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�j�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�[�E�E�E�E�E�E�E�̊K�E�E�E�E�E�E�E�w�E�E�E�E�E�E�E��E�E�E�E�E�E�E�Ԃ�\�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�񋓌^
enum class MenuState
{
	TopMenu,     // �E�E�E�E�E�E�E�g�E�E�E�E�E�E�E�b�E�E�E�E�E�E�E�v�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�j�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�[
	ConfigP1,    // 1P�E�E�E�E�E�E�E�̃L�E�E�E�E�E�E�E�[�E�E�E�E�E�E�E�R�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�t�E�E�E�E�E�E�E�B�E�E�E�E�E�E�E�O�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�
	ConfigP2,    // 2P�E�E�E�E�E�E�E�̃L�E�E�E�E�E�E�E�[�E�E�E�E�E�E�E�R�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�t�E�E�E�E�E�E�E�B�E�E�E�E�E�E�E�O�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�
	TrainingMode // �E�E�E�E�E�E�E�g�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�[�E�E�E�E�E�E�E�j�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�O�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�[�E�E�E�E�E�E�E�h�E�E�E�E�E�E�E�iUI�E�E�E�E�E�E�E��E�E�E�E�E�E�E�\�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�Ŏ��E�E�E�E�E�E�E�R�E�E�E�E�E�E�E�ɓ��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�j
};

// �E�E�E�E�E�E�E�g�E�E�E�E�E�E�E�b�E�E�E�E�E�E�E�v�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�j�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�[�E�E�E�E�E�E�E�̍��E�E�E�E�E�E�E�ڏ��E�E�E�E�E�E�E� (DirectWrite�E�E�E�E�E�E�E�ɍ��E�E�E�E�E�E�E��E�E�E��E�E�E��E�E�E��E�E�E�E�E�E�E�āE�E�E�E��E�E�E��E�E�E�E�E�E�E�C�E�E�E�E�E�E�E�h�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�ɂ��E�E�E�E�E�E�E��E�E�E�E�E�E�E�)
struct TopMenuItem
{
	const wchar_t* label;
};

// �E�E�E�E�E�E�E�R�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�t�E�E�E�E�E�E�E�B�E�E�E�E�E�E�E�O�E�E�E�E�E�E�E��E�E�E�E�E�E�E�ʂ̍��E�E�E�E�E�E�E�ڏ��E�E�E�E�E�E�E� (DirectWrite�E�E�E�E�E�E�E�ɍ��E�E�E�E�E�E�E��E�E�E��E�E�E��E�E�E��E�E�E�E�E�E�E�āE�E�E�E��E�E�E��E�E�E�E�E�E�E�C�E�E�E�E�E�E�E�h�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�ɂ��E�E�E�E�E�E�E��E�E�E�E�E�E�E�)
struct ConfigItem
{
	const wchar_t* label; // �E�E�E�E�E�E�E��E�E�E�E�E�E�E�ʂɕ\�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�e�E�E�E�E�E�E�E�L�E�E�E�E�E�E�E�X�E�E�E�E�E�E�E�g
	int* keyPtr;          // �E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�蓖�Ă�ύX�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�L�E�E�E�E�E�E�E�[�E�E�E�E�E�E�E�ϐ��E�E�E�E�E�E�E�̃|�E�E�E�E�E�E�E�C�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�^
	bool isDeviceSelect;  // �E�E�E�E�E�E�E�f�E�E�E�E�E�E�E�o�E�E�E�E�E�E�E�C�E�E�E�E�E�E�E�X�E�E�E�E�E�E�E�؂�ւ��E�E�E�E�E�E�E�p�E�E�E�E�E�E�E�̍��E�E�E�E�E�E�E�ڂ��E�E�E�E�E�E�E�ǂ��E�E�E�E�E�E�E��E�E�E�E�E�E�E�
};

class SceneKeyConfig : public SceneBase
{
public:
	void Init();
	void Uninit();
	void Update(float tick);
	void Draw();

	static bool s_isConfigSet;

private:
	// �E�E�E�E�E�E�E�w�E�E�E�E�E�E�E�肵�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�s�E�E�E�E�E�E�E�N�E�E�E�E�E�E�E�Z�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�W�E�E�E�E�E�E�E�ƃT�E�E�E�E�E�E�E�C�E�E�E�E�E�E�E�Y�E�E�E�E�E�E�E�ŒP�E�E�E�E�E�E�E�F�E�E�E�E�E�E�E�̎l�E�E�E�E�E�E�E�p�E�E�E�E�E�E�E�`�E�E�E�E�E�E�E��E�E�E�E�E�E�E�`�E�E�E�E�E�E�E�悷�E�E�E�E�E�E�E��E�E�E�E�E�E�E�
	void DrawRectPixel(float px, float py, float pw, float ph, DirectX::XMFLOAT4 color);

	// �E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�z�E�E�E�E�E�E�E�L�E�E�E�E�E�E�E�[�E�E�E�E�E�E�E�R�E�E�E�E�E�E�E�[�E�E�E�E�E�E�E�h�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�\�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�p�E�E�E�E�E�E�E�̕��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�擾�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�
	const wchar_t* GetKeyName(int vk);

	// �E�E�E�E�E�E�E�R�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�g�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�[�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�[�E�E�E�E�E�E�E�̃{�E�E�E�E�E�E�E�^�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�\�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�p�E�E�E�E�E�E�E�̕��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�擾�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�
	const wchar_t* GetPadButtonName(int button);

	// �E�E�E�E�E�E�E�I�E�E�E�E�E�E�E��E�E�E�Ẽf�E�E�E�E�E�E�E�o�E�E�E�E�E�E�E�C�E�E�E�E�E�E�E�X�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�擾�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�
	const wchar_t* GetDeviceName(InputDeviceType type);

	// �E�E�E�E�E�E�E�ݒ�|�E�E�E�E�E�E�E�C�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�^�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�݂̃f�E�E�E�E�E�E�E�o�E�E�E�E�E�E�E�C�E�E�E�E�E�E�E�X�E�E�E�E�E�E�E�ɍ��E�E�E�E�E�E�E��E�E�E��E�E�E��E�E�E��E�E�E�E�E�E�E�čX�E�E�E�E�E�E�E�V�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�
	void RefreshConfigPointers();

	MenuState m_menuState;

	// �E�E�E�E�E�E�E�g�E�E�E�E�E�E�E�b�E�E�E�E�E�E�E�v�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�j�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�[�E�E�E�E�E�E�E�p�E�E�E�E�E�E�E�ϐ�
	int m_topSelectedIndex;
	std::vector<TopMenuItem> m_topItems;

	// �E�E�E�E�E�E�E�R�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�t�E�E�E�E�E�E�E�B�E�E�E�E�E�E�E�O�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�j�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�[�E�E�E�E�E�E�E�p�E�E�E�E�E�E�E�ϐ�
	int m_configSelectedIndex;
	std::vector<ConfigItem> m_p1Items;
	std::vector<ConfigItem> m_p2Items;

	// �E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�j�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�[�E�E�E�E�E�E�E�W�E�E�E�E�E�E�E�J�E�E�E�E�E�E�E�A�E�E�E�E�E�E�E�j�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�[�E�E�E�E�E�E�E�V�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�̏c�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�X�E�E�E�E�E�E�E�P�E�E�E�E�E�E�E�[�E�E�E�E�E�E�E��E�E�E�E�E�E�E� (0.0f �E�E�E�E�E�E�E�` 1.0f)
	float m_windowScaleY;

	// �E�E�E�E�E�E�E�L�E�E�E�E�E�E�E�[�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�͑ҋ@�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�̃L�E�E�E�E�E�E�E�[�E�E�E�E�E�E�E�ϐ��E�E�E�E�E�E�E�ւ̃|�E�E�E�E�E�E�E�C�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�^�E�E�E�E�E�E�E�inullptr�E�E�E�E�E�E�E�Ȃ�ҋ@�E�E�E�E�E�E�E��E�E�E�E�E�E�E��E�E�E�E�E�E�E�Ă��E�E�E�E�E�E�E�Ȃ��E�E�E�E�E�E�E�j
	int* m_waitBindKeyPtr = nullptr;

	HitEffectPool m_hitEffects;

	// �v���C���[�`��i�A�E�g���C���E�J�����O���Q�[���Ƌ��ʉ��j
	PlayerRenderer m_playerRenderer;

	// �E�E�E�w�E�E�E�i�E�E�E�X�E�E�E�J�E�E�E�C�E�E�E�h�E�E�E�[�E�E�E��E�E�E��E�E�E�i�E�E�E�Q�E�E�E�[�E�E�E��E�E�E��E�E�E�V�E�E�E�[�E�E�E��E�E�E��E�E�E�Ɠ��E�E�E��E�E�E��E�E�E��E�E�E��E�E�E��E�E�E��E�E�E�ڂɂ��E�E�E�邽�E�E�E�߁j
	SkyDome* m_skyDome = nullptr;
};