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
#include <algorithm> 

// やられ状態クラスを使用するためにインクルード
#include "PlayerStateDamage.h"

using namespace DirectX;
using namespace DirectX::SimpleMath;

// グローバル変数: UI用テクスチャ管理
Texture* g_uiTex = nullptr;

// 設定ファイルパス
const char* SETTINGS_FILE = "player_settings.ini";

// --- ステージとカメラの移動制限設定 ---
const float STAGE_LIMIT_X = 14.0f;
const float CAMERA_LIMIT_X = 10.0f;

/**
 * @brief シーンの初期化処理
 */
void SceneBlank::Init()
{
	// ==================================================
	// シェーダーの読み込み
	// ==================================================
	Shader* shader[] = {
		CreateObj<VertexShader>("VS_SkinMeshAnimation"),
		CreateObj<PixelShader>("PS_TexColor"),
		CreateObj<VertexShader>("VS_Object")
	};
	const char* file[] = {
		"Assets/Shader/VS_SkinMeshAnimation.cso",
		"Assets/Shader/PS_TexColor.cso",
		"Assets/Shader/VS_Object.cso"
	};
	int shaderNum = _countof(shader);
	for (int i = 0; i < shaderNum; ++i)
	{
		if (FAILED(shader[i]->Load(file[i])))
		{
			MessageBox(NULL, file[i], "Shader Error", MB_OK);
		}
	}

	// スカイドーム
	CreateObj<Model>("SkyModel");
	Model* skyModel = GetObj<Model>("SkyModel");
	if (!skyModel->Load("Assets/Model/SkyDome/SkyDome.fbx", 1.0f, true, true))
	{
		MessageBox(NULL, "スカイドームモデルの読み込みに失敗しました", "Error", MB_OK);
	}
	skyModel->SetTexture("Assets/Model/SkyDome/sky.jpg");
	skyModel->SetPixelShader(GetObj<Shader>("PS_TexColor"));
	m_skyDome = new SkyDome();
	m_skyDome->Init(skyModel);

	// ==================================================
	// 2D画像 (HPバー) の生成と配置
	// ==================================================
	m_barMaxWidth = 500.0f;

	// 1P HP Bar
	m_hpBar = new Image2D();
	m_hpBarPos = { 330.0f, 80.0f };
	m_hpBar->Load("Assets/Texture/hp.png", m_hpBarPos.x, m_hpBarPos.y, m_barMaxWidth, 80.0f);

	// 2P HP Bar
	m_enemyhpBar = new Image2D();
	m_enemyHpBarPos = { 950.0f, 80.0f };
	m_enemyhpBar->Load("Assets/Texture/hp.png", m_enemyHpBarPos.x, m_enemyHpBarPos.y, m_barMaxWidth, 80.0f);


	// ==================================================
	// プレイヤー1 (自分) の生成
	// ==================================================
	CreateObj<Player>("Player");
	Player* player = GetObj<Player>("Player");
	player->SetInputType(PlayerInputType::PLAYER_1);

	// ==================================================
	// 設定ファイルの読み込み
	// ==================================================
	float moveSpeed = 2.0f;
	DirectX::XMFLOAT3 scale = { 1.0f, 1.0f, 1.0f };

	AttackParams lParams = player->GetLightPunchParams();
	AttackParams mParams = player->GetMediumPunchParams();

	std::ifstream ifs(SETTINGS_FILE);
	if (ifs.is_open())
	{
		ifs >> moveSpeed;
		ifs >> scale.x >> scale.y >> scale.z;

		// 立ち(Base)くらい判定
		for (int i = 0; i < (int)HurtboxType::COUNT; ++i) {
			DirectX::XMFLOAT2 ext, off;
			if (!ifs.eof()) ifs >> ext.x >> ext.y >> off.x >> off.y;
			player->SetHurtboxBase((HurtboxType)i, off, ext);
		}

		// しゃがみ(Crouch)くらい判定
		for (int i = 0; i < (int)HurtboxType::COUNT; ++i) {
			DirectX::XMFLOAT2 ext, off;
			if (!ifs.eof()) ifs >> ext.x >> ext.y >> off.x >> off.y;
			player->SetHurtboxCrouch((HurtboxType)i, off, ext);
		}

		// LightPunch
		if (!ifs.eof()) ifs >> lParams.totalDuration;
		if (!ifs.eof()) ifs >> lParams.hitboxStart;
		if (!ifs.eof()) ifs >> lParams.hitboxEnd;
		if (!ifs.eof()) ifs >> lParams.hitboxOffset.x >> lParams.hitboxOffset.y;
		if (!ifs.eof()) ifs >> lParams.hitboxExtents.x >> lParams.hitboxExtents.y;
		if (!ifs.eof()) ifs >> lParams.damage;
		if (!ifs.eof()) ifs >> lParams.hitFrame;
		if (!ifs.eof()) ifs >> lParams.blockFrame;
		if (!ifs.eof()) ifs >> lParams.headOffsetVal.x >> lParams.headOffsetVal.y;
		if (!ifs.eof()) ifs >> lParams.headSizeVal.x >> lParams.headSizeVal.y;
		if (!ifs.eof()) ifs >> lParams.bodyOffsetVal.x >> lParams.bodyOffsetVal.y;
		if (!ifs.eof()) ifs >> lParams.bodySizeVal.x >> lParams.bodySizeVal.y;
		if (!ifs.eof()) ifs >> lParams.legsOffsetVal.x >> lParams.legsOffsetVal.y;
		if (!ifs.eof()) ifs >> lParams.legsSizeVal.x >> lParams.legsSizeVal.y;

		// MediumPunch
		if (!ifs.eof()) ifs >> mParams.totalDuration;
		if (!ifs.eof()) ifs >> mParams.hitboxStart;
		if (!ifs.eof()) ifs >> mParams.hitboxEnd;
		if (!ifs.eof()) ifs >> mParams.hitboxOffset.x >> mParams.hitboxOffset.y;
		if (!ifs.eof()) ifs >> mParams.hitboxExtents.x >> mParams.hitboxExtents.y;
		if (!ifs.eof()) ifs >> mParams.damage;
		if (!ifs.eof()) ifs >> mParams.hitFrame;
		if (!ifs.eof()) ifs >> mParams.blockFrame;
		if (!ifs.eof()) ifs >> mParams.headOffsetVal.x >> mParams.headOffsetVal.y;
		if (!ifs.eof()) ifs >> mParams.headSizeVal.x >> mParams.headSizeVal.y;
		if (!ifs.eof()) ifs >> mParams.bodyOffsetVal.x >> mParams.bodyOffsetVal.y;
		if (!ifs.eof()) ifs >> mParams.bodySizeVal.x >> mParams.bodySizeVal.y;
		if (!ifs.eof()) ifs >> mParams.legsOffsetVal.x >> mParams.legsOffsetVal.y;
		if (!ifs.eof()) ifs >> mParams.legsSizeVal.x >> mParams.legsSizeVal.y;

		ifs.close();
	}

	player->SetMoveSpeed(moveSpeed);
	player->SetScale(scale);
	player->GetLightPunchParams() = lParams;
	player->GetMediumPunchParams() = mParams;


	// ==================================================
	//  モデルとアニメーションのロード (1P)
	// ==================================================
	if (!player->Load("Assets/Model/knight/Idle.fbx", 0.02f, true, false))
	{
		MessageBox(NULL, "プレイヤーモデルの読み込みに失敗しました。", "Model Load Error", MB_OK);
	}
	// 攻撃・ダメージはループなし(false)
	player->GetModel()->LoadAnimation("Assets/Model/knight/Walking.fbx", "Walk", true);
	player->GetModel()->LoadAnimation("Assets/Model/knight/WalkBack.fbx", "WalkBack", true);
	player->GetModel()->LoadAnimation("Assets/Model/knight/LightPunch.fbx", "LightPunch", true);
	player->GetModel()->LoadAnimation("Assets/Model/knight/MediumPunch.fbx", "MediumPunch", true);
	player->GetModel()->LoadAnimation("Assets/Model/knight/Jump.fbx", "Jump", true);
	player->GetModel()->LoadAnimation("Assets/Model/knight/Damage.fbx", "Damage", true);
	player->GetModel()->LoadAnimation("Assets/Model/knight/CrouchIdle.fbx", "CrouchIdle", true);

	player->SetPosition({ -2.0f, 0.0f, 0.0f });
	player->SetRotation({ 0.0f, DirectX::XM_PI / -2.0f, 0.0f });


	// ==================================================
	// プレイヤー2  の生成
	// ==================================================
	CreateObj<Player>("Player2");
	Player* player2 = GetObj<Player>("Player2");
	player2->SetInputType(PlayerInputType::PLAYER_2);
	player2->SetMoveSpeed(moveSpeed);

	// スケールXをマイナスにして左右反転 (ミラーリング)
	DirectX::XMFLOAT3 scaleP2 = scale;
	scaleP2.x *= -1.0f;
	player2->SetScale(scaleP2);

	player2->GetLightPunchParams() = lParams;
	player2->GetMediumPunchParams() = mParams;

	for (int i = 0; i < (int)HurtboxType::COUNT; ++i) {
		player2->SetHurtboxBase((HurtboxType)i,
			player->GetHurtboxBaseOffset((HurtboxType)i),
			player->GetHurtboxBaseExtents((HurtboxType)i));
		// 2Pにもしゃがみ判定をコピー
		player2->SetHurtboxCrouch((HurtboxType)i,
			player->GetHurtboxCrouchOffset((HurtboxType)i),
			player->GetHurtboxCrouchExtents((HurtboxType)i));
	}

	if (!player2->Load("Assets/Model/knight/Idle.fbx", 0.02f, true, false))
	{
		MessageBox(NULL, "プレイヤー2モデルの読み込みに失敗しました。", "Model Load Error", MB_OK);
	}
	// 2Pも攻撃・ダメージはループなし
	player2->GetModel()->LoadAnimation("Assets/Model/knight/Walking.fbx", "Walk", true);
	player2->GetModel()->LoadAnimation("Assets/Model/knight/WalkBack.fbx", "WalkBack", true);
	player2->GetModel()->LoadAnimation("Assets/Model/knight/LightPunch.fbx", "LightPunch", true);
	player2->GetModel()->LoadAnimation("Assets/Model/knight/MediumPunch.fbx", "MediumPunch", true);
	player2->GetModel()->LoadAnimation("Assets/Model/knight/Jump.fbx", "Jump", true);
	player2->GetModel()->LoadAnimation("Assets/Model/knight/Damage.fbx", "Damage", true);
	player2->GetModel()->LoadAnimation("Assets/Model/knight/CrouchIdle.fbx", "CrouchIdle", true);

	player2->SetPosition({ 2.0f, 0.0f, 0.0f });
	player2->SetRotation({ 0.0f, DirectX::XM_PI / 2.0f, 0.0f });

	// ==================================================
	// 7. カメラの初期配置
	// ==================================================
	CameraBase* pCamera = GetObj<CameraBase>("Camera");
	if (pCamera)
	{
		pCamera->SetPos({ 0.0f, 1.2f, -5.0f });
		pCamera->SetLook({ 0.0f, 1.0f, 4.0f });
	}

	g_uiTex = new Texture();
}

/**
 * @brief 終了処理
 */
void SceneBlank::Uninit()
{
	if (m_hpBar) delete m_hpBar;
	if (m_enemyhpBar) delete m_enemyhpBar;
	if (m_skyDome) { delete m_skyDome; m_skyDome = nullptr; }
	if (g_uiTex) { delete g_uiTex; g_uiTex = nullptr; }
}

/**
 * @brief 更新処理
 */
void SceneBlank::Update(float tick)
{
	Player* player = GetObj<Player>("Player");
	Player* player2 = GetObj<Player>("Player2");

	if (player) player->Update(tick);
	if (player2) player2->Update(tick);

	if (player && player2)
	{
		float x1 = player->GetPosition().x;
		float x2 = player2->GetPosition().x;

		// 現在のスケール設定（大きさ）を絶対値で取得しておく
		float absScale1 = fabsf(player->GetScale().x);
		float absScale2 = fabsf(player2->GetScale().x);

		DirectX::XMFLOAT3 s1 = player->GetScale();
		DirectX::XMFLOAT3 s2 = player2->GetScale();

		// 左向き(+90度) と 右向き(-90度) の定数
		float rotLeft = DirectX::XM_PI / 2.0f;
		float rotRight = DirectX::XM_PI / -2.0f;

		// 1Pが 2Pより「左」にいる場合 (通常の状態)
		if (x1 < x2)
		{
			// 1P: 左側配置 -> 右を向く (通常スケール)
			s1.x = absScale1; // プラス (反転なし)
			player->SetScale(s1);
			player->SetRotation({ 0.0f, rotRight, 0.0f });

			// 2P: 右側配置 -> 左を向く (反転スケール)
			s2.x = -absScale2; // マイナス (左右反転)
			player2->SetScale(s2);
			player2->SetRotation({ 0.0f, rotLeft, 0.0f });
		}
		// 1Pが 2Pより「右」にいる場合 (入れ替わった状態)
		else
		{
			// 1P: 右側配置 -> 左を向く (反転スケール)
			s1.x = -absScale1; // マイナス (左右反転)
			player->SetScale(s1);
			player->SetRotation({ 0.0f, rotLeft, 0.0f });

			// 2P: 左側配置 -> 右を向く (通常スケール)
			s2.x = absScale2; // プラス (反転なし)
			player2->SetScale(s2);
			player2->SetRotation({ 0.0f, rotRight, 0.0f });
		}
	}

	// ステージ端の制限
	if (player)
	{
		XMFLOAT3 pos = player->GetPosition();
		pos.x = std::clamp(pos.x, -STAGE_LIMIT_X, STAGE_LIMIT_X);
		player->SetPosition(pos);
	}
	if (player2)
	{
		XMFLOAT3 pos = player2->GetPosition();
		pos.x = std::clamp(pos.x, -STAGE_LIMIT_X, STAGE_LIMIT_X);
		player2->SetPosition(pos);
	}


	// 当たり判定と相互作用
	if (player && player2)
	{
		//  押し出し判定
		bool isColliding = player->CheckCollision(player2);
		player->SetIsColliding(isColliding);
		player2->SetIsColliding(isColliding);

		if (isColliding && !player->IsAttacking() && !player2->IsAttacking())
		{
			DirectX::BoundingBox box1 = player->GetHurtbox(HurtboxType::BODY);
			DirectX::BoundingBox box2 = player2->GetHurtbox(HurtboxType::BODY);

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

			pos1.x += pushVector.x;
			pos2.x -= pushVector.x;
			pos1.y += pushVector.y;
			pos2.y -= pushVector.y;

			pos1.x = std::clamp(pos1.x, -STAGE_LIMIT_X, STAGE_LIMIT_X);
			pos2.x = std::clamp(pos2.x, -STAGE_LIMIT_X, STAGE_LIMIT_X);

			player->SetPosition(pos1);
			player2->SetPosition(pos2);
		}

		// B. 攻撃判定 (Hitbox)
		// 1P -> 2P
		bool hit2 = false;
		if (player->IsAttacking() && !player->HasHit())
		{
			BoundingBox atk = player->GetActiveHitbox();
			if (atk.Intersects(player2->GetHurtbox(HurtboxType::HEAD)) ||
				atk.Intersects(player2->GetHurtbox(HurtboxType::BODY)) ||
				atk.Intersects(player2->GetHurtbox(HurtboxType::LEGS)))
			{
				hit2 = true;
			}
		}

		if (hit2)
		{
			AttackParams* params = player->GetCurrentAttackParams();
			int dmg = (params != nullptr) ? params->damage : 0;
			int stun = (params != nullptr) ? params->hitFrame : 30; // 硬直時間

			player2->ReceiveDamage(dmg);
			player->OnHit();

			player2->SetState(new PlayerStateDamage(stun));

			float ratio = player2->GetHpRatio();
			float currentWidth = m_barMaxWidth * ratio;
			float reduceWidth = m_barMaxWidth - currentWidth;
			m_enemyhpBar->SetSize(currentWidth, 80.0f);
			m_enemyhpBar->SetPosition(m_enemyHpBarPos.x - (reduceWidth / 2.0f), m_enemyHpBarPos.y);
		}

		// 2P -> 1P
		bool hit1 = false;
		if (player2->IsAttacking() && !player2->HasHit())
		{
			BoundingBox atk = player2->GetActiveHitbox();
			if (atk.Intersects(player->GetHurtbox(HurtboxType::HEAD)) ||
				atk.Intersects(player->GetHurtbox(HurtboxType::BODY)) ||
				atk.Intersects(player->GetHurtbox(HurtboxType::LEGS)))
			{
				hit1 = true;
			}
		}

		if (hit1)
		{
			AttackParams* params = player2->GetCurrentAttackParams();
			int dmg = (params != nullptr) ? params->damage : 0;
			int stun = (params != nullptr) ? params->hitFrame : 30;

			player->ReceiveDamage(dmg);
			player2->OnHit();

			player->SetState(new PlayerStateDamage(stun));

			float ratio = player->GetHpRatio();
			float currentWidth = m_barMaxWidth * ratio;
			float reduceWidth = m_barMaxWidth - currentWidth;
			m_hpBar->SetSize(currentWidth, 80.0f);
			m_hpBar->SetPosition(m_hpBarPos.x + (reduceWidth / 2.0f), m_hpBarPos.y);
		}
	}


	// カメラ制御
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

		if (m_skyDome)
		{
			m_skyDome->Update(pCamera->GetPos());
		}
	}
}

/**
 * @brief 描画処理
 */
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

	if (m_skyDome)
	{
		m_skyDome->Draw(pCamera->GetView(), pCamera->GetProj(), GetObj<Shader>("VS_Object"));
	}

	// 1. プレイヤー1
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

	// 2. プレイヤー2
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

	// 3. 2D UI
	SimpleUI::Clear();
	DirectX::XMFLOAT4X4 identity;
	DirectX::XMStoreFloat4x4(&identity, DirectX::XMMatrixIdentity());
	SimpleUI::SetMatrix(identity, identity, identity);

	if (m_hpBar) m_hpBar->Draw();
	if (m_enemyhpBar) m_enemyhpBar->Draw();

	SimpleUI::DrawAll();
}