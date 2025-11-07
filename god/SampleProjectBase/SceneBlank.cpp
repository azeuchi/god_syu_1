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
#include <system/imgui/imgui.h>

using namespace DirectX;
using namespace DirectX::SimpleMath;

Texture* g_uiTex = nullptr;

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

    // プレイヤー生成
    CreateObj<Player>("Player");
    Player* player = GetObj<Player>("Player");

    // 各種アニメーションの読み込み
    if (!player->Load("Assets/Model/knight/Idle.fbx", 0.02f, true, false))
    {
        MessageBox(NULL, "プレイヤーモデルの読み込みに失敗しました。", "Model Load Error", MB_OK);
    }
    player->GetModel()->LoadAnimation("Assets/Model/knight/Walking.fbx", "Walk", true);
    player->GetModel()->LoadAnimation("Assets/Model/knight/WalkBack.fbx", "WalkBack", true);


    // 初期位置・回転
    player->SetPosition({ 0.0f, 0.0f, 0.0f });
    player->SetRotation({ 0.0f, DirectX::XM_PI / -2.0f, 0.0f }); // 右向きからスタート

    // アニメーションの初期状態を設定
    m_currentState = { "Idle", 0 };
    m_previousState = { "Idle", 0 };
    m_blendFactor = 1.0f;

    g_uiTex = new Texture();
    g_uiTex->Create("Assets/UI/apple.jpg");
}

void SceneBlank::Uninit()
{
    if (g_uiTex) {
        delete g_uiTex;
        g_uiTex = nullptr;
    }
}

void SceneBlank::Update(float tick)
{
    Player* player = GetObj<Player>("Player");
    if (player) {
        player->Update(tick);

        // プレイヤーの向きと移動方向から状態を決定する
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
            else // 横方向の移動は、いったん前進として扱う
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
}

void SceneBlank::Draw()
{

    CameraBase* pCamera = GetObj<CameraBase>("Camera");
    LightBase* pLight = GetObj<LightBase>("Light");
    Player* player = GetObj<Player>("Player");

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

    if (player) {
        XMFLOAT3 pos = player->GetPosition();
        XMFLOAT3 rot = player->GetRotation();

        Matrix scaleMat = player->GetModel()->GetScaleBaseMatrix();
        Matrix rotMat = DirectX::XMMatrixRotationRollPitchYaw(rot.x, rot.y, rot.z);
        Matrix transMat = Matrix::CreateTranslation(pos.x, pos.y, pos.z);
        Matrix world = scaleMat * rotMat * transMat;
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

    // --- ここからUI描画 ----
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