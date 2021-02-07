#include "Backtrack.h"
#include "Aimbot.h"
#include "../Config.h"
#include "../SDK/Entity.h"
#include "../SDK/FrameStage.h"
#include "../SDK/LocalPlayer.h"
#include "../SDK/UserCmd.h"

#include "Resolver.h"

std::deque<Backtrack::Record> Backtrack::records[65];
std::deque<Backtrack::Record> Backtrack::extended_records[65];
std::deque<Backtrack::Record> Backtrack::invalid_record[2];


std::deque<Backtrack::IncomingSequence>Backtrack::sequences;
Backtrack::Cvars Backtrack::cvars;

bool Backtrack::ImportantTick(std::deque<Record> records) {
    for (Record record : records) {
        if (!valid(record.simulationTime)) {
            continue;
        }
        if (record.onshot || record.lbyUpdated) {
            return true;
        }
    }
    return false;
}

void Backtrack::update(FrameStage stage) noexcept
{
    if (!config->backtrack.enabled || !localPlayer || !localPlayer->isAlive()) {
        for (auto& record : records)
            record.clear();
        if (config->backtrack.extendedrecords) {
            for (auto& record : extended_records) {
                record.clear();
            }
        }
        return;
    }

    

    if (stage == FrameStage::RENDER_START) {

        Record record_inv{};
        Record invalid{};
        invalid.invalid = true;
        if (invalid_record[0].size() < 2) {
            invalid_record[0].push_front(invalid);
            invalid_record[0].push_front(invalid);
        }

        override.Set = false;

        for (int i = 1; i <= interfaces->engine->getMaxClients(); i++) {
            auto entity = interfaces->entityList->getEntity(i);
            if (!entity || entity == localPlayer.get() || entity->isDormant() || !entity->isAlive() || !entity->isOtherEnemy(localPlayer.get())) {
                records[i].clear();
                continue;
            }

            if (!records[i].empty() && (records[i].front().simulationTime == entity->simulationTime()))
                continue;


            Record record{ };
            record.origin = entity->getAbsOrigin();
            record.head = entity->getBonePosition(8);
            record.simulationTime = entity->simulationTime();
            record.mins = entity->getCollideable()->obbMins();
            record.max = entity->getCollideable()->obbMaxs();
            record.PreviousAct = -1;

            if (config->debug.animstatedebug.resolver.enabled) {
                Resolver::Record *r_record = &Resolver::PlayerRecords.at(entity->index());
                if (r_record && !r_record->invalid) {
                    record.lbyUpdated = r_record->lbyUpdated;
                    record.onshot = r_record->onshot;
                    record.move = r_record->move;
                    record.noDesync = r_record->noDesync;
                }
            }


            //record.wasVisible = entity->isVisible();
            //auto AnimState = entity->getAnimstate();

            /*
            //float heighest_weight = 0.0f;
            if (AnimState) {

                auto countBones = *(int*)(entity + 0x291C);
                record.countBones = countBones;

                for (int b = 0; b < entity->getAnimationLayerCount(); b++) {
                    auto AnimLayer = entity->getAnimationLayer(b);
                    int currAct = entity->getSequenceActivity(AnimLayer->sequence);
                    if (((currAct == ACT_CSGO_FIRE_PRIMARY) ||  (currAct == ACT_CSGO_FIRE_PRIMARY_OPT_1) || (currAct == ACT_CSGO_FIRE_PRIMARY_OPT_2)) && (AnimLayer->weight > 0.1) && (.9> AnimLayer->cycle)) {
                        record.PreviousAct = currAct;
                        record.onshot = true;
                        break;
                    }
                    else {
                        record.PreviousAct = currAct;
                    }
                };
            }
            */

            //record.prevhealth = entity->health();


            /*
            if (!(!records[i].empty() && records[i].size() && Backtrack::valid(records[i].front().simulationTime))) {
                record.missedshots = 0;
                record.prevhealth = entity->health();
                record.wasTargeted = false;
                record.wasUpdated = false;
                record.resolvedaddition = entity->eyeAngles().y;
            }
            else {
                record.missedshots = records[i].front().missedshots;
                //if ((records[i].front().prevhealth == entity->health) && records[i].front().wasTargeted && records[i].front().shotAt){
                //    record.missedshots++;
                //}
                //record.shotAt = false;
                record.prevhealth = entity->health();
                record.wasTargeted = records[i].front().wasTargeted;
                record.wasUpdated = records[i].front().wasUpdated;
                record.resolvedaddition = records[i].front().resolvedaddition;
                //record.prevAngles = records[i].front().prevAngles;
                record.lastworkmissed = records[i].front().lastworkmissed;

            }
            */




            entity->setupBones(record.matrix, 256, 0x7FF00, memory->globalVars->currenttime);

            records[i].push_front(record);

            int timeLimit = config->backtrack.timeLimit; if (timeLimit > 200) timeLimit = 200;
            while (records[i].size() > 3 && records[i].size() > static_cast<size_t>(timeToTicks(static_cast<float>(timeLimit) / 1000.f + getExtraTicks())))
                records[i].pop_back();

            if (auto invalid = std::find_if(std::cbegin(records[i]), std::cend(records[i]), [](const Record& rec) { return !valid(rec.simulationTime); }); invalid != std::cend(records[i]))
                records[i].erase(invalid, std::cend(records[i]));

            if (config->backtrack.extendedrecords) {
                if (extended_records[i].empty()) {
                    if (!(records[i].empty())) {
                        extended_records[i].push_front(records[i].front());
                    }
                }


                if ((!extended_records[i].empty()) && (!records[i].empty())) {
                    if (extended_records[i].back().invalid) {
                        extended_records[i].pop_back();
                    }
                    if ((records[i].size() > 1) && (extended_records[i].size() > 1)) {
                        if ((records[i].front().simulationTime - extended_records[i].front().simulationTime) > (config->backtrack.breadcrumbtime / 1000)) {
                            extended_records[i].push_front(records[i].front());
                        }
                    }
                    else if ((records[i].size() > 1) && (extended_records[i].size() <= 1)) {
                        extended_records[i].push_front(records[i].front());
                    }
                }
                if (extended_records[i].size() > 35) {
                    extended_records[i].erase(extended_records[i].begin(), extended_records[i].end());
                }
            }




        }
    }
}


#include "../Debug.h"
#include <numeric>
#include <sstream>
#include <codecvt>
#include <locale>
#include <string>
#include <iostream>
#include <cstddef>
void Backtrack::SetOverride(Entity* entity, Record record) noexcept {
    /* Sets Backtrack Override, will ignore backtrack calculation, and instead force the tick choosen to the one specified by the simtime
    in record
    entity = Entity ptr to target
    record = record choosen to override too
    */

    Debug::LogItem item;
    item.PrintToScreen = false;
    item.text.push_back(std::wstring{ L"[Backtrack] Overriding Backtrick record to record taken at simtime " + std::to_wstring(record.simulationTime) });
    if (record.lbyUpdated) {
        item.text.push_back(std::wstring{ L"[Backtrack] Backtrack record Contains Updating LBY of player"});
        record.lbyUpdated = false;
    }
    else if (record.onshot) {
        item.text.push_back(std::wstring{ L"[Backtrack] Backtrack record Contains Onshot tick" });
        record.onshot = false;
    }
    if (config->debug.backtrackCount) {
        Debug::LOG_OUT.push_front(item);
    }
    override.Set = true;
    override.entity = entity;
    override.record = record;
}


void Backtrack::run(UserCmd* cmd) noexcept
{
    if (!config->backtrack.enabled)
        return;

    if (!(cmd->buttons & UserCmd::IN_ATTACK))
        return;

    if (!localPlayer)
        return;
    
    if (config->backtrack.onKey) {
        if (GetAsyncKeyState(config->backtrack.fakeLatencyKey))
            config->backtrack.fakeLatency = !(config->backtrack.fakeLatency);
            if (config->backtrack.fakeLatency && (config->backtrack.timeLimit < 200)) {
                config->backtrack.timeLimit += 200;
            }
            else if (!(config->backtrack.fakeLatency) &&  config->backtrack.timeLimit > 200) {
                config->backtrack.timeLimit -= 200;
            }
    }
    
    auto localPlayerEyePosition = localPlayer->getEyePosition();

    auto bestFov{ 255.f };
    Entity * bestTarget{ };
    int bestTargetIndex{ };
    Vector bestTargetOrigin{ };
    Vector bestTargetHead{ };
    int bestRecord{ };

    const auto aimPunch = localPlayer->getAimPunch();

    if (!override.Set) {
        for (int i = 1; i <= interfaces->engine->getMaxClients(); i++) {
            auto entity = interfaces->entityList->getEntity(i);
            if (!entity || entity == localPlayer.get() || entity->isDormant() || !entity->isAlive()
                || !entity->isOtherEnemy(localPlayer.get()))
                continue;

            auto origin = entity->getAbsOrigin();
            auto head = entity->getBonePosition(8);

            auto angle = Aimbot::calculateRelativeAngle(localPlayerEyePosition, head, cmd->viewangles + (config->backtrack.recoilBasedFov ? aimPunch : Vector{ }));
            auto fov = std::hypotf(angle.x, angle.y);
            if (fov < bestFov) {
                bestFov = fov;
                bestTarget = entity;
                bestTargetIndex = i;
                bestTargetOrigin = origin;
                bestTargetHead = head;
            }
        }
    }

    if (bestTarget && !override.Set) {
        if (records[bestTargetIndex].size() <= 3 || (!config->backtrack.ignoreSmoke && memory->lineGoesThroughSmoke(localPlayer->getEyePosition(), bestTargetHead, 1)))
            return;

        bestFov = 255.f;

        for (size_t i = 0; i < records[bestTargetIndex].size(); i++) {
            auto& record = records[bestTargetIndex][i];
            if (!valid(record.simulationTime))
                continue;

            auto angle = Aimbot::calculateRelativeAngle(localPlayerEyePosition, record.head, cmd->viewangles + (config->backtrack.recoilBasedFov ? aimPunch : Vector{ }));
            auto fov = std::hypotf(angle.x, angle.y);
            if (fov < bestFov) {
                bestFov = fov;
                bestRecord = i;
            }
        }
    }

    if ((bestRecord || override.Set) && (cmd->buttons & UserCmd::IN_ATTACK)) {
        Record record;

        if (override.Set) {
            record = override.record;
            bestTarget = override.entity;
        }
        else {
            record = records[bestTargetIndex][bestRecord];
        }
        memory->setAbsOrigin(bestTarget, record.origin);

        /*This is pasted cause why not paste it? it's like 10 lines of code, what am I gonna do, chance the variable names?*/
        // Calculate lerp remainder
        float lerp_remainder = fmodf(getLerp(), memory->globalVars->intervalPerTick);

        // Get real simulation time and calculate interp fraction
        float real_target_time = record.simulationTime;
        float frac = 0.f;
        if (lerp_remainder > 0.f)
        {
            real_target_time += memory->globalVars->intervalPerTick - lerp_remainder;
            // Trigger float inaccuracies (if there is any) and get same frac as the server
            frac = (real_target_time - record.simulationTime) / memory->globalVars->intervalPerTick;
        }

        /* insert quick maths for origin/angle/mins/maxs interpolation */

        // Apply correct tick_count
        cmd->tickCount = timeToTicks((real_target_time + getLerp()));
        record.onshot = false;
        record.lbyUpdated = false;
    }
}

void Backtrack::AddLatencyToNetwork(NetworkChannel* network, float latency) noexcept
{
    for (auto& sequence : sequences)
    {
        if (memory->globalVars->serverTime() - sequence.servertime >= latency)
        {
            network->InReliableState = sequence.inreliablestate;
            network->InSequenceNr = sequence.sequencenr;
            break;
        }
    }
}

void Backtrack::UpdateIncomingSequences(bool reset) noexcept
{
    static float lastIncomingSequenceNumber = 0.f;

    int timeLimit = config->backtrack.timeLimit; if (timeLimit <= 200) { timeLimit = 0; }
    else { timeLimit = 200; }
    if (!config->backtrack.fakeLatency || timeLimit == 0)
        return;

    if (!localPlayer)
        return;

    auto network = interfaces->engine->getNetworkChannel();
    if (!network)
        return;

    if (network->InSequenceNr != lastIncomingSequenceNumber)
    {
        lastIncomingSequenceNumber = network->InSequenceNr;

        IncomingSequence sequence{ };
        sequence.inreliablestate = network->InReliableState;
        sequence.sequencenr = network->InSequenceNr;
        sequence.servertime = memory->globalVars->serverTime();
        sequences.push_front(sequence);
    }

    while (sequences.size() > 2048)
        sequences.pop_back();
}

int Backtrack::timeToTicks(float time) noexcept
{
    return static_cast<int>(0.5f + time / memory->globalVars->intervalPerTick);
}
