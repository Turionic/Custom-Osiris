#pragma once

#include "VirtualMethod.h"
#include "../Memory.h"
#include "ClientClass.h"



/* TODO: Setup classes https://github.com/ValveSoftware/source-sdk-2013/blob/master/mp/src/public/tier1/utlmemory.h */

class NetworkChannel {
public:
    VIRTUAL_METHOD(float, getLatency, 9, (int flow), (this, flow))
	/*
		__int32 vtable; //0x0000
		Netmsgbinder* msgbinder1;
		Netmsgbinder* msgbinder2;
		Netmsgbinder* msgbinder3;
		Netmsgbinder* msgbinder4;
		unsigned char m_bProcessingMessages;
		unsigned char m_bShouldDelete;
		char pad_0x0016[0x2]

	*/
	//std::byte pad[24];

	std::byte pad[20];
	unsigned char m_bProcessingMessages;
	unsigned char m_bShouldDelete;
	char pad_0x0016[0x2];

    int OutSequenceNr;
    int InSequenceNr;
    int OutSequenceNrAck;
    int OutReliableState;
    int InReliableState;
    int chokedPackets;
	//bf_write m_StreamReliable; //0x0030 
	std::byte  m_StreamReliable[24];
	//CUtlMemory m_ReliableDataBuffer; //0x0048 
	std::byte m_ReliableDataBuffer[12];
	//bf_write m_StreamUnreliable; //0x0054 
	std::byte  m_StreamUnreliable[24];
	//CUtlMemory m_UnreliableDataBuffer; //0x006C 
	std::byte  m_UnreliableDataBuffer[12];
	//bf_write m_StreamVoice; //0x0078 
	std::byte m_StreamVoice[24];
	//CUtlMemory m_VoiceDataBuffer; //0x0090 
	std::byte m_VoiceDataBuffer[12];
	__int32 m_Socket; //0x009C 
	__int32 m_StreamSocket; //0x00A0 
	__int32 m_MaxReliablePayloadSize; //0x00A4 
	char pad_0x00A8[0x4]; //0x00A8
	//netadr_t remote_address; //0x00AC 
	std::byte remote_address[12];
	float last_received; //0x00B8 
	char pad_0x00BC[0x4]; //0x00BC
	float connect_time; //0x00C0 
	char pad_0x00C4[0x4]; //0x00C4
	__int32 m_Rate; //0x00C8 
	float m_fClearTime; //0x00CC 
	char pad_0x00D0[0x8]; //0x00D0
	//CUtlVector m_WaitingList[0]; //0x00D8 
	std::byte  m_WaitingList0[12];
	//CUtlVector m_WaitingList[1]; //0x00EC 
	std::byte m_WaitingList1[12];
	char pad_0x0100[0x4120]; //0x0100
	__int32 m_PacketDrop; //0x4220 
	char m_Name[32]; //0x4224 
	__int32 m_ChallengeNr; //0x4244 
	float m_Timeout; //0x4248 
	//INetChannelHandler* m_MessageHandler; //0x424C 
	std::byte m_MessageHandler[4];
	//CUtlVector m_NetMessages; //0x4250 
	std::byte m_NetMessages[12];
	__int32 m_pDemoRecorder; //0x4264 
	__int32 m_nQueuedPackets; //0x4268 
	float m_flInterpolationAmount; //0x426C 
	float m_flRemoteFrameTime; //0x4270 
	float m_flRemoteFrameTimeStdDeviation; //0x4274 
	float m_nMaxRoutablePayloadSize; //0x4278 
	__int32 m_nSplitPacketSequence; //0x427C 
	char pad_0x4280[0x14]; //0x4280
};

class EventInfo {
public:
	enum {
		EVENT_INDEX_BITS = 8,
		EVENT_DATA_LEN_BITS = 11,
		MAX_EVENT_DATA = 192,  // ( 1<<8 bits == 256, but only using 192 below )
	};
	short class_id;
	float fire_delay;
	const void* m_send_table;
	const ClientClass* m_client_class;
	int bits;
	uint8_t* data;
	int flags;
	PAD(0x1C);
	EventInfo* m_next;
};

class ClientState
{
public:
	void ForceFullUpdate()
	{
		deltaTick = -1;
	}
	std::byte		pad0[0x9C];
	NetworkChannel* netChannel;
	int				challengeNr;
	std::byte		pad1[0x04];
	double          m_connect_time;
	int             m_retry_number;
	std::byte		padgay[0x54];
	int				signonState;
	std::byte		pad2[0x8];
	float           nextCmdTime;
	int				serverCount;
	int				currentSequence;
	char			pad99[8];
	struct {
		float		m_clock_offsets[16];
		int			m_cur_clock_offset;
		int			m_server_tick;
		int			m_client_tick;
	} m_clock_drift_mgr;
	int				deltaTick;
	bool			paused;
	std::byte		pad4[0x7];
	int				viewEntity;
	int				playerSlot;
	char			levelName[260];
	char			levelNameShort[80];
	char			groupName[80];
	char			szLastLevelNameShort[80]; // 0x032C
	std::byte		pad5[0xC];
	int				maxClients;
	char pad_030C[4083];
	uint32_t string_table_container;
	char pad_1303[14737];
	float			lastServerTickTime;
	bool			InSimulation;
	std::byte		pad7[0x3];
	int				oldTickcount;
	float			tickRemainder;
	float			frameTime;
	int				lastOutgoingCommand;
	int				chokedCommands;
	int				lastCommandAck;
	int				commandAck;
	int				soundSequence;
	//std::byte		pad8[0x50];
	int                m_last_progress_percent;
	bool            m_is_hltv;

	std::byte padfuck[0x4B];
	Vector			angViewPoint;
	std::byte		pad9[0xD0];
	EventInfo* pEvents;
};



class NetworkMessage
{
public:
	VIRTUAL_METHOD(void, SetNetChannel, 1, (NetworkChannel* netchann), (this, netchann))
		VIRTUAL_METHOD(void, SetReliable, 2, (bool state), (this, state))
		VIRTUAL_METHOD(bool, Process, 3, (), (this))
		//VIRTUAL_METHOD(bool, ReadFromBuffer, 4, (bf_read* buffer), (this, buffer))
		//VIRTUAL_METHOD(bool, WriteToBuffer, 5, (bf_write* buffer), (this, buffer))
		VIRTUAL_METHOD(bool, IsReliable, 6, (), (this))
		VIRTUAL_METHOD(int, getType, 7, (), (this))
		VIRTUAL_METHOD(int, getGroup, 8, (), (this))
		VIRTUAL_METHOD(const char*, getName, 9, (), (this))
		VIRTUAL_METHOD(void*, getNetworkChannel, 10, (), (this))
		VIRTUAL_METHOD(const char*, toString, 11, (), (this))
};

class NET_StringCmd : public NetworkMessage
{
	//DECLARE_NET_MESSAGE(StringCmd);

	int	getGroup() const { return 5; } // NetMessageProto::NET_Messages::net_StringCmd; }

	NET_StringCmd() { m_szCommand = NULL; };
	NET_StringCmd(const char* cmd) { m_szCommand = cmd; };

public:
	const char* m_szCommand;	// execute this command

private:
	char		m_szCommandBuffer[1024];	// buffer for received messages

};