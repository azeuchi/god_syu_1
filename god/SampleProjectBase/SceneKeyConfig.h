#pragma once
#include "SceneBase.hpp"
#include <vector>
#include <DirectXMath.h>
#include <d3d11.h>
#include "Player.h"

class HitEffect;
class SkyDome;

// ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽj・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ[・ｽE・ｽﾌ階・ｽE・ｽw・ｽE・ｽ・ｽE・ｽﾔゑｿｽ\・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ東^
enum class MenuState
{
	TopMenu,     // ・ｽE・ｽg・ｽE・ｽb・ｽE・ｽv・ｽE・ｽ・ｽE・ｽ・ｽE・ｽj・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ[
	ConfigP1,    // 1P・ｽE・ｽﾌキ・ｽE・ｽ[・ｽE・ｽR・ｽE・ｽ・ｽE・ｽ・ｽE・ｽt・ｽE・ｽB・ｽE・ｽO・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ
	ConfigP2,    // 2P・ｽE・ｽﾌキ・ｽE・ｽ[・ｽE・ｽR・ｽE・ｽ・ｽE・ｽ・ｽE・ｽt・ｽE・ｽB・ｽE・ｽO・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ
	TrainingMode // ・ｽE・ｽg・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ[・ｽE・ｽj・ｽE・ｽ・ｽE・ｽ・ｽE・ｽO・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ[・ｽE・ｽh・ｽE・ｽiUI・ｽE・ｽ・ｽE・ｽ\・ｽE・ｽ・ｽE・ｽ・ｽE・ｽﾅ趣ｿｽ・ｽE・ｽR・ｽE・ｽﾉ難ｿｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽj
};

// ・ｽE・ｽg・ｽE・ｽb・ｽE・ｽv・ｽE・ｽ・ｽE・ｽ・ｽE・ｽj・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ[・ｽE・ｽﾌ搾ｿｽ・ｽE・ｽﾚ擾ｿｽ・ｽE・ｽ (DirectWrite・ｽE・ｽﾉ搾ｿｽ・ｽE・ｽ・ｽ・ｽ・ｽ・ｽE・ｽﾄ・・ｽ・ｽ・ｽE・ｽC・ｽE・ｽh・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽﾉゑｿｽ・ｽE・ｽ・ｽE・ｽ)
struct TopMenuItem
{
	const wchar_t* label;
};

// ・ｽE・ｽR・ｽE・ｽ・ｽE・ｽ・ｽE・ｽt・ｽE・ｽB・ｽE・ｽO・ｽE・ｽ・ｽE・ｽﾊの搾ｿｽ・ｽE・ｽﾚ擾ｿｽ・ｽE・ｽ (DirectWrite・ｽE・ｽﾉ搾ｿｽ・ｽE・ｽ・ｽ・ｽ・ｽ・ｽE・ｽﾄ・・ｽ・ｽ・ｽE・ｽC・ｽE・ｽh・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽﾉゑｿｽ・ｽE・ｽ・ｽE・ｽ)
struct ConfigItem
{
	const wchar_t* label; // ・ｽE・ｽ・ｽE・ｽﾊに表・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽe・ｽE・ｽL・ｽE・ｽX・ｽE・ｽg
	int* keyPtr;          // ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ闢厄ｿｽﾄゑｿｽﾏ更・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽL・ｽE・ｽ[・ｽE・ｽﾏ撰ｿｽ・ｽE・ｽﾌポ・ｽE・ｽC・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ^
	bool isDeviceSelect;  // ・ｽE・ｽf・ｽE・ｽo・ｽE・ｽC・ｽE・ｽX・ｽE・ｽﾘゑｿｽﾖゑｿｽ・ｽE・ｽp・ｽE・ｽﾌ搾ｿｽ・ｽE・ｽﾚゑｿｽ・ｽE・ｽﾇゑｿｽ・ｽE・ｽ・ｽE・ｽ
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
	// ・ｽE・ｽw・ｽE・ｽ閧ｵ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽs・ｽE・ｽN・ｽE・ｽZ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽW・ｽE・ｽﾆサ・ｽE・ｽC・ｽE・ｽY・ｽE・ｽﾅ単・ｽE・ｽF・ｽE・ｽﾌ四・ｽE・ｽp・ｽE・ｽ`・ｽE・ｽ・ｽE・ｽ`・ｽE・ｽ謔ｷ・ｽE・ｽ・ｽE・ｽ
	void DrawRectPixel(float px, float py, float pw, float ph, DirectX::XMFLOAT4 color);

	// ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽz・ｽE・ｽL・ｽE・ｽ[・ｽE・ｽR・ｽE・ｽ[・ｽE・ｽh・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ\・ｽE・ｽ・ｽE・ｽ・ｽE・ｽp・ｽE・ｽﾌ包ｿｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ謫ｾ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ
	const wchar_t* GetKeyName(int vk);

	// ・ｽE・ｽR・ｽE・ｽ・ｽE・ｽ・ｽE・ｽg・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ[・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ[・ｽE・ｽﾌボ・ｽE・ｽ^・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ\・ｽE・ｽ・ｽE・ｽ・ｽE・ｽp・ｽE・ｽﾌ包ｿｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ謫ｾ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ
	const wchar_t* GetPadButtonName(int button);

	// ・ｽE・ｽI・ｽE・ｽ・ｽEﾌデ・ｽE・ｽo・ｽE・ｽC・ｽE・ｽX・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ謫ｾ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ
	const wchar_t* GetDeviceName(InputDeviceType type);

	// ・ｽE・ｽﾝ抵ｿｽ|・ｽE・ｽC・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ^・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽﾝのデ・ｽE・ｽo・ｽE・ｽC・ｽE・ｽX・ｽE・ｽﾉ搾ｿｽ・ｽE・ｽ・ｽ・ｽ・ｽ・ｽE・ｽﾄ更・ｽE・ｽV・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ
	void RefreshConfigPointers();

	MenuState m_menuState;

	// ・ｽE・ｽg・ｽE・ｽb・ｽE・ｽv・ｽE・ｽ・ｽE・ｽ・ｽE・ｽj・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ[・ｽE・ｽp・ｽE・ｽﾏ撰ｿｽ
	int m_topSelectedIndex;
	std::vector<TopMenuItem> m_topItems;

	// ・ｽE・ｽR・ｽE・ｽ・ｽE・ｽ・ｽE・ｽt・ｽE・ｽB・ｽE・ｽO・ｽE・ｽ・ｽE・ｽ・ｽE・ｽj・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ[・ｽE・ｽp・ｽE・ｽﾏ撰ｿｽ
	int m_configSelectedIndex;
	std::vector<ConfigItem> m_p1Items;
	std::vector<ConfigItem> m_p2Items;

	// ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽj・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ[・ｽE・ｽW・ｽE・ｽJ・ｽE・ｽA・ｽE・ｽj・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ[・ｽE・ｽV・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽﾌ縦・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽX・ｽE・ｽP・ｽE・ｽ[・ｽE・ｽ・ｽE・ｽ (0.0f ・ｽE・ｽ` 1.0f)
	float m_windowScaleY;

	// ・ｽE・ｽL・ｽE・ｽ[・ｽE・ｽ・ｽE・ｽ・ｽE・ｽﾍ待機・ｽE・ｽ・ｽE・ｽ・ｽE・ｽﾌキ・ｽE・ｽ[・ｽE・ｽﾏ撰ｿｽ・ｽE・ｽﾖのポ・ｽE・ｽC・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ^・ｽE・ｽinullptr・ｽE・ｽﾈゑｿｽﾒ機・ｽE・ｽ・ｽE・ｽ・ｽE・ｽﾄゑｿｽ・ｽE・ｽﾈゑｿｽ・ｽE・ｽj
	int* m_waitBindKeyPtr = nullptr;

	std::vector<HitEffect*> m_hitEffects;

	ID3D11RasterizerState* m_pCullFront = nullptr;
	ID3D11RasterizerState* m_pCullBack = nullptr;

	// ・ｽw・ｽi・ｽX・ｽJ・ｽC・ｽh・ｽ[・ｽ・ｽ・ｽi・ｽQ・ｽ[・ｽ・ｽ・ｽV・ｽ[・ｽ・ｽ・ｽﾆ難ｿｽ・ｽ・ｽ・ｽ・ｽ・ｽ・ｽ・ｽﾚにゑｿｽ・ｽ驍ｽ・ｽﾟ）
	SkyDome* m_skyDome = nullptr;
	// ・ｽX・ｽJ・ｽC・ｽh・ｽ[・ｽ・ｽ・ｽ`・ｽ・ｽp・ｽi・ｽJ・ｽ・ｽ・ｽ・ｽ・ｽO・ｽﾈゑｿｽ・ｽj
	ID3D11RasterizerState* m_pCullNone = nullptr;
	// スカイドーム(最奥)描画用：LESS_EQUAL の深度ステート
	ID3D11DepthStencilState* m_pDepthState3D = nullptr;
};