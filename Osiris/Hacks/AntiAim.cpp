#include "AntiAim.h"
#include "Animations.h"
#include "Resolver.h"
#include "../Interfaces.h"
#include "../SDK/Engine.h"
#include "../SDK/Entity.h"
#include "../SDK/EntityList.h"
#include "../SDK/NetworkChannel.h"
#include "../SDK/UserCmd.h"
#include "../SDK/GlobalVars.h"
#include "../SDK/WeaponData.h"
#include "../SDK/WeaponId.h"
#include "../SDK/ConVar.h"
#include "../Memory.h"
#include "../SDK/Vector.h"

#include "../Interfaces.h"
#include "../SDK/Surface.h"

#include <stdlib.h>
#include <cmath> 


//extern float AntiAim::lastlbyval = 0;

float AntiAim::DEG2RAD(float degree) {
    return (float)(degree * 22.0 / (180.0 * 7.0));
}

void AntiAim::CorrectMovement(float OldAngleY, UserCmd* pCmd, float fOldForward, float fOldSidemove)
{
    //side/forward move correction
    float deltaView = pCmd->viewangles.y - OldAngleY;
    float f1;
    float f2;

    if (OldAngleY < 0.f)
        f1 = 360.0f + OldAngleY;
    else
        f1 = OldAngleY;

    if (pCmd->viewangles.y < 0.0f)
        f2 = 360.0f + pCmd->viewangles.y;
    else
        f2 = pCmd->viewangles.y;

    if (f2 < f1)
        deltaView = abs(f2 - f1);
    else
        deltaView = 360.0f - abs(f1 - f2);
    deltaView = 360.0f - deltaView;

    pCmd->forwardmove = cos(DEG2RAD(deltaView)) * fOldForward + cos(DEG2RAD(deltaView + 90.f)) * fOldSidemove;
    pCmd->sidemove = sin(DEG2RAD(deltaView)) * fOldForward + sin(DEG2RAD(deltaView + 90.f)) * fOldSidemove;
}

AntiAim::playerAAInfo AntiAim::LocalPlayerAA;


extern bool AntiAim::lbyNextUpdated = false;
extern bool AntiAim::lbyNextUpdatedPrevtick = false; //ngl this is a terrible "fix"


static int timeToTicks(float time){  
	return static_cast<int>(0.5f + time / memory->globalVars->intervalPerTick); 
};


bool AntiAim::LBY_UPDATE(Entity* entity = localPlayer.get(), int TicksToPredict = 0, bool UseAnim = config->antiAim.useAnimState) {
	float servertime = memory->globalVars->serverTime();
	float Velocity;

	if (!(TicksToPredict == 0)) { servertime += memory->globalVars->intervalPerTick * TicksToPredict; }

	// if(!TicksToPredict){} <-- this is a shit way to determine whether to set lbyNextUpdate but oh well
	if (!UseAnim) {
		Velocity = entity->getVelocity().length2D();
	}
	else {
		AnimState* as_EntityAnimState = entity->getAnimstate();
		if (!as_EntityAnimState) {
			lbyNextUpdated = false;
			return false;
		}
		Velocity = as_EntityAnimState->speed_2d;
	}

	if (TicksToPredict == 0) { lbyNextUpdatedPrevtick = lbyNextUpdated; }
	
	if (Velocity > 0.1f) { //LBY updates on any velocity
		if (TicksToPredict == 0) { LocalPlayerAA.lbyNextUpdate = 0.22f + servertime; lbyNextUpdated = true;}
		return false; // FALSE, as I dont want to run LBY breaking code on movement!, however it does update!!!
	}


	if (LocalPlayerAA.lbyNextUpdate >= servertime) {
		lbyNextUpdated = false;
		return false;
	}
	else if (LocalPlayerAA.lbyNextUpdate < servertime) { // LBY ipdates on no velocity, .22s after last velocity, 1.1s after previous no-velocity
		if (TicksToPredict == 0) { LocalPlayerAA.lbyNextUpdate = servertime + 1.1f; lbyNextUpdated = true;}
		return true;
	}

	return false;
}



/*
	float updateTime;
	float lastUpdate;
	float wasmoving;
	bool performBreak;

	void lbyBreaker(CUserCmd *pCmd, bool &bSendPacket) {
		IClientEntity* LocalPlayer = hackManager.pLocal();
		float flServerTime = (float)(LocalPlayer->GetTickBase()  * Interfaces::Globals->interval_per_tick);
		float Velocity = LocalPlayer->GetVelocity().Length2D();

		if (!performBreak) {
			if (Velocity > 1.f && (LocalPlayer->GetFlags() & FL_ONGROUND)) {
				lastUpdate = flServerTime;
				wasmoving = true;
			}
			else {
				if (wasmoving && flServerTime - lastUpdate > 0.22f && (LocalPlayer->GetFlags() & FL_ONGROUND)) {
					wasmoving = false;
					performBreak = true;
				}
				else if (flServerTime - lastUpdate > 1.1f && (LocalPlayer->GetFlags() & FL_ONGROUND)) {
					performBreak = true;
				}
				else {
				}
			}
		}
		else {
			bSendPacket = false;
			pCmd->viewangles.y += 105.f;
			lastUpdate = flServerTime;
			performBreak = false;
		}
	}


*/
#include "../SDK/PhysicsSurfaceProps.h"
#include "../SDK/WeaponData.h"
#include "../SDK/EngineTrace.h"
static float handleBulletPenetration(SurfaceData* enterSurfaceData, const Trace& enterTrace, const Vector& direction, Vector& result, float penetration, float damage) noexcept
{
	Vector end;
	Trace exitTrace;
	__asm {
		mov ecx, end
		mov edx, enterTrace
	}
	if (!memory->traceToExit(enterTrace.endpos.x, enterTrace.endpos.y, enterTrace.endpos.z, direction.x, direction.y, direction.z, exitTrace))
		return -1.0f;

	SurfaceData* exitSurfaceData = interfaces->physicsSurfaceProps->getSurfaceData(exitTrace.surface.surfaceProps);

	float damageModifier = 0.16f;
	float penetrationModifier = (enterSurfaceData->penetrationmodifier + exitSurfaceData->penetrationmodifier) / 2.0f;

	if (enterSurfaceData->material == 71 || enterSurfaceData->material == 89) {
		damageModifier = 0.05f;
		penetrationModifier = 3.0f;
	}
	else if (enterTrace.contents >> 3 & 1 || enterTrace.surface.flags >> 7 & 1) {
		penetrationModifier = 1.0f;
	}

	if (enterSurfaceData->material == exitSurfaceData->material) {
		if (exitSurfaceData->material == 85 || exitSurfaceData->material == 87)
			penetrationModifier = 3.0f;
		else if (exitSurfaceData->material == 76)
			penetrationModifier = 2.0f;
	}

	damage -= 11.25f / penetration / penetrationModifier + damage * damageModifier + (exitTrace.endpos - enterTrace.endpos).squareLength() / 24.0f / penetrationModifier;

	result = exitTrace.endpos;
	return damage;
}

static float canScan(Entity* entity, float* fraction, Vector EyePos) noexcept
{
	if (!localPlayer)
		return false;


	auto activeWeapon = entity->getActiveWeapon();
	if (!activeWeapon)
		return false;
	const WeaponInfo* weaponData = activeWeapon->getWeaponData();

	Vector destination = EyePos;
	float damage{ static_cast<float>(weaponData->damage) };
	float tracefrac = 2.0f;
	Vector start{ entity->getEyePosition() };
	Vector direction{ destination - start }; 
	direction /= direction.length();

	int hitsLeft = 4;

	while (damage >= 1.0f && hitsLeft) {
		Trace trace;
		interfaces->engineTrace->traceRay({ start, destination }, 0x4600400B, entity, trace);

		if (trace.fraction == 1.0f)
			*fraction = trace.fraction;
			return 150;

		if (trace.entity == entity && trace.hitgroup > HitGroup::Generic && trace.hitgroup <= HitGroup::RightLeg) {
			damage = HitGroup::getDamageMultiplier(trace.hitgroup) * damage * powf(weaponData->rangeModifier, trace.fraction * weaponData->range / 500.0f);

			if (float armorRatio{ weaponData->armorRatio / 2.0f }; HitGroup::isArmored(trace.hitgroup, trace.entity->hasHelmet()))
				damage -= (trace.entity->armor() < damage * armorRatio / 2.0f ? trace.entity->armor() * 4.0f : damage) * (1.0f - armorRatio);

			*fraction = trace.fraction;
			return damage;


		}
		const auto surfaceData = interfaces->physicsSurfaceProps->getSurfaceData(trace.surface.surfaceProps);

		tracefrac = trace.fraction;
		if (0.1f > surfaceData->penetrationmodifier)
			break;

		damage = handleBulletPenetration(surfaceData, trace, direction, start, weaponData->penetration, damage);
	}
	*fraction = tracefrac;
	return damage;
}

#include "AutoWall.h"


//float AntiAim::BestAngle() noexcept {
//
//}

float AntiAim::ToWall() noexcept{
							/* --- TODO ---  */
	/* Replace shamless rip of aimbot code with a simple trace function!*/
	/* This code is just a shitty POC to myself, you want it better?    */
	/* Then fix it and stop looking through my source, cause its ass!   */
	//float angles[] = { 90, -90, 180, 0};
	//int side = -1;
	
	//float minFraction = 2.0f;
	// Autowall->EntityDamage(Resolver::TargetedEntity, eyePos);
	Entity* entity;
	if (!Resolver::TargetedEntity || Resolver::TargetedEntity->isDormant() || !Resolver::TargetedEntity->isAlive() || Resolver::TargetedEntity == localPlayer.get() || localPlayer->velocity().length2D() > 80) {
		return 180; // Will make this check for other entities at some point
	}
	else {
		entity = Resolver::TargetedEntity;
	}

	Vector ObbMin = Resolver::TargetedEntity->origin();
	Vector ObbMax = Resolver::TargetedEntity->origin();
	ObbMin += Resolver::TargetedEntity->getCollideable()->obbMins().x * 2.0f;
	ObbMax += Resolver::TargetedEntity->getCollideable()->obbMaxs().x * 2.0f;
	ObbMin += Resolver::TargetedEntity->getCollideable()->obbMins().y * 2.0f;
	ObbMax += Resolver::TargetedEntity->getCollideable()->obbMaxs().y * 2.0f;
	ObbMin.z = ObbMax.z;

	float offset = 180;

	float minDamage = ((Autowall->EntityDamageFromVector(Resolver::TargetedEntity, localPlayer->getRotatedBonePos((offset * 22.0) / (180.0 * 7.0)), ObbMin) + Autowall->EntityDamageFromVector(Resolver::TargetedEntity, localPlayer->getRotatedBonePos((offset * 22.0) / (180.0 * 7.0)), ObbMax)) / 2);
	if (minDamage < 20) {
		return 180;
	}

	for (int i = 90; i < 270; i += 45) {
		Vector point = localPlayer->getRotatedBonePos((i * 22.0) / (180.0 * 7.0));
		float Damage = ((Autowall->EntityDamageFromVector(Resolver::TargetedEntity, point, ObbMin) + Autowall->EntityDamageFromVector(Resolver::TargetedEntity, point, ObbMax))/2);
		if (Damage < 5) {
			minDamage = Damage;
			offset = i;
		}
		//Resolver::TargetedEntity
	}

	if (minDamage > 80) {
		return 180;
	}

	if (offset > 180) {
		offset = (offset - 180) + -180;
	}
	if (offset < -180) {
		offset = (offset + 180) + 180;
	}
	//else if (offset < 180) {
	//}


	return offset;


	

}

#include "Misc.h"


void AntiAim::run(UserCmd* cmd, const Vector& previousViewAngles, const Vector& currentViewAngles, bool& sendPacket) noexcept
{
	const auto activeWeapon = localPlayer->getActiveWeapon();
	if (!activeWeapon)
		return;

	if (activeWeapon->isGrenade()) {
		if (activeWeapon->isInThrow())
			return;
	}


    if (config->antiAim.enabled) {
        if (!localPlayer)
            return;
		 
		LocalPlayerAA.real = cmd->viewangles;
		

		if (config->antiAim.micromove) {
			if (!(config->antiAim.general.fakeWalk.enabled) && (localPlayer->getVelocity().length2D() < 1.016f)) {
				cmd->forwardmove = 0;
				if (cmd->buttons & UserCmd::IN_DUCK)
					cmd->sidemove = cmd->tickCount % 2 ? 3.25f : -3.25f;
				else
					cmd->sidemove = cmd->tickCount % 2 ? 1.0f : 1.0f;
			}
		}



        if (config->antiAim.Spin == true) {
            if ((!(cmd->buttons & cmd->IN_ATTACK) || (cmd->buttons & cmd->IN_ATTACK && activeWeapon->nextPrimaryAttack() > memory->globalVars->serverTime()))
                && !(cmd->buttons & cmd->IN_USE))
            {

                
                cmd->viewangles.y = currentViewAngles.y + 45 * (config->antiAim.state % 8);
                //cmd->viewangles.y += 45 * (config->antiAim.state % 8);
                
                if (!(config->antiAim.bJitter)) {
                    CorrectMovement(currentViewAngles.y, cmd, cmd->forwardmove, cmd->sidemove);
                }

                config->antiAim.state++;

            }
        }


        if (config->antiAim.yaw && (currentViewAngles == cmd->viewangles)) {

			if (config->antiAim.pitch && (cmd->viewangles.x == currentViewAngles.x))
				cmd->viewangles.x = config->antiAim.pitchAngle;

			float angle = config->antiAim.manYaw;
			// DESYNC
			//if (!config->antiAim.yaw) {
			//	cmd->viewangles.y += angle;
			//}

			if (config->antiAim.toWall) {
				LocalPlayerAA.real.y += ToWall();
			}
			else {
				LocalPlayerAA.real.y += angle;
			}


			if (LocalPlayerAA.lbyNextUpdate < (memory->globalVars->serverTime() - 8.0f)) {
				cmd->sidemove = cmd->tickCount % 2 ? 2.f : -2.f;
			}




			if ((interfaces->engine->getNetworkChannel()->chokedPackets < 8) && !(config->antiAim.general.fakeWalk.enabled && config->antiAim.general.fakeWalk.keyToggled) && !config->misc.testshit.toggled){ //&& !AntiAim::lbyNextUpdatedPrevtick
				sendPacket = cmd->tickCount % 2 ? false : true;
			}

			bool b_sendPacket = sendPacket;

			if (config->antiAim.swapPacket) {
				b_sendPacket = !sendPacket;
			}

			if (GetAsyncKeyState(config->antiAim.swapsidekey)) {
				LocalPlayerAA.b_Side = !LocalPlayerAA.b_Side;
				cmd->forwardmove = 10.0f;
			}

			bool side = LocalPlayerAA.b_Side;
			if (config->antiAim.swapsidesspam) {
					side = (cmd->tickCount % 10) ? !LocalPlayerAA.b_Side : LocalPlayerAA.b_Side;
					LocalPlayerAA.b_Side = side;
					cmd->viewangles.y += cmd->tickCount % 200 ? 0 : -58;
					cmd->viewangles.y += cmd->tickCount % 900 ? 0 : 180;
			}

			/*
			
			if(LBY_UPDATE()){

				cmd->viewangles.y = LocalPlayerAA.real.y + (v1 * (side ? 1 : -1));	

			}
			
			
			*/
			/*
			if (LBY_UPDATE()) {

				
				cmd->viewangles.y = LocalPlayerAA.real.y + (120 * (side ? 1 : -1));
				if (interfaces->engine->getNetworkChannel()->chokedPackets < 8) {
					sendPacket = false;
				}
			}
			else if (b_sendPacket) {
				// (localPlayer->getMaxDesyncAngle() * (side ? 1 : -1))
				cmd->viewangles.y = LocalPlayerAA.real.y - (120 + ((localPlayer->getMaxDesyncAngle()) * (side ? 1 : -1)));


			}
			else if (!b_sendPacket) {
				cmd->viewangles.y = LocalPlayerAA.real.y - (120 * (side ? 1 : -1));
			}
			*/

			if (config->antiAim.suppress979 && AntiAim::lbyNextUpdatedPrevtick && LocalPlayerAA.netset) {
				interfaces->engine->getNetworkChannel()->OutSequenceNr = LocalPlayerAA.netchan.OutSequenceNr;
				memory->clientState->lastOutgoingCommand = LocalPlayerAA.lastOutgoing;
				LocalPlayerAA.netset = false;
			}


			if (LBY_UPDATE(localPlayer.get()) && !config->antiAim.disableLBYbreaking) /*&& (localPlayer->velocity().length2D() < 2.0f)*/ {


				cmd->viewangles.y = LocalPlayerAA.real.y + (config->antiAim.v1 * (side ? 1 : -1));
				if (((interfaces->engine->getNetworkChannel()->chokedPackets < 8) && !config->misc.testshit.toggled) || config->antiAim.test.forceHide) {
					sendPacket = false;
				}

				if (config->antiAim.test.preserveCountOnLBY) {
					cmd->tickCount = config->antiAim.test.cmd.tickCount;
					cmd->commandNumber = config->antiAim.test.cmd.commandNumber;
				}

				if (config->antiAim.airstuckonLBY) { /*I Wonder what this will do*/
					cmd->tickCount = INT_MAX;
					cmd->commandNumber = INT_MAX;
				}

				/* 979 Suppression */
				if (config->antiAim.suppress979) {

					LocalPlayerAA.netchan.OutSequenceNr = interfaces->engine->getNetworkChannel()->OutSequenceNr+1;
					LocalPlayerAA.lastOutgoing = memory->clientState->lastOutgoingCommand+1;
					LocalPlayerAA.netset = true;
					interfaces->engine->getNetworkChannel()->OutSequenceNr = 150;
					memory->clientState->lastOutgoingCommand = 150;
					
				}
			}
			else if (LBY_UPDATE(localPlayer.get(), 1, false) && !config->antiAim.disableLBYbreaking && config->antiAim.preBreak) { /* Pre-Break */
								float v3;
				if (config->antiAim.v3 == -1) {
					v3 = localPlayer->getMaxDesyncAngle();
				}
				else {
					v3 = config->antiAim.v3;
				} 


				float real = LocalPlayerAA.real.y - (config->antiAim.v5 * (config->antiAim.v2 + (v3 * (side ? 1 : -1))));
				cmd->viewangles.y = real + ((120-config->antiAim.v1) * (side ? 1 : -1));
				LocalPlayerAA.PreBreakAngle = ((120 - config->antiAim.v1) * (side ? 1 : -1));


			} else {
				if (AntiAim::lbyNextUpdatedPrevtick && !AntiAim::lbyNextUpdated && config->antiAim.forcesendafterLBY) {
					sendPacket = true;
				}

				float v3;
				if (config->antiAim.v3 == -1) {
					v3 = localPlayer->getMaxDesyncAngle();
				}
				else {
					v3 = config->antiAim.v3;
				} 

				if (config->antiAim.bJitter) {
					//float chance = config->antiAim.JitterChance;


					std::srand(std::rand());

					int chance = std::rand() % 101;
					if (chance < config->antiAim.JitterChance) {
						if (config->antiAim.JitterRange > 0) {
							v3 = (std::rand() % config->antiAim.JitterRange) - (config->antiAim.JitterRange / 2);
						}
						else {
							if (v3 > 0) {
								v3 = (float)(std::rand() % (int)(v3*2.f)) - v3;
							}
						}
					}
					
					//v3 = (float)(rand() % (int)(v3 * 2.f)) - v3;
				}

				if (b_sendPacket || (Misc::LastSend == 0) || (Misc::LastSend == 2)) {
					// (localPlayer->getMaxDesyncAngle() * (side ? 1 : -1))
					cmd->viewangles.y = LocalPlayerAA.real.y - (config->antiAim.v5 * (config->antiAim.v2 + (v3 * (side ? 1 : -1))));

					if ((config->antiAim.v4 < 50) && (config->antiAim.v1 < 50)) {
						float val = (120 - (config->antiAim.v5 + config->antiAim.v1)); /* v1 Takes Priority */
						cmd->viewangles.y = LocalPlayerAA.real.y + ((val + config->antiAim.v5) * (side ? 1 : -1));
						LocalPlayerAA.PreBreakAngleFake = ((val + config->antiAim.v4) * (side ? 1 : -1)) * -1;
					}

				}
				else if (!b_sendPacket || (Misc::LastSend == 1) ) {
						cmd->viewangles.y = LocalPlayerAA.real.y - (config->antiAim.v4 * (side ? 1 : -1));
				}

			}



			/*
			if (LBY_UPDATE() && (localPlayer->velocity().length2D() < 10.0f)) {





				if (config->antiAim.secretdebug) {
					cmd->viewangles.y += angle + (localPlayer->getMaxDesyncAngle() * (side ? 1 : -1)); // * (config->antiAim.swapsides ? -1 : 1)
				}
				else {
					cmd->viewangles.y -= angle + (localPlayer->getMaxDesyncAngle() * (side ? 1 : -1)); // * (config->antiAim.swapsides ? -1 : 1)
				}

				sendPacket = false;

				if (config->antiAim.forceMovefixNoSend) {
					CorrectMovement(currentViewAngles.y, cmd, cmd->forwardmove, cmd->sidemove);
				}

			}
			else if (!b_sendPacket) {
				cmd->viewangles.y -= angle + (120 * (side ? -1 : 1)); // * ((config->antiAim.swapsides ? -1 : 1) * -1)
				
				
				if (config->antiAim.forceMovefixPost) {
					CorrectMovement(currentViewAngles.y, cmd, cmd->forwardmove, cmd->sidemove);
				}

			}
			else {
				cmd->viewangles.y += angle;

				if (config->antiAim.forceMovefixNoSend) {
					CorrectMovement(currentViewAngles.y, cmd, cmd->forwardmove, cmd->sidemove);
				}

			}
			*/


			if (config->antiAim.forceMovefix) {
				CorrectMovement(currentViewAngles.y, cmd, cmd->forwardmove, cmd->sidemove);
			}

			if (config->antiAim.forceMovefixPost) {
				CorrectMovement(currentViewAngles.y, cmd, cmd->forwardmove, cmd->sidemove);
			}




        }
		



		if (config->antiAim.legitaa) {
			if (interfaces->engine->getNetworkChannel()->chokedPackets == 0) {
				sendPacket = cmd->tickCount % 2 ? false : true;
			}

			if (LBY_UPDATE()) {
				cmd->viewangles.y += localPlayer->getMaxDesyncAngle() - config->antiAim.subtractor;
				sendPacket = true;
			}
			else {
				// do nothin;
			}
		}
		/*
		if (config->antiAim.legitaatest) {

			if (localPlayer->velocity().length2D() > 1.01001f) {
				lastlbyval = 0;
			}
			if (interfaces->engine->getNetworkChannel()->chokedPackets == 0) {
				sendPacket = cmd->tickCount % 2 ? false : true;
			}

			if (LBY_UPDATE()) {
				

				cmd->viewangles.y += config->antiAim.manYaw + localPlayer->getMaxDesyncAngle() + lastlbyval;

				if (lastlbyval < 116) {
					lastlbyval += 10;
				}
				
				sendPacket = false;
			}
			else if (!sendPacket) {
				cmd->viewangles.y += config->antiAim.manYaw - localPlayer->getMaxDesyncAngle();
			}
			else if (sendPacket){
				cmd->viewangles.y += config->antiAim.manYaw;
			}
		}
		*/



    }

}

void AntiAim::fakeWalk(UserCmd* cmd, bool& sendPacket, const Vector& currentViewAngles) noexcept
{
	if (config->antiAim.general.fakeWalk.key != 0) {
		if (config->antiAim.general.fakeWalk.keyMode == 0) {
			if (!GetAsyncKeyState(config->antiAim.general.fakeWalk.key))
			{
				config->antiAim.general.fakeWalk.keyToggled = false;
			}
			else
				config->antiAim.general.fakeWalk.keyToggled = true;
		}
		else {
			if (GetAsyncKeyState(config->antiAim.general.fakeWalk.key) & 1)
				config->antiAim.general.fakeWalk.keyToggled = !config->antiAim.general.fakeWalk.keyToggled;
		}
	}

	if (config->antiAim.general.fakeWalk.enabled && config->antiAim.general.fakeWalk.keyToggled)
	{
		if (interfaces->engine->getNetworkChannel()->chokedPackets < config->antiAim.general.fakeWalk.maxChoke)
		{
			sendPacket = false;
		}
		else if (interfaces->engine->getNetworkChannel()->chokedPackets == config->antiAim.general.fakeWalk.maxChoke)
		{
			sendPacket = false;
		}
		else if (interfaces->engine->getNetworkChannel()->chokedPackets == config->antiAim.general.fakeWalk.maxChoke + 1)
		{
			cmd->forwardmove = 0;

			if (cmd->buttons & UserCmd::IN_DUCK)
				cmd->sidemove = cmd->tickCount & 1 ? 3.25f : -3.25f;
			else
				cmd->sidemove = cmd->tickCount & 1 ? .9f : -.9f;

			sendPacket = false;
		}
		else
		{
			cmd->forwardmove = 0;

			if (cmd->buttons & UserCmd::IN_DUCK)
				cmd->sidemove = cmd->tickCount & 1 ? 3.25f : -3.25f;
			else
				cmd->sidemove = cmd->tickCount & 1 ? .9f : -.9f;

			sendPacket = true;
		}


		CorrectMovement(currentViewAngles.y, cmd, cmd->forwardmove, cmd->sidemove);
	}


}

/*

void C_CSGOPlayerAnimState::SetupVelocity()
{
	MDLCACHE_CRITICAL_SECTION();

	Vector velocity = m_vVelocity;
	if (Interfaces::EngineClient->IsHLTV() || Interfaces::EngineClient->IsPlayingDemo())
		pBaseEntity->GetAbsVelocity(velocity);
	else
		pBaseEntity->EstimateAbsVelocity(velocity);

	float spd = velocity.LengthSqr();

	if (spd > std::pow(1.2f * 260.0f, 2))
	{
		Vector velocity_normalized = velocity;
		VectorNormalizeFast(velocity_normalized);
		velocity = velocity_normalized * (1.2f * 260.0f);
	}

	m_flAbsVelocityZ = velocity.z;
	velocity.z = 0.0f;

	float leanspd = m_vecLastSetupLeanVelocity.LengthSqr();

	m_bIsAccelerating = velocity.
	Sqr() > leanspd;

	m_vVelocity = GetSmoothedVelocity(m_flLastClientSideAnimationUpdateTimeDelta * 2000.0f, velocity, m_vVelocity);

	m_vVelocityNormalized = VectorNormalizeReturn(m_vVelocity);

	float speed = std::fmin(m_vVelocity.Length(), 260.0f);
	m_flSpeed = speed;

	if (speed > 0.0f)
		m_vecLastAcceleratingVelocity = m_vVelocityNormalized;

	CBaseCombatWeapon *weapon = pBaseEntity->GetWeapon();
	pActiveWeapon = weapon;

	float flMaxMovementSpeed = 260.0f;
	if (weapon)
		flMaxMovementSpeed = std::fmax(weapon->GetMaxSpeed(), 0.001f);

	m_flSpeedNormalized = clamp(m_flSpeed / flMaxMovementSpeed, 0.0f, 1.0f);

	m_flRunningSpeed = m_flSpeed / (flMaxMovementSpeed * 0.520f);
	m_flDuckingSpeed = m_flSpeed / (flMaxMovementSpeed * 0.340f);

	if (m_flRunningSpeed < 1.0f)
	{
		if (m_flRunningSpeed < 0.5f)
		{
			float vel = m_flVelocityUnknown;
			float delta = m_flLastClientSideAnimationUpdateTimeDelta * 60.0f;
			float newvel;
			if ((80.0f - vel) <= delta)
			{
				if (-delta <= (80.0f - vel))
					newvel = 80.0f;
				else
					newvel = vel - delta;
			}
			else
			{
				newvel = vel + delta;
			}
			m_flVelocityUnknown = newvel;
		}
	}
	else
	{
		m_flVelocityUnknown = m_flSpeed;
	}

	bool bWasMovingLastUpdate = false;
	bool bJustStartedMovingLastUpdate = false;
	if (m_flSpeed <= 0.0f)
	{
		m_flTimeSinceStartedMoving = 0.0f;
		bWasMovingLastUpdate = m_flTimeSinceStoppedMoving <= 0.0f;
		m_flTimeSinceStoppedMoving += m_flLastClientSideAnimationUpdateTimeDelta;
	}
	else
	{
		m_flTimeSinceStoppedMoving = 0.0f;
		bJustStartedMovingLastUpdate = m_flTimeSinceStartedMoving <= 0.0f;
		m_flTimeSinceStartedMoving = m_flLastClientSideAnimationUpdateTimeDelta + m_flTimeSinceStartedMoving;
	}

	m_flCurrentFeetYaw = m_flGoalFeetYaw;
	m_flGoalFeetYaw = clamp(m_flGoalFeetYaw, -360.0f, 360.0f);

	float eye_feet_delta = AngleDiff(m_flEyeYaw, m_flGoalFeetYaw);

	float flRunningSpeed = clamp(m_flRunningSpeed, 0.0f, 1.0f);

	float flYawModifier = (((m_flGroundFraction * -0.3f) - 0.2f) * flRunningSpeed) + 1.0f;

	if (m_fDuckAmount > 0.0f)
	{
		float flDuckingSpeed = clamp(m_flDuckingSpeed, 0.0f, 1.0f);
		flYawModifier = flYawModifier + ((m_fDuckAmount * flDuckingSpeed) * (0.5f - flYawModifier));
	}

	float flMaxYawModifier = flYawModifier * m_flMaxYaw;
	float flMinYawModifier = flYawModifier * m_flMinYaw;

	if (eye_feet_delta <= flMaxYawModifier)
	{
		if (flMinYawModifier > eye_feet_delta)
			m_flGoalFeetYaw = fabs(flMinYawModifier) + m_flEyeYaw;
	}
	else
	{
		m_flGoalFeetYaw = m_flEyeYaw - fabs(flMaxYawModifier);
	}

	NormalizeAngle(m_flGoalFeetYaw);

	if (m_flSpeed > 0.1f || fabs(m_flAbsVelocityZ) > 100.0f)
	{
		m_flGoalFeetYaw = ApproachAngle(
			m_flEyeYaw,
			m_flGoalFeetYaw,
			((m_flGroundFraction * 20.0f) + 30.0f)
			* m_flLastClientSideAnimationUpdateTimeDelta);
	}
	else
	{
		m_flGoalFeetYaw = ApproachAngle(
			pBaseEntity->GetLowerBodyYaw(),
			m_flGoalFeetYaw,
			m_flLastClientSideAnimationUpdateTimeDelta * 100.0f);
	}

	C_Anim
	
	
	
	ationLayer *layer3 = pBaseEntity->GetAnimOverlay(3);
	if (layer3 && layer3->m_flWeight > 0.0f)
	{
		IncrementLayerCycle(3, false);
		LayerWeightAdvance(3);
	}

	if (m_flSpeed > 0.0f)
	{
		float velAngle = (atan2(-m_vVelocity.y, -m_vVelocity.x) * 180.0f) * (1.0f / M_PI);

		if (velAngle < 0.0f)
			velAngle += 360.0f;

		m_flGoalMoveDirGoalFeetDelta = AngleNormalize(AngleDiff(velAngle, m_flGoalFeetYaw));
	}

	m_flFeetVelDirDelta = AngleNormalize(AngleDiff(m_flGoalMoveDirGoalFeetDelta, m_flCurrentMoveDirGoalFeetDelta));

	if (bJustStartedMovingLastUpdate && m_flFeetYawRate <= 0.0f)
	{
		m_flCurrentMoveDirGoalFeetDelta = m_flGoalMoveDirGoalFeetDelta;

		C_AnimationLayer *layer = pBaseEntity->GetAnimOverlay(6);
		if (layer && layer->m_nSequence != -1)
		{
			if (*(DWORD*)((DWORD)pBaseEntity->pSeqdesc(layer->m_nSequence) + 0xC4) > 0)
			{
				int tag = ANIMTAG_UNINITIALIZED;

				if (std::fabs(AngleDiff(m_flCurrentMoveDirGoalFeetDelta, 180.0f)) > 22.5f)
				{
					if (std::fabs(AngleDiff(m_flCurrentMoveDirGoalFeetDelta, 135.0f)) > 22.5f)
					{
						if (std::fabs(AngleDiff(m_flCurrentMoveDirGoalFeetDelta, 90.0f)) > 22.5f)
						{
							if (std::fabs(AngleDiff(m_flCurrentMoveDirGoalFeetDelta, 45.0f)) > 22.5f)
							{
								if (std::fabs(AngleDiff(m_flCurrentMoveDirGoalFeetDelta, 0.0f)) > 22.5f)
								{
									if (std::fabs(AngleDiff(m_flCurrentMoveDirGoalFeetDelta, -45.0f)) > 22.5f)
									{
										if (std::fabs(AngleDiff(m_flCurrentMoveDirGoalFeetDelta, -90.0f)) > 22.5f)
										{
											if (std::fabs(AngleDiff(m_flCurrentMoveDirGoalFeetDelta, -135.0f)) <= 22.5f)
												tag = ANIMTAG_STARTCYCLE_NW;
										}
										else
										{
											tag = ANIMTAG_STARTCYCLE_W;
										}
									}
									else
									{
										tag = ANIMTAG_STARTCYCLE_SW;
									}
								}
								else
								{
									tag = ANIMTAG_STARTCYCLE_S;
								}
							}
							else
							{
								tag = ANIMTAG_STARTCYCLE_SE;
							}
						}
						else
						{
							tag = ANIMTAG_STARTCYCLE_E;
						}
					}
					else
					{
						tag = ANIMTAG_STARTCYCLE_NE;
					}
				}
				else
				{
					tag = ANIMTAG_STARTCYCLE_N;
				}
				m_flFeetCycle = pBaseEntity->GetFirstSequenceAnimTag(layer->m_nSequence, tag);
			}
		}

		if (m_flDuckRate >= 1.0f && !clientpad[0] && std::fabs(m_flFeetVelDirDelta) > 45.0f)
		{
			if (m_bOnGround)
			{
				if (pBaseEntity->GetUnknownAnimationFloat() <= 0.0f)
					pBaseEntity->DoUnknownAnimationCode(0.3f);
			}
		}
	}
	else
	{
		if (m_flDuckRate >= 1.0f
			&& !clientpad[0]
			&& std::fabs(m_flFeetVelDirDelta) > 100.0
			&& m_bOnGround
			&& pBaseEntity->GetUnknownAnimationFloat() <= 0.0)
		{
			pBaseEntity->DoUnknownAnimationCode(0.3f);
		}

		C_
 *layer = pBaseEntity->GetAnimOverlay(6);
		if (layer->m_flWeight >= 1.0f)
		{
			m_flCurrentMoveDirGoalFeetDelta = m_flGoalMoveDirGoalFeetDelta;
		}
		else
		{
			float flDuckSpeedClamp = clamp(m_flDuckingSpeed, 0.0f, 1.0f);
			float flRunningSpeed = clamp(m_flRunningSpeed, 0.0f, 1.0f);
			float flBiasMove = Bias(
			(m_fDuckAmount, flDuckSpeedClamp, flRunningSpeed), 0.18f);
			m_flCurrentMoveDirGoalFeetDelta = AngleNormalize(((flBiasMove + 0.1f) * m_flFeetVelDirDelta) + m_flCurrentMoveDirGoalFeetDelta);
		}
	}

	m_arrPoseParameters[4].SetValue(pBaseEntity, m_flCurrentMoveDirGoalFeetDelta);




	float eye_goalfeet_delta = AngleDiff(m_flEyeYaw - m_flGoalFeetYaw, 360.0f);

	float new_body_yaw_pose = 0.0f; //not initialized?

	if (eye_goalfeet_delta < 0.0f || m_flMaxYaw == 0.0f)
	{
		if (m_flMinYaw != 0.0f)
			new_body_yaw_pose = (eye_goalfeet_delta / m_flMinYaw) * -58.0f;
	}
	else
	{
		new_body_yaw_pose = (eye_goalfeet_delta / m_flMaxYaw) * 58.0f;
	}





	m_arrPoseParameters[6].SetValue(pBaseEntity, new_body_yaw_pose);

	float eye_pitch_normalized = AngleNormalize(m_flPitch);
	float new_body_pitch_pose;

	if (eye_pitch_normalized <= 0.0f)
		new_body_pitch_pose = (eye_pitch_normalized / m_flMaximumPitch) * -90.0f;
	else
		new_body_pitch_pose = (eye_pitch_normalized / m_flMinimumPitch) * 90.0f;

	m_arrPoseParameters[7].SetValue(pBaseEntity, new_body_pitch_pose);

	m_arrPoseParameters[1].SetValue(pBaseEntity, m_flRunningSpeed);

	m_arrPoseParameters[9].SetValue(pBaseEntity, m_flDuckRate * m_fDuckAmount);
}


*/


/*


m_flGoalFeetYaw = clamp(m_flGoalFeetYaw, -360.0f, 360.0f);

	float EyeFeetDelta = AngleDiff(m_flEyeYaw, m_flGoalFeetYaw);
	float flRunningSpeed = clamp(m_flRunningSpeed, 0.0f, 1.0f);
	float flYawModifier = (((m_flGroundFraction * -0.3f) - 0.2f) * flRunningSpeed) + 1.0f;

	if (m_fDuckAmount > 0.0f)  {

		float flDuckingSpeed = clamp(m_flDuckingSpeed, 0.0f, 1.0f);
		flYawModifier = flYawModifier + ((m_fDuckAmount * flDuckingSpeed) * (0.5f - flYawModifier));
	}

	float flMaxYawModifier = flYawModifier * m_flMaxYaw;
	float flMinYawModifier = flYawModifier * m_flMinYaw;

	if (EyeFeetDelta <= flMaxYawModifier) {

		if (flMinYawModifier > EyeFeetDelta)

			m_flGoalFeetYaw = fabs(flMinYawModifier) + m_flEyeYaw;
	}
	else {

		m_flGoalFeetYaw = m_flEyeYaw - fabs(flMaxYawModifier);
	}

	NormalizeAngle(m_flGoalFeetYaw);

	if (m_flSpeed > 0.1f || fabs(m_flAbsVelocityZ) > 100.0f) {

		m_flGoalFeetYaw = ApproachAngle(
			m_flEyeYaw,
			m_flGoalFeetYaw,
			((m_flGroundFraction * 20.0f) + 30.0f)
			* m_flLastClientSideAnimationUpdateTimeDelta);
	}
	else {

		m_flGoalFeetYaw = ApproachAngle(
			pBaseEntity->m_flLowerBodyYawTarget(),
			m_flGoalFeetYaw,
			m_flLastClientSideAnimationUpdateTimeDelta * 100.0f);
	}



	m_flGoalFeetYaw = clamp(m_flGoalFeetYaw, -360.0f, 360.0f);

	m_flGoalFeetYaw = clamp(m_flGoalFeetYaw, m_flEyeYaw + fMaxDesyncDelta, m_flEyeYaw - fMaxDesyncDelta);

	NormalizeAngle(m_flGoalFeetYaw);

	if (m_flSpeed > 0.1f || m_flAbsVelocityZ > 100.0f) {

		m_flGoalFeetYaw = m_flEyeYaw;
	}
	else {

		m_flGoalFeetYaw = pBaseEntity->m_flLowerBodyYawTarget();
	}

*/