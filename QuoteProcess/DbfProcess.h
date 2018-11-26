#pragma once

#include "DbfRecord.h"
#include "WRguard.h"
#include <string>
#include <map>
using  namespace std;

#define		STKLABEL_LEN			10			// 股号数据长度,国内市场股号编码兼容钱龙
#define		STKNAME_LEN				32			// 股名长度
extern UINT QuoteProcessThread(LPVOID);


//////////////////////////////////////////////////////////////////////////
#include "ocilib.hpp"
using namespace ocilib;
static Connection con;
//////////////////////////////////////////////////////////////////////////


#define MARKET_SH  0
#define MARKET_SZ  1

class CDbfProcess
{
public:
	CDbfProcess();
	~CDbfProcess();

	void CalcMinuteQuote();
	void CalcDayQuoteSh();
	void CalcDayQuoteSz();
	void CalcPreQuoteSh();
	void CalcPreQuoteSz();
	void ReadShDbfHead(CString dbfPath, int& headSize);
	void ReadSzDbfHead(CString dbfPath, int& headSize);

	void ReadMarketDbf(CString sh_ddf, CString sz_dfb);
	BOOL ReadDbfRecords(CFile* pFile, short sOffset, char* pcBuffer, long lSize);
	void ReadDbfTimeSh(CString dbfPaht, int* dbfDate, int* dbfTime);
	void ReadDbfTimeSz(CString dbfPaht, int* dbfDate, int* dbfTime);

	int AddRecord(CDbfRecord*);
	CDbfRecord* GetRecord(char, long);
	CDbfRecord* GetRecord(long);
	void ClearRecords();
	void ProcessData();
	bool InitDB();
	void dbReconnect();
	void executeSql(ostring sql);
	void ClearMemoryPreData(int market);
	void ClearMemoryMinData(int market);
	void ClearMemoryDayData(int market);
	void init();
	bool isInitDb;


public:

	typedef	struct tagMinutedQuoteDetail
	{
		long    date;            //日期
		long    time;            //时间
		double	openPrice;		 //分钟开盘价
		double  highPrice;		 //分钟最高价
		double	lowPrice;		 //分钟最低价	
		double	closePrice;		 //分钟收盘价	
		double  volume;          //分钟成交总量
		double	amount;		     //分钟成交总额
		double  VolumeToday;     //分钟成交总量累计，分钟开始的时间点
		double	amountToday;	 //分钟成交总额累计，分钟开始的时间点
	}MinutedQuoteDetail, *pMinutedQuoteDetail;

	typedef struct tagMinuteQuote
	{
		char       code[STKLABEL_LEN];	    //代码
		char       name[STKNAME_LEN];	    //名称	
		char       market[3];               //市场
		MinutedQuoteDetail	  minQuote;	    //分钟数据
		MinutedQuoteDetail	  preMinQuote;	//前一分钟数据
	}MinuteQuote, *pMinuteQuote;


	typedef	struct tagDayQuoteDetail
	{
		long    date;                //日期
		double	openPrice;		     //开盘价
		double  highPrice;		     //最高价
		double	lowPrice;		     //最低价	
		double	closePrice;		     //收盘价	
		double  VolumeToday;         //成交总量累计
		double	amountToday;	     //成交总额累计
	}DayQuoteDetail, *pDayQuoteDetail;

	typedef	struct tagPreQuoteDetail
	{
		long    date;                //日期
		double  prePrice;		     //前收价
	}PreQuoteDetail, *pPreQuoteDetail;

	typedef struct tagDayQuote
	{
		char       code[STKLABEL_LEN];	//代码
		char       name[STKNAME_LEN];	//名称	
		char       market[3];           //市场
		DayQuoteDetail	  dayQuote;	    //日数据
	}DayQuote, *pDayQuote;


	typedef struct tagPreQuote
	{
		char       code[STKLABEL_LEN];	//代码
		char       name[STKNAME_LEN];	//名称	
		PreQuoteDetail	  preQuote;	    //盘前数据
	}PreQuote, *pPreQuote;


	HANDLE		m_hEventKillThread;
	HANDLE		m_hEventThreadKilled;
	int			m_nDBFDelay;

	map<string, pMinuteQuote> m_minuteQuoteMapSh;
	map<string, pMinuteQuote> m_minuteQuoteMapSz;

	map<string, pDayQuote> m_dayQuoteMapSh;
	map<string, pDayQuote> m_dayQuoteMapSz;

	map<string, pPreQuote> m_preQuoteMapSh;
	map<string, pPreQuote> m_preQuoteMapSz;


	bool isDbfTimeChangeSh; //沪市行情切换
	bool isDbfTimeChangeSz; //深市行情切换
	bool isMinChangeSh;     //沪市行情分钟数据切换
	bool isMinChangeSz;     //深市行情分钟数据切换
	long curDateSh;         //沪市当前行情日期
	long curDateSz;         //深市当前行情日期
	long lPreDbfTimeSh;     //上次沪市dbf行情时间 
	long lPreDbfTimeSz;     //上次沪市dbf行情时间 
	long lPreDbfMinuteSh;   //沪市当前行情分钟时间
	long lPreDbfMinuteSz;   //深市当前行情分钟时间
	long lCurDbfMinuteSh;   //沪市当前行情分钟时间
	long lCurDbfMinuteSz;   //深市当前行情分钟时间
	bool isNewMinuteSh;     //沪市行情分钟数据切换
	long lMinuteQuoteTimeSh;//当前分钟数据的时间
	long lMinuteQuoteTimeSz;//当前分钟数据的时间



	bool InsertToDbShMinuteQuote(void);
	bool InsertToDbSzMinuteQuote(void);

	bool InsertToDbShDayQuote(void);
	bool InsertToDbSzDayQuote(void);

	bool InsertToDbShPreQuote(void);
	bool InsertToDbSzPreQuote(void);

	CWRGuard m_gDbfRecordGuard;
	CPtrArray m_aSortDbfRecord;

	CCreateDir* cd;
	void minute_to_csv(MinutedQuoteDetail& data, ofstream& outFile);
	void close_to_csv(DayQuoteDetail& data, ofstream& outFile);
	void PreClose_to_csv(PreQuoteDetail& data, ofstream& outFile);
	void Write_csv_to_file(char* fileName,const char* fileData, long fileSize);

	int		m_sSHHeadLength;
	int		m_sSZHeadLength;
	char* pcDbfRecordsSH = nullptr;
	char* pcDbfRecordsSZ = nullptr;

	char* pcDbfRecordsSHOne = nullptr;
	char* pcDbfRecordsSZOne = nullptr;

	

private:
	short		m_sShDbfRecords;
	short		m_sSzDbfRecords;
	void StartDbfhread();
	void StopDbfThread();
	CDbfProcess* m_pDBFThread;
};

