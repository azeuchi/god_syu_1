#pragma once

struct PlayerInputs;
struct AttackParams;

// 攻撃ボタン。並び順は、同時に押されたときに優先する順
enum class AttackButton
{
	LP, MP, HP,
	LK, MK, HK,
	COUNT
};

// 通常技。ボタンと姿勢（立ち／しゃがみ）の組み合わせで1つに決まる
enum class NormalMove
{
	LP, MP, HP, LK, MK, HK,
	CROUCH_LP, CROUCH_MP, CROUCH_HP, CROUCH_LK, CROUCH_MK, CROUCH_HK,
	COUNT
};
constexpr int NORMAL_MOVE_COUNT = (int)NormalMove::COUNT;
constexpr int ATTACK_BUTTON_COUNT = (int)AttackButton::COUNT;

// 通常技の表の1行。技を増やすときはここと NormalMoves.cpp の表に1行足す
struct NormalMoveInfo
{
	AttackButton button;
	bool crouch;
	const char* animName;   // アニメーション名。FBX のファイル名も同じにしている
	const char* paramFile;  // 技のパラメータ ini
	const char* label;      // デバッグシーンでの表示名
};

const NormalMoveInfo& GetNormalMoveInfo(NormalMove move);
NormalMove FindNormalMove(AttackButton button, bool crouch);

// 弱攻撃のボタンか（弱P・弱K。立ち／しゃがみは問わない）
inline bool IsLightButton(AttackButton button) { return button == AttackButton::LP || button == AttackButton::LK; }

bool IsButtonPressed(const PlayerInputs& inputs, AttackButton button);
void SetButtonPressed(PlayerInputs& inputs, AttackButton button, bool pressed);

// この技から、そのボタンの技へキャンセルできるか（受付時間やヒット条件は含まない）
bool CanCancelInto(const AttackParams& from, AttackButton button);

// --- 攻撃アニメの再生速度（緩急）。ゲーム中とデバッグ画面のコマ送りで同じ計算を使う ---

// 緩急（speedModifiers）の、技を出してから gameFrame フレーム目の再生倍率。
// 区間はデバッグ画面のタイムラインと同じく、技のフレーム（60fps）で指定する
float SpeedModifierAt(const AttackParams& params, float gameFrame);

// 攻撃アニメの基本の再生速度（1フレームに進むアニメのコマ数）。
// 緩急をかけた状態で、アニメ全体が技の全体時間にちょうど収まる速さにする
float AttackAnimBaseSpeed(const AttackParams& params, int animFrames, bool variableSpeed);

// 技を出してから gameFrame フレーム後のアニメのコマ。ゲーム中と同じく1フレームずつ進めて求める
float AnimFrameAtGameFrame(const AttackParams& params, float baseSpeed, int gameFrame, bool variableSpeed);
