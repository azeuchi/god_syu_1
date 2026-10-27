#pragma once
#include <DirectXMath.h>
#include <DirectXCollision.h>
#include <memory>
#include <vector>
#include "Texture.h"

class SimpleUI
{
public:
    // UI矩形の情報
    struct Rect
    {
        float x, y, w, h;
        DirectX::XMFLOAT4 color;
        Texture* texture = nullptr; // 画像用ポインタ
        DirectX::XMFLOAT2 uvPos = { 0.0f, 0.0f };   // 画像の切り出し開始位置（0?1）
        DirectX::XMFLOAT2 uvScale = { 1.0f, 1.0f }; // 画像の切り出し範囲（0?1）
    };

    // 矩形UIを追加
    //static void AddRect(float x, float y, float w, float h, DirectX::XMFLOAT4 color);

    // 矩形UI
    static void AddRect(float x, float y, float w, float h, DirectX::XMFLOAT4 color, Texture* tex = nullptr);

    // 画像の一部だけを切り出して貼る矩形UI（HPバーの減った分を削るのに使う）
    static void AddRect(float x, float y, float w, float h, DirectX::XMFLOAT4 color, Texture* tex,
                        DirectX::XMFLOAT2 uvPos, DirectX::XMFLOAT2 uvScale);

    // 全UI描画
    static void DrawAll();

    // 全UIクリア
    static void Clear();

    static void SetMatrix(const DirectX::XMFLOAT4X4& world,
                          const DirectX::XMFLOAT4X4& view,
                          const DirectX::XMFLOAT4X4& proj);

private:
    static std::vector<Rect> s_rects;
};