#pragma once

#include <vector>

#define PACKET_MIN_SIZE 44
#define PACKET_LENGTH_SIZE 4
#define SYSTEM_TYPE_CODE_SIZE 6
#define WORK_CODE_SIZE 4
#define JOB_CODE_SIZE 4
#define DATE_TIME_SIZE 14
#define RESPONSE_SIZE 4

#define DATA_COUNT_SIZE 3

typedef struct _CommonHeader
{
	byte length[PACKET_LENGTH_SIZE+1];
	byte systemTypeCode_Send[SYSTEM_TYPE_CODE_SIZE+1];
	byte systemTypeCode_Recv[SYSTEM_TYPE_CODE_SIZE+1];
	byte workCode[WORK_CODE_SIZE+1];
	byte jobCode[JOB_CODE_SIZE+1];
	byte dateTime[DATE_TIME_SIZE+1];
	byte response[RESPONSE_SIZE+1];
} CommonHeader;

typedef struct _MetaCommonHeader
{
	byte responseType;
	byte dataType;
	byte dataCount[DATA_COUNT_SIZE+1];
} MetaCommonHeader;

class CPacketHandler
{
private:
	byte STX;
	byte ETX;
	CommonHeader m_commonHeader;
	MetaCommonHeader m_metaCommonHeader;
	std::vector<byte> m_vtData;

public:
	CPacketHandler()
		: STX(0x02),
		ETX(0x03) { 
		ZeroMemory(&m_commonHeader, sizeof(CommonHeader));
		ZeroMemory(&m_metaCommonHeader, sizeof(MetaCommonHeader));
		m_vtData.clear();
	};

public:
	int MakePacket(byte* packet, int packetSize, int metaFlag);

	void SetCommonHeader(CommonHeader* commonHeader);
	void SetMetaCommonHeader(MetaCommonHeader* metaCommonHeader);
	void SetData(std::vector<byte>& data);
};

