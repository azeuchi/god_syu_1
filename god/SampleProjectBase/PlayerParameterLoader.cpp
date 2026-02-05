#include "PlayerParameterLoader.h"
#include "DebugLog.h"
#include <fstream>
#include <vector>

// 内部使用のヘルパー関数群
namespace
{
	// アニメーション付きボックスの読み込み
	void LoadAnimatedBoxes(std::ifstream& ifs, std::vector<AnimatedBox>& targetList)
	{
		size_t boxCount = 0;
		ifs >> boxCount;
		targetList.clear();
		for (size_t i = 0; i < boxCount; ++i)
		{
			AnimatedBox abox;
			size_t keyframeCount = 0;
			ifs >> keyframeCount;
			for (size_t k = 0; k < keyframeCount; ++k)
			{
				BoxKeyframe key;
				ifs >> key.frame;
				ifs >> key.data.offset.x >> key.data.offset.y >> key.data.extents.x >> key.data.extents.y;
				abox.keyframes.push_back(key);
			}
			targetList.push_back(abox);
		}
	}
}

void PlayerParameterLoader::LoadCommonSettings(Player* player, const char* filePath)
{
	if (!player) return;

	std::ifstream ifs(filePath);
	if (!ifs.is_open())
	{
		DebugLog::log(DebugLog::WARNING_LOG, "共通設定ファイルが見つかりません: %s", filePath);
		return;
	}

	float moveSpeed = 0.0f;
	DirectX::XMFLOAT3 scale = { 1.0f, 1.0f, 1.0f };

	ifs >> moveSpeed;
	ifs >> scale.x >> scale.y >> scale.z;

	// 立ち状態の当たり判定
	for (int i = 0; i < (int)HurtboxType::COUNT; ++i) {
		DirectX::XMFLOAT2 ext, off;
		if (!ifs.eof()) ifs >> ext.x >> ext.y >> off.x >> off.y;
		player->SetHurtboxBase((HurtboxType)i, off, ext);
	}

	// しゃがみ状態の当たり判定
	for (int i = 0; i < (int)HurtboxType::COUNT; ++i) {
		DirectX::XMFLOAT2 ext, off;
		if (!ifs.eof()) ifs >> ext.x >> ext.y >> off.x >> off.y;
		player->SetHurtboxCrouch((HurtboxType)i, off, ext);
	}

	player->SetMoveSpeed(moveSpeed);
	player->SetScale(scale);

	ifs.close();
}

void PlayerParameterLoader::LoadAttackParams(AttackParams& p, const char* filePath)
{
	std::ifstream ifs(filePath);
	if (!ifs.is_open())
	{
		DebugLog::log(DebugLog::WARNING_LOG, "攻撃設定ファイルが見つかりません: %s", filePath);
		return;
	}

	ifs >> p.totalDuration;
	ifs >> p.hitboxStart >> p.hitboxEnd;

	// 攻撃判定 (Hitbox)
	LoadAnimatedBoxes(ifs, p.hitboxes);
	// くらい判定 (Hurtbox)
	LoadAnimatedBoxes(ifs, p.hurtboxes);

	ifs >> p.damage >> p.hitFrame >> p.blockFrame >> p.hitStop >> p.knockback;
	ifs >> p.isDown;
	ifs >> p.cancelEnabled >> p.cancelStart >> p.cancelEnd;
	ifs >> p.cancelToLight >> p.cancelToMedium >> p.cancelToHeavyPunch >> p.cancelToMediumKick >> p.cancelToHeavy;

	// 速度変化リスト読み込み
	size_t count = 0;
	if (!ifs.eof()) ifs >> count;
	p.speedModifiers.clear();
	for (size_t k = 0; k < count; ++k) {
		AnimSpeedModifier mod;
		ifs >> mod.startFrame >> mod.endFrame >> mod.speed;
		p.speedModifiers.push_back(mod);
	}

	// 飛び道具設定読み込み
	if (!ifs.eof()) {
		ifs >> p.projectileSpeed >> p.projectileSize;
	}

	ifs.close();
}

void PlayerParameterLoader::LoadSettings(Player* player)
{
	if (!player) return;

	LoadCommonSettings(player, "Param_Common.ini");

	LoadAttackParams(player->GetLightPunchParams(), "Param_LightPunch.ini");
	LoadAttackParams(player->GetMediumPunchParams(), "Param_MediumPunch.ini");
	LoadAttackParams(player->GetHeavyPunchParams(), "Param_HeavyPunch.ini");
	LoadAttackParams(player->GetMediumKickParams(), "Param_MediumKick.ini");
	LoadAttackParams(player->GetHeavyKickParams(), "Param_HeavyKick.ini");

	LoadAttackParams(player->GetHadoukenLParams(), "Param_HadoukenL.ini");
	LoadAttackParams(player->GetHadoukenMParams(), "Param_HadoukenM.ini");
	LoadAttackParams(player->GetHadoukenHParams(), "Param_HadoukenH.ini");
}

void PlayerParameterLoader::CopyParameters(Player* src, Player* dst)
{
	if (!src || !dst) return;

	// パラメータをコピー
	dst->GetLightPunchParams() = src->GetLightPunchParams();
	dst->GetMediumPunchParams() = src->GetMediumPunchParams();
	dst->GetHeavyPunchParams() = src->GetHeavyPunchParams();
	dst->GetMediumKickParams() = src->GetMediumKickParams();
	dst->GetHeavyKickParams() = src->GetHeavyKickParams();
	dst->GetHadoukenLParams() = src->GetHadoukenLParams();
	dst->GetHadoukenMParams() = src->GetHadoukenMParams();
	dst->GetHadoukenHParams() = src->GetHadoukenHParams();

	// 当たり判定情報のコピー
	for (int i = 0; i < (int)HurtboxType::COUNT; ++i) {
		dst->SetHurtboxBase((HurtboxType)i,
			src->GetHurtboxBaseOffset((HurtboxType)i),
			src->GetHurtboxBaseExtents((HurtboxType)i));
		dst->SetHurtboxCrouch((HurtboxType)i,
			src->GetHurtboxCrouchOffset((HurtboxType)i),
			src->GetHurtboxCrouchExtents((HurtboxType)i));
	}
}