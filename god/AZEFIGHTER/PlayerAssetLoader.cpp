#include "PlayerAssetLoader.h"
#include "Player.h"
#include "Model.h"
#include <string>

namespace
{
	struct AnimEntry { const char* file; const char* name; };

	// knight モデルの共通アニメーション一覧
	const AnimEntry kCommonAnims[] = {
		{ "Assets/Model/knight/Walking.fbx",     "Walk" },
		{ "Assets/Model/knight/WalkBack.fbx",    "WalkBack" },
		{ "Assets/Model/knight/CrouchIdle.fbx",  "CrouchIdle" },
		{ "Assets/Model/knight/Jump.fbx",        "Jump" },
		{ "Assets/Model/knight/Damage.fbx",      "Damage" },
		{ "Assets/Model/knight/Down.fbx",        "Down" },
		{ "Assets/Model/knight/WakeUp.fbx",      "WakeUp" },
		{ "Assets/Model/knight/Hadouken.fbx",    "Hadouken" },
		{ "Assets/Model/knight/Shoryuken.fbx",   "Shoryuken" },
		{ "Assets/Model/knight/Death.fbx",       "Death" },
	};
}

void PlayerAssetLoader::LoadCommonAnimations(Player* player)
{
	if (!player) return;
	Model* m = player->GetModel();
	if (!m) return;

	for (const auto& a : kCommonAnims)
	{
		m->LoadAnimation(a.file, a.name, true);
	}

	// 通常技は技の表から読む。FBX のファイル名はアニメーション名と同じにしている
	for (int i = 0; i < NORMAL_MOVE_COUNT; ++i)
	{
		const char* name = GetNormalMoveInfo((NormalMove)i).animName;
		const std::string file = std::string("Assets/Model/knight/") + name + ".fbx";
		m->LoadAnimation(file.c_str(), name, true);
	}
}