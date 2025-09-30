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

using namespace DirectX;
using namespace DirectX::SimpleMath;

Texture* g_uiTex = nullptr;

void SceneBlank::Init()
{
    // シェーダーの読み込み 
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

    // アイドルアニメーション（モデルの基本情報として）の読み込み
    if (!player->Load("Assets/Model/knight/Idle.fbx", 0.02f, true, false))
    {
        MessageBox(NULL, "プレイヤーモデルの読み込みに失敗しました。\nファイルパスやファイル名を確認してください。", "Model Load Error", MB_OK);
    }
    player->GetModel()->LoadAnimation("Assets/Model/knight/Walking.fbx", "Walk", true);

    // 初期位置・回転
    player->SetPosition({ 0.0f, 0.0f, 0.0f });
    player->SetRotation({ 0.0f, DirectX::XM_PI / -2.0f, 0.0f });

    // 追加: アニメーションの初期状態を設定
    m_currentState = { "Idle", 0 };
    m_previousState = { "Idle", 0 }; // 最初はどちらもアイドル
    m_blendFactor = 1.0f;            // ブレンドはしない

    // テクスチャ生成
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

        // 変更: 状態遷移とブレンド率の更新ロジック
        // プレイヤーの速度から目標となる状態を決定
        Vector3 velocity = player->GetVelocity();
        PlayerState targetState = (fabs(velocity.x) > 0.0f || fabs(velocity.z) > 0.0f) ? PlayerState::WALKING : PlayerState::IDLE;

        // 目標のアニメーション名を取得
        const char* targetAnimName = (targetState == PlayerState::WALKING) ? "Walk" : "Idle";

        // 現在再生中のアニメーションと目標が違う場合、遷移処理を開始
        if (strcmp(m_currentState.name, targetAnimName) != 0)
        {
            m_previousState = m_currentState;     // 今の状態を過去の状態として保存
            m_currentState.name = targetAnimName; // 新しい状態を設定
            m_currentState.frame = 0;             // 新しいアニメーションは最初から再生
            m_blendFactor = 0.0f;                 // ブレンドを開始
        }

        // ブレンド率を更新
        if (m_blendFactor < 1.0f)
        {
            m_blendFactor += tick / m_transitionDuration;
            if (m_blendFactor > 1.0f)
            {
                m_blendFactor = 1.0f;
            }
        }

        // 現在のアニメーションのフレームを更新
        m_currentState.frame++;
        // 遷移中のアニメーション（前のアニメーション）はフレームを固定するため更新しない
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

        // 変更: 新しいブレンド用関数を呼び出す
        player->GetModel()->UpdateWithBlend(
            m_currentState.name, m_currentState.frame,
            m_previousState.name, m_previousState.frame,
            m_blendFactor);

        player->SetVertexShader(shader[0]);
        player->SetPixelShader(shader[1]);

        player->Draw();
        player->DrawBoundingBox();
    }

    // --- UI描画 ---
    constexpr float screenWidth = 1280.0f;
    constexpr float screenHeight = 720.0f;
    float uiWidth = 200.0f;
    float uiHeight = 50.0f;
    float x = screenWidth - uiWidth - 20.0f;
    float y = 20.0f;

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