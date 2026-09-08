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

	
	if (!player->HasHit()) return false;

	const PlayerInputs& inputs = player->GetInputs();

	// 弱パンチへのキャンセル
	if (params.cancelToLight && inputs.LightPunch)
	{
		player->SetCurrentAttackParams(&player->GetLightPunchParams());
		player->SetState(std::make_unique<LightPunch>());
		return true;
	}

	// 中パンチへのキャンセル
	if (params.cancelToMedium && inputs.MediumPunch)
	{
		player->SetCurrentAttackParams(&player->GetMediumPunchParams());
		player->SetState(std::make_unique<MediumPunch>());
		return true;
	}

	// 大パンチへのキャンセル
	if (params.cancelToHeavyPunch && inputs.HeavyPunch)
	{
		player->SetCurrentAttackParams(&player->GetHeavyPunchParams());
		player->SetState(std::make_unique<HeavyPunch>());
		return true;
	}

	// 中キックへのキャンセル
	if (params.cancelToMediumKick && inputs.MediumKick)
	{
		player->SetCurrentAttackParams(&player->GetMediumKickParams());
		player->SetState(std::make_unique<MediumKick>());
		return true;
	}

	// 大キックへのキャンセル
	if (params.cancelToHeavy && inputs.HeavyKick)
	{
		player->SetCurrentAttackParams(&player->GetHeavyKickParams());
		player->SetState(std::make_unique<HeavyKick>());
		return true;
	}

	return false;
}