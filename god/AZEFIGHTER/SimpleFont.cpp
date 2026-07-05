#include "SimpleFont.h"
#include <d2d1.h>
#include <dwrite.h>
#include <string>
#include <wchar.h>

// Direct2DとDirectWriteのライブラリをリンク
#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")

static ID3D11Device* g_d3dDevice = nullptr;
static ID3D11DeviceContext* g_d3dContext = nullptr;

static ID2D1Factory* g_d2dFactory = nullptr;
static IDWriteFactory* g_dwriteFactory = nullptr;
static ID2D1RenderTarget* g_d2dRenderTarget = nullptr;
static ID2D1SolidColorBrush* g_brush = nullptr;

// --- D3D11の状態保存用変数 ---
static ID3D11BlendState* g_blendState = nullptr;
static FLOAT g_blendFactor[4];
static UINT g_sampleMask;
static ID3D11DepthStencilState* g_depthStencilState = nullptr;
static UINT g_stencilRef;
static ID3D11RasterizerState* g_rasterizerState = nullptr;
static ID3D11InputLayout* g_inputLayout = nullptr;
static D3D11_PRIMITIVE_TOPOLOGY g_topology;
static ID3D11Buffer* g_vertexBuffers[4] = { nullptr };
static UINT g_strides[4] = { 0 };
static UINT g_offsets[4] = { 0 };
static ID3D11Buffer* g_indexBuffer = nullptr;
static DXGI_FORMAT g_indexFormat;
static UINT g_indexOffset = 0;
static ID3D11VertexShader* g_vs = nullptr;
static ID3D11PixelShader* g_ps = nullptr;
static ID3D11GeometryShader* g_gs = nullptr;
static ID3D11ShaderResourceView* g_psSRV = nullptr;
static ID3D11SamplerState* g_psSampler = nullptr;

void SimpleFont::Init(ID3D11Device* device, ID3D11DeviceContext* context)
{
    g_d3dDevice = device;
    g_d3dContext = context;

    D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &g_d2dFactory);
    DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), reinterpret_cast<IUnknown**>(&g_dwriteFactory));
}

void SimpleFont::Uninit()
{
    if (g_brush) { g_brush->Release(); g_brush = nullptr; }
    if (g_d2dRenderTarget) { g_d2dRenderTarget->Release(); g_d2dRenderTarget = nullptr; }
    if (g_dwriteFactory) { g_dwriteFactory->Release(); g_dwriteFactory = nullptr; }
    if (g_d2dFactory) { g_d2dFactory->Release(); g_d2dFactory = nullptr; }
}

void SimpleFont::Begin()
{
    if (g_d2dFactory == nullptr) return;

    // Direct2Dに設定を書き換えられる前に、現在のD3D11の全設定を保存する
    g_d3dContext->OMGetBlendState(&g_blendState, g_blendFactor, &g_sampleMask);
    g_d3dContext->OMGetDepthStencilState(&g_depthStencilState, &g_stencilRef);
    g_d3dContext->RSGetState(&g_rasterizerState);
    g_d3dContext->IAGetInputLayout(&g_inputLayout);
    g_d3dContext->IAGetPrimitiveTopology(&g_topology);
    g_d3dContext->IAGetVertexBuffers(0, 4, g_vertexBuffers, g_strides, g_offsets);
    g_d3dContext->IAGetIndexBuffer(&g_indexBuffer, &g_indexFormat, &g_indexOffset);
    g_d3dContext->VSGetShader(&g_vs, nullptr, nullptr);
    g_d3dContext->PSGetShader(&g_ps, nullptr, nullptr);
    g_d3dContext->GSGetShader(&g_gs, nullptr, nullptr);
    g_d3dContext->PSGetShaderResources(0, 1, &g_psSRV);
    g_d3dContext->PSGetSamplers(0, 1, &g_psSampler);

    // レンダーターゲットの作成（初回のみ）
    if (g_d2dRenderTarget == nullptr)
    {
        ID3D11RenderTargetView* rtv = nullptr;
        g_d3dContext->OMGetRenderTargets(1, &rtv, nullptr);
        if (rtv)
        {
            ID3D11Resource* res = nullptr;
            rtv->GetResource(&res);
            if (res)
            {
                IDXGISurface* surface = nullptr;
                res->QueryInterface(__uuidof(IDXGISurface), (void**)&surface);
                if (surface)
                {
                    D2D1_RENDER_TARGET_PROPERTIES props = D2D1::RenderTargetProperties(
                        D2D1_RENDER_TARGET_TYPE_DEFAULT,
                        D2D1::PixelFormat(DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_PREMULTIPLIED)
                    );

                    g_d2dFactory->CreateDxgiSurfaceRenderTarget(surface, &props, &g_d2dRenderTarget);
                    if (g_d2dRenderTarget)
                    {
                        g_d2dRenderTarget->CreateSolidColorBrush(D2D1::ColorF(1, 1, 1, 1), &g_brush);
                    }
                    surface->Release();
                }
                res->Release();
            }
            rtv->Release();
        }
    }

    if (g_d2dRenderTarget)
    {
        g_d2dRenderTarget->BeginDraw();
    }
}

void SimpleFont::End()
{
    if (g_d2dRenderTarget)
    {
        g_d2dRenderTarget->EndDraw();
    }

    // Direct2Dの描画が終わったので、保存しておいたD3D11の設定を全て復元する
    g_d3dContext->OMSetBlendState(g_blendState, g_blendFactor, g_sampleMask);
    g_d3dContext->OMSetDepthStencilState(g_depthStencilState, g_stencilRef);
    g_d3dContext->RSSetState(g_rasterizerState);
    g_d3dContext->IASetInputLayout(g_inputLayout);
    g_d3dContext->IASetPrimitiveTopology(g_topology);
    g_d3dContext->IASetVertexBuffers(0, 4, g_vertexBuffers, g_strides, g_offsets);
    g_d3dContext->IASetIndexBuffer(g_indexBuffer, g_indexFormat, g_indexOffset);
    g_d3dContext->VSSetShader(g_vs, nullptr, 0);
    g_d3dContext->PSSetShader(g_ps, nullptr, 0);
    g_d3dContext->GSSetShader(g_gs, nullptr, 0);
    g_d3dContext->PSSetShaderResources(0, 1, &g_psSRV);
    g_d3dContext->PSSetSamplers(0, 1, &g_psSampler);

    // 参照カウントを減らす
    if (g_blendState) { g_blendState->Release(); g_blendState = nullptr; }
    if (g_depthStencilState) { g_depthStencilState->Release(); g_depthStencilState = nullptr; }
    if (g_rasterizerState) { g_rasterizerState->Release(); g_rasterizerState = nullptr; }
    if (g_inputLayout) { g_inputLayout->Release(); g_inputLayout = nullptr; }
    for (int i = 0; i < 4; ++i) { if (g_vertexBuffers[i]) { g_vertexBuffers[i]->Release(); g_vertexBuffers[i] = nullptr; } }
    if (g_indexBuffer) { g_indexBuffer->Release(); g_indexBuffer = nullptr; }
    if (g_vs) { g_vs->Release(); g_vs = nullptr; }
    if (g_ps) { g_ps->Release(); g_ps = nullptr; }
    if (g_gs) { g_gs->Release(); g_gs = nullptr; }
    if (g_psSRV) { g_psSRV->Release(); g_psSRV = nullptr; }
    if (g_psSampler) { g_psSampler->Release(); g_psSampler = nullptr; }
}

void SimpleFont::Draw(const wchar_t* text, float x, float y, float size, DirectX::XMFLOAT4 color, const wchar_t* fontName)
{
    if (!g_d2dRenderTarget || !g_dwriteFactory || !g_brush) return;

    g_brush->SetColor(D2D1::ColorF(color.x, color.y, color.z, color.w));

    IDWriteTextFormat* format = nullptr;

    // まず指定されたフォント（Noto Sans JP）で作成を試みる
    HRESULT hr = g_dwriteFactory->CreateTextFormat(
        fontName, nullptr,
        DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
        size, L"ja-jp", &format
    );

    // もしフォント名が違うなどで失敗した場合、Windows標準の「メイリオ」で安全に表示する
    if (FAILED(hr))
    {
        g_dwriteFactory->CreateTextFormat(
            L"Meiryo", nullptr,
            DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
            size, L"ja-jp", &format
        );
    }

    if (format)
    {
        D2D1_RECT_F rect = D2D1::RectF(x, y, x + 2000.0f, y + 2000.0f);
        g_d2dRenderTarget->DrawText(text, (UINT32)wcslen(text), format, &rect, g_brush);
        format->Release();
    }
}