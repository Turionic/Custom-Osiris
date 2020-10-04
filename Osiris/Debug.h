#pragma once
#include <vector>
#include "Interfaces.h"
#include "Hacks/Backtrack.h"
#include <deque>
enum class FrameStage;
class GameEvent;
struct UserCmd;


namespace Debug{





	struct LogItem {
		//int id = 0;
		std::vector<std::wstring> text = {};
		float time_of_creation = 0.0f;
		bool PrintToScreen = true;
		bool PrintToConsole = true;
		std::array<int, 3> color = { 255,255,255 };
	};


	extern std::deque<LogItem> LOG_OUT;





	struct screen {
		int Width = 0;
		int Height = 0;
		int CurrPosW = 0;
		int CurrPosH = 5;
		int CurrColumnMax = 0;

	};

	extern screen Screen;
	
	struct coords {
		int x;
		int y;
	};
	
	struct ColorInfo {
		bool enabled;
		float r;
		float g;
		float b;
		float a;
	};
	
	// Output

	void PrintLog();

	void DrawBox(coords start, coords end);
	void CustomHUD();
	void DrawGraphBox(coords start, coords end, float min_val, float max_val, float val, float ratio, std::wstring name);
	bool SetupTextPos(std::vector <std::wstring>& Text);
	void Draw_Text(std::vector <std::wstring> &Text);
	void AnimStateMonitor() noexcept;
	std::vector<std::wstring> formatRecord(Backtrack::Record record, Entity* entity, int index);

	void BacktrackMonitor() noexcept;
	void DrawDesyncInfo();
	void run();

	//input 

	void Logger(GameEvent *event);

	
	void AnimStateModifier() noexcept;

}
