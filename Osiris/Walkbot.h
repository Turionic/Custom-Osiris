#pragma once
#include "SDK/UserCmd.h"
#include "Hacks/Walkbot/nav_file.h"
#include <unordered_map>
#include "SDK/GameEvent.h"

namespace Walkbot {
	bool InitLoad();
	void Run(UserCmd* cmd);

	extern nav_mesh::nav_file map_nav;
	extern std::vector<nav_mesh::vec3_t> curr_path;
	void HeadToPoint(UserCmd* cmd);
	void DeterminePointToTravel();
	void TraversePath(UserCmd* cmd);
	void InsurePath();
	void SetupBuy();
	void SetupTime(GameEvent* event);


	extern std::string NAVFILE;
	extern std::string exception;
	extern bool nav_fileExcept;
	extern bool currentTarget;

	struct nav_positions {
		int A;
		int B;
		int T_Spawn;
		int CT_Spawn;
		std::vector<int> mainPath;
		std::string mapname;
	};

	//extern std::map<std::string,bombsite> bombsites;


	extern std::unordered_map<std::string, nav_positions> maps;
	extern nav_positions nav_inf;
	extern float freezeEnd;
	extern std::vector<int> mainPath;

}