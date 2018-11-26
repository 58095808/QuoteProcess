#include <map>
using namespace std;
#pragma once


extern long OS_cur_hms_mis;  //��ǰʱ��(HHMMSSMIS)
extern long OS_cur_hms;	   //��ǰʱ��(HHMMSS)
extern long OS_cur_date;	   //��ǰ����(YYYYMMDD)




extern int shMasterDate;
extern int szMasterDate;
extern int shMasterTime;
extern int szMasterTime;
extern int shSlaveDate;
extern int szSlaveDate;
extern int shSlaveTime;
extern int szSlaveTime;

extern bool isDayQuoteFinishSh; //�Ƿ����������Ѵ������
extern bool isDayQuoteFinishSz; //�Ƿ������������Ѵ������

extern bool isPreQuoteFinishSh; //�Ƿ�����ǰ�����Ѵ������
extern bool isPreQuoteFinishSz; //�Ƿ�������ǰ�����Ѵ������
extern bool isIniFinish;

extern bool  isTradeDate; //�Ƿ�Ϊ������



extern TCHAR __main_home__[MAX_PATH];		//��·��(TCHAR)
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

extern map<string, string> __nontrading_day_map__;	//�ǽ����ձ�
void loadNontradingDay(void);
