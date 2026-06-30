#ifndef __SCENE_DEBUG_H__
#define __SCENE_DEBUG_H__

#include "SceneBase.hpp"
#include "PlayerRenderer.h"
#include <d3d11.h>

// ・ｽE・ｽE・ｽE・ｽO・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ骭ｾ・ｽE・ｽE・ｽE・ｽi・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽﾌゑｿｽ Player.h ・ｽE・ｽE・ｽE・ｽﾅ抵ｿｽ`・ｽE・ｽE・ｽE・ｽj
class Player;
struct AttackParams;
class SkyDome;

/**
 * @brief ・ｽE・ｽE・ｽE・ｽf・ｽE・ｽE・ｽE・ｽo・ｽE・ｽE・ｽE・ｽb・ｽE・ｽE・ｽE・ｽO・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽp・ｽE・ｽE・ｽE・ｽV・ｽE・ｽE・ｽE・ｽ[・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ
 * * ・ｽE・ｽE・ｽE・ｽQ・ｽE・ｽE・ｽE・ｽ[・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽﾌス・ｽE・ｽE・ｽE・ｽe・ｽE・ｽE・ｽE・ｽ[・ｽE・ｽE・ｽE・ｽg・ｽE・ｽE・ｽE・ｽ}・ｽE・ｽE・ｽE・ｽV・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ(FSM)・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽg・ｽE・ｽE・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽE・ｽE・ｽA・ｽE・ｽE・ｽE・ｽ闢ｮ・ｽE・ｽE・ｽE・ｽﾅア・ｽE・ｽE・ｽE・ｽj・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ[・ｽE・ｽE・ｽE・ｽV・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ笏ｻ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽｧ御す・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽV・ｽE・ｽE・ｽE・ｽ[・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽB
 * ・ｽE・ｽE・ｽE・ｽﾈ会ｿｽ・ｽE・ｽE・ｽE・ｽﾌ機・ｽE・ｽE・ｽE・ｽ\・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ:
 * 1. ・ｽE・ｽE・ｽE・ｽU・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽZ・ｽE・ｽE・ｽE・ｽﾌパ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ[・ｽE・ｽE・ｽE・ｽ^・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ (・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽt・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ[・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽA・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽA・ｽE・ｽE・ｽE・ｽd・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽA・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽT・ｽE・ｽE・ｽE・ｽC・ｽE・ｽE・ｽE・ｽY・ｽE・ｽE・ｽE・ｽﾈゑｿｽ)
 * 2. ・ｽE・ｽE・ｽE・ｽv・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽC・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ[・ｽE・ｽE・ｽE・ｽﾌ奇ｿｽ{・ｽE・ｽE・ｽE・ｽu・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ轤｢・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ(Hurtbox)・ｽE・ｽE・ｽE・ｽv・ｽE・ｽE・ｽE・ｽﾌ抵ｿｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ
 * 3. ・ｽE・ｽE・ｽE・ｽA・ｽE・ｽE・ｽE・ｽj・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ[・ｽE・ｽE・ｽE・ｽV・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽﾌコ・ｽE・ｽE・ｽE・ｽ}・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽm・ｽE・ｽE・ｽE・ｽF
 * 4. ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽﾊのフ・ｽE・ｽE・ｽE・ｽ@・ｽE・ｽE・ｽE・ｽC・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽﾛ托ｿｽ/・ｽE・ｽE・ｽE・ｽﾇみ搾ｿｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ
 */
class SceneDebug : public SceneBase
{
public:
	// --- ・ｽE・ｽE・ｽE・ｽV・ｽE・ｽE・ｽE・ｽ[・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽC・ｽE・ｽE・ｽE・ｽt・ｽE・ｽE・ｽE・ｽT・ｽE・ｽE・ｽE・ｽC・ｽE・ｽE・ｽE・ｽN・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ ---
	void Init();   // ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ (・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ\・ｽE・ｽE・ｽE・ｽ[・ｽE・ｽE・ｽE・ｽX・ｽE・ｽE・ｽE・ｽﾇみ搾ｿｽ・ｽE・ｽE・ｽE・ｽﾝ、・ｽE・ｽE・ｽE・ｽﾏ撰ｿｽ・ｽE・ｽE・ｽE・ｽﾌ擾ｿｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ)
	void Uninit(); // ・ｽE・ｽE・ｽE・ｽI・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ (・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ\・ｽE・ｽE・ｽE・ｽ[・ｽE・ｽE・ｽE・ｽX・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ)
	void Update(float tick); // ・ｽE・ｽE・ｽE・ｽX・ｽE・ｽE・ｽE・ｽV・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ (・ｽE・ｽE・ｽE・ｽA・ｽE・ｽE・ｽE・ｽj・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ[・ｽE・ｽE・ｽE・ｽV・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽi・ｽE・ｽE・ｽE・ｽs・ｽE・ｽE・ｽE・ｽA・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽﾍ擾ｿｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ)
	void Draw();   // ・ｽE・ｽE・ｽE・ｽ`・ｽE・ｽE・ｽE・ｽ謠茨ｿｽ・ｽE・ｽE・ｽE・ｽ (・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽf・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ`・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽAGUI・ｽE・ｽE・ｽE・ｽ`・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ)

private:
	float m_fps = 0.0f; // ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽﾝゑｿｽFPS
	bool m_showImGui;   // GUI・ｽE・ｽE・ｽE・ｽﾌ表・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ/・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ\・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽt・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽO

	// --- ・ｽE・ｽE・ｽE・ｽf・ｽE・ｽE・ｽE・ｽo・ｽE・ｽE・ｽE・ｽb・ｽE・ｽE・ｽE・ｽO・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽp・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽo・ｽE・ｽE・ｽE・ｽﾏ撰ｿｽ ---

	// ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽﾝ、・ｽE・ｽE・ｽE・ｽU・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽA・ｽE・ｽE・ｽE・ｽj・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ[・ｽE・ｽE・ｽE・ｽV・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽﾄ撰ｿｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽﾇゑｿｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ
	// (true・ｽE・ｽE・ｽE・ｽﾈゑｿｽU・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽﾌ描・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽA・ｽE・ｽE・ｽE・ｽI・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽt・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ[・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽﾌ監趣ｿｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽs・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ)
	bool m_isAttacking = false;

	// ・ｽE・ｽw・ｽE・ｽi・ｽE・ｽX・ｽE・ｽJ・ｽE・ｽC・ｽE・ｽh・ｽE・ｽ[・ｽE・ｽ・ｽE・ｽ・ｽE・ｽi・ｽE・ｽQ・ｽE・ｽ[・ｽE・ｽ・ｽE・ｽ・ｽE・ｽV・ｽE・ｽ[・ｽE・ｽ・ｽE・ｽ・ｽE・ｽﾆ難ｿｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽﾚにゑｿｽ・ｽE・ｽ驍ｽ・ｽE・ｽﾟ）
	SkyDome* m_skyDome = nullptr;
	PlayerRenderer m_playerRenderer; // プレイヤー描画（アウトライン共通）

	// ・ｽE・ｽE・ｽE・ｽA・ｽE・ｽE・ｽE・ｽj・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ[・ｽE・ｽE・ｽE・ｽV・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ齊橸ｿｽ・ｽE・ｽE・ｽE・ｽ~・ｽE・ｽE・ｽE・ｽt・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽO
	// (true・ｽE・ｽE・ｽE・ｽﾈゑｿｽA・ｽE・ｽE・ｽE・ｽj・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ[・ｽE・ｽE・ｽE・ｽV・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽi・ｽE・ｽE・ｽE・ｽﾟなゑｿｽ・ｽE・ｽE・ｽE・ｽA・ｽE・ｽE・ｽE・ｽﾜゑｿｽ・ｽE・ｽE・ｽE・ｽﾍコ・ｽE・ｽE・ｽE・ｽ}・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ閭ゑｿｽ[・ｽE・ｽE・ｽE・ｽh)
	bool m_isPaused = false;

	// ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽﾝの再撰ｿｽ・ｽE・ｽE・ｽE・ｽt・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ[・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ (ImGui・ｽE・ｽE・ｽE・ｽﾅの表・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽE・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽp)
	// Player・ｽE・ｽE・ｽE・ｽN・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽX・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ float ・ｽE・ｽE・ｽE・ｽt・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ[・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽﾆは別に、・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽﾅ管暦ｿｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽﾄ難ｿｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ
	int m_currentFrame = 0;

	// ・ｽE・ｽE・ｽE・ｽA・ｽE・ｽE・ｽE・ｽj・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ[・ｽE・ｽE・ｽE・ｽV・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽﾄ撰ｿｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽx・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽp・ｽE・ｽE・ｽE・ｽ^・ｽE・ｽE・ｽE・ｽC・ｽE・ｽE・ｽE・ｽ}・ｽE・ｽE・ｽE・ｽ[
	// ・ｽE・ｽE・ｽE・ｽf・ｽE・ｽE・ｽE・ｽo・ｽE・ｽE・ｽE・ｽb・ｽE・ｽE・ｽE・ｽO・ｽE・ｽE・ｽE・ｽV・ｽE・ｽE・ｽE・ｽ[・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽﾍ可変フ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ[・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ[・ｽE・ｽE・ｽE・ｽg・ｽE・ｽE・ｽE・ｽﾅ難ｿｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽA・ｽE・ｽE・ｽE・ｽA・ｽE・ｽE・ｽE・ｽj・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ[・ｽE・ｽE・ｽE・ｽV・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ60FPS・ｽE・ｽE・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ
	// ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽm・ｽE・ｽE・ｽE・ｽﾉコ・ｽE・ｽE・ｽE・ｽ}・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ閧ｵ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽﾟ、・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽﾔゑｿｽﾏ算・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽﾄ撰ｿｽ・ｽE・ｽE・ｽE・ｽ艪ｷ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ
	float m_animTimer = 0.0f;

	// --- ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽﾖ撰ｿｽ ---

	// ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽﾝのプ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽC・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ[・ｽE・ｽE・ｽE・ｽﾝ抵ｿｽ・ｽE・ｽE・ｽE・ｽ "player_settings.ini" ・ｽE・ｽE・ｽE・ｽﾉ保托ｿｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ
	void SavePlayerSettings();

	// ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽpGUI (ImGui) ・ｽE・ｽE・ｽE・ｽﾌ描・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽs・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ
	void DrawImGui();

	// ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽﾝ・・ｽE・ｽ・ｽE・ｽ・ｽE・ｽE・ｽE・ｽW・ｽE・ｽE・ｽE・ｽI・ｽE・ｽE・ｽE・ｽ{・ｽE・ｽE・ｽE・ｽ^・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽﾅ選・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽﾄゑｿｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽZ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ AttackParams ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽﾔゑｿｽ
	// (・ｽE・ｽE・ｽE・ｽU・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽZ・ｽE・ｽE・ｽE・ｽﾈ外・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽI・ｽE・ｽE・ｽE・ｽ・ｽE・ｽEﾌ場合・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ nullptr)
	AttackParams* GetSelectedParams(Player* player);

	// ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽE・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽE・ｽE・ｽE・ｽE・ｽL・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽZ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽE・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽx・ｽE・ｽE・ｽE・ｽﾏ会ｿｽ・ｽE・ｽE・ｽE・ｽE・ｽE・ｽE・ｽE・ｽﾄ撰ｿｽ・ｽE・ｽE・ｽE・ｽﾊ置・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽﾅ表・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ^・ｽE・ｽE・ｽE・ｽC・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽC・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ
	// ・ｽE・ｽE・ｽE・ｽg・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽb・ｽE・ｽE・ｽE・ｽN・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽN・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽb・ｽE・ｽE・ｽE・ｽN・ｽE・ｽE・ｽE・ｽ^・ｽE・ｽE・ｽE・ｽh・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽb・ｽE・ｽE・ｽE・ｽO・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽﾆゑｿｽ・ｽE・ｽE・ｽE・ｽﾌフ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ[・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽﾖ移難ｿｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ・ｽE・ｽE・ｽE・ｽ
	void DrawTimeline(AttackParams* params);
};

#endif // __SCENE_DEBUG_H__