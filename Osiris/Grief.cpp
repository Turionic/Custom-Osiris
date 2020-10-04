#include "Grief.h"
#include "SDK/Surface.h"
#include "SDK/EntityList.h"
#include "Interfaces.h"
#include "SDK/Entity.h"
#include "SDK/Surface.h"
#include "SDK/LocalPlayer.h"
#include "Config.h"
#include <mutex>
#include <numeric>
#include <sstream>
#include <codecvt>
#include <locale>


Grief::G_Player Players[65];

void Grief::TeamDamageCounter() {
	interfaces->surface->setTextFont(Surface::font);

	if (!config->grief.teamDamageOverlay || !localPlayer)
		return;

	const auto [screenWidth, screenHeight] = interfaces->surface->getScreenSize();

	//int max;

	std::wstring BASESTRING = L"AAAAAAAAAAAAAAAAAAAAAA"; /* Ghetto But It Works */
	const auto [text_size_w, text_size_h] = interfaces->surface->getTextSize(Surface::font, BASESTRING.c_str());

	Debug::Screen.CurrPosH = (screenHeight / 2) - (text_size_h * 2) - (text_size_h/2) - 2;

	for (int i = 1; i <= interfaces->engine->getMaxClients(); i++) {
		if (i > 65)
			continue;

		auto entity = interfaces->entityList->getEntity(i);

		if (!entity || entity->isDormant()) {
			if (!(i > 65) && (Players[i].valid == true) ) {
				Players[i].valid = false;
			}
			continue;
		}

		if ((entity == localPlayer.get())) {
			continue;
		}

		if (!Players[i].valid && !localPlayer->isOtherEnemy(entity)) {
			Players[i].valid = true;
			Players[i].UserID = entity->getUserId();
			Players[i].Name = std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(entity->getPlayerName(true));
			Players[i].CurrDamage = 0;
			Players[i].CurrKills = 0;
		}

		if (Players[i].UserID != entity->getUserId()) {
			Players[i].valid = false;
		}


	}

	for(int i = 0; i < 65; i ++){
		if (!Players[i].valid) {
			continue;
		}
		int currPosinW = 5;

		/* Draw Health Bar*/

		interfaces->surface->setDrawColor(config->debug.box.color[0], config->debug.box.color[1], config->debug.box.color[2], 180);
		interfaces->surface->drawFilledRect(currPosinW, Debug::Screen.CurrPosH, currPosinW + text_size_w, Debug::Screen.CurrPosH + text_size_h);

		int Pos = (currPosinW + text_size_w) - (((text_size_w + currPosinW) - currPosinW) * (Players[i].CurrDamage / 500.f));

		interfaces->surface->setDrawColor(0, 230, 0, 255);
		interfaces->surface->drawFilledRect( currPosinW, Debug::Screen.CurrPosH, Pos, Debug::Screen.CurrPosH + text_size_h);

		if (Pos > 0) {

			interfaces->surface->setDrawColor(180, 0, 0, 230);
			interfaces->surface->drawFilledRect(Pos+1, Debug::Screen.CurrPosH, currPosinW + text_size_w, Debug::Screen.CurrPosH + text_size_h);
		}

		interfaces->surface->setDrawColor(config->debug.box.color[0], config->debug.box.color[1], config->debug.box.color[2], 180);
		interfaces->surface->drawOutlinedRect(currPosinW, Debug::Screen.CurrPosH, currPosinW + text_size_w, Debug::Screen.CurrPosH + text_size_h);

		/*Draw Damage*/
		std::wstring damage{ L" (" + std::to_wstring(Players[i].CurrDamage) + L"/500 DMG) " };
		//const auto [text_size_kills_w, text_size_kills_h] = interfaces->surface->getTextSize(Surface::font, kills.c_str());
		interfaces->surface->setTextColor(1,1,1,255);
		interfaces->surface->setTextPosition(currPosinW, Debug::Screen.CurrPosH);
		interfaces->surface->printText(damage.c_str());


		currPosinW += text_size_w + 2;
		/* Draw Kills*/

		std::wstring kills{ L" (" + std::to_wstring(Players[i].CurrKills/2) + L"/3 Kills) " };
		const auto [text_size_kills_w, text_size_kills_h] = interfaces->surface->getTextSize(Surface::font, kills.c_str());

		interfaces->surface->setTextColor(config->debug.animStateMon.color);
		interfaces->surface->setTextPosition(currPosinW, Debug::Screen.CurrPosH);
		interfaces->surface->printText(kills.c_str());
		currPosinW += text_size_kills_w;

		/* Draw Name */
		const auto [text_size_name_w, text_size_name_h] = interfaces->surface->getTextSize(Surface::font, Players[i].Name.c_str());

		interfaces->surface->setTextColor(config->debug.animStateMon.color);
		interfaces->surface->setTextPosition(currPosinW, Debug::Screen.CurrPosH);
		interfaces->surface->printText(Players[i].Name.c_str());
		//currPosinW += text_size_name_w;
		Debug::Screen.CurrPosH += text_size_h + 2;

	}

}

#include "fnv.h"

void Grief::CalculateTeamDamage(GameEvent* event) {
	uint32_t eventHash = fnv::hashRuntime(event->getName());
	Entity* attacker = interfaces->entityList->getEntity(interfaces->engine->getPlayerForUserID(event->getInt("attacker")));
	Entity* player = interfaces->entityList->getEntity(interfaces->engine->getPlayerForUserID(event->getInt("userid")));

	if (!attacker || !player)
		return;

	if (attacker->isOtherEnemy(player))
		return;

	if (attacker->isOtherEnemy(localPlayer.get()))
		return;

	if ((attacker == localPlayer.get()))
		return;

	if (!Players[attacker->index()].valid) {
		Players[attacker->index()].valid = true;
		Players[attacker->index()].Name = std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(attacker->getPlayerName(true));
	}

	if (eventHash = fnv::hash("player_hurt")) {
		Players[attacker->index()].CurrDamage += event->getInt("dmg_health");

		if (event->getInt("health") <= 0) {
			Players[attacker->index()].CurrKills += 1;
		}

	}//else if (eventHash = fnv::hash("player_death")) {
	//	Players[attacker->index()].CurrKills += 1;
	//}
}