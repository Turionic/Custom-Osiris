#pragma once
#include "../SDK/matrix3x4.h"
#include "../SDK/Vector.h"
#include "../SDK/Entity.h"
#include "Backtrack.h"
#include <array>

struct UserCmd;

namespace Animations
{
	void update(UserCmd*, bool& sendPacket) noexcept;
	void real() noexcept;
	void fake() noexcept;

	void players() noexcept;
	void setup(Entity*, Backtrack::Record) noexcept;

	void setupMoveFix(Entity* entity) noexcept;
	void finishMoveFix(Entity* entity) noexcept;

	void finishSetup(Entity*) noexcept;

	/*
	struct Datas
	{
		bool sendPacket;
		bool lby;
		bool gotMatrix;
		float fakeAngle;
		Vector viewangles;
		matrix3x4 fakematrix[256];
	};
	extern Datas data;
	*/

	struct Players
	{
		Players() : once(false), chokedPackets(0), simtime(0) { }
		matrix3x4 matrix[256];
		std::array<float, 24> poses;
		std::array<AnimationLayer, 15> networked_layers;
		float abs;
		float simtime;
		int chokedPackets;
		int averagefakelag = 1;
		bool once;
		Vector velocity;
		Vector prevOrigin;
		int countBones;
		bool hasBackup{ false };
	};

	struct Backup
	{
		Vector origin;
		Vector max;
		Vector mins;
		int* boneCache;
		int countBones;
		bool hasBackup{ false };
	};

	struct Datas
	{
		bool sendPacket;
		bool lby;
		bool gotMatrix;
		float fakeAngle;
		Vector viewangles;
		matrix3x4 fakematrix[256];
		std::array<Players, 65> player;
		std::array<Backup, 65> backup;
		std::array<Backup, 65> backupResolver;
		std::array<Backup, 65> lastest;
	};
	extern Datas data;

}

