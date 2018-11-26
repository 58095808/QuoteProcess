#include "stdafx.h"
#include "InitFile.h"


InitFile::InitFile()
{
}


InitFile::~InitFile()
{
}




void InitFile::LoadIniString()
{
	errno_t err;
	FILE* pFile;
	err = fopen_s(&pFile, "QuoteProcess.ini", "r");

	if (err == 0)
	{


		char pcBuf[1024];

		while (fscanf_s(pFile, "%s", pcBuf, 1024) != EOF)
		{
			char* pcKey = strchr(pcBuf, ':');
			if (pcKey)
			{
				IniString* pIni = new IniString;
				if (pIni)
				{
					*pcKey = 0;
					pIni->m_strKey = pcBuf;
					pIni->m_strValue = pcKey + 1;

					m_aIniString.Add(pIni);
				}
			}
		}

		fclose(pFile);
	}
}

void InitFile::DeleteIniString()
{
	int nIni = m_aIniString.GetSize();
	for (int i = 0; i < nIni; i++)
	{
		IniString* pIni = (IniString*)m_aIniString[0];
		m_aIniString.RemoveAt(0);
		delete pIni;
	}
}

void InitFile::SaveIniString()
{
	_wsetlocale(0, L"chs");
	FILE* pFile;
	fopen_s(&pFile, "QuoteProcess.ini", "w");
	if (pFile)
	{
		int nIni = m_aIniString.GetSize();
		for (int i = 0; i < nIni; i++)
		{
			IniString* pIni = (IniString*)m_aIniString[i];
			_ftprintf(pFile, _T("%s:%s\n"), pIni->m_strKey, pIni->m_strValue);
		}

		fclose(pFile);
	}
}

CString InitFile::GetIniString(CString strKey, CString strDefault)
{
	int nIni = m_aIniString.GetSize();
	for (int i = 0; i < nIni; i++)
	{
		IniString* pIni = (IniString*)m_aIniString[i];
		if (pIni->m_strKey == strKey)
			return pIni->m_strValue;
	}
	return strDefault;
}

void InitFile::WriteIniString(CString strKey, CString strValue)
{
	int nIni = m_aIniString.GetSize();
	for (int i = 0; i < nIni; i++)
	{
		IniString* pIni = (IniString*)m_aIniString[i];
		if (pIni->m_strKey == strKey)
		{
			pIni->m_strValue = strValue;
			return;
		}
	}
	IniString* pIni = new IniString;
	if (pIni)
	{
		pIni->m_strKey = strKey;
		pIni->m_strValue = strValue;

		m_aIniString.Add(pIni);
	}
}

int InitFile::GetIniInt(CString strKey, int nDefault)
{
	int nIni = m_aIniString.GetSize();
	for (int i = 0; i < nIni; i++)
	{
		IniString* pIni = (IniString*)m_aIniString[i];
		if (pIni->m_strKey == strKey)
			return _wtoi(pIni->m_strValue);
	}
	return nDefault;
}

void InitFile::WriteIniInt(CString strKey, int nValue)
{
	int nIni = m_aIniString.GetSize();
	for (int i = 0; i < nIni; i++)
	{
		IniString* pIni = (IniString*)m_aIniString[i];
		if (pIni->m_strKey == strKey)
		{
			pIni->m_strValue.Format(_T("%d"), nValue);
			return;
		}
	}
	IniString* pIni = new IniString;
	if (pIni)
	{
		pIni->m_strKey = strKey;
		pIni->m_strValue.Format(_T("%d"), nValue);

		m_aIniString.Add(pIni);
	}
}