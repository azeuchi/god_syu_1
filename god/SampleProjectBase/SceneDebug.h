#ifndef __SCENE_DEBUG_H__
#define __SCENE_DEBUG_H__

#include "SceneBase.hpp"
#include <d3d11.h>

// ・ｽE・ｽO・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ骭ｾ・ｽE・ｽi・ｽE・ｽ・ｽE・ｽ・ｽE・ｽﾌゑｿｽ Player.h ・ｽE・ｽﾅ抵ｿｽ`・ｽE・ｽj
class Player;
struct AttackParams;
class SkyDome;

/**
 * @brief ・ｽE・ｽf・ｽE・ｽo・ｽE・ｽb・ｽE・ｽO・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽp・ｽE・ｽV・ｽE・ｽ[・ｽE・ｽ・ｽE・ｽ
 * * ・ｽE・ｽQ・ｽE・ｽ[・ｽE・ｽ・ｽE・ｽ・ｽE・ｽﾌス・ｽE・ｽe・ｽE・ｽ[・ｽE・ｽg・ｽE・ｽ}・ｽE・ｽV・ｽE・ｽ・ｽE・ｽ(FSM)・ｽE・ｽ・ｽE・ｽ・ｽE・ｽg・ｽE・ｽ・ｽ・ｽ・ｽ・ｽE・ｽA・ｽE・ｽ闢ｮ・ｽE・ｽﾅア・ｽE・ｽj・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ[・ｽE・ｽV・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ笏ｻ・ｽE・ｽ・ｽE・ｽｧ御す・ｽE・ｽ・ｽE・ｽV・ｽE・ｽ[・ｽE・ｽ・ｽE・ｽ・ｽE・ｽB
 * ・ｽE・ｽﾈ会ｿｽ・ｽE・ｽﾌ機・ｽE・ｽ\・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ:
 * 1. ・ｽE・ｽU・ｽE・ｽ・ｽE・ｽ・ｽE・ｽZ・ｽE・ｽﾌパ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ[・ｽE・ｽ^・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ (・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽt・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ[・ｽE・ｽ・ｽE・ｽ・ｽE・ｽA・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽA・ｽE・ｽd・ｽE・ｽ・ｽE・ｽ・ｽE・ｽA・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽT・ｽE・ｽC・ｽE・ｽY・ｽE・ｽﾈゑｿｽ)
 * 2. ・ｽE・ｽv・ｽE・ｽ・ｽE・ｽ・ｽE・ｽC・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ[・ｽE・ｽﾌ奇ｿｽ{・ｽE・ｽu・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ轤｢・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ(Hurtbox)・ｽE・ｽv・ｽE・ｽﾌ抵ｿｽ・ｽE・ｽ・ｽE・ｽ
 * 3. ・ｽE・ｽA・ｽE・ｽj・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ[・ｽE・ｽV・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽﾌコ・ｽE・ｽ}・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽm・ｽE・ｽF
 * 4. ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽﾊのフ・ｽE・ｽ@・ｽE・ｽC・ｽE・ｽ・ｽE・ｽ・ｽE・ｽﾛ托ｿｽ/・ｽE・ｽﾇみ搾ｿｽ・ｽE・ｽ・ｽE・ｽ
 */
class SceneDebug : public SceneBase
{
public:
	// --- ・ｽE・ｽV・ｽE・ｽ[・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽC・ｽE・ｽt・ｽE・ｽT・ｽE・ｽC・ｽE・ｽN・ｽE・ｽ・ｽE・ｽ ---
	void Init();   // ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ (・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ\・ｽE・ｽ[・ｽE・ｽX・ｽE・ｽﾇみ搾ｿｽ・ｽE・ｽﾝ、・ｽE・ｽﾏ撰ｿｽ・ｽE・ｽﾌ擾ｿｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ)
	void Uninit(); // ・ｽE・ｽI・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ (・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ\・ｽE・ｽ[・ｽE・ｽX・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ)
	void Update(float tick); // ・ｽE・ｽX・ｽE・ｽV・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ (・ｽE・ｽA・ｽE・ｽj・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ[・ｽE・ｽV・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽi・ｽE・ｽs・ｽE・ｽA・ｽE・ｽ・ｽE・ｽ・ｽE・ｽﾍ擾ｿｽ・ｽE・ｽ・ｽE・ｽ)
	void Draw();   // ・ｽE・ｽ`・ｽE・ｽ謠茨ｿｽ・ｽE・ｽ (・ｽE・ｽ・ｽE・ｽ・ｽE・ｽf・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ`・ｽE・ｽ・ｽE・ｽAGUI・ｽE・ｽ`・ｽE・ｽ・ｽE・ｽ)

private:
	float m_fps = 0.0f; // ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽﾝゑｿｽFPS
	bool m_showImGui;   // GUI・ｽE・ｽﾌ表・ｽE・ｽ・ｽE・ｽ/・ｽE・ｽ・ｽE・ｽ\・ｽE・ｽ・ｽE・ｽ・ｽE・ｽt・ｽE・ｽ・ｽE・ｽ・ｽE・ｽO

	// --- ・ｽE・ｽf・ｽE・ｽo・ｽE・ｽb・ｽE・ｽO・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽp・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽo・ｽE・ｽﾏ撰ｿｽ ---

	// ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽﾝ、・ｽE・ｽU・ｽE・ｽ・ｽE・ｽ・ｽE・ｽA・ｽE・ｽj・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ[・ｽE・ｽV・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽﾄ撰ｿｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽﾇゑｿｽ・ｽE・ｽ・ｽE・ｽ
	// (true・ｽE・ｽﾈゑｿｽU・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽﾌ描・ｽE・ｽ・ｽE・ｽ・ｽE・ｽA・ｽE・ｽI・ｽE・ｽ・ｽE・ｽ・ｽE・ｽt・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ[・ｽE・ｽ・ｽE・ｽ・ｽE・ｽﾌ監趣ｿｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽs・ｽE・ｽ・ｽE・ｽ)
	bool m_isAttacking = false;

	// ・ｽw・ｽi・ｽX・ｽJ・ｽC・ｽh・ｽ[・ｽ・ｽ・ｽi・ｽQ・ｽ[・ｽ・ｽ・ｽV・ｽ[・ｽ・ｽ・ｽﾆ難ｿｽ・ｽ・ｽ・ｽ・ｽ・ｽ・ｽ・ｽﾚにゑｿｽ・ｽ驍ｽ・ｽﾟ）
	SkyDome* m_skyDome = nullptr;
	// ・ｽX・ｽJ・ｽC・ｽh・ｽ[・ｽ・ｽ・ｽ`・ｽ・ｽp・ｽi・ｽJ・ｽ・ｽ・ｽ・ｽ・ｽO・ｽﾈゑｿｽ・ｽj
	ID3D11RasterizerState* m_pCullNone = nullptr;
	// スカイドーム(最奥)描画用：LESS_EQUAL の深度ステート
	ID3D11DepthStencilState* m_pDepthState3D = nullptr;

	// ・ｽE・ｽA・ｽE・ｽj・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ[・ｽE・ｽV・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ齊橸ｿｽ・ｽE・ｽ~・ｽE・ｽt・ｽE・ｽ・ｽE・ｽ・ｽE・ｽO
	// (true・ｽE・ｽﾈゑｿｽA・ｽE・ｽj・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ[・ｽE・ｽV・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽi・ｽE・ｽﾟなゑｿｽ・ｽE・ｽA・ｽE・ｽﾜゑｿｽ・ｽE・ｽﾍコ・ｽE・ｽ}・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ閭ゑｿｽ[・ｽE・ｽh)
	bool m_isPaused = false;

	// ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽﾝの再撰ｿｽ・ｽE・ｽt・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ[・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ (ImGui・ｽE・ｽﾅの表・ｽE・ｽ・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽp)
	// Player・ｽE・ｽN・ｽE・ｽ・ｽE・ｽ・ｽE・ｽX・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ float ・ｽE・ｽt・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ[・ｽE・ｽ・ｽE・ｽ・ｽE・ｽﾆは別に、・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽﾅ管暦ｿｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽﾄ難ｿｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ
	int m_currentFrame = 0;

	// ・ｽE・ｽA・ｽE・ｽj・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ[・ｽE・ｽV・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽﾄ撰ｿｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽx・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽp・ｽE・ｽ^・ｽE・ｽC・ｽE・ｽ}・ｽE・ｽ[
	// ・ｽE・ｽf・ｽE・ｽo・ｽE・ｽb・ｽE・ｽO・ｽE・ｽV・ｽE・ｽ[・ｽE・ｽ・ｽE・ｽ・ｽE・ｽﾍ可変フ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ[・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ[・ｽE・ｽg・ｽE・ｽﾅ難ｿｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽA・ｽE・ｽA・ｽE・ｽj・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ[・ｽE・ｽV・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ60FPS・ｽE・ｽ・ｽ・ｽ・ｽ・ｽE・ｽ・ｽE・ｽ
	// ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽm・ｽE・ｽﾉコ・ｽE・ｽ}・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ閧ｵ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽﾟ、・ｽE・ｽ・ｽE・ｽ・ｽE・ｽﾔゑｿｽﾏ算・ｽE・ｽ・ｽE・ｽ・ｽE・ｽﾄ撰ｿｽ・ｽE・ｽ艪ｷ・ｽE・ｽ・ｽE・ｽ
	float m_animTimer = 0.0f;

	// --- ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽﾖ撰ｿｽ ---

	// ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽﾝのプ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽC・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ[・ｽE・ｽﾝ抵ｿｽ・ｽE・ｽ "player_settings.ini" ・ｽE・ｽﾉ保托ｿｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ
	void SavePlayerSettings();

	// ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽpGUI (ImGui) ・ｽE・ｽﾌ描・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽs・ｽE・ｽ・ｽE・ｽ
	void DrawImGui();

	// ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽﾝ・・ｽ・ｽ・ｽE・ｽW・ｽE・ｽI・ｽE・ｽ{・ｽE・ｽ^・ｽE・ｽ・ｽE・ｽ・ｽE・ｽﾅ選・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽﾄゑｿｽ・ｽE・ｽ・ｽE・ｽZ・ｽE・ｽ・ｽE・ｽ AttackParams ・ｽE・ｽ・ｽE・ｽﾔゑｿｽ
	// (・ｽE・ｽU・ｽE・ｽ・ｽE・ｽ・ｽE・ｽZ・ｽE・ｽﾈ外・ｽE・ｽ・ｽE・ｽI・ｽE・ｽ・ｽEﾌ場合・ｽE・ｽ・ｽE・ｽ nullptr)
	AttackParams* GetSelectedParams(Player* player);

	// ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽE・ｽE・ｽL・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽZ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽ・ｽE・ｽx・ｽE・ｽﾏ会ｿｽ・ｽE・ｽE・ｽE・ｽﾄ撰ｿｽ・ｽE・ｽﾊ置・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽﾅ表・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ^・ｽE・ｽC・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽC・ｽE・ｽ・ｽE・ｽ
	// ・ｽE・ｽg・ｽE・ｽ・ｽE・ｽ・ｽE・ｽb・ｽE・ｽN・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽN・ｽE・ｽ・ｽE・ｽ・ｽE・ｽb・ｽE・ｽN・ｽE・ｽ^・ｽE・ｽh・ｽE・ｽ・ｽE・ｽ・ｽE・ｽb・ｽE・ｽO・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽﾆゑｿｽ・ｽE・ｽﾌフ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ[・ｽE・ｽ・ｽE・ｽ・ｽE・ｽﾖ移難ｿｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ
	void DrawTimeline(AttackParams* params);
};

#endif // __SCENE_DEBUG_H__