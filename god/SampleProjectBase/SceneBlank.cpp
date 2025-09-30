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

    // アイドルアニメーションの読み込み
    if (!player->Load("Assets/Model/knight/Idle.fbx", 0.02f,true , false))
    {
        // 読み込みに失敗した場合
        MessageBox(NULL, "プレイヤーモデルの読み込みに失敗しました。\nファイルパスやファイル名を確認してください。", "Model Load Error", MB_OK);
    }

    // 初期位置・回転
    player->SetPosition({ 0.0f, 0.0f, 0.0f });
    player->SetRotation({ 0.0f, DirectX::XM_PI / -2.0f, 0.0f });

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
    // アニメーションフレーム更新
    m_Frame++;

    // プレイヤー操作
    Player* player = GetObj<Player>("Player");
    if (player) {
        player->Update(tick);
    }
}

void SceneBlank::Draw()
{
    CameraBase* pCamera = GetObj<CameraBase>("Camera");
    LightBase* pLight = GetObj<LightBase>("Light");
    Player* player = GetObj<Player>("Player");

    // 行列・ライト・カメラ情報
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

    // シェーダーの取得 (アニメーション対応シェーダーを取得)
    Shader* shader[] = {
        GetObj<Shader>("VS_SkinMeshAnimation"),
        GetObj<Shader>("PS_TexColor"),
    };

    // プレイヤーのワールド行列（座標反映）
    if (player) {
        XMFLOAT3 pos = player->GetPosition();
        XMFLOAT3 rot = player->GetRotation();

        // ワールド行列を生成
        Matrix scaleMat = player->GetModel()->GetScaleBaseMatrix();
        Matrix rotMat = DirectX::XMMatrixRotationRollPitchYaw(rot.x, rot.y, rot.z);
        Matrix transMat = Matrix::CreateTranslation(pos.x, pos.y, pos.z);
        Matrix world = scaleMat * rotMat * transMat;

        // シェーダーに渡すために転置
        XMStoreFloat4x4(&mat[0], XMMatrixTranspose(world));

      
        // 定数バッファの更新
        shader[0]->WriteBuffer(0, mat);     // 頂点シェーダーには行列のみ渡す
        shader[1]->WriteBuffer(0, light);   // ピクセルシェーダーにはライト情報を渡す
        shader[1]->WriteBuffer(1, camera);  // ピクセルシェーダーにはカメラ情報を渡す

        // アニメーションの更新
        player->GetModel()->UpdateAnimation("Idle", m_Frame);

        // シェーダーをセット
        player->SetVertexShader(shader[0]);
        player->SetPixelShader(shader[1]);

        // プレイヤー描画
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