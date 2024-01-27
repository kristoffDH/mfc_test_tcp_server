#include "pch.h"
#include "CPacketHandler.h"

int CPacketHandler::MakePacket(byte* packet, int packetSize, int metaFlag)
{
	//if (packet_size < (PACKET_MIN_SIZE + m_vtData.size()))
	//{
	//	return -1;
	//}

	int nPos = 0;

	// STX
	memcpy(packet + nPos, &STX, 1);
	nPos++;

	// Common Header
	memcpy(packet + nPos, m_commonHeader.length, PACKET_LENGTH_SIZE);
	nPos += PACKET_LENGTH_SIZE;

	memcpy(packet + nPos, m_commonHeader.systemTypeCode_Send, SYSTEM_TYPE_CODE_SIZE);
	nPos += SYSTEM_TYPE_CODE_SIZE;

	memcpy(packet + nPos, m_commonHeader.systemTypeCode_Recv, SYSTEM_TYPE_CODE_SIZE);
	nPos += SYSTEM_TYPE_CODE_SIZE;

	memcpy(packet + nPos, m_commonHeader.workCode, WORK_CODE_SIZE);
	nPos += WORK_CODE_SIZE;

	memcpy(packet + nPos, m_commonHeader.jobCode, JOB_CODE_SIZE);
	nPos += JOB_CODE_SIZE;

	memcpy(packet + nPos, m_commonHeader.dateTime, DATE_TIME_SIZE);
	nPos += DATE_TIME_SIZE;

	memcpy(packet + nPos, m_commonHeader.response, RESPONSE_SIZE);
	nPos += RESPONSE_SIZE;

	// Meta Common Header
	if (metaFlag) {
		memcpy(packet + nPos, &m_metaCommonHeader.responseType, 1);
		nPos++;

		memcpy(packet + nPos, &m_metaCommonHeader.dataType, 1);
		nPos++;

		memcpy(packet + nPos, m_metaCommonHeader.dataCount, DATA_COUNT_SIZE);
		nPos += DATA_COUNT_SIZE;
	}

	// DATA 
	memcpy(packet + nPos, m_vtData.data(), m_vtData.size());
	nPos += m_vtData.size();

	// ETX
	memcpy(packet + nPos, &ETX, 1);
	nPos++;

	// packet Length °è»ê
	int length = nPos - 6;
	byte packetLength[5];

	sprintf_s((char*)packetLength, sizeof(packetLength), "%04d", length);

	memcpy(packet + 1, &packetLength, PACKET_LENGTH_SIZE);

	return length;
}

void CPacketHandler::SetCommonHeader(CommonHeader* commonHeader)
{
	memcpy(&m_commonHeader, commonHeader, sizeof(CommonHeader));
}

void CPacketHandler::SetMetaCommonHeader(MetaCommonHeader* metaCommonHeader)
{
	memcpy(&m_metaCommonHeader, metaCommonHeader, sizeof(MetaCommonHeader));
}

void CPacketHandler::SetData(std::vector<byte>& data)
{
	m_vtData.resize(data.size());
	std::copy(data.begin(), data.end(), m_vtData.begin());
}