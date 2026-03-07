#include "Main.h"
#include "Defines.h"
#include <memory>
#include "DirectX.h"
#include "Geometory.h"
#include "Sprite.h"
#include "Input.h"
#include "SceneRoot.h"

#include <system/imgui/imgui.h>
#include <system/imgui/imgui_impl_dx11.h>
#include <system/imgui/imgui_impl_win32.h>


//--- グローバル変数
std::shared_ptr<SceneRoot> g_pScene;

HRESULT Init(HWND hWnd, UINT width, UINT height)
{
	HRESULT hr;
	hr = InitDirectX(hWnd, width, height, false);
	if (FAILED(hr)) { return hr; }
	Geometory::Init();
	Sprite::Init();
	InitInput();

	// ImGuiの初期化
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // キーボードコントロールを有効化
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;  // ゲームパッドコントロールを有効化
	ImGui::StyleColorsDark(); // ダークスタイル
	// Win32とDirectX11バックエンドの初期化
	ImGui_ImplWin32_Init(hWnd);
	ImGui_ImplDX11_Init(GetDevice(), GetContext()); 


	// シーン作成
	g_pScene = std::make_shared<SceneRoot>();
	g_pScene->Init();
	SetWindowText(hWnd, (APP_TITLE + g_pScene->GetSceneName()).c_str());

	// 初期リソース作成
	auto rtv = g_pScene->CreateObj<RenderTarget>("RTV");
	rtv->CreateFromScreen();
	auto dsv = g_pScene->CreateObj<DepthStencil>("DSV");
	hr = dsv->Create(width, height, false);

	SetRenderTargets(1, &rtv, dsv);

	return hr;
}

void Uninit()
{
	// ImGuiの終了処理
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	g_pScene->Uninit();
	g_pScene.reset();
	UninitInput();
	Sprite::Uninit();
	Geometory::Uninit();
	UninitDirectX();
}

void Update(HWND hWnd, float tick)
{
	UpdateInput();
	g_pScene->_update(tick);

	if (g_pScene->isSceneChange()) {
		SetWindowText(hWnd, (APP_TITLE + g_pScene->GetSceneName()).c_str());
	}
}

void Draw()
{
	auto rtv = g_pScene->GetObj<RenderTarget>("RTV");
	auto dsv = g_pScene->GetObj<DepthStencil>("DSV");
	float color[4] = { 0.1f, 0.2f, 0.3f, 1.0f };
	GetContext()->ClearRenderTargetView(rtv->GetView(), color);
	GetContext()->ClearDepthStencilView(dsv->GetView(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);


	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
	g_pScene->_draw();

	
	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

	SwapDirectX();
}

// EOF