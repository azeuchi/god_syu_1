#include "DirectX.h"
#include "Texture.h"

// グローバル変数
ID3D11Device* g_pDevice;
ID3D11DeviceContext* g_pContext;
IDXGISwapChain* g_pSwapChain;
ID3D11RasterizerState* g_pRasterizerState[3];
ID3D11BlendState* g_pBlendState[BLEND_MAX];
ID3D11SamplerState* g_pSamplerState[SAMPLER_MAX];
ID3D11DepthStencilState* g_pDepthStencilState[DEPTH_MAX];

ID3D11Device* GetDevice()
{
	return g_pDevice;
}

ID3D11DeviceContext* GetContext()
{
	return g_pContext;
}

IDXGISwapChain* GetSwapChain()
{
	return g_pSwapChain;
}

HRESULT InitDirectX(HWND hWnd, UINT width, UINT height, bool fullscreen)
{
	HRESULT	hr = E_FAIL;
	DXGI_SWAP_CHAIN_DESC sd;
	ZeroMemory(&sd, sizeof(sd));						// ゼロクリア
	sd.BufferDesc.Width = width;						// バックバッファの幅
	sd.BufferDesc.Height = height;						// バックバッファの高さ
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;	// バックバッファフォーマット(R,G,B,A)
	sd.BufferDesc.RefreshRate.Numerator = 60;
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.BufferCount = 1;
	sd.OutputWindow = hWnd;
	sd.Windowed = !fullscreen;

	UINT createDeviceFlags = 0;
#ifdef _DEBUG
	createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	// Direct2D（フォント描画など）のキャンバスをDirect3Dの画面に重ねるために必要な設定
	createDeviceFlags |= D3D11_CREATE_DEVICE_BGRA_SUPPORT;

	D3D_FEATURE_LEVEL featureLevels[] =
	{
		D3D_FEATURE_LEVEL_11_0,
	};
	UINT numFeatureLevels = ARRAYSIZE(featureLevels);

	hr = D3D11CreateDeviceAndSwapChain(
		nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags,
		featureLevels, numFeatureLevels, D3D11_SDK_VERSION, &sd,
		&g_pSwapChain, &g_pDevice, nullptr, &g_pContext);

	if (FAILED(hr))
		return hr;

	// ポリゴンの裏表を描画するかどうかを制御するラスタライザステートの作成
	D3D11_RASTERIZER_DESC rd;
	ZeroMemory(&rd, sizeof(rd));
	rd.FillMode = D3D11_FILL_SOLID;
	rd.CullMode = D3D11_CULL_NONE;
	rd.DepthClipEnable = TRUE;
	rd.MultisampleEnable = FALSE;
	g_pDevice->CreateRasterizerState(&rd, &g_pRasterizerState[0]);

	rd.CullMode = D3D11_CULL_FRONT;
	g_pDevice->CreateRasterizerState(&rd, &g_pRasterizerState[1]);

	rd.CullMode = D3D11_CULL_BACK;
	g_pDevice->CreateRasterizerState(&rd, &g_pRasterizerState[2]);

	// 画像やエフェクトの重ね合わせ方（半透明、加算など）を制御するブレンドステートの作成
	D3D11_BLEND_DESC bd;
	ZeroMemory(&bd, sizeof(bd));
	bd.RenderTarget[0].BlendEnable = FALSE;
	bd.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
	g_pDevice->CreateBlendState(&bd, &g_pBlendState[BLEND_NONE]);

	bd.RenderTarget[0].BlendEnable = TRUE;
	bd.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	bd.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	bd.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	bd.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	bd.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
	bd.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	g_pDevice->CreateBlendState(&bd, &g_pBlendState[BLEND_ALPHA]);

	bd.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
	bd.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
	g_pDevice->CreateBlendState(&bd, &g_pBlendState[BLEND_ADD]);

	bd.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	bd.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
	g_pDevice->CreateBlendState(&bd, &g_pBlendState[BLEND_ADDALPHA]);

	bd.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	bd.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
	bd.RenderTarget[0].BlendOp = D3D11_BLEND_OP_REV_SUBTRACT;
	g_pDevice->CreateBlendState(&bd, &g_pBlendState[BLEND_SUB]);

	bd.RenderTarget[0].SrcBlend = D3D11_BLEND_INV_DEST_COLOR;
	bd.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
	bd.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	g_pDevice->CreateBlendState(&bd, &g_pBlendState[BLEND_SCREEN]);

	// テクスチャの拡大縮小時の補間方法（滑らかにするか、ドット感を残すか）を定義するサンプラーステートの作成
	D3D11_SAMPLER_DESC sampDesc;
	ZeroMemory(&sampDesc, sizeof(sampDesc));
	sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	sampDesc.MinLOD = 0;
	sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
	g_pDevice->CreateSamplerState(&sampDesc, &g_pSamplerState[SAMPLER_LINEAR]);

	sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
	g_pDevice->CreateSamplerState(&sampDesc, &g_pSamplerState[SAMPLER_POINT]);

	// 手前にある物体が奥の物体を隠す処理（Zバッファ）を制御する深度ステンシルステートの作成
	D3D11_DEPTH_STENCIL_DESC dsd;
	ZeroMemory(&dsd, sizeof(dsd));
	dsd.DepthEnable = TRUE;
	dsd.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	dsd.DepthFunc = D3D11_COMPARISON_LESS;
	dsd.StencilEnable = FALSE;
	g_pDevice->CreateDepthStencilState(&dsd, &g_pDepthStencilState[DEPTH_ENABLE_WRITE_TEST]);

	dsd.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	g_pDevice->CreateDepthStencilState(&dsd, &g_pDepthStencilState[DEPTH_ENABLE_TEST]);

	dsd.DepthEnable = FALSE;
	g_pDevice->CreateDepthStencilState(&dsd, &g_pDepthStencilState[DEPTH_DISABLE]);

	return S_OK;
}

void UninitDirectX()
{
	for (int i = 0; i < DEPTH_MAX; ++i) SAFE_RELEASE(g_pDepthStencilState[i]);
	for (int i = 0; i < SAMPLER_MAX; ++i) SAFE_RELEASE(g_pSamplerState[i]);
	for (int i = 0; i < BLEND_MAX; ++i) SAFE_RELEASE(g_pBlendState[i]);
	for (int i = 0; i < 3; ++i) SAFE_RELEASE(g_pRasterizerState[i]);

	SAFE_RELEASE(g_pSwapChain);
	SAFE_RELEASE(g_pContext);
	SAFE_RELEASE(g_pDevice);
}

void SwapDirectX()
{
	// 描画が完了したバックバッファを画面に転送する
	if (g_pSwapChain)
	{
		g_pSwapChain->Present(1, 0);
	}
}

void SetRenderTargets(UINT num, RenderTarget** ppViews, DepthStencil* pView)
{
	static ID3D11RenderTargetView* rtvs[4];

	if (num > 4) num = 4;
	for (UINT i = 0; i < num; ++i)
		rtvs[i] = ppViews[i]->GetView();
	g_pContext->OMSetRenderTargets(num, rtvs, pView ? pView->GetView() : nullptr);

	// ビューポートの設定
	D3D11_VIEWPORT vp;
	vp.TopLeftX = 0.0f;
	vp.TopLeftY = 0.0f;
	vp.Width = (float)ppViews[0]->GetWidth();
	vp.Height = (float)ppViews[0]->GetHeight();
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	g_pContext->RSSetViewports(1, &vp);
}

void SetCullingMode(D3D11_CULL_MODE cull)
{
	// 描画時にポリゴンのどちらの面を省略するかを設定する
	switch (cull)
	{
	case D3D11_CULL_NONE: g_pContext->RSSetState(g_pRasterizerState[0]); break;
	case D3D11_CULL_FRONT: g_pContext->RSSetState(g_pRasterizerState[1]); break;
	case D3D11_CULL_BACK: g_pContext->RSSetState(g_pRasterizerState[2]); break;
	}
}

void SetDepthTest(DepthState state)
{
	g_pContext->OMSetDepthStencilState(g_pDepthStencilState[state], 0);
}

void SetBlendMode(BlendMode mode)
{
	float blendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	g_pContext->OMSetBlendState(g_pBlendState[mode], blendFactor, 0xFFFFFFFF);
}