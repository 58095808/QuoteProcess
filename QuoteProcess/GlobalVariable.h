#include <map>
using namespace std;
#pragma once


extern long OS_cur_hms_mis;  //当前时间(HHMMSSMIS)
extern long OS_cur_hms;	   //当前时间(HHMMSS)
extern long OS_cur_date;	   //当前日期(YYYYMMDD)




extern int shMasterDate;
extern int szMasterDate;
extern int shMasterTime;
extern int szMasterTime;
extern int shSlaveDate;
extern int szSlaveDate;
extern int shSlaveTime;
extern int szSlaveTime;

extern bool isDayQuoteFinishSh; //是否沪市日数据已处理完成
extern bool isDayQuoteFinishSz; //是否深市日数据已处理完成

extern bool isPreQuoteFinishSh; //是否沪市盘前数据已处理完成
extern bool isPreQuoteFinishSz; //是否深市盘前数据已处理完成
extern bool isIniFinish;

extern bool  isTradeDate; //是否为交易日



extern TCHAR __main_home__[MAX_PATH];		//主路径(TCHAR)
extern TCHAR __iniSysConfig__[MAX_PATH];
extern void write_log(LPCTSTR lpFormat, ...);
extern CRITICAL_SECTION __cs_logfile1__;
extern TCHAR dbStatus[100];
extern TCHAR errInfo[100];
extern TCHAR strStaticMinSh[100];
extern TCHAR StaticCloseSh[100];
extern TCHAR strStaticMinSz[100];
extern TCHAR StaticCloseSz[100];

extern CString  m_strDbUser;
extern CString m_strDbPwd;
extern CString m_strDbName;

extern map<string, string> __nontrading_day_map__;	//非交易日表
void loadNontradingDay(void);
