#include "PlayerStateAir.h"
#include "PlayerStateIdle.h"
#include "Player.h"

void PlayerStateAir::Update(Player* player, float tick)
{
	// --- 共通の着地チェック ---
	if (!player->GetIsJumping())
	{
		// 着地したら待機状態へ遷移
		player->SetState(new PlayerStateIdle());
		return;
	}

	// 着地していなければ子クラスの挙動を実行
	UpdateBehavior(player, tick);
}