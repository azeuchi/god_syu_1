#pragma once
#include "SceneBase.hpp"
#include <vector>
#include <DirectXMath.h>
#include <d3d11.h>
#include "Player.h"

class HitEffect;

// メニューの階層状態を表す列挙型
enum class MenuState
{
	TopMenu,     // トップメニュー
	ConfigP1,    // 1Pのキーコンフィグ画面
	ConfigP2,    // 2Pのキーコンフィグ画面
	TrainingMode // トレーニングモード（UI非表示で自由に動かせる）
};

// トップメニューの項目情報 (DirectWriteに合わせてワイド文字列にする)
struct TopMenuItem
{
	const wchar_t* label;
};

// コンフィグ画面の項目情報 (DirectWriteに合わせてワイド文字列にする)
struct ConfigItem
{
	const wchar_t* label; // 画面に表示するテキスト
	int* keyPtr;          // 割り当てを変更するキー変数のポインタ
	bool isDeviceSelect;  // デバイス切り替え用の項目かどうか
};

class SceneKeyConfig : public SceneBase
{
public:
	void Init();
	void Uninit();
	void Update(float tick);
	void Draw();

	static bool s_isConfigSet;

private:
	// 指定したピクセル座標とサイズで単色の四角形を描画する
	void DrawRectPixel(float px, float py, float pw, float ph, DirectX::XMFLOAT4 color);

	// 仮想キーコードから表示用の文字列を取得する
	const wchar_t* GetKeyName(int vk);

	// コントローラーのボタンから表示用の文字列を取得する
	const wchar_t* GetPadButtonName(int button);

	// 選択中のデバイス名を取得する
	const wchar_t* GetDeviceName(InputDeviceType type);

	// 設定ポインタを現在のデバイスに合わせて更新する
	void RefreshConfigPointers();

	MenuState m_menuState;

	// トップメニュー用変数
	int m_topSelectedIndex;
	std::vector<TopMenuItem> m_topItems;

	// コンフィグメニュー用変数
	int m_configSelectedIndex;
	std::vector<ConfigItem> m_p1Items;
	std::vector<ConfigItem> m_p2Items;

	// メニュー展開アニメーションの縦方向スケール (0.0f ～ 1.0f)
	float m_windowScaleY;

	// キー入力待機中のキー変数へのポインタ（nullptrなら待機していない）
	int* m_waitBindKeyPtr = nullptr;

	std::vector<HitEffect*> m_hitEffects;

	ID3D11RasterizerState* m_pCullFront = nullptr;
	ID3D11RasterizerState* m_pCullBack = nullptr;
};