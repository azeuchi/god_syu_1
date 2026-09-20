#include "NormalMoves.h"
#include "Player.h"
#include <iterator>

namespace
{
	// 並びは NormalMove と同じ順にする
	const NormalMoveInfo MOVE_TABLE[] =
	{
		{ AttackButton::LP, false, "LightPunch",        "Param/Param_LightPunch.ini",        "LP"   },
		{ AttackButton::MP, false, "MediumPunch",       "Param/Param_MediumPunch.ini",       "MP"   },
		{ AttackButton::HP, false, "HeavyPunch",        "Param/Param_HeavyPunch.ini",        "HP"   },
		{ AttackButton::LK, false, "LightKick",         "Param/Param_LightKick.ini",         "LK"   },
		{ AttackButton::MK, false, "MediumKick",        "Param/Param_MediumKick.ini",        "MK"   },
		{ AttackButton::HK, false, "HeavyKick",         "Param/Param_HeavyKick.ini",         "HK"   },
		{ AttackButton::LP, true,  "CrouchLightPunch",  "Param/Param_CrouchLightPunch.ini",  "屈LP" },
		{ AttackButton::MP, true,  "CrouchMediumPunch", "Param/Param_CrouchMediumPunch.ini", "屈MP" },
		{ AttackButton::HP, true,  "CrouchHeavyPunch",  "Param/Param_CrouchHeavyPunch.ini",  "屈HP" },
		{ AttackButton::LK, true,  "CrouchLightKick",   "Param/Param_CrouchLightKick.ini",   "屈LK" },
		{ AttackButton::MK, true,  "CrouchMediumKick",  "Param/Param_CrouchMediumKick.ini",  "屈MK" },
		{ AttackButton::HK, true,  "CrouchHeavyKick",   "Param/Param_CrouchHeavyKick.ini",   "屈HK" },
	};
	static_assert(std::size(MOVE_TABLE) == NORMAL_MOVE_COUNT, "MOVE_TABLE と NormalMove の数が合っていない");
}

const NormalMoveInfo& GetNormalMoveInfo(NormalMove move)
{
	return MOVE_TABLE[(int)move];
}

NormalMove FindNormalMove(AttackButton button, bool crouch)
{
	for (int i = 0; i < NORMAL_MOVE_COUNT; ++i)
	{
		if (MOVE_TABLE[i].button == button && MOVE_TABLE[i].crouch == crouch) return (NormalMove)i;
	}
	return NormalMove::LP;
}

bool IsButtonPressed(const PlayerInputs& inputs, AttackButton button)
{
	switch (button)
	{
	case AttackButton::LP: return inputs.LightPunch;
	case AttackButton::MP: return inputs.MediumPunch;
	case AttackButton::HP: return inputs.HeavyPunch;
	case AttackButton::LK: return inputs.LightKick;
	case AttackButton::MK: return inputs.MediumKick;
	case AttackButton::HK: return inputs.HeavyKick;
	default:               return false;
	}
}

void SetButtonPressed(PlayerInputs& inputs, AttackButton button, bool pressed)
{
	switch (button)
	{
	case AttackButton::LP: inputs.LightPunch = pressed;  break;
	case AttackButton::MP: inputs.MediumPunch = pressed; break;
	case AttackButton::HP: inputs.HeavyPunch = pressed;  break;
	case AttackButton::LK: inputs.LightKick = pressed;   break;
	case AttackButton::MK: inputs.MediumKick = pressed;  break;
	case AttackButton::HK: inputs.HeavyKick = pressed;   break;
	default: break;
	}
}

bool CanCancelInto(const AttackParams& from, AttackButton button)
{
	switch (button)
	{
	case AttackButton::LP: return from.cancelToLight;
	case AttackButton::MP: return from.cancelToMedium;
	case AttackButton::HP: return from.cancelToHeavyPunch;
	case AttackButton::LK: return from.cancelToLightKick;
	case AttackButton::MK: return from.cancelToMediumKick;
	case AttackButton::HK: return from.cancelToHeavy;
	default:               return false;
	}
}
