#pragma once

// CPUの強さ
enum class CpuLevel
{
	EASY,
	NORMAL,
	HARD,
	COUNT
};

// メニューで選んだ対戦形式。シーンをまたいで SceneGame に渡す
struct MatchConfig
{
	static inline bool vsCpu = false;
	static inline CpuLevel cpuLevel = CpuLevel::NORMAL;
};
