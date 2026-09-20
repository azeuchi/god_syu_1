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

bool IsButtonPressed(const PlayerInputs& inputs, AttackButton button);
void SetButtonPressed(PlayerInputs& inputs, AttackButton button, bool pressed);

// この技から、そのボタンの技へキャンセルできるか（受付時間やヒット条件は含まない）
bool CanCancelInto(const AttackParams& from, AttackButton button);
