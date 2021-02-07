#pragma once

#include <array>
#include <memory>
#include <string>
#include <type_traits>
#include <Windows.h>
#include <Psapi.h>
#include "SDK/ModelInfo.h"

class ClientMode;
class ClientState;

class Entity;
class GameEventDescriptor;
class GameEventManager;
class Input;
class ItemSystem;
class KeyValues;
class MoveHelper;
class MoveData;
class ViewRender;
class WeaponSystem;
class MemAlloc;

class IViewRenderBeams;

struct ActiveChannels;
struct Channel;
struct GlobalVars;
struct GlowObjectManager;
struct Trace;
struct Vector;

inline DWORD oCL_Move;
typedef void(__cdecl* CL_MoveFn)(float&, bool&);


inline LPVOID oCNET_SendNetMessage;

class Memory {
public:
    Memory() noexcept;

    uintptr_t present;
    uintptr_t reset;

    ClientMode* clientMode;


    Input* input;
    GlobalVars* globalVars;
    GlowObjectManager* glowObjectManager;

    bool* disablePostProcessing;

    std::add_pointer_t<void __fastcall(const char*)> loadSky;
    std::add_pointer_t<void __fastcall(const char*, const char*)> setClanTag;
    uintptr_t cameraThink;
    std::add_pointer_t<bool __stdcall(const char*)> acceptMatch;
    std::add_pointer_t<bool __cdecl(Vector, Vector, short)> lineGoesThroughSmoke;
    int(__fastcall* getSequenceActivity)(void*, StudioHdr*, int);
    bool(__thiscall* isOtherEnemy)(Entity*, Entity*);
    uintptr_t hud;
    int*(__thiscall* findHudElement)(uintptr_t, const char*);
    int(__thiscall* clearHudWeapon)(int*, int);
    std::add_pointer_t<ItemSystem* __cdecl()> itemSystem;
    void(__thiscall* setAbsOrigin)(Entity*, const Vector&);
    uintptr_t listLeaves;
    int* dispatchSound;
    std::add_pointer_t<bool __cdecl(float, float, float, float, float, float, Trace&)> traceToExit;
    ViewRender* viewRender;
    uintptr_t drawScreenEffectMaterial;
    std::add_pointer_t<bool __stdcall(const char*, const char*)> submitReport;
    uint8_t* fakePrime;
    std::add_pointer_t<void __cdecl(const char* msg, ...)> debugMsg;
    std::add_pointer_t<void __cdecl(const std::array<std::uint8_t, 4>& color, const char* msg, ...)> conColorMsg;
    float* vignette;
    int(__thiscall* equipWearable)(void* wearable, void* player);
    int* predictionRandomSeed;
    MoveData* moveData;
    MoveHelper* moveHelper;
    std::uintptr_t keyValuesFromString;
    KeyValues*(__thiscall* keyValuesFindKey)(KeyValues* keyValues, const char* keyName, bool create);
    void(__thiscall* keyValuesSetString)(KeyValues* keyValues, const char* value);
    WeaponSystem* weaponSystem;
    std::add_pointer_t<const char** __fastcall(const char* playerModelName)> getPlayerViewmodelArmConfigForPlayerModel;
    GameEventDescriptor* (__thiscall* getEventDescriptor)(GameEventManager* _this, const char* name, int* cookie);
    void(__thiscall* setAbsAngle)(Entity*, const Vector&);
    uintptr_t UpdateState;
    uintptr_t CreateState;
    uintptr_t InvalidateBoneCache;
    uintptr_t CNetChan_SendNetMessage;



    ClientState* clientState;
    void* WriteUsercmdDeltaToBufferReturn;
    uintptr_t WriteUsercmd;
    CL_MoveFn* CL_MoveCall;
    
    MemAlloc* memalloc;

    IViewRenderBeams* renderBeams;

    ActiveChannels* activeChannels;
    Channel* channels;

    std::uintptr_t returnSequenceLocation();

    static std::uintptr_t findPattern_ex(const wchar_t* module, const char* pattern) noexcept {
        return findPattern(module, pattern);
    }

private:
    static std::uintptr_t findPattern(const wchar_t* module, const char* pattern) noexcept
    {
        static auto id = 0;
        ++id;

        if (HMODULE moduleHandle = GetModuleHandleW(module)) {
            if (MODULEINFO moduleInfo; GetModuleInformation(GetCurrentProcess(), moduleHandle, &moduleInfo, sizeof(moduleInfo))) {
                auto start = static_cast<const char*>(moduleInfo.lpBaseOfDll);
                const auto end = start + moduleInfo.SizeOfImage;

                auto first = start;
                auto second = pattern;

                while (first < end && *second) {
                    if (*first == *second || *second == '?') {
                        ++first;
                        ++second;
                    } else {
                        first = ++start;
                        second = pattern;
                    }
                }

                if (!*second)
                    return reinterpret_cast<std::uintptr_t>(start);
            }
        }
        MessageBoxA(NULL, ("Failed to find pattern #" + std::to_string(id) + '!').c_str(), "Osiris", MB_OK | MB_ICONWARNING);
        return 0;
    }
};

inline std::unique_ptr<const Memory> memory;
