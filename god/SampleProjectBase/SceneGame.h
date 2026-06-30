#pragma once
#include "SceneBase.hpp"
#include "SkyDome.h"
#include <DirectXMath.h> 
#include <d3d11.h>       
#include "HitEffectPool.h"
#include "BattleUIManager.h" // UI�Ǘ��N���X
#include "BattleCollision.h" // �Փ˔���Ǘ��N���X
#include "PlayerRenderer.h"

/**
 * @brief ���C���̃Q�[���V�[��
 * �i���Q�[���̖{�ԓ���p�V�[��
 */
class SceneGame : public SceneBase
{
public:
	void Init();   // ������
	void Uninit(); // �I��
	void Update(float tick); // �X�V
	void Draw();   // �`��

	// SceneRoot����Q�Ƃ��邽�߂̐ÓI�t���O
	static bool s_isGameSet;

private:
	// ���̃��E���h�̂��߂Ɉʒu��HP�����Z�b�g����֐�
	void ResetRound();

	// �J�����V�F�C�N�iTrauma�����j
	void AddCameraTrauma(float amount, float dirX); // �q�b�g���ɗh���������
	void UpdateCameraShake(float tick);             // ���t���[���̌����ƃI�t�Z�b�g�v�Z

private:
	// UI�}�l�[�W��
	BattleUIManager* m_uiManager;

	SkyDome* m_skyDome;

	// �v���C���[�`��i�A�E�g���C���E�J�����O���Q�[���Ƌ��ʉ��j
	PlayerRenderer m_playerRenderer;

	//�G�t�F�N�g�Ǘ��p���X�g
	HitEffectPool m_hitEffects;

	RoundPhase m_currentPhase = RoundPhase::READY;
	float m_phaseTimer = 0.0f;

	// �`��ݒ�
	ID3D11DepthStencilState* m_pDepthState = nullptr;         // �X�J�C�h�[���p�iZ�������݂���j
	ID3D11DepthStencilState* m_pDepthStateNoWrite = nullptr; // �e�E���p�iZ�������݂Ȃ��j
	ID3D11BlendState* m_pBlendState = nullptr;                // �������p

	// �A�E�g���C���E�J�����O�ݒ�p���X�^���C�U�[�X�e�[�g
	ID3D11RasterizerState* m_pCullNone = nullptr;       // �J�����O�Ȃ��i�X�J�C�h�[���p�j

	// �V���h�E�}�b�v�E���p�ݒ�
	ID3D11Texture2D* m_shadowMapTex = nullptr;
	ID3D11RenderTargetView* m_shadowRTV = nullptr;
	ID3D11ShaderResourceView* m_shadowSRV = nullptr;
	ID3D11Texture2D* m_shadowDepthTex = nullptr;
	ID3D11DepthStencilView* m_shadowDSV = nullptr;
	ID3D11SamplerState* m_pSamplerState = nullptr;      // �e�p�T���v���[
	D3D11_VIEWPORT m_shadowViewport = {};
	ID3D11BlendState* m_pMultiplyBlend = nullptr;

	ID3D11Buffer* m_quadVB = nullptr; // ���̃|���S���p

	struct SpriteParam {
		DirectX::XMFLOAT4 offset_size;
		DirectX::XMFLOAT4 uvPos_scale;
		DirectX::XMFLOAT4 color;
	};

	struct ShadowVertex {
		DirectX::XMFLOAT3 pos;
		DirectX::XMFLOAT2 uv;
	};

	// �q�b�g�X�g�b�v���o�p�^�C�}�[
	float m_hitStopTimer = 0.0f;

	// �q�b�g�V�F�C�N�i�U���j����p
	float m_shakeTimerP1 = 0.0f; // 1P�̐U���c�莞��
	float m_shakeTimerP2 = 0.0f; // 2P�̐U���c�莞��
	DirectX::XMFLOAT3 m_shakeOffsetP1 = { 0.0f, 0.0f, 0.0f }; // 1P�̕`�悸�炵��
	DirectX::XMFLOAT3 m_shakeOffsetP2 = { 0.0f, 0.0f, 0.0f }; // 2P�̕`�悸�炵��

	// �J�����V�F�C�N�iTrauma�����j����p
	float m_cameraTrauma = 0.0f;                                 // �h��̒~�ϗʁi0.0����1.0�j
	float m_cameraShakeKickDir = 0.0f;                           // �����̉������i-1/+1�j
	DirectX::XMFLOAT3 m_cameraShakeOffset = { 0.0f, 0.0f, 0.0f }; // ���t���[���̗h��I�t�Z�b�g

	// ���E���h�Ǘ��p
	int m_winCountP1 = 0;      // 1P�̏������E���h��
	int m_winCountP2 = 0;      // 2P�̏������E���h��
	bool m_isRoundOver = false; // ���E���h���I��������ǂ����̃t���O
	float m_roundEndTimer = 0.0f; // ���E���h�I����̑ҋ@�^�C�}�[ (2�b)

	// KO���o�p
	bool m_isKOStage = false; // ����KO���o�t�F�[�Y���ǂ���

	// �X���[���[�V�������o�p
	bool m_isSlowMotion = false;        // �X���[���[�V��������
	float m_slowMotionTimer = 0.0f;     // �X���[���[�V�����c�莞��
	float m_slowMotionDuration = 0.0f;  // �X���[���[�V�����̑S�̎��ԁi��Ԍv�Z�p�j

	// KO���̃J�����Y�[���p
	DirectX::XMFLOAT3 m_cameraZoomStartPos;  // �Y�[���J�n�n�_
	DirectX::XMFLOAT3 m_cameraZoomStartLook; // �Y�[���J�n���̒����_
	DirectX::XMFLOAT3 m_cameraZoomTargetPos; // �Y�[���ڕW�n�_
	DirectX::XMFLOAT3 m_cameraZoomTargetLook;// �Y�[���ڕW�����_
};