#pragma once

#include "Debug.h"
#include "SDK/GameEvent.h"


namespace Grief {
	void TeamDamageCounter();
	void CalculateTeamDamage(GameEvent* event);


	struct G_Player {
		bool valid = false;
		int CurrDamage = 0;
		int CurrKills = 0;
		int UserID = 0;
		std::wstring Name = L"";
	};

	//extern G_Player Players[65];


}

