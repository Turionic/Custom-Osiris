#pragma once
#include "../SDK/UserCmd.h"
//struct UserCmd;

namespace Tickbase
{
	void shiftTicks(int, UserCmd*, bool = false) noexcept;
	void run(UserCmd*) noexcept;

	struct Tick
	{
		int	maxUsercmdProcessticks{ 17 }; //on valve servers this is 8 ticks, always do +1 command
		int ticksAllowedForProcessing{ maxUsercmdProcessticks };
		int chokedPackets{ 0 };
		int fakeLag{ 0 };
		int tickshift{ 0 };
		int tickbase{ 0 };
		int commandNumber{ 0 };
		int ticks{ 0 };
	};

	struct ForceCMD {
		UserCmd cmd;
		bool setcmd = false;
		bool dtPrevTick = false;
	};

	extern ForceCMD newCmd;

	inline std::unique_ptr<Tick> tick;
}
