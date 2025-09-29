#include "math.h"
#include "SceneBlank.h"
#include "Geometory.h"
#include "DebugLog.h"
#include "Model.h"
#include "CameraBase.h"
#include "LightBase.h"
#include "Shader.h"
#include "Player.h" // 追加
#include "SimpleUI.h" // 追加
#include "Texture.h"

using namespace DirectX;
using namespace DirectX::SimpleMath;

Texture* g_uiTex = nullptr; // ←グローバル変数の本体を定義

void SceneBlank::Init()
{
    // シェーダーの読み込み
    Shader* shader[] = {
        CreateObj<VertexShader>("VS_Object"),
        CreateObj<PixelShader>("PS_TexColor"),
    };
    const char* file[] = {
        "Assets/Shader/VS_Object.cso",
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
    player->Load("Assets/Model/knight/knight.fbx", 0.02f, false, true);
    // 初期位置
    player->SetPosition({ 0.0f, 0.0f, 0.0f });

    player->SetRotation({ 0.0f, DirectX::XM_PI / 2.0f, 0.0f });

    // テクスチャ生成
    g_uiTex = new Texture();
    g_uiTex->Create("Assets/UI/apple.jpg"); // ここでパスを指定
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

    Shader* shader[] = {
        GetObj<Shader>("VS_Object"),
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
        shader[0]->WriteBuffer(0, mat);
        shader[0]->WriteBuffer(1, light);
        shader[1]->WriteBuffer(0, light);
        shader[1]->WriteBuffer(1, camera);

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
    float uiWidth = 200.0f;   // ← 画像の幅（ピクセル単位）
    float uiHeight = 50.0f;   // ← 画像の高さ（ピクセル単位）
    float x = screenWidth - uiWidth - 20.0f; // ← X座標（右端から20px内側）
    float y = 20.0f;                        // ← Y座標（上端から20px下）

    // ピクセル→NDC変換
    float ndcX = (x / screenWidth) * 2.0f - 1.0f;
    float ndcY = 1.0f - (y / screenHeight) * 2.0f;
    float ndcW = (uiWidth / screenWidth) * 2.0f;
    float ndcH = (uiHeight / screenHeight) * 2.0f;

    // UIリストをクリアして再追加（毎フレーム呼ぶ場合）
    SimpleUI::Clear();
    // 画像付きUIを右上に追加
    // 画像テクスチャはInitで生成したものをグローバルやメンバで保持しておくこと
    SimpleUI::AddRect(ndcX, ndcY, ndcW, ndcH, { 1,1,1,1 }, g_uiTex);

    // ビュー・プロジェクション行列を単位行列にしてUI描画
    DirectX::XMFLOAT4X4 identity;
    DirectX::XMStoreFloat4x4(&identity, DirectX::XMMatrixIdentity());
    SimpleUI::SetMatrix(identity, identity, identity);
    SimpleUI::DrawAll();
}