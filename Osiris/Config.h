#pragma once

#include <array>
#include <filesystem>
#include <memory>
#include <string>

#include "imgui/imgui.h"
#include "nSkinz/config_.hpp"
#include "ConfigStructs.h"
#include "Multipoints.h"

class Config {
public:
    explicit Config(const char*) noexcept;
    void load(size_t) noexcept;
    void save(size_t) const noexcept;
    void add(const char*) noexcept;
    void remove(size_t) noexcept;
    void rename(size_t, const char*) noexcept;
    void reset() noexcept;
    void listConfigs() noexcept;

    constexpr auto& getConfigs() noexcept
    {
        return configs;
    }





    struct Color {
        std::array<float, 3> color{ 1.0f, 1.0f, 1.0f };
        bool rainbow{ false };
        float rainbowSpeed{ 0.6f };
    };

    struct ColorToggle : public Color {
        bool enabled{ false };
    };


    struct grief_ {
        bool teamDamageOverlay{ false };
    } grief;


    struct debug {
        ColorToggle desync_info;
        ColorToggle animStateMon;
        ColorToggle box;
        ColorToggle CustomHUD;
        ColorToggle DamageLog;
        ColorToggle networkchannel;
        ColorToggle clientstate;
        ColorToggle globalvars;
        ColorToggle ResolverOut;
        //out
        bool AnimExtras{ false };
        bool backtrackCount{ false };
        bool resolverDebug{ false };
        int overlay{ 13 };
        int entityid{ 0 };
        bool overlayall{ false };
        bool weight{ false };
        bool showall{ false };
        bool fourfifty{ false };
        bool forcesetupBones{ false };
        bool showshots{ false };
        bool aimbotcoutdebug{ false };
        bool airstucktoggle{ false };
        bool airstuckprovmove{ false };
        bool procmovereturn{ false };
        bool testLag{ false };


        int netSub{ 10 };
        bool in{ false };
        bool out{ false };
        bool outack{ false };
        bool Count{ true };
        bool number{ true };
        bool writedeltastuck{ false };

        //in
        struct {
            bool enabled{ false };
            ColorToggle color;
            int entityid{ 0 };
            bool newesttick{ false };
            bool findactive{ false };
        } backtrack;

        struct {

            bool enabled{ true };
            struct {
                bool enabled{ true };
                bool basicResolver{ true };
                bool overRide{ false };
                int missed_shots{ 0 };
                int missedoffset{ 0 };
                bool goforkill{ false };
                bool missedshotsreset{ false };
            }resolver;

            bool manual{ false };
        } animstatedebug;

        struct {
            bool enabled;
            bool baim;
            bool resolver;
            bool choke;
        } indicators;


        bool ResolverRecords{ false };
        bool TargetOnly{ false };
        bool TraceLimit{ false };
        bool AnimModifier{ true };
        float GoalFeetYaw{ 0.0f };
        float Pitch{ 0.0f };
        float Yaw{ 0.0f };

        struct Graphs { /* Make Custom Color Class */
            bool enabled{ true };
            bool FPSBar{ true };
            bool Ping{ true };
            bool FakeLag{ false };
        } graph;
        bool spoofconvar{ false };
        bool showlagcomp{ false };
        bool showimpacts{ false };
        bool drawother{ false };
        bool grenadeTraject{ false };
        bool tracer{ false };
        bool bullethits{ false };
        bool engineprediction{ true };
        bool movefix{ false };
        bool Animfix{ false };
        bool veloFix{ false };
        bool forceSendOnShot{ false };
        float ABS{ 0.0f };
    } debug;

    struct Aimbot {
        bool enabled{ false };
        bool onKey{ false };
        int key{ 0 };
        int keyMode{ 0 };
        bool aimlock{ false };
        bool silent{ false };
        bool friendlyFire{ false };
        bool visibleOnly{ true };
        bool scopedOnly{ true };
        bool ignoreFlash{ false };
        bool ignoreSmoke{ false };
        bool autoShot{ false };
        bool autoScope{ false };
        float fov{ 0.0f };
        float smooth{ 1.0f };
        int bone{ 0 };
        float maxAimInaccuracy{ 1.0f };
        float maxShotInaccuracy{ 1.0f };
        int minDamage{ 1 };
        float minDamageVis{ 1.0f };
        bool killshot{ false };
        bool betweenShots{ true };
        bool multipointenabled{ false };
        float multidistance{ 0.1f };
        float extrapointdist{ 1.0f };
        float hitChance{ 0.0f };
        float maxTriggerInaccuracy{ 0.0f };
        bool autoStop{ 0 };
        bool oldstyle{ false };
        bool optimize{ false };
        bool baim{ false };
        int baimkey{ 0 };
        int pointstoScan{ 200 };
        bool ensureHC{ false };
        bool considerRecoilInHC{ false };
        bool multiincrease{ false };
        bool dynamicpoints{ false };
        bool veloPointSort{ false };

        bool hitboxes[Multipoints::HITBOX_LAST_ENTRY] = {
            false,
            false,
            false,
            false,
            false,
            false,
            false,
            false,
            false,
            false,
            false,
            false,
            false,
            false,
            false,
            false,
            false,
            false,
        };

        int baimshots{ 8 };
        bool onshot{ false };
        bool pelvisAimOnLBYUpdate{ false };
        bool ShootForLBY{ false };
        bool prioritizeEventBT{ false };
        bool PrioritizeHeadOnNoDesync{false};
        bool OnshotOrDesyncless{ false };
    };
    std::array<Aimbot, 40> aimbot;

    struct Triggerbot {
        bool enabled = false;
        bool friendlyFire = false;
        bool scopedOnly = true;
        bool ignoreFlash = false;
        bool ignoreSmoke = false;
        bool killshot = false;
        bool onKey = false;
        int key = 0;
        int hitgroup = 0;
        int shotDelay = 0;
        int minDamage = 1;
        float burstTime = 0.0f;
    };
    std::array<Triggerbot, 40> triggerbot;

    struct AntiAim {
        bool ignoreSmoke{ false };
        bool enabled{ false };
        bool pitch{ false };
        bool yaw{ false };
        float DeSyncManualSet{ 0.0f };
        float DeSyncMultiplier{ 0.0f };
        int DeSyncMultiplierVariance{ 0 };
        bool bJitter{ false };
        int JitterRange{ 1 };
        int JitterChance{ 100 };
        bool currAng{ false };
        float pitchAngle{ 0.0f };
        bool Spin{ false };
        int state{ 1 };
        bool KeyYaw{ false };
        int BackwardYKey{ false };
        int RightYKey{ false };
        int LeftYKey{ false };
        int ForwardYKey{ false };
        int currYaw{ 0 };
        float manYaw{ 0.0f };
        float clamped{ 0.0f };
        bool currViewBypass{ false };
        bool swapPacket{ false };
        bool swapsides{ false };
        int swapsidekey{ 0 };
        bool forceMovefix{ false };
        bool forceMovefixPost{ false };
        bool forceMovefixSend{ false };
        bool forceMovefixNoSend{ false };
        bool secretdebug{ false };
        bool toWall{ false };
        float LBYValue{ 0 };
        bool forcesendafterLBY{ false };
        bool useAnimState{ false };
        bool airstuckonLBY{ false };
        bool disableLBYbreaking{ false };

        struct test {
            bool forceHide = false;
            bool preserveCountOnLBY = false;
            struct cout {
                int tickCount = 0;
                int commandNumber = 0;
            } cmd;
        } test;

        bool blah{ true };
        bool suppress979{ false };
        bool preBreak{ false };
        int v1{ 120 };
        int v2{ 0 };
        int v3{ -1 };
        int v4{ 120 };
        int v5{ 1 };
        int v6{ 0 };

        struct {
            bool enabled{ false };

            struct {
                bool enabled{ false };
                int key{ 0 };
                int keyMode{ 0 };
                bool keyToggled{ false };
                int maxChoke{ 3 };
            } fakeWalk;
        } general;

        //float --- config->AntiAim.lastlby;
        //float --- config->AntiAim.lbyNextUpdate;
        float lbyNextUpdate{ 0.0f };
        float lastlby{ 0.0f };
        float lastlbyval{ 0.0f };
        float subtractor{ 0.0f };
        bool legitaa{ false };
        bool legitaatest{ false };
        bool micromove{ false };
        bool swapsidesspam;

    } antiAim;


    struct Backtrack {
        bool enabled{ false };
        bool ignoreSmoke{ false };
        bool recoilBasedFov{ false };
        int timeLimit{ 0 };
        bool fakeLatency{ false };
        bool onKey{ false };
        int fakeLatencyKey{ 0 };
        bool backtrackAll{ false };
        int step{ 1 };
        bool extendedrecords{ false };
        float breadcrumbtime{ 800.0f };
        float breadexisttime{ 50.0f };
    } backtrack;

    struct Glow : ColorA {
        bool enabled{ false };
        bool healthBased{ false };
        int style{ 0 };
    };
    std::array<Glow, 21> glow;

    struct Chams {
        struct Material : ColorA {
            bool enabled = false;
            bool healthBased = false;
            bool blinking = false;
            bool wireframe = false;
            bool cover = false;
            bool ignorez = false;
            int material = 0;
        };
        std::array<Material, 7> materials;



    };

    std::unordered_map<std::string, Chams> chams;

    struct StreamProofESP {
        std::unordered_map<std::string, Player> allies;
        std::unordered_map<std::string, Player> enemies;
        std::unordered_map<std::string, Weapon> weapons;
        std::unordered_map<std::string, Projectile> projectiles;
        std::unordered_map<std::string, Shared> lootCrates;
        std::unordered_map<std::string, Shared> otherEntities;
    } streamProofESP;

    struct Font {
        ImFont* tiny;
        ImFont* medium;
        ImFont* big;
    };

    std::vector<std::string> systemFonts{ "Default" };
    std::unordered_map<std::string, Font> fonts;

    struct Visuals {
        bool disablePostProcessing{ false };
        bool inverseRagdollGravity{ false };
        bool noFog{ false };
        bool no3dSky{ false };
        bool noAimPunch{ false };
        bool noViewPunch{ false };
        bool noHands{ false };
        bool noSleeves{ false };
        bool noWeapons{ false };
        bool noSmoke{ false };
        bool noBlur{ false };
        bool noScopeOverlay{ false };
        bool noGrass{ false };
        bool noShadows{ false };
        bool wireframeSmoke{ false };
        bool zoom{ false };
        int zoomKey{ 0 };
        bool thirdperson{ false };
        int thirdpersonKey{ 0 };
        int thirdpersonDistance{ 0 };
        int viewmodelFov{ 0 };
        int fov{ 0 };
        int farZ{ 0 };
        int flashReduction{ 0 };
        float brightness{ 0.0f };
        int skybox{ 0 };


        ColorToggle all;
        


        /*Renderables Window*/


        ColorToggle stolenfromrifk;
        ColorToggle bulletTracers;
        ColorToggle bulletTracersEnemy;
        ColorToggle HitSkeleton;
        ColorToggle grenadeBeams;
        ColorToggle espbulletTracers;
        ColorToggle grenadeBounce;
        ColorToggle world;
        ColorToggle PrecacheWorld;
        ColorToggle NonCachedWorld;
        ColorToggle sky;
        ColorToggle props;
        ColorToggle PrecacheProps;
        ColorToggle NonCached;
        ColorToggle smokecolor;
        ColorToggle NightMode; //ambient light
        ColorToggle dlight;
        float dlightRadius{ 300.f };
        int dlightExponent{ 5 };
        /*End Of Renderables Window*/

        bool matgrey{ false };

        bool deagleSpinner{ false };
        int screenEffect{ 0 };
        int hitEffect{ 0 };
        float hitEffectTime{ 0.6f };
        int hitMarker{ 0 };
        float hitMarkerTime{ 0.6f };
        int playerModelT{ 0 };
        int playerModelCT{ 0 };



        struct ColorCorrection {
            bool enabled = false;
            float blue = 0.0f;
            float red = 0.0f;
            float mono = 0.0f;
            float saturation = 0.0f;
            float ghost = 0.0f;
            float green = 0.0f;
            float yellow = 0.0f;
        } colorCorrection;

        bool Ignore_precache{ true };
        bool Precache_Only{ false };
        bool smoketrans{ false };
        float smokealpha{ 1000.0f };
        bool worldtrans{ false };
        float alphaval{ 1000.0f };
        bool proptrans{ false };
        float propalphaval{ 1000.0f };
        bool smokeflagmanual{ false };
        int customflag{ 0 };
        int beamseg{ 2 };

    } visuals;

    std::array<item_setting, 36> skinChanger;

    struct Sound {
        int chickenVolume{ 100 };

        struct Player {
            int masterVolume{ 100 };
            int headshotVolume{ 100 };
            int weaponVolume{ 100 };
            int footstepVolume{ 100 };
        };

        std::array<Player, 3> players;
    } sound;

    struct Style {
        int menuStyle{ 0 };
        int menuColors{ 0 };
    } style;

    struct Misc {


        bool hideshots{ false };


        int menuKey{ 0x2D }; // VK_INSERT
        bool antiAfkKick{ false };
        bool autoStrafe{ false };
        bool bunnyHop{ false };
        bool customClanTag{ false };
        bool clocktag{ false };
        char clanTag[16];
        bool animatedClanTag{ false };
        bool fastDuck{ false };
        bool moonwalk{ false };
        bool edgejump{ false };
        int edgejumpkey{ 0 };
        bool slowwalk{ false };
        int slowwalkKey{ 0 };
        bool sniperCrosshair{ false };
        bool recoilCrosshair{ false };
        bool autoPistol{ false };
        bool autoReload{ false };
        bool autoAccept{ false };
        bool radarHack{ false };
        bool revealRanks{ false };
        bool revealMoney{ false };
        bool revealSuspect{ false };
        ColorToggle spectatorList;
        ColorToggle watermark;
        bool fixAnimationLOD{ false };
        bool fixBoneMatrix{ false };
        bool fixMovement{ false };
        bool disableModelOcclusion{ false };
        float aspectratio{ 0 };
        bool killMessage{ false };
        std::string killMessageString{ "Gotcha!" };
        bool nameStealer{ false };
        bool disablePanoramablur{ false };
        int banColor{ 6 };
        std::string banText{ "Cheater has been permanently banned from official CS:GO servers." };
        bool fastPlant{ false };
        ColorToggle bombTimer{ 1.0f, 0.55f, 0.0f };
        bool quickReload{ false };
        bool prepareRevolver{ false };
        int prepareRevolverKey{ 0 };
        int hitSound{ 0 };
        int chokedPackets{ 0 };
        int chokedPacketsKey{ 0 };
        float fakelagspeed{ 70.0f };
        bool peeklag{ false };
        int quickHealthshotKey{ 0 };
        bool nadePredict{ false };
        bool fixTabletSignal{ false };
        float maxAngleDelta{ 255.0f };
        bool fakePrime{ false };
        int killSound{0};
        bool showall{false};
        int maxSpeed{0};
        bool svpurebypass{ false };
        bool ForceSpectator{ false };
        struct {
            bool enabled = { false };
            bool toggled = { false };
            int key = { 0 };

        } testshit;
        //bool testshit{ false };



        bool showDamagedone{ true };

            bool perfectShot{ false };
            bool Counter{ false };
            int CounterKey{ 0 };
            //bool Choke{ false };
            int psState{ 0 };
            int nextState{ 0 };

        std::string customKillSound;
        std::string customHitSound;
        PurchaseList purchaseList;

        bool animStateMon{ false };
        int overlay{ 13 };
        int entityid{ 0 };
        bool overlayall{ false };
        bool weight{ false };

        int airstuckkey{ 0 };
        bool airstuck{ false };

        struct {
            bool enabled{ false };
            int doubletapspeed{ 0 };
            int dtKey{ 0 };
        } dt;

        bool walkbot{ false };
        int walkbotspeed{ 255 };

        bool doorspam{ false };

    } misc;

    struct Reportbot {
        bool enabled{ false };
        bool textAbuse{ false };
        bool griefing{ false };
        bool wallhack{ true };
        bool aimbot{ true };
        bool other{ true };
        int target{ 0 };
        int delay{ 1 };
        int rounds{ 1 };
    } reportbot;

    void scheduleFontLoad(const std::string& name) noexcept;
    bool loadScheduledFonts() noexcept;
private:
    std::vector<std::string> scheduledFonts{ "Default" };
    std::filesystem::path path;
    std::vector<std::string> configs;
};

inline std::unique_ptr<Config> config;
