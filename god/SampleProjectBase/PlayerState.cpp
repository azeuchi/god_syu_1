#include "PlayerState.h"
#include "Player.h"

// 遷移先の各ステートをインクルード
#include "LightPunch.h"
#include "MediumPunch.h"
#include "HeavyPunch.h"
#include "MediumKick.h"
#include "HeavyKick.h"

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

	const PlayerInputs& inputs = player->GetInputs();

	// 1. 弱パンチへのキャンセル
	if (params.cancelToLight && inputs.LightPunch)
	{
		player->SetCurrentAttackParams(&player->GetLightPunchParams());
		player->SetState(new LightPunch());
		return true;
	}

	// 2. 中パンチへのキャンセル
	if (params.cancelToMedium && inputs.MediumPunch)
	{
		player->SetCurrentAttackParams(&player->GetMediumPunchParams());
		player->SetState(new MediumPunch());
		return true;
	}

	// 3. 大パンチへのキャンセル
	if (params.cancelToHeavyPunch && inputs.HeavyPunch)
	{
		player->SetCurrentAttackParams(&player->GetHeavyPunchParams());
		player->SetState(new HeavyPunch());
		return true;
	}

	// 4. 中キックへのキャンセル
	if (params.cancelToMediumKick && inputs.MediumKick)
	{
		player->SetCurrentAttackParams(&player->GetMediumKickParams());
		player->SetState(new MediumKick());
		return true;
	}

	// 5. 大キックへのキャンセル
	if (params.cancelToHeavy && inputs.HeavyKick)
	{
		player->SetCurrentAttackParams(&player->GetHeavyKickParams());
		player->SetState(new HeavyKick());
		return true;
	}

	return false;
}