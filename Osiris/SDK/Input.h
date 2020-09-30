#pragma once

#include "Vector.h"
#include "VirtualMethod.h"
class Input {
public:
    std::byte pad[12];
    bool isTrackIRAvailable;
    bool isMouseInitialized;
    bool isMouseActive;
    std::byte pad1[158];
    bool isCameraInThirdPerson;
    std::byte pad2;
    Vector cameraOffset;

    UserCmd* GetUserCmd(int nSlot, int sequence_number)
    {
        //typedef UserCmd* (__thiscall* GetUserCmd_t)(void*, int, int);
        return VirtualMethod::call<UserCmd*, 8, int, int>(this, nSlot, sequence_number);
    }

};

