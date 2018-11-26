#pragma once

#include "DbfRecord.h"
#include "WRguard.h"
#include <string>
#include <map>
using  namespace std;

#define		STKLABEL_LEN			10			// �ɺ����ݳ���,�����г��ɺű������Ǯ��
#define		STKNAME_LEN				32			// ��������
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
		long    date;            //����
		long    time;            //ʱ��
		double	openPrice;		 //���ӿ��̼�
		double  highPrice;		 //������߼�
		double	lowPrice;		 //������ͼ�	
		double	closePrice;		 //�������̼�	
		double  volume;          //���ӳɽ�����
		double	amount;		     //���ӳɽ��ܶ�
		double  VolumeToday;     //���ӳɽ������ۼƣ����ӿ�ʼ��ʱ���
		double	amountToday;	 //���ӳɽ��ܶ��ۼƣ����ӿ�ʼ��ʱ���
	}MinutedQuoteDetail, *pMinutedQuoteDetail;

	typedef struct tagMinuteQuote
	{
		char       code[STKLABEL_LEN];	    //����
		char       name[STKNAME_LEN];	    //����	
		char       market[3];               //�г�
		MinutedQuoteDetail	  minQuote;	    //��������
		MinutedQuoteDetail	  preMinQuote;	//ǰһ��������
	}MinuteQuote, *pMinuteQuote;


	typedef	struct tagDayQuoteDetail
	{
		long    date;                //����
		double	openPrice;		     //���̼�
		double  highPrice;		     //��߼�
		double	lowPrice;		     //��ͼ�	
		double	closePrice;		     //���̼�	
		double  VolumeToday;         //�ɽ������ۼ�
		double	amountToday;	     //�ɽ��ܶ��ۼ�
	}DayQuoteDetail, *pDayQuoteDetail;

	typedef	struct tagPreQuoteDetail
	{
		long    date;                //����
		double  prePrice;		     //ǰ�ռ�
	}PreQuoteDetail, *pPreQuoteDetail;

	typedef struct tagDayQuote
	{
		char       code[STKLABEL_LEN];	//����
		char       name[STKNAME_LEN];	//����	
		char       market[3];           //�г�
		DayQuoteDetail	  dayQuote;	    //������
	}DayQuote, *pDayQuote;


	typedef struct tagPreQuote
	{
		char       code[STKLABEL_LEN];	//����
		char       name[STKNAME_LEN];	//����	
		PreQuoteDetail	  preQuote;	    //��ǰ����
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


	bool isDbfTimeChangeSh; //���������л�
	bool isDbfTimeChangeSz; //���������л�
	bool isMinChangeSh;     //����������������л�
	bool isMinChangeSz;     //����������������л�
	long curDateSh;         //���е�ǰ��������
	long curDateSz;         //���е�ǰ��������
	long lPreDbfTimeSh;     //�ϴλ���dbf����ʱ�� 
	long lPreDbfTimeSz;     //�ϴλ���dbf����ʱ�� 
	long lPreDbfMinuteSh;   //���е�ǰ�������ʱ��
	long lPreDbfMinuteSz;   //���е�ǰ�������ʱ��
	long lCurDbfMinuteSh;   //���е�ǰ�������ʱ��
	long lCurDbfMinuteSz;   //���е�ǰ�������ʱ��
	bool isNewMinuteSh;     //����������������л�
	long lMinuteQuoteTimeSh;//��ǰ�������ݵ�ʱ��
	long lMinuteQuoteTimeSz;//��ǰ�������ݵ�ʱ��



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

