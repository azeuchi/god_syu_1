#pragma once
#include <d3d11.h>
#include <DirectXMath.h>

// DirectWriteおよびDirect2Dを利用してフォントを描画するクラス
class SimpleFont
{
public:
    // 初期化 (D2DとDWriteのファクトリを作成)
    static void Init(ID3D11Device* device, ID3D11DeviceContext* context);

    // 終了処理
    static void Uninit();

    // 描画開始 (現在のバックバッファをD2Dのキャンバスとして取得する)
    static void Begin();

    // 描画終了
    static void End();

    // 文字列の描画 (DirectWriteは標準でUTF-16のワイド文字(wchar_t)を使用する)
    // fontNameにはWindowsにインストールされているフォント名(例: L"Noto Sans JP")を指定する
    static void Draw(const wchar_t* text, float x, float y, float size = 24.0f, DirectX::XMFLOAT4 color = { 1.0f, 1.0f, 1.0f, 1.0f }, const wchar_t* fontName = L"Noto Sans JP");
};