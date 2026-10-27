// SceneDebug.cpp
#include "math.h"
#include <filesystem>
#include "SceneDebug.h"
#include "Geometory.h"
#include "DebugLog.h"
#include "Model.h"
#include "SkyDome.h"
#include "CameraBase.h"
#include "LightBase.h"
#include "Shader.h"
#include "Player.h" 
#include "Texture.h"
#include "Input.h" 
#include <system/imgui/imgui.h>
#include <fstream> 
#include <cmath> 
#include <string>
#include <cstdio>
#include "PlayerParameterLoader.h"
#include "PlayerAssetLoader.h"
#include "CameraShake.h"
#include "SceneTraining.h"

using namespace DirectX;
using namespace DirectX::SimpleMath;

Texture* g_uiTex_debug = nullptr;

const float FRAME_TIME_60FPS = 1.0f / 60.0f;

// 技の選択番号。0 から通常技を技の表（NormalMove）の順に並べ、そのあとに動作と必殺技を続ける
enum DebugMoveType
{
	DEBUG_DOWN = NORMAL_MOVE_COUNT,
	DEBUG_WAKEUP,
	DEBUG_HADOU_L, DEBUG_HADOU_M, DEBUG_HADOU_H,
	DEBUG_SHORYU_L, DEBUG_SHORYU_M, DEBUG_SHORYU_H,
};
static int s_currentAttackType = 0;

static bool IsNormalType(int type) { return type >= 0 && type < NORMAL_MOVE_COUNT; }
static bool IsShoryukenType(int type) { return type >= DEBUG_SHORYU_L && type <= DEBUG_SHORYU_H; }
// 体に攻撃判定が出る技。発生・持続フレームを編集できる
static bool IsStrikeType(int type) { return IsNormalType(type) || IsShoryukenType(type); }
// フレームデータを持つ技。ダウン・起き上がり以外
static bool HasFrameData(int type) { return type != DEBUG_DOWN && type != DEBUG_WAKEUP; }

// カメラシェイク確認用（デバッグシーンでのプレビュー状態）
static float s_dbgShakeTrauma = 0.0f;
static float s_dbgShakeKickDir = 1.0f;
static DirectX::XMFLOAT3 s_dbgShakeOffset = { 0.0f, 0.0f, 0.0f };

static const char* GetCurrentAnimName()
{
	if (IsNormalType(s_currentAttackType)) return GetNormalMoveInfo((NormalMove)s_currentAttackType).animName;
	if (s_currentAttackType == DEBUG_DOWN) return "Down";
	if (s_currentAttackType == DEBUG_WAKEUP) return "WakeUp";
	if (IsShoryukenType(s_currentAttackType)) return "Shoryuken";
	return "Hadouken";
}

// 判定ボックス(BoundingBox)のリストを、指定色のワイヤーフレームで描画する
static void DrawBoxListWire(const std::vector<DirectX::BoundingBox>& boxes, const DirectX::XMFLOAT4& color)
{
	Geometory::SetColor(color);
	static const int edge[4][2] = { {0,1},{1,2},{2,3},{3,0} };
	for (const auto& box : boxes)
	{
		DirectX::XMFLOAT3 corners[8];
		box.GetCorners(corners);
		for (int e = 0; e < 4; ++e)
		{
			Geometory::AddLine(corners[edge[e][0]], corners[edge[e][1]]);
		}
	}
}

void SceneDebug::SavePlayerSettings()
{
	Player* player = GetObj<Player>("Player");
	if (!player) return;

	// 保存先フォルダが無ければ作る（初回や削除時でも保存できるように）
	std::filesystem::create_directories("Param");

	// 共通設定の保存
	{
		std::ofstream ofs("Param/Param_Common.ini");
		if (ofs.is_open())
		{
			ofs << player->GetMoveSpeed() << std::endl;
			DirectX::XMFLOAT3 scale = player->GetScale();
			ofs << scale.x << " " << scale.y << " " << scale.z << std::endl;

			// 立ち(Base)のくらい判定
			for (int i = 0; i < (int)HurtboxType::COUNT; ++i) {
				DirectX::XMFLOAT2 ext = player->GetHurtboxBaseExtents((HurtboxType)i);
				DirectX::XMFLOAT2 off = player->GetHurtboxBaseOffset((HurtboxType)i);
				ofs << ext.x << " " << ext.y << " " << off.x << " " << off.y << std::endl;
			}

			// しゃがみ(Crouch)のくらい判定
			for (int i = 0; i < (int)HurtboxType::COUNT; ++i) {
				DirectX::XMFLOAT2 ext = player->GetHurtboxCrouchExtents((HurtboxType)i);
				DirectX::XMFLOAT2 off = player->GetHurtboxCrouchOffset((HurtboxType)i);
				ofs << ext.x << " " << ext.y << " " << off.x << " " << off.y << std::endl;
			}
			ofs.close();
		}
	}

	// 各攻撃パラメータの保存用ヘルパー関数
	auto SaveOneAttack = [&](const char* filename, const AttackParams& params) {
		std::ofstream ofs(filename);
		if (!ofs.is_open()) return;

		// 形式バージョン
		ofs << "FGPARAM 2" << std::endl;

		// タイミング・判定
		ofs << params.totalDuration << std::endl;
		ofs << params.hitboxStart << std::endl;
		ofs << params.hitboxEnd << std::endl;

		// 攻撃判定(Hitbox)はキーフレーム式のまま保存
		auto WriteAnimatedBoxes = [&](const std::vector<AnimatedBox>& boxes) {
			ofs << boxes.size() << std::endl;
			for (const auto& abox : boxes) {
				ofs << abox.keyframes.size() << std::endl;
				for (const auto& key : abox.keyframes) {
					ofs << key.frame << std::endl;
					ofs << key.data.offset.x << " " << key.data.offset.y << std::endl;
					ofs << key.data.extents.x << " " << key.data.extents.y << std::endl;
				}
			}
			};
		WriteAnimatedBoxes(params.hitboxes);

		// くらい判定(Hurtbox)は区間付き固定ボックスとして保存
		ofs << params.moveHurtboxes.size() << std::endl;
		for (const auto& wh : params.moveHurtboxes) {
			ofs << wh.offset.x << " " << wh.offset.y << " "
				<< wh.extents.x << " " << wh.extents.y << " "
				<< wh.startFrame << " " << wh.endFrame << std::endl;
		}

		// ゲームバランス
		ofs << params.damage << std::endl;
		ofs << params.hitFrame << std::endl;
		ofs << params.blockFrame << std::endl;
		ofs << params.hitStop << std::endl;
		ofs << params.knockback << std::endl;
		ofs << params.isDown << std::endl;

		// キャンセル設定
		ofs << params.cancelEnabled << std::endl;
		ofs << params.cancelStart << std::endl;
		ofs << params.cancelEnd << std::endl;
		ofs << params.cancelToLight << " " << params.cancelToMedium << " " << params.cancelToHeavyPunch << " " << params.cancelToMediumKick << " " << params.cancelToHeavy << std::endl;

		// 速度変化リスト
		size_t speedModCount = params.speedModifiers.size();
		ofs << speedModCount << std::endl;
		for (const auto& mod : params.speedModifiers) {
			ofs << mod.startFrame << " " << mod.endFrame << " " << mod.speed << std::endl;
		}

		// 飛び道具設定
		ofs << params.projectileSpeed << std::endl;
		ofs << params.projectileSize << std::endl;

		// 無敵・前進速度
		ofs << params.invincibleEnd << std::endl;
		ofs << params.forwardSpeed << std::endl;

		// 弱Kへのキャンセル（弱Kを足したときに増えた項目なので末尾に置く）
		ofs << params.cancelToLightKick << std::endl;

		// 必殺技へのキャンセル（あとから足した項目なので末尾に置く）
		ofs << params.cancelToSpecial << std::endl;

		ofs.close();
		};

	// 各ファイルの保存実行
	for (int i = 0; i < NORMAL_MOVE_COUNT; ++i)
	{
		const NormalMove move = (NormalMove)i;
		SaveOneAttack(GetNormalMoveInfo(move).paramFile, player->GetNormalParams(move));
	}

	SaveOneAttack("Param/Param_HadoukenL.ini", player->GetHadoukenLParams());
	SaveOneAttack("Param/Param_HadoukenM.ini", player->GetHadoukenMParams());
	SaveOneAttack("Param/Param_HadoukenH.ini", player->GetHadoukenHParams());
	SaveOneAttack("Param/Param_ShoryukenL.ini", player->GetShoryukenLParams());
	SaveOneAttack("Param/Param_ShoryukenM.ini", player->GetShoryukenMParams());
	SaveOneAttack("Param/Param_ShoryukenH.ini", player->GetShoryukenHParams());

	DebugLog::log(DebugLog::INFO_LOG, "設定を分割ファイルとして保存しました。");
}

SceneDebug::~SceneDebug() = default;

void SceneDebug::Init()
{
	// --- シェーダー読み込み ---
	Shader* shader[] = {
		CreateObj<VertexShader>("VS_SkinMeshAnimation"),
		CreateObj<PixelShader>("PS_TexColor"),
	};
	const char* file[] = {
		"Assets/Shader/VS_SkinMeshAnimation.cso",
		"Assets/Shader/PS_TexColor.cso",
	};
	int shaderNum = _countof(shader);
	for (int i = 0; i < shaderNum; ++i)
	{
		if (FAILED(shader[i]->Load(file[i])))
		{
			MessageBox(NULL, file[i], "Shader Error", MB_OK);
		}
	}

	// --- スカイドームを用意 ---
	m_skyDome = std::make_unique<SkyDome>();
	m_skyDome->Setup(this);

	// --- プレイヤー描画ヘルパーを用意---
	m_playerRenderer.Setup(this);

	// --- プレイヤー生成 ---
	CreateObj<Player>("Player");
	Player* player = GetObj<Player>("Player");

	player->SetInputType(PlayerInputType::AI);

	// --- 設定ファイルからパラメータ読み込み ---
	PlayerParameterLoader::LoadSettings(player);

	// --- モデル・アニメーションロード---
	player->Load("Assets/Model/knight/Idle.fbx", Player::MODEL_SCALE, true, false);
	PlayerAssetLoader::LoadCommonAnimations(player);

	// 初期状態は Idle (待機)
	player->Debug_SetAnimation("Idle", true);

	player->SetPosition({ 0.0f, 0.0f, 0.0f });
	player->SetRotation({ 0.0f, DirectX::XM_PI / -2.0f, 0.0f });

	m_showImGui = true;
	g_uiTex_debug = new Texture();

	m_isAttacking = false;
	m_isPaused = false;
	m_currentFrame = 0;
	m_animTimer = 0.0f;
}

void SceneDebug::Uninit()
{
	// グローバルのテクスチャは D3D デバイスが生きているうちに解放する。
	// unique_ptr にすると解放がプログラム終了時になり、その時点ではデバイスが無いので落ちる
	if (g_uiTex_debug) {
		delete g_uiTex_debug;
		g_uiTex_debug = nullptr;
	}

}

void SceneDebug::Update(float tick)
{
	if (tick > 0.0f) { m_fps = 1.0f / tick; }
	else { m_fps = 0.0f; }

	if (IsKeyTrigger(VK_TAB)) {
		m_showImGui = !m_showImGui;
	}

	Player* player = GetObj<Player>("Player");
	if (!player) return;

	// --- パラメータの同期 ---
	AttackParams* params = GetSelectedParams(player);

	player->SetCurrentAttackParams(params);

	// --- 速度計算用ロジック ---
	float speed = 1.0f;
	const char* animName = GetCurrentAnimName();

	if (m_isAttacking || HasFrameData(s_currentAttackType)) {
		int totalAnimFrames = player->GetModel()->GetAnimationTotalFrame(animName);

		// ゲーム中と同じ速さ（緩急込みでアニメ全体が全体時間に収まる速さ）
		if (params) {
			speed = AttackAnimBaseSpeed(*params, totalAnimFrames, Player::s_variableSpeed);
		}
	}

	player->SetAnimationSpeed(speed);

	// --- アニメーション制御 ---
	if (!m_isPaused && m_isAttacking)
	{
		// 再生中は経過フレームを数えるだけ。姿勢は下でコマ送りと同じ計算で出す
		m_animTimer += tick;
		while (m_animTimer >= FRAME_TIME_60FPS)
		{
			m_animTimer -= FRAME_TIME_60FPS;
			m_currentFrame++;
		}
	}

	// 姿勢は、ゲーム中と同じく緩急込みで1フレームずつ進めた位置にする（再生中もコマ送りも同じ）
	{
		int modelFrame = params
			? static_cast<int>(AnimFrameAtGameFrame(*params, speed, m_currentFrame, Player::s_variableSpeed))
			: static_cast<int>(m_currentFrame * speed);
		player->Debug_SetAnimation(animName, false);
		player->Debug_SetFrame(modelFrame);
	}

	player->SetAttackTimer((float)m_currentFrame);

	player->UpdateModelBlend();
	player->UpdateAttackBoxes();

	// --- 攻撃終了判定 ---
	if (m_isAttacking)
	{
		int totalGameFrames = 0;
		if (params) {
			totalGameFrames = static_cast<int>(std::round(params->totalDuration * 60.0f));
		}
		else {
			totalGameFrames = player->GetModel()->GetAnimationTotalFrame(GetCurrentAnimName());
		}

		if (m_currentFrame >= totalGameFrames)
		{
			player->Debug_SetAnimation("Idle", true);
			m_isAttacking = false;
			m_isPaused = false;
			m_currentFrame = 0;
		}
	}

	CameraBase* camera = GetObj<CameraBase>("Camera");
	if (camera)
	{
		DirectX::XMFLOAT3 playerPos = player->GetPosition();
		float targetZ = -3.5f;
		float heightOffset = 1.2f;
		DirectX::XMFLOAT3 targetPos = { playerPos.x, playerPos.y + heightOffset, targetZ };
		DirectX::XMFLOAT3 targetLook = { playerPos.x, playerPos.y + 1.0f, 0.0f };
		camera->SetPos(targetPos);
		camera->SetLook(targetLook);

		// スカイドームをカメラ位置に追従させる
		if (m_skyDome) m_skyDome->Update(camera->GetPos());
	}

	// カメラシェイク（プレビュー）の更新：traumaを減衰させ揺れオフセットを計算する
	CameraShake::Tick(s_dbgShakeTrauma, s_dbgShakeKickDir, tick, s_dbgShakeOffset);
}

void SceneDebug::Draw()
{
	CameraBase* pCamera = GetObj<CameraBase>("Camera");

	// カメラシェイク（プレビュー）：描画の間だけカメラをずらし、後で元へ戻す
	XMFLOAT3 camShakeBasePos = pCamera->GetPos();
	XMFLOAT3 camShakeBaseLook = pCamera->GetLook();
	{
		XMFLOAT3 sp = { camShakeBasePos.x + s_dbgShakeOffset.x, camShakeBasePos.y + s_dbgShakeOffset.y, camShakeBasePos.z };
		XMFLOAT3 sl = { camShakeBaseLook.x + s_dbgShakeOffset.x, camShakeBaseLook.y + s_dbgShakeOffset.y, camShakeBaseLook.z };
		pCamera->SetPos(sp);
		pCamera->SetLook(sl);
	}

	Player* player = GetObj<Player>("Player");
	if (player) {
		// スカイドームはプレイヤーより先に描く
		if (m_skyDome) m_skyDome->DrawWithState(pCamera->GetView(), pCamera->GetProj());

		// アウトラインと本体を描く
		m_playerRenderer.DrawOutline(this, player);
		m_playerRenderer.DrawBody(this, player);

		// ----------------------------------------------------
		// 判定ボックスの表示制御
		// 現在フレームのくらい判定(緑)・攻撃判定(赤)を常に重ねて表示する。
		// ----------------------------------------------------
		bool isEditingAttack = HasFrameData(s_currentAttackType);

		if (isEditingAttack) {
			// 攻撃判定は発生期間内だけ出ているものとして扱う
			bool showHitbox = false;
			AttackParams* pParams = player->GetCurrentAttackParams();
			if (pParams) {
				float currentTime = (float)m_currentFrame * FRAME_TIME_60FPS;
				showHitbox = (currentTime >= pParams->hitboxStart && currentTime < pParams->hitboxEnd);
			}
			player->SetActiveHitbox(showHitbox);

			// 表示用の判定を毎フレーム作り直す (区間外でも追加くらいを正しく反映するため)
			player->UpdateAttackBoxes();

			// 基本のくらい判定は常時表示。技固有の追加くらい判定は区間中だけ重ねて描く
			player->DrawBoundingBox();
			DrawBoxListWire(player->GetActiveHurtboxes(), XMFLOAT4(0.1f, 0.9f, 0.2f, 1.0f));

			// 攻撃判定は赤で描く (発生期間外は中身が空なので何も出ない)
			DrawBoxListWire(player->GetActiveHitboxes(), XMFLOAT4(1.0f, 0.15f, 0.1f, 1.0f));
		}
		else {
			// Down / WakeUp などは基本のくらい判定のみ表示する
			player->SetActiveHitbox(false);
			player->DrawBoundingBox();
		}
	}

	// カメラシェイクを元に戻す（UI描画や次フレームへ揺れを残さない）
	pCamera->SetPos(camShakeBasePos);
	pCamera->SetLook(camShakeBaseLook);

	if (m_showImGui)
	{
		DrawImGui();
	}
}

void SceneDebug::DrawImGui()
{
	Player* player = GetObj<Player>("Player");

	ImGui::Begin("Debug Settings");
	ImGui::Text("FPS: %.1f", m_fps);
	if (ImGui::Button("トレーニングへ")) { SceneTraining::s_requestEnter = true; }

	if (!player) {
		ImGui::End();
		return;
	}

	if (ImGui::CollapsingHeader("カメラシェイク"))
	{
		ImGui::SliderFloat("最大ずれ幅", &CameraShake::s_maxOffset, 0.0f, 0.6f, "%.3f");
		ImGui::SliderFloat("減衰(毎秒)", &CameraShake::s_decay, 0.5f, 8.0f, "%.2f");
		ImGui::SliderFloat("指向性", &CameraShake::s_kickRatio, 0.0f, 1.0f, "%.2f");
		ImGui::SliderFloat("硬直基準", &CameraShake::s_hitStopRef, 0.05f, 0.40f, "%.3f");
		ImGui::SliderFloat("テスト方向", &s_dbgShakeKickDir, -1.0f, 1.0f, "%.2f");

		ImGui::Text("トラウマ: %.2f", s_dbgShakeTrauma);

		if (ImGui::Button("弱テスト"))  { CameraShake::AddTrauma(s_dbgShakeTrauma, CameraShake::TraumaFromHitStop(0.08f)); }
		ImGui::SameLine();
		if (ImGui::Button("中テスト")) { CameraShake::AddTrauma(s_dbgShakeTrauma, CameraShake::TraumaFromHitStop(0.12f)); }
		ImGui::SameLine();
		if (ImGui::Button("強テスト"))  { CameraShake::AddTrauma(s_dbgShakeTrauma, CameraShake::TraumaFromHitStop(0.18f)); }

		if (ImGui::Button("値リセット")) { CameraShake::ResetParams(); }
	}

	if (ImGui::CollapsingHeader("コンボ / フレームデータ"))
	{
		// 全攻撃技（名前と params ポインタ）
		struct MoveEntry { const char* name; AttackParams* p; };
		std::vector<MoveEntry> moves;
		for (int i = 0; i < NORMAL_MOVE_COUNT; ++i)
		{
			moves.push_back({ GetNormalMoveInfo((NormalMove)i).label, &player->GetNormalParams((NormalMove)i) });
		}
		moves.push_back({ "波動L", &player->GetHadoukenLParams() });
		moves.push_back({ "波動M", &player->GetHadoukenMParams() });
		moves.push_back({ "波動H", &player->GetHadoukenHParams() });
		moves.push_back({ "昇竜L", &player->GetShoryukenLParams() });
		moves.push_back({ "昇竜M", &player->GetShoryukenMParams() });
		moves.push_back({ "昇竜H", &player->GetShoryukenHParams() });
		const int moveCount = (int)moves.size();

		// 秒 → フレーム（60FPS換算）
		auto ToF = [](float sec) { return (int)std::lround(sec * 60.0f); };

		auto Startup = [&](const AttackParams* p) { return ToF(p->hitboxStart); };
		auto Active  = [&](const AttackParams* p) { return ToF(p->hitboxEnd) - ToF(p->hitboxStart); };
		auto Recover = [&](const AttackParams* p) { return ToF(p->totalDuration) - ToF(p->hitboxEnd); };
		auto Total   = [&](const AttackParams* p) { return ToF(p->totalDuration); };
		// hitFrame をそのまま有利フレームとして表示（エンジンが攻撃側の硬直を補正）
		auto NetAdv  = [&](const AttackParams* p) { return p->hitFrame; };

		ImGui::TextDisabled("On Hit=有利F(=hitFrame)。キャンセルは有利Fに関係なく繋がる");

		// --- フレームデータ表 ---
		if (ImGui::BeginTable("FrameTable", 7, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
		{
			ImGui::TableSetupColumn("技");
			ImGui::TableSetupColumn("発生");
			ImGui::TableSetupColumn("持続");
			ImGui::TableSetupColumn("硬直");
			ImGui::TableSetupColumn("全体");
			ImGui::TableSetupColumn("ヒット時");
			ImGui::TableSetupColumn("ガード時");
			ImGui::TableHeadersRow();
			for (int i = 0; i < moveCount; ++i)
			{
				AttackParams* p = moves[i].p;
				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0); ImGui::TextUnformatted(moves[i].name);
				ImGui::TableSetColumnIndex(1); ImGui::Text("%d", Startup(p));
				ImGui::TableSetColumnIndex(2); ImGui::Text("%d", Active(p));
				ImGui::TableSetColumnIndex(3); ImGui::Text("%d", Recover(p));
				ImGui::TableSetColumnIndex(4); ImGui::Text("%d", Total(p));
				ImGui::TableSetColumnIndex(5); ImGui::Text("%+d", NetAdv(p));
				ImGui::TableSetColumnIndex(6); ImGui::Text("%+d", p->blockFrame);
			}
			ImGui::EndTable();
		}

		ImGui::Separator();

		// --- 連結ビュー（始動技を選ぶと、つながる技を一覧）---
		static int s_comboStarter = 0;
		std::vector<const char*> starterNames;
		for (const MoveEntry& m : moves) starterNames.push_back(m.name);
		ImGui::Combo("始動技", &s_comboStarter, starterNames.data(), moveCount);

		AttackParams* a = moves[s_comboStarter].p;
		int adv = NetAdv(a);
		ImGui::Text("%s : ヒット時 %+d", starterNames[s_comboStarter], adv);

		// キャンセル可否。通常技はボタンごとのフラグ、必殺技は「必殺技へ」のフラグで決まる。
		// 波動拳もキャンセルで出せるが、弾が出るのが遅くつながらないので、ここには昇竜拳だけを出す
		const int firstShoryuken = NORMAL_MOVE_COUNT + 3;
		auto CanCancelTo = [&](const AttackParams* from, int targetIndex) -> bool {
			if (!from->cancelEnabled) return false;
			if (IsNormalType(targetIndex)) return CanCancelInto(*from, GetNormalMoveInfo((NormalMove)targetIndex).button);
			return from->cancelToSpecial && targetIndex >= firstShoryuken;
		};

		ImGui::Text("繋がる技:");
		bool anyConnect = false;
		for (int j = 0; j < moveCount; ++j)
		{
			AttackParams* b = moves[j].p;
			int bStartup = Startup(b);
			bool link = (adv >= bStartup);
			bool cancel = CanCancelTo(a, j);
			if (!link && !cancel) continue;

			anyConnect = true;
			std::string tag;
			if (link)   { tag += "[Link +" + std::to_string(adv - bStartup) + "]"; }
			if (cancel) { tag += (tag.empty() ? "" : " "); tag += "[Cancel]"; }
			ImGui::BulletText("%s  %s  (startup %dF)", starterNames[j], tag.c_str(), bStartup);
		}
		if (!anyConnect) ImGui::BulletText("(none)");
	}

	if (ImGui::CollapsingHeader("アニメーション制御", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::Text("技を選択:");

		ImGui::Text("パンチ:"); ImGui::SameLine();
		ImGui::RadioButton("L##P", &s_currentAttackType, (int)NormalMove::LP); ImGui::SameLine();
		ImGui::RadioButton("M##P", &s_currentAttackType, (int)NormalMove::MP); ImGui::SameLine();
		ImGui::RadioButton("H##P", &s_currentAttackType, (int)NormalMove::HP);

		ImGui::Text("キック:"); ImGui::SameLine();
		ImGui::RadioButton("L##K", &s_currentAttackType, (int)NormalMove::LK); ImGui::SameLine();
		ImGui::RadioButton("M##K", &s_currentAttackType, (int)NormalMove::MK); ImGui::SameLine();
		ImGui::RadioButton("H##K", &s_currentAttackType, (int)NormalMove::HK);

		ImGui::Text("屈P:"); ImGui::SameLine();
		ImGui::RadioButton("L##CP", &s_currentAttackType, (int)NormalMove::CROUCH_LP); ImGui::SameLine();
		ImGui::RadioButton("M##CP", &s_currentAttackType, (int)NormalMove::CROUCH_MP); ImGui::SameLine();
		ImGui::RadioButton("H##CP", &s_currentAttackType, (int)NormalMove::CROUCH_HP);

		ImGui::Text("屈K:"); ImGui::SameLine();
		ImGui::RadioButton("L##CK", &s_currentAttackType, (int)NormalMove::CROUCH_LK); ImGui::SameLine();
		ImGui::RadioButton("M##CK", &s_currentAttackType, (int)NormalMove::CROUCH_MK); ImGui::SameLine();
		ImGui::RadioButton("H##CK", &s_currentAttackType, (int)NormalMove::CROUCH_HK);

		ImGui::Text("必殺技:"); ImGui::SameLine();
		ImGui::RadioButton("波動L", &s_currentAttackType, DEBUG_HADOU_L); ImGui::SameLine();
		ImGui::RadioButton("波動M", &s_currentAttackType, DEBUG_HADOU_M); ImGui::SameLine();
		ImGui::RadioButton("波動H", &s_currentAttackType, DEBUG_HADOU_H);
		ImGui::Dummy(ImVec2(ImGui::CalcTextSize("必殺技:").x, 0.0f)); ImGui::SameLine();
		ImGui::RadioButton("昇竜L", &s_currentAttackType, DEBUG_SHORYU_L); ImGui::SameLine();
		ImGui::RadioButton("昇竜M", &s_currentAttackType, DEBUG_SHORYU_M); ImGui::SameLine();
		ImGui::RadioButton("昇竜H", &s_currentAttackType, DEBUG_SHORYU_H);

		ImGui::Text("その他:"); ImGui::SameLine();
		ImGui::RadioButton("ダウン", &s_currentAttackType, DEBUG_DOWN); ImGui::SameLine();
		ImGui::RadioButton("起き上がり", &s_currentAttackType, DEBUG_WAKEUP);

		if (!m_isAttacking)
		{
			auto StartAttack = [&](bool startPaused) {
				const char* animName = GetCurrentAnimName();
				player->Debug_SetAnimation(animName, true);

				m_isAttacking = true;
				m_isPaused = startPaused;
				m_currentFrame = 0;
				m_animTimer = 0.0f;
				// しゃがみ技はくらい判定もしゃがみの形で確認できるようにする
				player->SetIsCrouching(IsNormalType(s_currentAttackType) && GetNormalMoveInfo((NormalMove)s_currentAttackType).crouch);
				};

			if (ImGui::Button("再生")) StartAttack(false);
			ImGui::SameLine();
			if (ImGui::Button("コマ送り")) StartAttack(true);
		}
		else
		{
			ImGui::TextColored(ImVec4(1, 0, 0, 1), ">> PLAYING <<");
			ImGui::SameLine();
			if (ImGui::Button("停止")) {
				m_isAttacking = false;
				player->Debug_SetAnimation("Idle", true);
			}
		}

		ImGui::SameLine();
		ImGui::Checkbox("一時停止", &m_isPaused);

		if (m_isPaused || !m_isAttacking)
		{
			ImGui::SameLine();
			// 下のフレーム入力欄（- / +）と並びを揃え、戻すボタンを左にする
			if (ImGui::Button("-1")) m_currentFrame--;
			ImGui::SameLine();
			if (ImGui::Button("+1")) m_currentFrame++;

			ImGui::InputInt("フレーム", &m_currentFrame);
			if (m_currentFrame < 0) m_currentFrame = 0;
		}
		else
		{
			ImGui::Text("フレーム: %d", m_currentFrame);
		}

		// タイムライン (横軸でタイミングを確認し、クリック／ドラッグで頭出し)
		ImGui::Separator();
		DrawTimeline(GetSelectedParams(player));
	}

	AttackParams* pParams = GetSelectedParams(player);

	if (pParams && ImGui::CollapsingHeader("攻撃パラメータ", ImGuiTreeNodeFlags_DefaultOpen))
	{
		AttackParams& params = *pParams;

		int totalFrames = static_cast<int>(std::round(params.totalDuration / FRAME_TIME_60FPS));
		int startFrames = static_cast<int>(std::round(params.hitboxStart / FRAME_TIME_60FPS));
		int endFrames = static_cast<int>(std::round(params.hitboxEnd / FRAME_TIME_60FPS));
		int hitStopFrames = static_cast<int>(std::round(params.hitStop / FRAME_TIME_60FPS));

		if (ImGui::InputInt("全体フレーム", &totalFrames)) {
			if (totalFrames < 1) totalFrames = 1;
			params.totalDuration = totalFrames * FRAME_TIME_60FPS;
		}

		if (IsStrikeType(s_currentAttackType)) {
			ImGui::Text("攻撃判定区間(参考):");
			if (ImGui::InputInt("開始フレーム", &startFrames)) {
				if (startFrames < 0) startFrames = 0;
				params.hitboxStart = startFrames * FRAME_TIME_60FPS;
			}
			if (ImGui::InputInt("終了フレーム", &endFrames)) {
				if (endFrames < 0) endFrames = 0;
				params.hitboxEnd = endFrames * FRAME_TIME_60FPS;
			}
			if (IsShoryukenType(s_currentAttackType)) {
				int invincibleFrames = static_cast<int>(std::round(params.invincibleEnd / FRAME_TIME_60FPS));
				if (ImGui::InputInt("無敵終了フレーム", &invincibleFrames)) {
					if (invincibleFrames < 0) invincibleFrames = 0;
					params.invincibleEnd = invincibleFrames * FRAME_TIME_60FPS;
				}
			}
			ImGui::SliderFloat("前進速度", &params.forwardSpeed, 0.0f, 10.0f);
		}
		else {
			ImGui::Text("飛び道具設定:");
			ImGui::SliderFloat("弾速", &params.projectileSpeed, 1.0f, 30.0f);
			ImGui::SliderFloat("弾サイズ", &params.projectileSize, 0.1f, 5.0f);
		}

		auto EditAnimatedBoxList = [&](const char* label, std::vector<AnimatedBox>& boxList, bool isHurtbox) {
			if (ImGui::TreeNode(label))
			{
				for (int i = 0; i < (int)boxList.size(); ++i)
				{
					ImGui::PushID(i);
					if (isHurtbox) {
						const char* names[] = { "Head", "Body", "Legs" };
						const char* name = (i < 3) ? names[i] : "Extra";
						ImGui::Text("ボックス #%d (%s)", i, name);
					}
					else {
						ImGui::Text("攻撃判定 #%d", i);
					}

					ImGui::SameLine();
					if (ImGui::Button("グループ削除")) {
						boxList.erase(boxList.begin() + i);
						ImGui::PopID();
						break;
					}

					AnimatedBox& abox = boxList[i];
					if (ImGui::TreeNode("キーフレーム"))
					{
						for (int k = 0; k < (int)abox.keyframes.size(); ++k)
						{
							ImGui::PushID(k);
							BoxKeyframe& key = abox.keyframes[k];

							ImGui::Text("キー #%d", k);
							ImGui::SameLine();
							if (ImGui::Button("キー削除")) {
								abox.keyframes.erase(abox.keyframes.begin() + k);
								ImGui::PopID();
								break;
							}

							int frame = (int)key.frame;
							if (ImGui::InputInt("フレーム", &frame)) key.frame = (float)frame;

							ImGui::SliderFloat2("オフセット", &key.data.offset.x, -2.0f, 2.0f);
							ImGui::SliderFloat2("サイズ", &key.data.extents.x, 0.1f, 2.0f);

							ImGui::Separator();
							ImGui::PopID();
						}

						if (ImGui::Button("キー追加"))
						{
							BoxKeyframe newKey;
							newKey.frame = (float)m_currentFrame;
							if (!abox.keyframes.empty()) newKey.data = abox.keyframes.back().data;
							else {
								newKey.data.offset = { 0.8f, 1.5f };
								newKey.data.extents = { 0.3f, 0.3f };
							}
							abox.keyframes.push_back(newKey);
						}
						ImGui::TreePop();
					}
					ImGui::Separator();
					ImGui::PopID();
				}

				if (ImGui::Button(isHurtbox ? "くらい判定追加" : "Add Hitbox"))
				{
					AnimatedBox newBox;
					newBox.keyframes.push_back({ 0.0f, {{0.5f, 1.5f}, {0.3f, 0.3f}} });
					boxList.push_back(newBox);
				}

				ImGui::TreePop();
			}
			};

		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.5f, 0.5f, 1.0f));
		EditAnimatedBoxList("Hitboxes (Red)", params.hitboxes, false);
		ImGui::PopStyleColor();

		// くらい判定(Hurtbox): 基本判定は常時有効。ここでは区間限定の追加判定を編集する
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 1.0f, 0.5f, 1.0f));
		bool hurtOpen = ImGui::TreeNode("技中くらい判定(緑/追加)");
		ImGui::PopStyleColor();
		if (hurtOpen)
		{
		

			for (int i = 0; i < (int)params.moveHurtboxes.size(); ++i)
			{
				ImGui::PushID(i);
				WindowedHurtbox& wh = params.moveHurtboxes[i];

				ImGui::Text("ボックス #%d", i);
				ImGui::SameLine();
				if (ImGui::Button("削除")) {
					params.moveHurtboxes.erase(params.moveHurtboxes.begin() + i);
					ImGui::PopID();
					break;
				}

				// 出現〜消滅の区間 (フレーム)
				if (ImGui::SliderInt("開始F", &wh.startFrame, 0, totalFrames)) {
					if (wh.startFrame > wh.endFrame) wh.endFrame = wh.startFrame;
				}
				if (ImGui::SliderInt("終了F", &wh.endFrame, 0, totalFrames)) {
					if (wh.endFrame < wh.startFrame) wh.startFrame = wh.endFrame;
				}
				ImGui::SliderFloat2("オフセット", &wh.offset.x, -2.0f, 2.0f);
				ImGui::SliderFloat2("サイズ", &wh.extents.x, 0.1f, 2.0f);
				ImGui::Separator();
				ImGui::PopID();
			}

			if (ImGui::Button("くらい判定追加"))
			{
				// 現在フレームを開始に、少しだけ持続する区間で追加する
				WindowedHurtbox wh;
				wh.startFrame = m_currentFrame;
				wh.endFrame = m_currentFrame + 3;
				if (wh.endFrame > totalFrames) wh.endFrame = totalFrames;
				wh.offset = { 0.0f, 1.0f };
				wh.extents = { 0.3f, 0.3f };
				params.moveHurtboxes.push_back(wh);
			}
			ImGui::TreePop();
		}


		ImGui::Separator();
		ImGui::Text("ゲームデータ");
		ImGui::InputInt("ダメージ", &params.damage);
		ImGui::InputInt("有利F(ヒット)", &params.hitFrame);
		ImGui::InputInt("有利F(ガード)", &params.blockFrame);

		if (ImGui::InputInt("ヒットストップ(F)", &hitStopFrames)) {
			if (hitStopFrames < 0) hitStopFrames = 0;
			params.hitStop = hitStopFrames * FRAME_TIME_60FPS;
		}

		ImGui::SliderFloat("ノックバック", &params.knockback, 0.0f, 5.0f);
		ImGui::Checkbox("ダウンさせる", &params.isDown);

		if (ImGui::TreeNode("キャンセル設定"))
		{
			ImGui::Checkbox("キャンセル有効", &params.cancelEnabled);
			if (params.cancelEnabled) {
				int cancelStartF = static_cast<int>(std::round(params.cancelStart / FRAME_TIME_60FPS));
				int cancelEndF = static_cast<int>(std::round(params.cancelEnd / FRAME_TIME_60FPS));
				if (ImGui::SliderInt("開始フレーム", &cancelStartF, 0, totalFrames)) params.cancelStart = cancelStartF * FRAME_TIME_60FPS;
				if (ImGui::SliderInt("終了フレーム", &cancelEndF, 0, totalFrames)) params.cancelEnd = cancelEndF * FRAME_TIME_60FPS;

				ImGui::Checkbox("弱P", &params.cancelToLight);
				ImGui::SameLine(); ImGui::Checkbox("中P", &params.cancelToMedium);
				ImGui::SameLine(); ImGui::Checkbox("強P", &params.cancelToHeavyPunch);
				ImGui::Checkbox("弱K", &params.cancelToLightKick);
				ImGui::SameLine(); ImGui::Checkbox("中K", &params.cancelToMediumKick);
				ImGui::SameLine(); ImGui::Checkbox("強K", &params.cancelToHeavy);
				ImGui::Checkbox("必殺技（波動拳・昇竜拳）", &params.cancelToSpecial);
			}
			ImGui::TreePop();
		}

		if (ImGui::TreeNode("速度変化"))
		{
			for (int i = 0; i < (int)params.speedModifiers.size(); ++i)
			{
				ImGui::PushID(i);
				AnimSpeedModifier& mod = params.speedModifiers[i];
				ImGui::Text("変化 #%d", i);
				ImGui::SameLine();
				if (ImGui::Button("削除")) {
					params.speedModifiers.erase(params.speedModifiers.begin() + i);
					ImGui::PopID();
					break;
				}
				int sFrame = (int)mod.startFrame;
				int eFrame = (int)mod.endFrame;
				if (ImGui::SliderInt("開始F", &sFrame, 0, totalFrames)) mod.startFrame = (float)sFrame;
				if (ImGui::SliderInt("終了F", &eFrame, 0, totalFrames)) mod.endFrame = (float)eFrame;
				ImGui::SliderFloat("速度倍率", &mod.speed, 0.1f, 5.0f);
				ImGui::Separator();
				ImGui::PopID();
			}
			if (ImGui::Button("変化を追加"))
			{
				params.speedModifiers.push_back({ 0.0f, 5.0f, 1.0f });
			}
			ImGui::TreePop();
		}
	}

	if (ImGui::CollapsingHeader("基本くらい判定設定"))
	{
		const char* hurtboxNames[] = { "Head", "Body", "Legs" };

		if (ImGui::TreeNode("立ち(基本)"))
		{
			for (int i = 0; i < (int)HurtboxType::COUNT; ++i)
			{
				ImGui::PushID(i);
				ImGui::Text("%s", hurtboxNames[i]);
				DirectX::XMFLOAT2 ext = player->GetHurtboxBaseExtents((HurtboxType)i);
				DirectX::XMFLOAT2 off = player->GetHurtboxBaseOffset((HurtboxType)i);
				bool changed = false;
				if (ImGui::SliderFloat2("サイズ", &ext.x, 0.1f, 5.0f)) changed = true;
				if (ImGui::SliderFloat2("オフセット", &off.x, -5.0f, 5.0f)) changed = true;
				if (changed) player->SetHurtboxBase((HurtboxType)i, off, ext);
				ImGui::PopID();
			}
			ImGui::TreePop();
		}

		if (ImGui::TreeNode("しゃがみ"))
		{
			for (int i = 0; i < (int)HurtboxType::COUNT; ++i)
			{
				ImGui::PushID(100 + i);
				ImGui::Text("%s", hurtboxNames[i]);
				DirectX::XMFLOAT2 ext = player->GetHurtboxCrouchExtents((HurtboxType)i);
				DirectX::XMFLOAT2 off = player->GetHurtboxCrouchOffset((HurtboxType)i);
				bool changed = false;
				if (ImGui::SliderFloat2("サイズ", &ext.x, 0.1f, 5.0f)) changed = true;
				if (ImGui::SliderFloat2("オフセット", &off.x, -5.0f, 5.0f)) changed = true;
				if (changed) player->SetHurtboxCrouch((HurtboxType)i, off, ext);
				ImGui::PopID();
			}
			ImGui::TreePop();
		}
	}

	ImGui::Separator();
	if (ImGui::Button("全設定を保存", ImVec2(-1, 40)))
	{
		SavePlayerSettings();
	}

	ImGui::End();
}
// 現在選択中の技に対応する AttackParams を返す
// Update() と DrawImGui() の両方から呼ぶことで、技選択の対応表を一箇所にまとめる
AttackParams* SceneDebug::GetSelectedParams(Player* player)
{
	if (!player) return nullptr;

	if (IsNormalType(s_currentAttackType)) return &player->GetNormalParams((NormalMove)s_currentAttackType);

	switch (s_currentAttackType)
	{
	case DEBUG_HADOU_L: return &player->GetHadoukenLParams();
	case DEBUG_HADOU_M: return &player->GetHadoukenMParams();
	case DEBUG_HADOU_H: return &player->GetHadoukenHParams();
	case DEBUG_SHORYU_L: return &player->GetShoryukenLParams();
	case DEBUG_SHORYU_M: return &player->GetShoryukenMParams();
	case DEBUG_SHORYU_H: return &player->GetShoryukenHParams();
	default: return nullptr; // Down / WakeUp などフレームデータを持たない動作
	}
}

void SceneDebug::DrawTimeline(AttackParams* params)
{
	// フレームデータを持たない動作 (Down/WakeUp) では何も出さない
	if (!params)
	{
		return;
	}

	// 内部パラメータは「秒」で保持しているので、60FPS基準でフレームへ換算する
	const float toFrame = 1.0f / FRAME_TIME_60FPS; // = 60
	int totalF = static_cast<int>(std::round(params->totalDuration * toFrame));
	if (totalF < 1) totalF = 1;
	int hitStartF = static_cast<int>(std::round(params->hitboxStart * toFrame));
	int hitEndF = static_cast<int>(std::round(params->hitboxEnd * toFrame));

	ImDrawList* dl = ImGui::GetWindowDrawList();

	// 描画開始位置とトラックの寸法
	const ImVec2 origin = ImGui::GetCursorScreenPos();
	const float labelW = 64.0f;
	const float laneH = 16.0f;
	const float laneGap = 4.0f;
	const float rulerH = 16.0f;
	float fullW = ImGui::GetContentRegionAvail().x;
	float trackW = fullW - labelW;
	if (trackW < 60.0f) trackW = 60.0f;
	const float pxPerFrame = trackW / static_cast<float>(totalF);
	const float trackX0 = origin.x + labelW;

	auto FrameToX = [&](float f) { return trackX0 + f * pxPerFrame; };

	enum Lane { LANE_PHASE, LANE_HIT, LANE_HURT, LANE_CANCEL, LANE_SPEED, LANE_COUNT };
	auto LaneTop = [&](int lane) { return origin.y + rulerH + lane * (laneH + laneGap); };
	const float bottomY = LaneTop(LANE_COUNT - 1) + laneH;
	const float totalH = bottomY - origin.y;

	const ImU32 colText = ImGui::GetColorU32(ImGuiCol_Text);
	const ImU32 colTick = IM_COL32(150, 150, 150, 120);
	const ImU32 colTrack = IM_COL32(255, 255, 255, 18);
	const ImU32 colTrackEdge = IM_COL32(255, 255, 255, 40);
	const ImU32 colStartup = IM_COL32(140, 140, 130, 200);
	const ImU32 colActive = IM_COL32(226, 75, 74, 235);
	const ImU32 colRecover = IM_COL32(240, 165, 60, 210);
	const ImU32 colHit = IM_COL32(226, 75, 74, 235);
	const ImU32 colHurt = IM_COL32(120, 190, 90, 150);
	const ImU32 colCancel = IM_COL32(55, 138, 221, 200);
	const ImU32 colSpeed = IM_COL32(140, 120, 230, 200);
	const ImU32 colPlayhead = IM_COL32(80, 190, 255, 255);

	ImGui::InvisibleButton("##timeline", ImVec2(fullW, totalH));
	if (ImGui::IsItemActive())
	{
		float mx = ImGui::GetIO().MousePos.x;
		int f = static_cast<int>(std::round((mx - trackX0) / pxPerFrame));
		if (f < 0) f = 0;
		if (f > totalF) f = totalF;
		m_currentFrame = f;
		m_isPaused = true;
	}

	auto DrawLaneBase = [&](int lane) {
		float y0 = LaneTop(lane);
		dl->AddRectFilled(ImVec2(trackX0, y0), ImVec2(trackX0 + trackW, y0 + laneH), colTrack, 3.0f);
		dl->AddRect(ImVec2(trackX0, y0), ImVec2(trackX0 + trackW, y0 + laneH), colTrackEdge, 3.0f);
		};
	auto DrawBand = [&](int lane, float fStart, float fEnd, ImU32 col) {
		if (fEnd <= fStart) return;
		float y0 = LaneTop(lane);
		dl->AddRectFilled(ImVec2(FrameToX(fStart), y0 + 1.0f),
			ImVec2(FrameToX(fEnd), y0 + laneH - 1.0f), col, 3.0f);
		};
	auto DrawLaneLabel = [&](int lane, const char* name) {
		dl->AddText(ImVec2(origin.x, LaneTop(lane) + 1.0f), colText, name);
		};

	for (int i = 0; i < LANE_COUNT; ++i) DrawLaneBase(i);

	int step = (totalF <= 20) ? 4 : (totalF <= 40 ? 5 : 10);
	for (int f = 0; f <= totalF; f += step)
	{
		float x = FrameToX((float)f);
		dl->AddLine(ImVec2(x, origin.y + rulerH - 4.0f), ImVec2(x, bottomY), colTick);
		char buf[16];
		snprintf(buf, sizeof(buf), "%d", f);
		dl->AddText(ImVec2(x + 2.0f, origin.y), colText, buf);
	}

	// Phase: 発生 / 持続 / 硬直
	DrawLaneLabel(LANE_PHASE, "Phase");
	DrawBand(LANE_PHASE, 0.0f, (float)hitStartF, colStartup);
	DrawBand(LANE_PHASE, (float)hitStartF, (float)hitEndF, colActive);
	DrawBand(LANE_PHASE, (float)hitEndF, (float)totalF, colRecover);

	// Hitbox: 攻撃判定が出ている区間
	DrawLaneLabel(LANE_HIT, "Hitbox");
	DrawBand(LANE_HIT, (float)hitStartF, (float)hitEndF, colHit);

	// Hurtbox: 基本判定は常時、技固有の追加判定は区間だけ濃く描く
	DrawLaneLabel(LANE_HURT, "Hurtbox");
	DrawBand(LANE_HURT, 0.0f, (float)totalF, colHurt);
	for (const auto& wh : params->moveHurtboxes)
	{
		DrawBand(LANE_HURT, (float)wh.startFrame, (float)wh.endFrame, IM_COL32(99, 153, 34, 235));
	}
	// 無敵の区間は、くらい判定が効いていないことが分かるよう金色で上書きする
	if (params->invincibleEnd > 0.0f)
	{
		DrawBand(LANE_HURT, 0.0f, std::round(params->invincibleEnd * toFrame), IM_COL32(240, 200, 60, 235));
	}

	// Cancel: 次の技へ移行できる窓（有効時のみ）
	DrawLaneLabel(LANE_CANCEL, "Cancel");
	if (params->cancelEnabled)
	{
		int cs = static_cast<int>(std::round(params->cancelStart * toFrame));
		int ce = static_cast<int>(std::round(params->cancelEnd * toFrame));
		DrawBand(LANE_CANCEL, (float)cs, (float)ce, colCancel);
	}

	// Speed: 再生速度変化の区間
	DrawLaneLabel(LANE_SPEED, "Speed");
	for (const auto& mod : params->speedModifiers)
	{
		DrawBand(LANE_SPEED, mod.startFrame, mod.endFrame, colSpeed);
	}

	// 再生位置（プレイヘッド）を全レーンを貫く縦線で描く
	float px = FrameToX((float)m_currentFrame);
	dl->AddLine(ImVec2(px, origin.y + rulerH - 6.0f), ImVec2(px, bottomY), colPlayhead, 2.0f);
	dl->AddTriangleFilled(
		ImVec2(px - 4.0f, origin.y + rulerH - 6.0f),
		ImVec2(px + 4.0f, origin.y + rulerH - 6.0f),
		ImVec2(px, origin.y + rulerH), colPlayhead);

	char fbuf[24];
	snprintf(fbuf, sizeof(fbuf), "%d / %dF", m_currentFrame, totalF);
	dl->AddText(ImVec2(trackX0, bottomY + 2.0f), colText, fbuf);

	ImGui::Dummy(ImVec2(fullW, ImGui::GetTextLineHeight() + 2.0f));
}
