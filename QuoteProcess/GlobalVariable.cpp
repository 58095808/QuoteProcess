#include "stdafx.h"

#include <string>
using namespace std;


long OS_cur_hms_mis =0;  //当前时间(HHMMSSMIS)
long OS_cur_hms = 0;	   //当前时间(HHMMSS)
long OS_cur_date = 0;	   //当前日期(YYYYMMDD)



int shMasterDate = 0;
int szMasterDate = 0;
int shMasterTime = 0;
int szMasterTime = 0;
int shSlaveDate = 0;
int szSlaveDate = 0;
int shSlaveTime = 0;
int szSlaveTime = 0;


bool isDayQuoteFinishSh = false; //是否沪市日数据已处理完成
bool isDayQuoteFinishSz = false; //是否深市日数据已处理完成

bool isPreQuoteFinishSh = false; //是否沪市盘前数据已处理完成
bool isPreQuoteFinishSz = false; //是否深市盘前数据已处理完成

bool isTradeDate = false; //是否为交易日


bool isIniFinish = false; //盘前初始化是否完成

TCHAR __main_home__[MAX_PATH] = { 0 };		//主路径(TCHAR)
TCHAR __iniSysConfig__[MAX_PATH] = { 0 };;


CString m_strDbUser=_T("");
CString m_strDbPwd = _T("");
CString m_strDbName = _T("");
TCHAR dbStatus[100] = { 0 };
TCHAR errInfo[100] = { 0 };
TCHAR strStaticMinSh[100] = { 0 };
TCHAR StaticCloseSh[100] = { 0 };
TCHAR strStaticMinSz[100] = { 0 };
TCHAR StaticCloseSz[100] = { 0 };
TCHAR __logdata__[2048] = { 0 };


map<string, string> __nontrading_day_map__;	//非交易日表

CRITICAL_SECTION __cs_logfile1__;
void write_log(LPCTSTR lpFormat, ...)
{
	SYSTEMTIME st = { 0 };
	GetLocalTime(&st);

	HANDLE hFile = INVALID_HANDLE_VALUE;
	va_list arg_ptr;

	DWORD bytes(0);
	SECURITY_ATTRIBUTES sa = { 0 };
	sa.nLength = sizeof(sa);
	EnterCriticalSection(&__cs_logfile1__);
	wsprintf(__logdata__, TEXT("%slog\\QuoteProcess%04d%02d%02d.txt"), __main_home__, st.wYear, st.wMonth, st.wDay);
	hFile = CreateFile(__logdata__, GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE, &sa, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile != INVALID_HANDLE_VALUE)
	{
		SetFilePointer(hFile, 0, NULL, FILE_END);
		wsprintf(__logdata__, TEXT("%02d:%02d:%02d.%03d  "), st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);

		va_start(arg_ptr, lpFormat);
		vswprintf(__logdata__ + 13, 2048, lpFormat, arg_ptr);
		va_end(arg_ptr);

		lstrcat(__logdata__, TEXT("\r\n"));
		WriteFile(hFile, __logdata__, sizeof(TCHAR)*lstrlen(__logdata__), &bytes, NULL);
		CloseHandle(hFile);
	}
	LeaveCriticalSection(&__cs_logfile1__);
}


void loadNontradingDay(void)
{
	TCHAR fullpath[MAX_PATH] = { 0 };
	wsprintf(fullpath, _T("%snontradingnormal.day"), __main_home__);
	SECURITY_ATTRIBUTES sa = { 0 };
	sa.nLength = sizeof(sa);
	HANDLE hFile = CreateFile(fullpath, GENERIC_READ, FILE_SHARE_READ, &sa, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile != INVALID_HANDLE_VALUE)
	{

		DWORD szFile = GetFileSize(hFile, NULL);
		char* paTemp = new char[szFile];
		memset(paTemp, 0x00, szFile);
		DWORD bytes(0);
		ReadFile(hFile, paTemp, szFile, &bytes, NULL);

		int i(0), j(0), k(0), l(0), s(0);
		char lineData[10] = { 0 };
		for (; i<(int)szFile; i++)
		{
			if (paTemp[i] == '\r' || paTemp[i] == '\n')
			{
				//do nothing
				if (i != j)
				{
					memset(lineData, 0x00, sizeof(lineData));
					memcpy(lineData, &paTemp[j], i - j);
					string key(lineData);
					__nontrading_day_map__[key] = key;
				}
				j = i + 1;
			}
		}
		CloseHandle(hFile);
		delete paTemp;
	}
}
