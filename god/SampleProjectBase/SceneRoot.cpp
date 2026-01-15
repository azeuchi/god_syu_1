#include "SceneRoot.h"
#include <stdio.h>
#include "CameraDCC.h"
#include "MoveLight.h"
#include "Model.h"
#include "Input.h"
#include "Geometory.h"

// 各シーンのインクルード
#include "SceneVisual.h"
#include "SceneBlank.h"
#include "SceneDebug.h" 
#include "SceneTitle.h" 
#include	"SceneResult.h"
#include "DebugLog.h"

#define STR(var) #var

//--- 定数定義
enum SceneKind
{
	SCENE_TITLE,    // タイトル
	SCENE_GAME,		// ゲーム本編
	SCENE_RESULT,   // リザルト
	SCENE_DEBUG,    // デバッグ
	SCENE_MAX
};

/// <summary>
/// 現在の m_index に基づいてサブシーンを作成する
/// </summary>
void SceneRoot::ChangeScene()
{
	switch (m_index)
	{
	case SCENE_TITLE:
		AddSubScene<SceneTitle>();
		m_sceneName = "SCENE_TITLE";
		break;

	case SCENE_GAME:
		AddSubScene<SceneBlank>();
		m_sceneName = "SCENE_GAME";
		break;

	case SCENE_RESULT:
		AddSubScene<SceneResult>();
		m_sceneName = "SCENE_RESULT";
		break;

	case SCENE_DEBUG:
		AddSubScene<SceneDebug>();
		m_sceneName = "SCENE_DEBUG";
		break;

	default:
		// 万が一未定義のシーンに来たらタイトルへ戻すなどの安全策
		m_index = SCENE_TITLE;
		AddSubScene<SceneTitle>();
		m_sceneName = "SCENE_TITLE(Fallback)";
		break;
	}

	DebugLog::log(DebugLog::INFO_LOG, "SceneName = " + m_sceneName);
	m_isChangeScene = true;
}

/// <summary>
/// シーン遷移の実行
/// 古いシーンを消して新しいシーンを作る処理をまとめたもの
/// </summary>
void SceneRoot::Transition(int nextScene)
{
	m_index = nextScene;  // 次のシーンIDをセット
	RemoveSubScene();     // 現在のシーンを削除
	ChangeScene();        // 新しいシーンを作成
}

//--- 構造体
struct ViewSetting
{
	DirectX::XMFLOAT3 camPos;
	DirectX::XMFLOAT3 camLook;
	DirectX::XMFLOAT3 camUp;
	float lightRadXZ;
	float lightRadY;
	float lightH;
	float lightSV;
	int index;
};
const char* SettingFileName = "Assets/setting.dat";

void SceneRoot::Init()
{
	// 保存データの読み込み
	ViewSetting setting =
	{
		DirectX::XMFLOAT3(0.0f, 1.0f, -5.0f),
		DirectX::XMFLOAT3(0.0f, 1.0f, 0.0f),
		DirectX::XMFLOAT3(0.0f, 1.0f, 0.0f),
		0.0f, DirectX::XM_PIDIV4,
		0.0f, 1.0f,
		SCENE_TITLE
	};
	FILE* fp;
	fopen_s(&fp, SettingFileName, "rb");
	if (fp)
	{
		fread(&setting, sizeof(ViewSetting), 1, fp);
		fclose(fp);
	}

	// カメラの作成
	CameraBase* pCamera = CreateObj<CameraDCC>("Camera");
	pCamera->SetPos(setting.camPos);
	pCamera->SetLook(setting.camLook);
	pCamera->SetUp(setting.camUp);

	// ライトの作成
	MoveLight* pLight = CreateObj<MoveLight>("Light");
	pLight->SetRot(setting.lightRadXZ, setting.lightRadY);
	pLight->SetHSV(setting.lightH, setting.lightSV);
	pLight->UpdateParam();

	// モデルの読み込み
	CreateObj<Model>("Model");
	GetObj<Model>("Model")->Load("Assets/Model/spot/spot.fbx", 1.0f, true);

	// カメラのリセット
	pCamera->SetPos(DirectX::XMFLOAT3(0.0f, 1.0f, -5.0f));
	pCamera->SetLook(DirectX::XMFLOAT3(0.0f, 1.0f, 0.0f));
	pCamera->SetUp(DirectX::XMFLOAT3(0.0f, 1.0f, 0.0f));


	// 最初はタイトルから開始する
	m_index = SCENE_TITLE;
	ChangeScene();
}

void SceneRoot::Uninit()
{
	CameraBase* pCamera = GetObj<CameraBase>("Camera");
	MoveLight* pLight = GetObj<MoveLight>("Light");
	ViewSetting setting =
	{
		pCamera->GetPos(),
		pCamera->GetLook(),
		pCamera->GetUp(),
		pLight->GetRotXZ(), pLight->GetRotY(),
		pLight->GetH(), pLight->GetSV(),
		m_index
	};
	FILE* fp;
	fopen_s(&fp, SettingFileName, "wb");
	if (fp)
	{
		fwrite(&setting, sizeof(ViewSetting), 1, fp);
		fclose(fp);
	}
}

void SceneRoot::Update(float tick)
{
	m_isChangeScene = false;
	CameraBase* pCamera = GetObj<CameraBase>("Camera");
	LightBase* pLight = GetObj<LightBase>("Light");

	// カメラとライトの更新（全シーン共通）
	pCamera->Update();
	pLight->Update();


	//----------------------------------------------------------
	// シーン遷移ロジック
	// 現在のシーンによって、遷移条件を切り替える
	//----------------------------------------------------------
	switch (m_index)
	{
		//  タイトル画面の時
	case SCENE_TITLE:
		// エンターキーでゲーム本編へ
		if (IsKeyTrigger(VK_RETURN))
		{
			Transition(SCENE_GAME);
		}
		break;

		// ゲーム本編の時
	case SCENE_GAME:
		// Nキーでデバッグ画面
			if (IsKeyTrigger('N'))
			{
				Transition(SCENE_DEBUG);
			}	

		// ゲームセットになったらリザルトへ (静的フラグをチェック)
		if (SceneBlank::s_isGameSet)
		{
			Transition(SCENE_RESULT);
			SceneBlank::s_isGameSet = false; // フラグを戻しておく
		}

		break;

		// デバッグ画面の時
	case SCENE_DEBUG:
		// Nキーでゲーム画面に戻る
		if (IsKeyTrigger('N'))
		{
			Transition(SCENE_GAME);
		}
		break;

		// リザルト画面
	case SCENE_RESULT:
		// エンターキーでに戻る
		if (IsKeyTrigger(VK_RETURN))
		{
			Transition(SCENE_TITLE);
		}
		break;
	}



	if (IsKeyTrigger('R'))
	{

	}
}

void SceneRoot::Draw()
{
	CameraBase* pCamera = GetObj<CameraBase>("Camera");
	LightBase* pLight = GetObj<LightBase>("Light");

	DirectX::XMFLOAT4X4 fmat;
	DirectX::XMStoreFloat4x4(&fmat, DirectX::XMMatrixIdentity());
	Geometory::SetWorld(fmat);
	Geometory::SetView(pCamera->GetView());
	Geometory::SetProjection(pCamera->GetProj());

	// グリッド線の描画はリザルト以外で有効
	if (m_index != SCENE_RESULT)
	{
		const int GridSize = 10;

		// 通常の網掛け (グレー)
		Geometory::SetColor(DirectX::XMFLOAT4(0.3f, 0.3f, 0.3f, 1.0f));
		for (int i = 1; i <= GridSize; ++i)
		{
			float g = (float)i;
			// 床 (XZ平面)
			Geometory::AddLine(DirectX::XMFLOAT3(g, 0.0f, -GridSize), DirectX::XMFLOAT3(g, 0.0f, GridSize));
			Geometory::AddLine(DirectX::XMFLOAT3(-g, 0.0f, -GridSize), DirectX::XMFLOAT3(-g, 0.0f, GridSize));
			Geometory::AddLine(DirectX::XMFLOAT3(-GridSize, 0.0f, g), DirectX::XMFLOAT3(GridSize, 0.0f, g));
			Geometory::AddLine(DirectX::XMFLOAT3(-GridSize, 0.0f, -g), DirectX::XMFLOAT3(GridSize, 0.0f, -g));

			// 壁 (ステージ枠)
			// 高さ方向（Y軸）への積み上げ（横線）
			Geometory::AddLine(DirectX::XMFLOAT3(-GridSize, g, GridSize), DirectX::XMFLOAT3(GridSize, g, GridSize)); // 奥
			Geometory::AddLine(DirectX::XMFLOAT3(-GridSize, g, -GridSize), DirectX::XMFLOAT3(GridSize, g, -GridSize)); // 手前
			Geometory::AddLine(DirectX::XMFLOAT3(GridSize, g, -GridSize), DirectX::XMFLOAT3(GridSize, g, GridSize)); // 右
			Geometory::AddLine(DirectX::XMFLOAT3(-GridSize, g, -GridSize), DirectX::XMFLOAT3(-GridSize, g, GridSize)); // 左

			// 垂直方向（Y軸）の線（縦線）
			// 奥面と手前面の縦線
			Geometory::AddLine(DirectX::XMFLOAT3(g, 0.0f, GridSize), DirectX::XMFLOAT3(g, GridSize, GridSize));
			Geometory::AddLine(DirectX::XMFLOAT3(-g, 0.0f, GridSize), DirectX::XMFLOAT3(-g, GridSize, GridSize));
			Geometory::AddLine(DirectX::XMFLOAT3(g, 0.0f, -GridSize), DirectX::XMFLOAT3(g, GridSize, -GridSize));
			Geometory::AddLine(DirectX::XMFLOAT3(-g, 0.0f, -GridSize), DirectX::XMFLOAT3(-g, GridSize, -GridSize));

			// 左面と右面の縦線
			Geometory::AddLine(DirectX::XMFLOAT3(GridSize, 0.0f, g), DirectX::XMFLOAT3(GridSize, GridSize, g));
			Geometory::AddLine(DirectX::XMFLOAT3(GridSize, 0.0f, -g), DirectX::XMFLOAT3(GridSize, GridSize, -g));
			Geometory::AddLine(DirectX::XMFLOAT3(-GridSize, 0.0f, g), DirectX::XMFLOAT3(-GridSize, GridSize, g));
			Geometory::AddLine(DirectX::XMFLOAT3(-GridSize, 0.0f, -g), DirectX::XMFLOAT3(-GridSize, GridSize, -g));
		}

		// ----------------------------------------------------
		// 中心線だけ赤くする
		// ----------------------------------------------------
		Geometory::SetColor(DirectX::XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f)); // 赤

		// 床の十字 (X軸, Z軸)
		Geometory::AddLine(DirectX::XMFLOAT3(-GridSize, 0.0f, 0.0f), DirectX::XMFLOAT3(GridSize, 0.0f, 0.0f));
		Geometory::AddLine(DirectX::XMFLOAT3(0.0f, 0.0f, -GridSize), DirectX::XMFLOAT3(0.0f, 0.0f, GridSize));

		// 壁の縦センターライン (奥・手前・左・右)
		// 奥壁 (Z = GridSize) の中心 (X=0)
		Geometory::AddLine(DirectX::XMFLOAT3(0.0f, 0.0f, GridSize), DirectX::XMFLOAT3(0.0f, GridSize, GridSize));
		// 手前壁 (Z = -GridSize) の中心 (X=0)
		Geometory::AddLine(DirectX::XMFLOAT3(0.0f, 0.0f, -GridSize), DirectX::XMFLOAT3(0.0f, GridSize, -GridSize));
		// 左壁 (X = -GridSize) の中心 (Z=0)
		Geometory::AddLine(DirectX::XMFLOAT3(-GridSize, 0.0f, 0.0f), DirectX::XMFLOAT3(-GridSize, GridSize, 0.0f));
		// 右壁 (X = GridSize) の中心 (Z=0)
		Geometory::AddLine(DirectX::XMFLOAT3(GridSize, 0.0f, 0.0f), DirectX::XMFLOAT3(GridSize, GridSize, 0.0f));


		Geometory::DrawLines();
	}

	// オブジェクト描画
	pCamera->Draw();
	pLight->Draw();
}

bool SceneRoot::isSceneChange()
{
	return m_isChangeScene;
}

std::string SceneRoot::GetSceneName()
{
	return m_sceneName;
}