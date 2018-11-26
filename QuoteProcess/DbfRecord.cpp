#include "stdafx.h"
#include "DbfRecord.h"




CDbfRecord::CDbfRecord()
{
	InitializeCriticalSection(&m_rCritical);

	m_cSecurityMarket = -1;
	m_lSecurityCode = -1l;
	memset(m_pcName, 0, 8);

	m_lQuoteDate = 0;
	m_lQuoteTime = 0;

	Initial();
}

CDbfRecord::~CDbfRecord()
{

}


void CDbfRecord::Initial()
{
	EnterCriticalSection(&m_rCritical);

	memset(m_plRecordValues, 0, sizeof(m_plRecordValues));

	LeaveCriticalSection(&m_rCritical);
}

long CDbfRecord::GetID()
{
	return m_cSecurityMarket * 1000000l + m_lSecurityCode;
}

bool CDbfRecord::IsIndex()
{
	if ((m_cSecurityMarket == 0 && m_lSecurityCode < 100) || (m_cSecurityMarket == 1 && m_lSecurityCode / 10000 == 39))
		return true;
	else
		return false;
}

void CDbfRecord::ReadDbfRecordSH(char* pcBuffer)
{
	EnterCriticalSection(&m_rCritical);

	if (memcmp(m_pcBuffer, pcBuffer, SHDBFRECORDSIZE))
	{
		char pcBuf[32];
		double fValue;
		char* pcRead = pcBuffer;

		pcRead = pcBuffer + 7;
		memcpy(m_pcName, pcRead, 8);

		pcRead = pcBuffer + 15;
		memcpy(pcBuf, pcRead, 8);
		pcBuf[8] = 0;
		fValue = (double)atof(pcBuf);
		m_plRecordValues[PRECLOSE] = fValue;

		pcRead = pcBuffer + 23;
		memcpy(pcBuf, pcRead, 8);
		pcBuf[8] = 0;
		fValue = (double)atof(pcBuf);
		m_plRecordValues[OPEN] = fValue;

		pcRead = pcBuffer + 31;
		memcpy(pcBuf, pcRead, 12);
		pcBuf[12] = 0;
		fValue = (double)atof(pcBuf);
		m_plRecordValues[AMOUNT] = fValue;

		pcRead = pcBuffer + 83;
		memcpy(pcBuf, pcRead, 10);
		pcBuf[10] = 0;
		fValue = (double)atof(pcBuf);
		m_plRecordValues[VOLUMN] = fValue;

		pcRead = pcBuffer + 43;
		memcpy(pcBuf, pcRead, 8);
		pcBuf[8] = 0;
		fValue = (double)atof(pcBuf);
		m_plRecordValues[HIGH] = fValue;

		pcRead = pcBuffer + 51;
		memcpy(pcBuf, pcRead, 8);
		pcBuf[8] = 0;
		fValue = (double)atof(pcBuf);
		m_plRecordValues[LOW] = fValue;

		pcRead = pcBuffer + 59;
		memcpy(pcBuf, pcRead, 8);
		pcBuf[8] = 0;
		fValue = (double)atof(pcBuf);
		m_plRecordValues[PRICE] = fValue;

		pcRead = pcBuffer + 67;
		memcpy(pcBuf, pcRead, 8);
		pcBuf[8] = 0;
		fValue = (double)atof(pcBuf);
		m_plRecordValues[PBUY1] = fValue;

		pcRead = pcBuffer + 75;
		memcpy(pcBuf, pcRead, 8);
		pcBuf[8] = 0;
		fValue = (double)atof(pcBuf);
		m_plRecordValues[PSELL1] = fValue;

		pcRead = pcBuffer + 101;
		memcpy(pcBuf, pcRead, 10);
		pcBuf[10] = 0;
		fValue = (double)atof(pcBuf);
		m_plRecordValues[VBUY1] = fValue;

		pcRead = pcBuffer + 111;
		memcpy(pcBuf, pcRead, 8);
		pcBuf[8] = 0;
		fValue = (double)atof(pcBuf);
		m_plRecordValues[PBUY2] = fValue;

		pcRead = pcBuffer + 119;
		memcpy(pcBuf, pcRead, 10);
		pcBuf[10] = 0;
		fValue = (double)atof(pcBuf);
		m_plRecordValues[VBUY2] = fValue;

		pcRead = pcBuffer + 129;
		memcpy(pcBuf, pcRead, 8);
		pcBuf[8] = 0;
		fValue = (double)atof(pcBuf);
		m_plRecordValues[PBUY3] = fValue;

		pcRead = pcBuffer + 137;
		memcpy(pcBuf, pcRead, 10);
		pcBuf[10] = 0;
		fValue = (double)atof(pcBuf);
		m_plRecordValues[VBUY3] = fValue;

		pcRead = pcBuffer + 147;
		memcpy(pcBuf, pcRead, 10);
		pcBuf[10] = 0;
		fValue = (double)atof(pcBuf);
		m_plRecordValues[VSELL1] = fValue;

		pcRead = pcBuffer + 157;
		memcpy(pcBuf, pcRead, 8);
		pcBuf[8] = 0;
		fValue = (double)atof(pcBuf);
		m_plRecordValues[PSELL2] = fValue;

		pcRead = pcBuffer + 165;
		memcpy(pcBuf, pcRead, 10);
		pcBuf[10] = 0;
		fValue = (double)atof(pcBuf);
		m_plRecordValues[VSELL2] = fValue;

		pcRead = pcBuffer + 175;
		memcpy(pcBuf, pcRead, 8);
		pcBuf[8] = 0;
		fValue = (double)atof(pcBuf);
		m_plRecordValues[PSELL3] = fValue;

		pcRead = pcBuffer + 183;
		memcpy(pcBuf, pcRead, 10);
		pcBuf[10] = 0;
		fValue = (double)atof(pcBuf);
		m_plRecordValues[VSELL3] = fValue;

		pcRead = pcBuffer + 193;
		memcpy(pcBuf, pcRead, 8);
		pcBuf[8] = 0;
		fValue = (double)atof(pcBuf);
		m_plRecordValues[PBUY4] = fValue;

		pcRead = pcBuffer + 201;
		memcpy(pcBuf, pcRead, 10);
		pcBuf[10] = 0;
		fValue = (double)atof(pcBuf);
		m_plRecordValues[VBUY4] = fValue;

		pcRead = pcBuffer + 211;
		memcpy(pcBuf, pcRead, 8);
		pcBuf[8] = 0;
		fValue = (double)atof(pcBuf);
		m_plRecordValues[PBUY5] = fValue;

		pcRead = pcBuffer + 219;
		memcpy(pcBuf, pcRead, 10);
		pcBuf[10] = 0;
		fValue = (double)atof(pcBuf);
		m_plRecordValues[VBUY5] = fValue;

		pcRead = pcBuffer + 229;
		memcpy(pcBuf, pcRead, 8);
		pcBuf[8] = 0;
		fValue = (double)atof(pcBuf);
		m_plRecordValues[PSELL4] = fValue;

		pcRead = pcBuffer + 237;
		memcpy(pcBuf, pcRead, 10);
		pcBuf[10] = 0;
		fValue = (double)atof(pcBuf);
		m_plRecordValues[VSELL4] = fValue;

		pcRead = pcBuffer + 247;
		memcpy(pcBuf, pcRead, 8);
		pcBuf[8] = 0;
		fValue = (double)atof(pcBuf);
		m_plRecordValues[PSELL5] = fValue;

		pcRead = pcBuffer + 255;
		memcpy(pcBuf, pcRead, 10);
		pcBuf[10] = 0;
		fValue = (double)atof(pcBuf);
		m_plRecordValues[VSELL5] = fValue;

		memcpy(m_pcBuffer, pcBuffer, SHDBFRECORDSIZE);
	}

	LeaveCriticalSection(&m_rCritical);

}

void CDbfRecord::ReadDbfRecordSz(char* pcBuffer)
{
	EnterCriticalSection(&m_rCritical);

	if (memcmp(m_pcBuffer, pcBuffer, SZDBFRECORDSIZE))
	{
		char pcBuf[32];
		double fValue;
		char* pcRead = pcBuffer;

		pcRead = pcBuffer + 7;
		memcpy(m_pcName, pcRead, 8);

		pcRead = pcBuffer + 15;
		memcpy(pcBuf, pcRead, 9);
		pcBuf[9] = 0;
		fValue = (double)atof(pcBuf);
		m_plRecordValues[PRECLOSE] = fValue;

		pcRead = pcBuffer + 24;
		memcpy(pcBuf, pcRead, 9);
		pcBuf[9] = 0;
		fValue = (double)atof(pcBuf);
		m_plRecordValues[OPEN] = fValue;

		pcRead = pcBuffer + 33;
		memcpy(pcBuf, pcRead, 9);
		pcBuf[9] = 0;
		fValue = (double)atof(pcBuf);
		m_plRecordValues[PRICE] = fValue;

		pcRead = pcBuffer + 42;
		memcpy(pcBuf, pcRead, 12);
		pcBuf[12] = 0;
		fValue = (double)atof(pcBuf);
		m_plRecordValues[VOLUMN] = fValue;

		pcRead = pcBuffer + 54;
		memcpy(pcBuf, pcRead, 17);
		pcBuf[17] = 0;
		fValue = (double)atof(pcBuf);
		m_plRecordValues[AMOUNT] = fValue;

		pcRead = pcBuffer + 80;
		memcpy(pcBuf, pcRead, 9);
		pcBuf[9] = 0;
		fValue = (double)atof(pcBuf);
		m_plRecordValues[HIGH] = fValue;

		pcRead = pcBuffer + 89;
		memcpy(pcBuf, pcRead, 9);
		pcBuf[9] = 0;
		fValue = (double)atof(pcBuf);
		if (fValue > 9999.0f)
			fValue = 0;
		m_plRecordValues[LOW] = fValue;

		pcRead = pcBuffer + 142;
		memcpy(pcBuf, pcRead, 9);
		pcBuf[9] = 0;
		fValue = (double)atof(pcBuf);
		m_plRecordValues[PSELL5] = fValue;

		pcRead = pcBuffer + 151;
		memcpy(pcBuf, pcRead, 12);
		pcBuf[12] = 0;
		fValue = (double)atof(pcBuf);
		m_plRecordValues[VSELL5] = fValue;

		pcRead = pcBuffer + 163;
		memcpy(pcBuf, pcRead, 9);
		pcBuf[9] = 0;
		fValue = (double)atof(pcBuf);
		m_plRecordValues[PSELL4] = fValue;

		pcRead = pcBuffer + 172;
		memcpy(pcBuf, pcRead, 12);
		pcBuf[12] = 0;
		fValue = (double)atof(pcBuf);
		m_plRecordValues[VSELL4] = fValue;

		pcRead = pcBuffer + 184;
		memcpy(pcBuf, pcRead, 9);
		pcBuf[9] = 0;
		fValue = (double)atof(pcBuf);
		m_plRecordValues[PSELL3] = fValue;

		pcRead = pcBuffer + 193;
		memcpy(pcBuf, pcRead, 12);
		pcBuf[12] = 0;
		fValue = (double)atof(pcBuf);
		m_plRecordValues[VSELL3] = fValue;

		pcRead = pcBuffer + 205;
		memcpy(pcBuf, pcRead, 9);
		pcBuf[9] = 0;
		fValue = (double)atof(pcBuf);
		m_plRecordValues[PSELL2] = fValue;

		pcRead = pcBuffer + 214;
		memcpy(pcBuf, pcRead, 12);
		pcBuf[12] = 0;
		fValue = (double)atof(pcBuf);
		m_plRecordValues[VSELL2] = fValue;

		pcRead = pcBuffer + 226;
		memcpy(pcBuf, pcRead, 9);
		pcBuf[9] = 0;
		fValue = (double)atof(pcBuf);
		m_plRecordValues[PSELL1] = fValue;

		pcRead = pcBuffer + 235;
		memcpy(pcBuf, pcRead, 12);
		pcBuf[12] = 0;
		fValue = (double)atof(pcBuf);
		m_plRecordValues[VSELL1] = fValue;

		pcRead = pcBuffer + 247;
		memcpy(pcBuf, pcRead, 9);
		pcBuf[9] = 0;
		fValue = (double)atof(pcBuf);
		m_plRecordValues[PBUY1] = fValue;

		pcRead = pcBuffer + 256;
		memcpy(pcBuf, pcRead, 12);
		pcBuf[12] = 0;
		fValue = (double)atof(pcBuf);
		m_plRecordValues[VBUY1] = fValue;

		pcRead = pcBuffer + 268;
		memcpy(pcBuf, pcRead, 9);
		pcBuf[9] = 0;
		fValue = (double)atof(pcBuf);
		m_plRecordValues[PBUY2] = fValue;

		pcRead = pcBuffer + 277;
		memcpy(pcBuf, pcRead, 12);
		pcBuf[12] = 0;
		fValue = (double)atof(pcBuf);
		m_plRecordValues[VBUY2] = fValue;

		pcRead = pcBuffer + 289;
		memcpy(pcBuf, pcRead, 9);
		pcBuf[9] = 0;
		fValue = (double)atof(pcBuf);
		m_plRecordValues[PBUY3] = fValue;

		pcRead = pcBuffer + 298;
		memcpy(pcBuf, pcRead, 12);
		pcBuf[12] = 0;
		fValue = (double)atof(pcBuf);
		m_plRecordValues[VBUY3] = fValue;

		pcRead = pcBuffer + 310;
		memcpy(pcBuf, pcRead, 9);
		pcBuf[9] = 0;
		fValue = (double)atof(pcBuf);
		m_plRecordValues[PBUY4] = fValue;

		pcRead = pcBuffer + 319;
		memcpy(pcBuf, pcRead, 12);
		pcBuf[12] = 0;
		fValue = (double)atof(pcBuf);
		m_plRecordValues[VBUY4] = fValue;

		pcRead = pcBuffer + 331;
		memcpy(pcBuf, pcRead, 9);
		pcBuf[9] = 0;
		fValue = (double)atof(pcBuf);
		m_plRecordValues[PBUY5] = fValue;

		pcRead = pcBuffer + 340;
		memcpy(pcBuf, pcRead, 12);
		pcBuf[12] = 0;
		fValue = (double)atof(pcBuf);
		m_plRecordValues[VBUY5] = fValue;

		memcpy(m_pcBuffer, pcBuffer, SZDBFRECORDSIZE);
	}
	LeaveCriticalSection(&m_rCritical);
}
