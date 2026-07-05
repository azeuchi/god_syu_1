#include "SimpleUI.h"
#include "Sprite.h"
#include "DebugLog.h" 

static DirectX::XMFLOAT4X4 s_world, s_view, s_proj;

std::vector<SimpleUI::Rect> SimpleUI::s_rects;

void SimpleUI::AddRect(float x, float y, float w, float h, DirectX::XMFLOAT4 color, Texture* tex)
{
    s_rects.push_back({ x, y, w, h, color, tex });
}

void SimpleUI::SetMatrix(const DirectX::XMFLOAT4X4& world,
                         const DirectX::XMFLOAT4X4& view,
                         const DirectX::XMFLOAT4X4& proj)
{
    s_world = world;
    s_view = view;
    s_proj = proj;
}

void SimpleUI::DrawAll()
{
    static bool logged = false; // ログ一度だけ用

    // 行列をセット
    Sprite sprite;
    sprite.SetWorld(s_world);
    sprite.SetView(s_view);
    sprite.SetProjection(s_proj);

    for (const auto& rect : s_rects)
    {
        sprite.SetOffset({ rect.x, rect.y });
        sprite.SetSize({ rect.w, rect.h });
        sprite.SetColor(rect.color);
        sprite.SetTexture(rect.texture);

        // 画像付きUIの場合ログ出力（1回だけ）
        if (rect.texture != nullptr && !logged) {
            DebugLog::log(DebugLog::INFO_LOG, "画像付きUIを描画 x=", rect.x, ", y=", rect.y, ", w=", rect.w, ", h=", rect.h);
            logged = true;
        }

        sprite.Draw();
    }
}

void SimpleUI::Clear()
{
    s_rects.clear();
}