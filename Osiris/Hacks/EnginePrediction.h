#pragma once

struct UserCmd;

namespace EnginePrediction
{
    void run(UserCmd* cmd) noexcept;
    int getFlags() noexcept;
}


/*
namespace PredictionSys {
	void RunEnginePred(UserCmd*) noexcept;
	void EndEnginePred() noexcept;
}
*/
