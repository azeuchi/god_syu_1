#include "SceneRoot.h"
#include <stdio.h>
#include "CameraDCC.h"
#include "MoveLight.h"
#include "Model.h"
#include "Input.h"
#include "Geometory.h"
#include "Sprite.h"
#include "DirectX.h"


#include "SceneGame.h"
#include "SceneDebug.h" 
#include "SceneTitle.h" 
#include "SceneResult.h"
#include "SceneKeyConfig.h"
#include "SceneTraining.h"
#include "DebugLog.h"

#define STR(var) #var

float SceneRoot::s_sceneFade = 0.0f;

// フェードの速さ。1/この値 秒で暗転しきる
static const float FADE_SPEED = 2.5f;

//--- 定数定義
enum SceneKind
{
	SCENE_TITLE,    // タイトル
	SCENE_KEYCONFIG,// キーコンフィグ
	SCENE_GAME,		// ゲーム本編
	SCENE_RESULT,   // リザルト
	SCENE_DEBUG,    // デバッグ
	SCENE_TRAINING, // トレーニング
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

	case SCENE_KEYCONFIG:
		AddSubScene<SceneKeyConfig>();
		m_sceneName = "SCENE_KEYCONFIG";
		break;

	case SCENE_GAME:
		AddSubScene<SceneGame>();
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

	case SCENE_TRAINING:
		AddSubScene<SceneTraining>();
		m_sceneName = "SCENE_TRAINING";
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
	// すぐには切り替えず、フェードアウト完了後に切り替える。多重要求は無視
	if (m_pendingScene != -1) return;
	m_pendingScene = nextScene;
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

	// カメラのリセット
	pCamera->SetPos(DirectX::XMFLOAT3(0.0f, 1.0f, -5.0f));
	pCamera->SetLook(DirectX::XMFLOAT3(0.0f, 1.0f, 0.0f));
	pCamera->SetUp(DirectX::XMFLOAT3(0.0f, 1.0f, 0.0f));


	// フェード用のブレンド・深度ステート
	D3D11_BLEND_DESC bd = {};
	bd.RenderTarget[0].BlendEnable = TRUE;
	bd.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	bd.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	bd.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	bd.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	bd.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
	bd.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	bd.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
	GetDevice()->CreateBlendState(&bd, &m_pFadeBlend);

	D3D11_DEPTH_STENCIL_DESC dd = {};
	dd.DepthEnable = TRUE;
	dd.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	dd.DepthFunc = D3D11_COMPARISON_ALWAYS;
	GetDevice()->CreateDepthStencilState(&dd, &m_pFadeDepth);

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

	if (m_pFadeBlend) { m_pFadeBlend->Release(); m_pFadeBlend = nullptr; }
	if (m_pFadeDepth) { m_pFadeDepth->Release(); m_pFadeDepth = nullptr; }
}

void SceneRoot::Update(float tick)
{
	m_isChangeScene = false;
	CameraBase* pCamera = GetObj<CameraBase>("Camera");
	LightBase* pLight = GetObj<LightBase>("Light");

	// カメラとライトの更新（全シーン共通）
	pCamera->Update();
	pLight->Update();

	// フェード進行。tickが跳ねてもフェードが飛ばないよう上限をかける
	float fadeTick = (tick > 0.033f) ? 0.033f : tick;
	if (m_pendingScene != -1)
	{
		if (m_fadeAlpha >= 1.0f)
		{
			// 真っ黒の1フレームを表示してから切り替える。ロードの止まりを黒画面で隠すため
			m_index = m_pendingScene;
			m_pendingScene = -1;
			s_sceneFade = 0.0f;
			RemoveSubScene();
			ChangeScene();
		}
		else
		{
			m_fadeAlpha += fadeTick * FADE_SPEED;
			if (m_fadeAlpha > 1.0f) m_fadeAlpha = 1.0f;
		}
	}
	else if (m_fadeAlpha > 0.0f)
	{
		m_fadeAlpha -= fadeTick * FADE_SPEED;
		if (m_fadeAlpha < 0.0f) m_fadeAlpha = 0.0f;
	}


	//----------------------------------------------------------
	// シーン遷移ロジック
	// 現在のシーンによって、遷移条件を切り替える
	//----------------------------------------------------------
	switch (m_index)
	{
		//  タイトル画面の時
	case SCENE_TITLE:
		if (IsKeyTrigger(VK_RETURN))
		{
			Transition(SCENE_KEYCONFIG);
		}
		break;

	case SCENE_KEYCONFIG:
		if (SceneKeyConfig::s_isConfigSet)
		{
			Transition(SCENE_GAME);
			SceneKeyConfig::s_isConfigSet = false;
		}
		// メニューの Debug Room からデバッグ画面へ（リリースでも有効）
		if (SceneKeyConfig::s_requestDebug)
		{
			SceneKeyConfig::s_requestDebug = false;
			Transition(SCENE_DEBUG);
		}
		break;

		// ゲーム本編の時
	case SCENE_GAME:
#ifdef _DEBUG
		// Nキーでデバッグ画面
		if (IsKeyTrigger('N'))
		{
			Transition(SCENE_DEBUG);
		}
#endif

		// ゲームセットになったらリザルトへ (静的フラグをチェック)
		if (SceneGame::s_isGameSet)
		{
			Transition(SCENE_RESULT);
			SceneGame::s_isGameSet = false; // フラグを戻しておく
		}

		break;

		// デバッグ画面の時
	case SCENE_DEBUG:
		// Nキーでゲーム画面に戻る
		if (IsKeyTrigger('N'))
		{
			Transition(SCENE_GAME);
		}
		// GUIボタン「Go to Training」でトレーニングへ
		if (SceneTraining::s_requestEnter)
		{
			SceneTraining::s_requestEnter = false;
			Transition(SCENE_TRAINING);
		}
		break;

		// トレーニング画面の時
	case SCENE_TRAINING:
		// GUIボタンでゲーム/デバッグへ
		if (SceneTraining::s_requestGoGame)
		{
			SceneTraining::s_requestGoGame = false;
			Transition(SCENE_GAME);
		}
		if (SceneTraining::s_requestGoDebug)
		{
			SceneTraining::s_requestGoDebug = false;
			Transition(SCENE_DEBUG);
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

	// フェードの黒い全画面矩形。グリッド線も含めた画面全体に最後にかける
	// シーン遷移とゲーム内演出（ラウンド切替）の濃い方を採用する
	float fade = (m_fadeAlpha > s_sceneFade) ? m_fadeAlpha : s_sceneFade;
	if (fade > 0.0f)
	{
		float bf[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
		GetContext()->OMSetBlendState(m_pFadeBlend, bf, 0xffffffff);
		GetContext()->OMSetDepthStencilState(m_pFadeDepth, 0);

		DirectX::XMFLOAT4X4 ident, world;
		DirectX::XMStoreFloat4x4(&ident, DirectX::XMMatrixIdentity());
		DirectX::XMStoreFloat4x4(&world, DirectX::XMMatrixTranspose(DirectX::XMMatrixScaling(2.0f, 2.0f, 1.0f)));
		Sprite::SetWorld(world);
		Sprite::SetView(ident);
		Sprite::SetProjection(ident);
		Sprite::SetTexture(nullptr);
		Sprite::SetOffset({ 0.0f, 0.0f });
		Sprite::SetSize({ 1.0f, 1.0f });
		Sprite::SetUVPos({ 0.0f, 0.0f });
		Sprite::SetUVScale({ 1.0f, 1.0f });
		Sprite::SetColor({ 0.0f, 0.0f, 0.0f, fade });
		Sprite::Draw();

		GetContext()->OMSetBlendState(nullptr, bf, 0xffffffff);
		GetContext()->OMSetDepthStencilState(nullptr, 0);
	}
}

bool SceneRoot::isSceneChange()
{
	return m_isChangeScene;
}

std::string SceneRoot::GetSceneName()
{
	return m_sceneName;
}