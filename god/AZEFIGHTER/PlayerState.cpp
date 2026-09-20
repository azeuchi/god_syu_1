#include "PlayerState.h"
#include "Player.h"

#include "NormalAttack.h"

bool PlayerState::CheckCancel(Player* player, float stateTimer, const AttackParams& params)
{
	// キャンセルが無効なら何もしない
	if (!params.cancelEnabled)
	{
		return false;
	}

	// キャンセル受付期間外なら何もしない
	if (stateTimer < params.cancelStart || stateTimer > params.cancelEnd)
	{
		return false;
	}

	
	if (!player->HasHit()) return false;

	const PlayerInputs& inputs = player->GetInputs();

	// ボタンの優先順は地上で技を出すときと同じ。下を入れていればしゃがみ技へ繋ぐ
	for (int i = 0; i < ATTACK_BUTTON_COUNT; ++i)
	{
		const AttackButton button = (AttackButton)i;
		if (!IsButtonPressed(inputs, button) || !CanCancelInto(params, button)) continue;

		NormalAttack::Start(player, FindNormalMove(button, inputs.moveDown));
		return true;
	}

	return false;
}