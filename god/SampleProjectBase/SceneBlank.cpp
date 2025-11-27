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
#include "UISprite.h"
#include "Image2D.h"
#include <fstream> 

using namespace DirectX;
using namespace DirectX::SimpleMath;

Texture* g_uiTex = nullptr;
const char* SETTINGS_FILE = "player_settings.ini";

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

	// --- 2D画像読み込み ---
	// 元の座標とサイズに戻しました
	m_barMaxWidth = 500.0f;

	// 1P (左側)
	m_hpBar = new Image2D();
	m_hpBarPos = { 330.0f, 80.0f };
	m_hpBar->Load("Assets/Texture/hp.png", m_hpBarPos.x, m_hpBarPos.y, m_barMaxWidth, 80.0f);

	// 2P (右側)
	m_enemyhpBar = new Image2D();
	m_enemyHpBarPos = { 950.0f, 80.0f };
	m_enemyhpBar->Load("Assets/Texture/hp.png", m_enemyHpBarPos.x, m_enemyHpBarPos.y, m_barMaxWidth, 80.0f);


	// プレイヤー1生成
	CreateObj<Player>("Player");
	Player* player = GetObj<Player>("Player");
	player->SetInputType(PlayerInputType::PLAYER_1);

	// --- 設定ファイルの読み込み ---
	float moveSpeed = 2.0f;
	DirectX::XMFLOAT3 scale = { 1.0f, 1.0f, 1.0f };
	DirectX::XMFLOAT2 boxExtents = { 0.5f, 1.0f };
	DirectX::XMFLOAT2 boxOffset = { 0.0f, 1.0f };
	AttackParams params = player->GetLightPunchParams();

	std::ifstream ifs(SETTINGS_FILE);
	if (ifs.is_open())
	{
		ifs >> moveSpeed;
		ifs >> scale.x >> scale.y >> scale.z;
		ifs >> boxExtents.x >> boxExtents.y;
		ifs >> boxOffset.x >> boxOffset.y;

		if (!ifs.eof()) ifs >> params.totalDuration;
		if (!ifs.eof()) ifs >> params.hitboxStart;
		if (!ifs.eof()) ifs >> params.hitboxEnd;
		if (!ifs.eof()) ifs >> params.hitboxOffset.x >> params.hitboxOffset.y;
		if (!ifs.eof()) ifs >> params.hitboxExtents.x >> params.hitboxExtents.y;
		if (!ifs.eof()) ifs >> params.damage;
		if (!ifs.eof()) ifs >> params.hitFrame;
		if (!ifs.eof()) ifs >> params.blockFrame;

		ifs.close();
	}

	player->SetMoveSpeed(moveSpeed);
	player->SetScale(scale);
	player->SetBoundingBoxExtents(boxExtents);
	player->SetBoundingBoxOffset(boxOffset);
	player->GetLightPunchParams() = params;


	if (!player->Load("Assets/Model/knight/Idle.fbx", 0.02f, true, false))
	{
		MessageBox(NULL, "プレイヤーモデルの読み込みに失敗しました。", "Model Load Error", MB_OK);
	}
	player->GetModel()->LoadAnimation("Assets/Model/knight/Walking.fbx", "Walk", true);
	player->GetModel()->LoadAnimation("Assets/Model/knight/WalkBack.fbx", "WalkBack", true);
	player->GetModel()->LoadAnimation("Assets/Model/knight/LightPunch.fbx", "LightPunch", true);
	player->GetModel()->LoadAnimation("Assets/Model/knight/Jump.fbx", "Jump", true);

	player->SetPosition({ -2.0f, 0.0f, 0.0f });
	player->SetRotation({ 0.0f, DirectX::XM_PI / -2.0f, 0.0f });


	// プレイヤー2 (相手) の生成
	CreateObj<Player>("Player2");
	Player* player2 = GetObj<Player>("Player2");
	player2->SetInputType(PlayerInputType::PLAYER_2);

	player2->SetMoveSpeed(moveSpeed);
	player2->SetScale(scale);
	player2->SetBoundingBoxExtents(boxExtents);
	player2->SetBoundingBoxOffset(boxOffset);
	player2->GetLightPunchParams() = params;

	if (!player2->Load("Assets/Model/knight/Idle.fbx", 0.02f, true, false))
	{
		MessageBox(NULL, "プレイヤー2モデルの読み込みに失敗しました。", "Model Load Error", MB_OK);
	}
	player2->GetModel()->LoadAnimation("Assets/Model/knight/Walking.fbx", "Walk", true);
	player2->GetModel()->LoadAnimation("Assets/Model/knight/WalkBack.fbx", "WalkBack", true);
	player2->GetModel()->LoadAnimation("Assets/Model/knight/LightPunch.fbx", "LightPunch", true);
	player2->GetModel()->LoadAnimation("Assets/Model/knight/Jump.fbx", "Jump", true);

	player2->SetPosition({ 2.0f, 0.0f, 0.0f });
	player2->SetRotation({ 0.0f, DirectX::XM_PI / 2.0f, 0.0f });

	CameraBase* pCamera = GetObj<CameraBase>("Camera");
	if (pCamera)
	{
		pCamera->SetPos({ 0.0f, 1.2f, -5.0f });
		pCamera->SetLook({ 0.0f, 1.0f, 4.0f });
	}

	g_uiTex = new Texture();
}

void SceneBlank::Uninit()
{
	if (m_hpBar) delete m_hpBar;
	if (m_enemyhpBar) delete m_enemyhpBar;

	if (g_uiTex) {
		delete g_uiTex;
		g_uiTex = nullptr;
	}
}

void SceneBlank::Update(float tick)
{
	Player* player = GetObj<Player>("Player");
	Player* player2 = GetObj<Player>("Player2");

	if (player) {
		player->Update(tick);
	}
	if (player2) {
		player2->Update(tick);
	}


	// --- 当たり判定チェック ---
	if (player && player2)
	{
		// 1. 押し出し判定
		DirectX::BoundingBox box1 = player->GetBoundingBox();
		DirectX::BoundingBox box2 = player2->GetBoundingBox();
		bool isColliding = box1.Intersects(box2);

		player->SetIsColliding(isColliding);
		player2->SetIsColliding(isColliding);

		if (isColliding)
		{
			DirectX::XMFLOAT3 pos1 = player->GetPosition();
			DirectX::XMFLOAT3 pos2 = player2->GetPosition();
			float deltaX = box1.Center.x - box2.Center.x;
			float sumExtentsX = box1.Extents.x + box2.Extents.x;
			float overlapX = sumExtentsX - abs(deltaX);
			float deltaY = box1.Center.y - box2.Center.y;
			float sumExtentsY = box1.Extents.y + box2.Extents.y;
			float overlapY = sumExtentsY - abs(deltaY);
			DirectX::XMFLOAT3 pushVector = { 0.0f, 0.0f, 0.0f };

			if (overlapX < overlapY)
			{
				float pushAmount = overlapX / 2.0f;
				float direction = (deltaX > 0.0f) ? 1.0f : -1.0f;
				pushVector.x = direction * pushAmount;
			}
			else
			{
				float pushAmount = overlapY / 2.0f;
				float direction = (deltaY > 0.0f) ? 1.0f : -1.0f;
				pushVector.y = direction * pushAmount;
			}
			player->SetPosition({
				pos1.x + pushVector.x,
				pos1.y + pushVector.y,
				pos1.z
				});
			player2->SetPosition({
				pos2.x - pushVector.x,
				pos2.y - pushVector.y,
				pos2.z
				});
		}

		// --- 2. 攻撃判定 (Hitbox) のチェック & HP更新 ---

		// 1P -> 2P (ヒット)
		if (player->IsAttacking() && !player->HasHit() && player->GetActiveHitbox().Intersects(box2))
		{
			AttackParams& params = player->GetLightPunchParams();
			player2->ReceiveDamage(params.damage);
			player->OnHit();

			// --- 2P HPバーの更新 (右側) ---
			float ratio = player2->GetHpRatio();
			float currentWidth = m_barMaxWidth * ratio;

			// 減った幅を計算
			float reduceWidth = m_barMaxWidth - currentWidth;

			m_enemyhpBar->SetSize(currentWidth, 80.0f);

			// 減った幅の半分だけ「左(-)」にずらす
			m_enemyhpBar->SetPosition(m_enemyHpBarPos.x - (reduceWidth / 2.0f), m_enemyHpBarPos.y);
		}

		// 2P -> 1P (ヒット)
		if (player2->IsAttacking() && !player2->HasHit() && player2->GetActiveHitbox().Intersects(box1))
		{
			AttackParams& params = player2->GetLightPunchParams();
			player->ReceiveDamage(params.damage);
			player2->OnHit();

			// --- 1P HPバーの更新 (左側) ---
			float ratio = player->GetHpRatio();
			float currentWidth = m_barMaxWidth * ratio;

			// 減った幅を計算
			float reduceWidth = m_barMaxWidth - currentWidth;

			m_hpBar->SetSize(currentWidth, 80.0f);

			// 減った幅の半分だけ「右(+)」にずらす
			m_hpBar->SetPosition(m_hpBarPos.x + (reduceWidth / 2.0f), m_hpBarPos.y);
		}
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
		player->DrawHitbox();
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
		player2->DrawHitbox();
	}

	// --- 2D UIの描画 ---

	// 準備
	SimpleUI::Clear();
	DirectX::XMFLOAT4X4 identity;
	DirectX::XMStoreFloat4x4(&identity, DirectX::XMMatrixIdentity());
	SimpleUI::SetMatrix(identity, identity, identity);

	// それぞれの画像を描画登録 
	if (m_hpBar) m_hpBar->Draw();
	if (m_enemyhpBar) m_enemyhpBar->Draw();

	// まとめて描画実行
	SimpleUI::DrawAll();
}