// WRGuard.cpp: implementation of the CWRGuard class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "WRGuard.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CWRGuard::CWRGuard()
{
	m_hNoWriter = CreateEvent(NULL, FALSE, TRUE, NULL);
	m_hNoReader = CreateEvent(NULL, TRUE, TRUE, NULL);
	m_lReaderCount = 0;
}

CWRGuard::~CWRGuard()
{
	CloseHandle(m_hNoReader);
	CloseHandle(m_hNoWriter);
}

DWORD CWRGuard::BeginWrite(DWORD dwTimeOut)
{
	HANDLE		pHandle[2];

	pHandle[0] = m_hNoWriter;
	pHandle[1] = m_hNoReader;

	return WaitForMultipleObjects(2, pHandle, TRUE, dwTimeOut);
}

void CWRGuard::EndWrite()
{
	SetEvent(m_hNoWriter);
}

DWORD CWRGuard::BeginRead(DWORD dwTimeOut)
{
	DWORD		dw;

	dw = WaitForSingleObject(m_hNoWriter, dwTimeOut);

	if (WAIT_TIMEOUT!=dw)
	{
		if (0==m_lReaderCount)
			ResetEvent(m_hNoReader);
		m_lReaderCount++;
	}

	SetEvent(m_hNoWriter);

	return dw;
}

void CWRGuard::EndRead()
{
	WaitForSingleObject(m_hNoWriter, INFINITE);

	m_lReaderCount--;
	if (0==m_lReaderCount)
		SetEvent(m_hNoReader);

	SetEvent(m_hNoWriter);
}
