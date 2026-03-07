#include "SceneTitle.h"
#include "DebugLog.h"
#include "SimpleUI.h"
#include "CameraBase.h"
#include "LightBase.h"
#include "Shader.h"
#include <math.h>

// アクションのアニメーションリスト
static const char* ACTION_ANIMS[] = {
	"LightPunch", "MediumPunch", "HeavyKick", "Jump"
};
static const int ACTION_COUNT = sizeof(ACTION_ANIMS) / sizeof(ACTION_ANIMS[0]);

void SceneTitle::Init()
{
	DebugLog::log(DebugLog::INFO_LOG, "--- Title Scene Init ---");

	// シェーダーの読み込み
	Shader* vsSkin = GetObj<Shader>("VS_SkinMeshAnimation");
	if (!vsSkin)
	{
		vsSkin = CreateObj<VertexShader>("VS_SkinMeshAnimation");
		vsSkin->Load("Assets/Shader/VS_SkinMeshAnimation.cso");
	}

	Shader* psColor = GetObj<Shader>("PS_TexColor");
	if (!psColor)
	{
		psColor = CreateObj<PixelShader>("PS_TexColor");
		psColor->Load("Assets/Shader/PS_TexColor.cso");
	}

	Shader* vsObj = GetObj<Shader>("VS_Object");
	if (!vsObj)
	{
		vsObj = CreateObj<VertexShader>("VS_Object");
		vsObj->Load("Assets/Shader/VS_Object.cso");
	}

	// スカイドームの生成
	CreateObj<Model>("SkyModel");
	Model* skyModel = GetObj<Model>("SkyModel");
	skyModel->Load("Assets/Model/SkyDome/SkyDome.fbx", 1.0f, true, true);
	skyModel->SetTexture("Assets/Model/SkyDome/SkyDome.png");
	skyModel->SetPixelShader((PixelShader*)psColor);
	m_skyDome = new SkyDome();
	m_skyDome->Init(skyModel);

	// モデルの生成と読み込み
	m_pTitleModel = new Model();
	if (m_pTitleModel->Load("Assets/Model/knight/Idle.fbx", 0.017f, true, false))
	{
		m_pTitleModel->LoadAnimation("Assets/Model/knight/Idle.fbx", "Idle", true);
		m_pTitleModel->LoadAnimation("Assets/Model/knight/LightPunch.fbx", "LightPunch", true);
		m_pTitleModel->LoadAnimation("Assets/Model/knight/MediumPunch.fbx", "MediumPunch", true);
		m_pTitleModel->LoadAnimation("Assets/Model/knight/HeavyKick.fbx", "HeavyKick", true);
		m_pTitleModel->LoadAnimation("Assets/Model/knight/Jump.fbx", "Jump", true);
	}
	m_currentAnimName = "Idle";
	m_currentFrame = 0.0f;
	m_isLoop = true;
	m_actionTimer = 2.0f;

	// Image2Dのインスタンス生成
	m_pImage = new Image2D();
	m_PressEnter = new Image2D();
	m_pParticleImg = new Image2D();

	// 画像読み込み
	m_pImage->Load("Assets/Texture/AZEFIGHTER.png", 650.0f, 100.0f, 676, 369.0f);
	m_pImage->SetColor(1.0f, 1.0f, 1.0f, 1.0f);
	m_PressEnter->Load("Assets/Texture/PressEnter.png", 640, 550, 500, 200.0f);
	m_pParticleImg->Load("Assets/Texture/particle.png", 0, 0, 0, 0);

	m_blinkTimer = 0.0f;
	m_spawnTimer = 0.0f;
	m_particles.clear();
	m_cameraAngle = 0.0f;

	// 1. 半透明ブレンドステート
	D3D11_BLEND_DESC blendDesc = {};

	//  AlphaToCoverageを有効にする

	blendDesc.AlphaToCoverageEnable = TRUE;

	blendDesc.IndependentBlendEnable = FALSE;
	blendDesc.RenderTarget[0].BlendEnable = TRUE;
	blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
	blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
	GetDevice()->CreateBlendState(&blendDesc, &m_pBlendState);

	// 2. UI用の深度ステート (深度書き込みON)
	D3D11_DEPTH_STENCIL_DESC depthDescUI = {};

	// 深度書き込みを有効にする
	depthDescUI.DepthEnable = TRUE;
	depthDescUI.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	depthDescUI.DepthFunc = D3D11_COMPARISON_ALWAYS; // 常に手前に描画
	depthDescUI.StencilEnable = FALSE;
	GetDevice()->CreateDepthStencilState(&depthDescUI, &m_pDepthStateUI);

	// 3. 3D用の深度ステート
	D3D11_DEPTH_STENCIL_DESC depthDesc3D = {};
	depthDesc3D.DepthEnable = TRUE;
	depthDesc3D.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	depthDesc3D.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
	depthDesc3D.StencilEnable = FALSE;
	GetDevice()->CreateDepthStencilState(&depthDesc3D, &m_pDepthState3D);

	// 4. スカイドーム用：カリングなしラスタライザーステート
	D3D11_RASTERIZER_DESC rsDesc = {};
	rsDesc.FillMode = D3D11_FILL_SOLID;
	rsDesc.CullMode = D3D11_CULL_NONE; // ここが重要
	rsDesc.FrontCounterClockwise = FALSE;
	rsDesc.DepthBias = 0;
	rsDesc.SlopeScaledDepthBias = 0.0f;
	rsDesc.DepthBiasClamp = 0.0f;
	rsDesc.DepthClipEnable = FALSE;
	rsDesc.ScissorEnable = FALSE;
	rsDesc.MultisampleEnable = FALSE;
	rsDesc.AntialiasedLineEnable = FALSE;
	GetDevice()->CreateRasterizerState(&rsDesc, &m_pCullNone);
}

void SceneTitle::Uninit()
{
	DebugLog::log(DebugLog::INFO_LOG, "--- Title Scene Uninit ---");

	float blendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	GetContext()->OMSetBlendState(nullptr, blendFactor, 0xffffffff);
	GetContext()->OMSetDepthStencilState(nullptr, 0);

	if (m_pImage)
	{
		delete m_pImage;
		delete m_PressEnter;
		delete m_pParticleImg;
		m_pImage = nullptr;
		m_PressEnter = nullptr;
		m_pParticleImg = nullptr;
	}

	if (m_pTitleModel) { delete m_pTitleModel; m_pTitleModel = nullptr; }
	if (m_skyDome) { delete m_skyDome; m_skyDome = nullptr; }

	if (m_pBlendState) { m_pBlendState->Release(); m_pBlendState = nullptr; }
	if (m_pDepthStateUI) { m_pDepthStateUI->Release(); m_pDepthStateUI = nullptr; }
	if (m_pDepthState3D) { m_pDepthState3D->Release(); m_pDepthState3D = nullptr; }
	if (m_pCullNone) { m_pCullNone->Release(); m_pCullNone = nullptr; }
}

void SceneTitle::Update(float tick)
{
	// カメラ制御
	CameraBase* pCamera = GetObj<CameraBase>("Camera");
	if (pCamera)
	{
		m_cameraAngle += tick * 0.5f;
		float radius = 6.5f;
		float camX = sinf(m_cameraAngle) * radius;
		float camZ = cosf(m_cameraAngle) * radius;
		float camY = 3.5f;
		pCamera->SetPos({ camX, camY, camZ });
		pCamera->SetLook({ 0.0f, 1.2f, 0.0f });
		if (m_skyDome) m_skyDome->Update(pCamera->GetPos());
	}

	// モデルのアニメーション制御
	if (m_pTitleModel)
	{
		m_currentFrame += tick * 60.0f;
		int totalFrame = m_pTitleModel->GetAnimationTotalFrame(m_currentAnimName.c_str());
		if (totalFrame <= 0) totalFrame = 1;

		if (m_currentFrame >= (float)totalFrame)
		{
			if (m_isLoop) {
				m_currentFrame = 0.0f;
			}
			else {
				m_currentAnimName = "Idle";
				m_currentFrame = 0.0f;
				m_isLoop = true;
				m_actionTimer = 2.0f + (float)(rand() % 20) * 0.1f;
			}
		}
		m_pTitleModel->UpdateAnimation(m_currentAnimName.c_str(), (int)m_currentFrame);

		if (m_currentAnimName == "Idle")
		{
			m_actionTimer -= tick;
			if (m_actionTimer <= 0.0f)
			{
				int idx = rand() % ACTION_COUNT;
				m_currentAnimName = ACTION_ANIMS[idx];
				m_currentFrame = 0.0f;
				m_isLoop = false;
			}
		}
	}

	// パーティクルの更新処理
	m_spawnTimer += tick;
	if (m_spawnTimer > 0.1f)
	{
		m_spawnTimer = 0.0f;
		Particle p;
		p.x = (float)(rand() % 1280);
		p.y = 750.0f;
		p.size = 20.0f + (rand() % 30);
		p.speed = 50.0f + (rand() % 100);
		p.maxLife = 3.0f + (rand() % 20) / 10.0f;
		p.life = p.maxLife;
		p.drift = (float)(rand() % 100) * 0.02f;
		m_particles.push_back(p);
	}
	for (auto it = m_particles.begin(); it != m_particles.end(); )
	{
		it->y -= it->speed * tick;
		it->x += sinf(it->y * 0.01f) * it->drift;
		it->life -= tick;
		if (it->life <= 0.0f) it = m_particles.erase(it);
		else ++it;
	}

	// エンターキー画像の点滅処理
	m_blinkTimer += tick;
	float wave = (sinf(m_blinkTimer * 3.0f) + 1.0f) * 0.5f; // 0.0 ～ 1.0 の波


	float alpha = 0.7f + 0.3f * wave;

	if (m_PressEnter) m_PressEnter->SetColor(1.0f, 1.0f, 1.0f, alpha);
}
void SceneTitle::Draw()
{
	// ------------------------------------------------
	// 1. 3Dオブジェクトの描画 (深度有効)
	// ------------------------------------------------
	float blendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	GetContext()->OMSetBlendState(nullptr, blendFactor, 0xffffffff);

	// 3D描画用深度設定
	if (m_pDepthState3D)
	{
		GetContext()->OMSetDepthStencilState(m_pDepthState3D, 0);
	}
	else
	{
		GetContext()->OMSetDepthStencilState(nullptr, 0);
	}

	CameraBase* pCamera = GetObj<CameraBase>("Camera");
	LightBase* pLight = GetObj<LightBase>("Light");

	Shader* vsSkin = GetObj<Shader>("VS_SkinMeshAnimation");
	Shader* psColor = GetObj<Shader>("PS_TexColor");
	Shader* vsObj = GetObj<Shader>("VS_Object");

	DirectX::XMFLOAT4X4 mat[3];
	DirectX::XMStoreFloat4x4(&mat[0], DirectX::XMMatrixIdentity());
	mat[1] = pCamera->GetView();
	mat[2] = pCamera->GetProj();

	DirectX::XMFLOAT3 lightDir = pLight->GetDirection();
	DirectX::XMFLOAT4 lightParam[] = {
		pLight->GetDiffuse(),
		pLight->GetAmbient(),
		{lightDir.x, lightDir.y, lightDir.z, 0.0f}
	};
	DirectX::XMFLOAT3 cPos = pCamera->GetPos();
	DirectX::XMFLOAT4 camParam[] = { {cPos.x, cPos.y, cPos.z, 0.0f} };

	// スカイドームの描画
	if (m_skyDome)
	{
		// スカイドーム用にカリングなしを設定
		if (m_pCullNone) GetContext()->RSSetState(m_pCullNone);

		m_skyDome->Draw(pCamera->GetView(), pCamera->GetProj(), vsObj);

		// プレイヤーモデル等の描画前にデフォルト(裏面カリング)に戻す
		GetContext()->RSSetState(nullptr);
	}

	if (m_pTitleModel)
	{
		using namespace DirectX;
		XMMATRIX mScale = XMMatrixScaling(1.0f, 1.0f, 1.0f);
		XMMATRIX mBaseScale = m_pTitleModel->GetScaleBaseMatrix();
		XMMATRIX mRot = XMMatrixRotationY(XM_PI);
		XMMATRIX mTrans = XMMatrixTranslation(0.0f, 0.0f, 0.0f);
		XMMATRIX mWorld = mBaseScale * mScale * mRot * mTrans;
		XMStoreFloat4x4(&mat[0], XMMatrixTranspose(mWorld));

		vsSkin->WriteBuffer(0, mat);
		psColor->WriteBuffer(0, lightParam);
		psColor->WriteBuffer(1, camParam);

		m_pTitleModel->SetVertexShader((VertexShader*)vsSkin);
		m_pTitleModel->SetPixelShader((PixelShader*)psColor);
		m_pTitleModel->Draw();
	}

	// ------------------------------------------------
	// 2. UIの描画 (AlphaToCoverage有効 & 深度有効)
	// ------------------------------------------------

	if (m_pDepthStateUI)
	{
		GetContext()->OMSetDepthStencilState(m_pDepthStateUI, 0);
	}

	if (m_pBlendState)
	{
		GetContext()->OMSetBlendState(m_pBlendState, blendFactor, 0xffffffff);
	}

	SimpleUI::Clear();
	DirectX::XMFLOAT4X4 identity;
	DirectX::XMStoreFloat4x4(&identity, DirectX::XMMatrixIdentity());
	SimpleUI::SetMatrix(identity, identity, identity);

	// パーティクル
	if (m_pParticleImg)
	{
		for (const auto& p : m_particles)
		{
			float pAlpha = p.life / p.maxLife;
			m_pParticleImg->Draw(p.x, p.y, p.size, p.size, pAlpha * 0.7f);
		}
	}

	// ロゴ
	if (m_pImage) m_pImage->Draw();

	// Enter
	if (m_PressEnter) m_PressEnter->Draw();

	// 描画実行
	SimpleUI::DrawAll();

	// 後始末
	GetContext()->OMSetBlendState(nullptr, blendFactor, 0xffffffff);
	GetContext()->OMSetDepthStencilState(nullptr, 0);
}