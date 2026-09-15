#pragma once
#include "SceneBase.hpp"
#include <memory>
#include <vector>
#include <DirectXMath.h>
#include <d3d11.h>
#include "Player.h"
#include "PlayerRenderer.h"
#include "HitEffectPool.h"
#include "MatchConfig.h"

class HitEffect;
class SkyDome;

// メニューの階層状態を表す列挙型
enum class MenuState
{
	TopMenu,     // トップメニュー
	ConfigP1,    // 1Pのキーコンフィグ画面
	ConfigP2,    // 2Pのキーコンフィグ画面
	TrainingMode, // トレーニングモード（UI非表示で自由に動かせる）
	CpuSelect     // CPU戦の強さを選ぶ
};

// トップメニューで決定したときの動作
enum class TopMenuAction
{
	ConfigP1,
	ConfigP2,
	Training,
	VsPlayer,
	VsCpu,
	Debug
};

// トップメニューの項目情報 (DirectWriteに合わせてワイド文字列にする)
struct TopMenuItem
{
	const wchar_t* label;
	TopMenuAction action; // 並び順を変えても動作がずれないよう、番号ではなく動作で持つ
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

	// SkyDome を前方宣言のまま unique_ptr で持つため、デストラクタは cpp 側で定義する
	~SceneKeyConfig();

	static bool s_isConfigSet;
	// デバッグシーンへの遷移要求（SceneRootが監視）
	static bool s_requestDebug;

private:
	// 指定したピクセル座標とサイズで単色の四角形を描画する
	void DrawRectPixel(float px, float py, float pw, float ph, DirectX::XMFLOAT4 color);

	// CPU戦の強さを選ぶ小窓を、VS CPU の項目の下に描く
	void DrawCpuSelect();

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
	int m_cpuSelectedIndex = (int)CpuLevel::NORMAL;
	std::vector<TopMenuItem> m_topItems;

	// コンフィグメニュー用変数
	int m_configSelectedIndex;
	std::vector<ConfigItem> m_p1Items;
	std::vector<ConfigItem> m_p2Items;

	// メニュー展開アニメーションの縦方向スケール (0.0f ～ 1.0f)
	float m_windowScaleY;

	// キー入力待機中のキー変数へのポインタ（nullptrなら待機していない）
	int* m_waitBindKeyPtr = nullptr;

	HitEffectPool m_hitEffects;

	// プレイヤー描画の共通ヘルパー
	PlayerRenderer m_playerRenderer;

	// 背景スカイドーム
	std::unique_ptr<SkyDome> m_skyDome;
};