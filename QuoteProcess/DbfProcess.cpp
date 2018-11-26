
#include "stdafx.h"
#include "DbfProcess.h"
#include "minicsv.h"



/*
#define ELPP_STL_LOGGING
#define ELPP_THREAD_SAFE
#include "easylogging++.h"
INITIALIZE_EASYLOGGINGPP
*/


//////////////////////////////////////////////////////////////////////////
//操作oracle
#include <iostream>
#include <fstream>
#include <iomanip>
#include <sstream>

#if defined(OCI_CHARSET_WIDE)
#define ocout             std::wcout
#define oostringstream    std::wostringstream
#else
#define ocout             std::cout
#define oostringstream    std::ostringstream
#endif


#define otext(s) OTEXT(s)
#define oendl  std::endl

#ifdef _MSC_VER

#if defined(OCI_CHARSET_WIDE)
#pragma comment(lib, "ocilibw.lib")
#elif defined(OCI_CHARSET_ANSI)
#pragma comment(lib, "ociliba.lib")
#endif

#endif

//////////////////////////////////////////////////////////////////////////

#define size5M 1024 * 1024 * 1024 * 5L



CDbfProcess::CDbfProcess()
{

	lMinuteQuoteTimeSh = 0;//当前分钟数据的时间
	lMinuteQuoteTimeSz = 0;//当前分钟数据的时间
	isInitDb = false;
	m_sSHHeadLength = 0;
	m_sSZHeadLength = 0;
	m_hEventKillThread = CreateEvent(NULL, TRUE, FALSE, NULL);
	m_hEventThreadKilled = CreateEvent(NULL, TRUE, FALSE, NULL);
	cd = new CCreateDir();
}


CDbfProcess::~CDbfProcess()
{
}


void CDbfProcess::init(){
	lPreDbfTimeSh = 0; //上次沪市dbf行情时间 
	lPreDbfTimeSz = 0; //上次沪市dbf行情时间 
}

BOOL CDbfProcess::ReadDbfRecords(CFile* pFile, short sOffset, char* pcBuffer, long lSize)
{
	BOOL bRet;
	TRY
	{
		pFile->Seek(sOffset, CFile::begin);
		pFile->Read(pcBuffer, lSize);

		bRet = TRUE;
	}
	CATCH_ALL(e)
	{
		bRet = FALSE;
	}
	END_CATCH_ALL

		return bRet;
}


bool CDbfProcess::InitDB()
{
	ostring home;

	try
	{
		Environment::Initialize(Environment::Default | Environment::Threaded, home);

		Environment::EnableWarnings(true);

		char User[30] = { 0 };
		char Pwd[30] = { 0 };
		char DbName[30] = { 0 };

		WideCharToMultiByte(CP_ACP, 0, m_strDbUser.GetString(), -1, User, 30, NULL, NULL);
		WideCharToMultiByte(CP_ACP, 0, m_strDbPwd.GetString(), -1, Pwd, 30, NULL, NULL);
		WideCharToMultiByte(CP_ACP, 0, m_strDbName.GetString(), -1, DbName, 30, NULL, NULL);

		con.Open(DbName, User, Pwd, Environment::SessionDefault);

		//con.Close();
	}
	catch (std::exception &ex)
	{
		ocout << ex.what() << oendl;
		write_log(_T("初始化数据库失败：%hs"), ex.what());
		wsprintf(dbStatus, _T("%hs"), ex.what());
	}

	//已经初始化过标记
	isInitDb = true;

	if (con)
	{
		wsprintf(dbStatus, _T("初始化数据库成功"));
		write_log(_T("初始化数据库成功"));
		return true;
	}
	else
	{
		write_log(_T("初始化数据库失败，清查看日志"));
		return false;
	}
}


/* --------------------------------------------------------------------------------------------- *
* execute_ddl
* --------------------------------------------------------------------------------------------- */

void CDbfProcess::executeSql(ostring sql)
{
	try
	{
		Statement st(con);
		st.Execute(sql);
	}
	catch (Exception &ex)
	{
		switch (ex.GetType().GetValue())
		{
		case Exception::OracleError:
			ocout << otext("Oracle Error => ");
			break;
		case Exception::OracleWarning:
			ocout << otext("Oracle Error => ");
			break;
		case Exception::OcilibError:
			ocout << otext("OCILIB Error => ");
			break;
		default:
			ocout << otext("Unknown Error => ");
			break;
		}

		ocout << ex.what() << oendl;
		write_log(_T("执行SQL报错：%hs"), ex.what());
	}
}



void CDbfProcess::dbReconnect()
{
	int count = 0;
	try
	{
		//如果程序还没有初始化数据库，先不需要进行数据库重连操作
		if (!isInitDb)
			return;

		if (con)
		{
			Statement st(con);
			st.Execute("select 1 from dual");
			Resultset rs = st.GetResultset();
			while (rs.Next())
			{
				std::cout << rs.Get<ostring>(1) << std::endl;
				ostring rt = rs.Get<ostring>(1);
			}
			count = rs.GetCount();
		}
		else
		{
			Environment::Cleanup();
			InitDB();
		}
	}
	catch (std::exception &ex)
	{
		std::cout << ex.what() << std::endl;
		con.Close();
		Environment::Cleanup();
		InitDB();
	}
}

void CDbfProcess::ReadShDbfHead(CString dbfPath, int& headSize)
{
	CFile rFile;
	if (rFile.Open(dbfPath, CFile::modeRead | CFile::shareDenyNone))
	{
		try
		{
			rFile.Seek(4, CFile::begin);
			rFile.Read(&m_sShDbfRecords, 2); //文件中的记录条数。
			rFile.Seek(8, CFile::begin);
			rFile.Read(&headSize, 2);  //文件头中的字节数
			rFile.Close();
		}
		catch (...)
		{
			rFile.Close();
			write_log(_T("ReadShDbfHead  读取DBF失败"));
		}
	}
}

void CDbfProcess::ReadSzDbfHead(CString dbfPath, int& headSize)
{

	CFile rFile;
	if (rFile.Open(dbfPath, CFile::modeRead | CFile::shareDenyNone))
	{
		try
		{
			rFile.Seek(4, CFile::begin);
			rFile.Read(&m_sSzDbfRecords, 2); //文件中的记录条数。
			rFile.Seek(8, CFile::begin);
			rFile.Read(&headSize, 2); //文件头中的字节数
			rFile.Close();
		}
		catch (...)
		{
			rFile.Close();
			write_log(_T("ReadSzDbfHead  读取DBF失败"));
		}

	}
}

void CDbfProcess::StartDbfhread()
{
	m_pDBFThread = new CDbfProcess;
}

void CDbfProcess::StopDbfThread()
{

}



void CDbfProcess::ReadDbfTimeSz(CString dbfPaht, int* dbfDate, int* dbfTime)
{
	//只读第一行
	int rowNum = 1;
	CFile rFile;
	try{
		//读取深市DBF文件
		if (rFile.Open(dbfPaht, CFile::modeRead | CFile::shareDenyNone))
		{
			if (rowNum > 0)
			{
				long lDbfByteSize = SZDBFRECORDSIZE*rowNum;
				if (!pcDbfRecordsSZOne)
				{
					pcDbfRecordsSZOne = new char[lDbfByteSize];
				}

				memset(pcDbfRecordsSZOne, 0, lDbfByteSize);
				if (pcDbfRecordsSZOne)
				{
					if (ReadDbfRecords(&rFile, m_sSZHeadLength, pcDbfRecordsSZOne, lDbfByteSize))
					{
						char* pCurrent = pcDbfRecordsSZOne;
						for (short i = 0; i < rowNum; i++, pCurrent += SZDBFRECORDSIZE)
						{
							if (WaitForSingleObject(m_hEventKillThread, 0) == WAIT_OBJECT_0)
								break;
							//dbf第一条记录
							if (i == 0)
							{
								char pcBuf[16];
								long lDbfLastTime, lDbfLastDate;
								memset(pcBuf, NULL, sizeof(pcBuf));
								memcpy(pcBuf, pCurrent + 7, 8);
								lDbfLastDate = atol(pcBuf);

								memset(pcBuf, NULL, sizeof(pcBuf));
								memcpy(pcBuf, pCurrent + 73, 8);
								lDbfLastTime = atol(pcBuf);

								*dbfDate = lDbfLastDate;
								*dbfTime = lDbfLastTime;
								break;
							}
						}
					}
					else{
						write_log(_T("读取深圳DBF失败:%s"), dbfPaht);
					}
					//delete[] pcDbfRecords;
					//pcDbfRecords = nullptr;
				}
			}
			rFile.Close();
		}
		else{
			wsprintf(errInfo, _T("深圳DBF文件打不开,请检查路径"));
			write_log(_T("深圳DBF文件打不开:%s"), dbfPaht);
		}
	}
	catch (...)
	{
		write_log(_T("process sz dbf Exception"));
	}
}



void CDbfProcess::ReadDbfTimeSh(CString dbfPaht, int* dbfDate, int* dbfTime)
{

	//只读第一行
	int rowNum = 1;
	CFile rFile;
	try
	{
		if (rFile.Open(dbfPaht, CFile::modeRead | CFile::shareDenyNone))
		{
			if (rowNum > 0)
			{
				long lDbfByteSize = SHDBFRECORDSIZE*rowNum;
				if (!pcDbfRecordsSHOne)
				{
					pcDbfRecordsSHOne = new char[lDbfByteSize];
				}
				memset(pcDbfRecordsSHOne, 0, lDbfByteSize);

				if (pcDbfRecordsSHOne)
				{
					if (ReadDbfRecords(&rFile, m_sSHHeadLength, pcDbfRecordsSHOne, lDbfByteSize))
					{
						char* pCurrent = pcDbfRecordsSHOne;

						for (short i = 0; i < rowNum; i++, pCurrent += SHDBFRECORDSIZE)
						{
							if (WaitForSingleObject(m_hEventKillThread, 0) == WAIT_OBJECT_0)
								break;
							//dbf第一条记录
							if (i == 0)
							{
								char pcBuf[16];
								memset(pcBuf, NULL, sizeof(pcBuf));
								long lDbfLastTime, lDbfLastDate;

								//dbf行情最新时间 时分秒
								memset(pcBuf, NULL, sizeof(pcBuf));
								memcpy(pcBuf, pCurrent + 7, 8);
								lDbfLastTime = atol(pcBuf);


								//dbf行情最新日期
								memset(pcBuf, NULL, sizeof(pcBuf));
								memcpy(pcBuf, pCurrent + 43, 8);
								lDbfLastDate = atol(pcBuf);


								*dbfDate = lDbfLastDate;
								*dbfTime = lDbfLastTime;

								break;
							}
						}
					}
					else{
						write_log(_T("读取上海DBF失败:%s"), dbfPaht);
					}
					//delete[] pcDbfRecords;
					//pcDbfRecords = nullptr;
				}
			}
			rFile.Close();
		}
		else{
			wsprintf(errInfo, _T("上海DBF文件打不开,请检查路径"));
			write_log(_T("上海DBF文件打不开:%s"), dbfPaht);
		}

	}
	catch (...)
	{
		write_log(_T("process sh dbf Exception"));
	}


}

void CDbfProcess::ReadMarketDbf(CString sh_dbf, CString sz_dbf)
{


	CFile rFile;
	try
	{
		if (rFile.Open(sh_dbf, CFile::modeRead | CFile::shareDenyNone))
		{
			if (m_sShDbfRecords > 0)
			{
				long lDbfByteSize = SHDBFRECORDSIZE*m_sShDbfRecords;
				if (!pcDbfRecordsSH)
				{
					pcDbfRecordsSH = new char[lDbfByteSize];
				}
				memset(pcDbfRecordsSH, 0, lDbfByteSize);

				if (pcDbfRecordsSH)
				{
					if (ReadDbfRecords(&rFile, m_sSHHeadLength, pcDbfRecordsSH, lDbfByteSize))
					{
						char* pCurrent = pcDbfRecordsSH;

						for (short i = 0; i < m_sShDbfRecords; i++, pCurrent += SHDBFRECORDSIZE)
						{
							if (WaitForSingleObject(m_hEventKillThread, 0) == WAIT_OBJECT_0)
								break;
							//dbf第一条记录
							if (i == 0)
							{
								char pcBuf[16];
								memset(pcBuf, NULL, sizeof(pcBuf));
								long lDbfLastTime, lDbfLastDate;

								//dbf行情最新时间 时分秒
								memset(pcBuf, NULL, sizeof(pcBuf));
								memcpy(pcBuf, pCurrent + 7, 8);
								lDbfLastTime = atol(pcBuf);


								//dbf行情最新日期
								memset(pcBuf, NULL, sizeof(pcBuf));
								memcpy(pcBuf, pCurrent + 43, 8);
								lDbfLastDate = atol(pcBuf);
								curDateSh = lDbfLastDate;


								if (curDateSz != OS_cur_date || curDateSh != OS_cur_date)
								{
									write_log(_T("行情日期没有切换SH:curDateSz %d curDateSh %d OS_cur_date %d"), curDateSz, curDateSh, OS_cur_date);
									break;
								}
									


								write_log(_T("CurDbfQuoteTime-SH = %d"), lDbfLastTime);

								if (lDbfLastTime > lPreDbfTimeSh){
									lPreDbfTimeSh = lDbfLastTime;
									isDbfTimeChangeSh = true;
								}
								else if (lDbfLastTime < lPreDbfTimeSh)
								{
									write_log(_T("沪市行情时间回退 lDbfLastTime %d lDbfLastTimeShPre %d"), lDbfLastTime, lPreDbfTimeSh);
									break;
								}
								else if (lDbfLastTime == lPreDbfTimeSh)
								{
									break;
								}


								//dbf行情最新时间 时分 分钟行情切换用
								lCurDbfMinuteSh = (int)(lDbfLastTime / 100);


								if (0 == lPreDbfMinuteSh)
								{
									//第一次启动的时候需要
									lPreDbfMinuteSh = lCurDbfMinuteSh;
								}




								if (lPreDbfMinuteSh != lCurDbfMinuteSh)
								{
									write_log(_T("当前上海行情分钟切换 lPreDbfMinuteSh  %d lCurDbfMinuteSh %d"), lPreDbfMinuteSh, lCurDbfMinuteSh);
									lPreDbfMinuteSh = lCurDbfMinuteSh;
									isMinChangeSh = true;
								}
							}
							//第二条记录开始
							else
							{
								long lNameCode;
								char pcBuf[7];
								memcpy(pcBuf, pCurrent + 1, 6);	// 第一个byte是删除标记！
								pcBuf[6] = 0;
								lNameCode = atol(pcBuf);


								//A股证券 综合或成份指  如果不是这两个品种就继续下一个
								if (CString(pcBuf).Left(1) != "6"  && CString(pcBuf).Left(3) != "000")
								{
									continue;
								}

								if (lNameCode > 0)
								{
									long lID = lNameCode;
									CDbfRecord* pRecord = GetRecord(lID);

									if (pRecord == nullptr)
									{
										pRecord = new CDbfRecord;
										if (pRecord)
										{
											pRecord->m_cSecurityMarket = 0;
											pRecord->m_lSecurityCode = lNameCode;
											AddRecord(pRecord);
										}
									}
									if (pRecord)
										pRecord->ReadDbfRecordSH(pCurrent);
								}
							}
						}
					}
					else{
						write_log(_T("读取上海DBF失败:%s"), sh_dbf);
					}
					//delete[] pcDbfRecordsSH;
					//pcDbfRecordsSH = nullptr;
				}
			}
			rFile.Close();
		}
		else{
			wsprintf(errInfo, _T("上海DBF文件打不开,请检查路径"));
			write_log(_T("上海DBF文件打不开:%s"), sh_dbf);
		}

	}
	catch (...)
	{
		write_log(_T("process sh dbf Exception "));
	}


	try{
		//读取深市DBF文件
		if (rFile.Open(sz_dbf, CFile::modeRead | CFile::shareDenyNone))
		{
			if (m_sSzDbfRecords > 0)
			{
				long lDbfByteSize = SZDBFRECORDSIZE*m_sSzDbfRecords;
				if (!pcDbfRecordsSZ)
				{
					pcDbfRecordsSZ = new char[lDbfByteSize];
				}
				memset(pcDbfRecordsSZ, 0, lDbfByteSize);
				if (pcDbfRecordsSZ)
				{
					if (ReadDbfRecords(&rFile, m_sSZHeadLength, pcDbfRecordsSZ, lDbfByteSize))
					{
						char* pCurrent = pcDbfRecordsSZ;

						for (short i = 0; i < m_sSzDbfRecords; i++, pCurrent += SZDBFRECORDSIZE)
						{
							if (WaitForSingleObject(m_hEventKillThread, 0) == WAIT_OBJECT_0)
								break;
							//dbf第一条记录
							if (i == 0)
							{
								char pcBuf[16];
								long lDbfLastTime, lDbfLastDate;
								memset(pcBuf, NULL, sizeof(pcBuf));
								memcpy(pcBuf, pCurrent + 7, 8);
								lDbfLastDate = atol(pcBuf);
								curDateSz = lDbfLastDate;

								memset(pcBuf, NULL, sizeof(pcBuf));
								memcpy(pcBuf, pCurrent + 73, 8);
								lDbfLastTime = atol(pcBuf);



								if (curDateSz != OS_cur_date || curDateSh != OS_cur_date)
								{
									write_log(_T("行情日期没有切换SZ:curDateSz %d curDateSh %d OS_cur_date %d"), curDateSz, curDateSh, OS_cur_date);
									break;
								}

								write_log(_T("CurDbfQuoteTime-SZ = %d"), lDbfLastTime);


								if (lDbfLastTime > lPreDbfTimeSz){
									lPreDbfTimeSz = lDbfLastTime;
									isDbfTimeChangeSz = true;
								}
								else if (lDbfLastTime < lPreDbfTimeSz)
								{
									write_log(_T("深市行情时间回退 lDbfLastTime %d lDbfLastTimeSzPre %d"), lDbfLastTime, lPreDbfTimeSz);
									break;
								}
								else if (lDbfLastTime == lPreDbfTimeSz)
								{
									break;
								}

								//dbf行情最新时间 时分 分钟行情切换用
								lCurDbfMinuteSz = (int)(lDbfLastTime / 100);
								if (0 == lPreDbfMinuteSz)
								{
									//第一次启动的时候需要
									lPreDbfMinuteSz = lCurDbfMinuteSz;
								}

								


								if (lPreDbfMinuteSz != lCurDbfMinuteSz)
								{
									write_log(_T("当前深圳行情分钟切换 lPreDbfMinuteSz  %d lCurDbfMinuteSz %d"), lPreDbfMinuteSz, lCurDbfMinuteSz);
									lPreDbfMinuteSz = lCurDbfMinuteSz;
									isMinChangeSz = true;
								}
							}
							//dbf第二条记录开始
							else
							{
								long lSecurityCode;
								char pcBuf[7];
								memcpy(pcBuf, pCurrent + 1, 6);	// 第一个byte是删除标记！
								pcBuf[6] = 0;


								//A股证券 综合或成份指  如果不是这两个品种就继续下一个
								if (CString(pcBuf).Left(2) != "00"  && CString(pcBuf).Left(2) != "30"   && CString(pcBuf).Left(3) != "399")
								{
									continue;
								}

								lSecurityCode = atol(pcBuf);
								if (lSecurityCode > 0)
								{
									long lID = lSecurityCode + 1000000l;
									CDbfRecord* pRecord = GetRecord(lID);

									if (pRecord == nullptr)
									{
										pRecord = new CDbfRecord;
										if (pRecord)
										{
											pRecord->m_cSecurityMarket = 1;
											pRecord->m_lSecurityCode = lSecurityCode;
											AddRecord(pRecord);
										}
									}
									if (pRecord)
										pRecord->ReadDbfRecordSz(pCurrent);
								}
							}
						}
					}
					else{
						write_log(_T("读取深圳DBF失败:%s"), sh_dbf);
					}
					//delete[] pcDbfRecordsSZ;
					//pcDbfRecordsSZ = nullptr;
				}
			}
			rFile.Close();
		}
		else{
			wsprintf(errInfo, _T("深圳DBF文件打不开,请检查路径"));
			write_log(_T("深圳DBF文件打不开:"), sz_dbf);
		}
	}
	catch (...)
	{
		write_log(_T("process sz dbf Exception"));
	}


}


void CDbfProcess::ProcessData(){

	write_log(_T("calculate start......"));


	//计算盘前数据
	if (isPreQuoteFinishSh == false)
	{
		CalcPreQuoteSh();
	}

	if (isPreQuoteFinishSz == false)
	{
		CalcPreQuoteSz();
	}



	//计算分钟数据
	//清理分钟数据内存
	CalcMinuteQuote();


	//计算日数据
	if (isDayQuoteFinishSh == false)
	{
		CalcDayQuoteSh();
	}

	if (isDayQuoteFinishSz == false)
	{
		CalcDayQuoteSz();
	}
	write_log(_T("calculate end......"));
}


void CDbfProcess::ClearRecords()

{
	m_gDbfRecordGuard.BeginWrite(INFINITE);

	int nRecords = m_aSortDbfRecord.GetSize();
	for (int i = 0; i < nRecords; i++)
	{
		CDbfRecord* p = (CDbfRecord*)m_aSortDbfRecord[0];
		m_aSortDbfRecord.RemoveAt(0);
		delete p;
	}

	m_gDbfRecordGuard.EndWrite();
}

CDbfRecord* CDbfProcess::GetRecord(char cStation, long lNameCode)
{
	return GetRecord(cStation * 1000000l + lNameCode);
}

CDbfRecord* CDbfProcess::GetRecord(long lRecordID)
{
	m_gDbfRecordGuard.BeginRead(INFINITE);

	CDbfRecord* pRet = nullptr;
	int nLeft = 0;
	int nRight = m_aSortDbfRecord.GetSize() - 1;
	for (; nLeft <= nRight;)
	{
		int nMid = (nLeft + nRight) / 2;
		CDbfRecord* pRecord = (CDbfRecord*)m_aSortDbfRecord[nMid];
		long lID = pRecord->GetID();
		if (lRecordID == lID)
		{
			pRet = pRecord;
			break;
		}
		if (lRecordID < lID)
			nRight = nMid - 1;
		else
			nLeft = nMid + 1;
	}

	m_gDbfRecordGuard.EndRead();

	return pRet;
}

int CDbfProcess::AddRecord(CDbfRecord* pRecord)
{
	m_gDbfRecordGuard.BeginWrite(INFINITE);

	long lRecordID = pRecord->GetID();

	int nRet = -1;
	int nLeft = 0;
	int nRight = m_aSortDbfRecord.GetSize() - 1;

	if (nRight < nLeft)
	{
		m_aSortDbfRecord.Add(pRecord);
		nRet = 0;
	}
	else
	{
		for (; nLeft <= nRight;)
		{
			CDbfRecord* pG = (CDbfRecord*)m_aSortDbfRecord[nLeft];
			long lID = pG->GetID();
			if (lRecordID < lID)
			{
				m_aSortDbfRecord.InsertAt(nLeft, pRecord);
				nRet = nLeft;
				break;
			}
			if (lRecordID == lID)
				break;

			pG = (CDbfRecord*)m_aSortDbfRecord[nRight];
			lID = pG->GetID();
			if (lRecordID > lID)
			{
				m_aSortDbfRecord.InsertAt(nRight + 1, pRecord);
				nRet = nRight + 1;
				break;
			}
			if (lRecordID == lID)
				break;

			int nMid = (nLeft + nRight) / 2;
			pG = (CDbfRecord*)m_aSortDbfRecord[nMid];
			lID = pG->GetID();
			if (lRecordID > lID)
				nLeft = nMid + 1;
			else if (lRecordID == lID)
				break;
			else
				nRight = nMid - 1;
		}
	}
	m_gDbfRecordGuard.EndWrite();

	return nRet;
}



//分钟数据计算
void CDbfProcess::CalcMinuteQuote()
{

	//write_log(_T("CalcMinuteQuote start"));
	m_gDbfRecordGuard.BeginRead(INFINITE);
	int nDbfRecodeSize = m_aSortDbfRecord.GetSize();
	if (nDbfRecodeSize == 0)
	{
		return;
	}

	CDbfRecord* pRecord = nullptr;
	for (int i = 0; i < nDbfRecodeSize; i++)
	{
		pRecord = (CDbfRecord*)m_aSortDbfRecord[i];
		EnterCriticalSection(&(pRecord->m_rCritical));

		char cMarketFlag = pRecord->m_cSecurityMarket;
		long lNameCode = pRecord->m_lSecurityCode;
		double fVolumn = pRecord->m_plRecordValues[VOLUMN];
		double fAmount = pRecord->m_plRecordValues[AMOUNT];
		double fNewPrice = pRecord->m_plRecordValues[PRICE];
		char symbol[20] = { 0 };

		LeaveCriticalSection(&(pRecord->m_rCritical));

		if (cMarketFlag == MARKET_SH)
		{
			//沪市股票第一位是0标志
			sprintf_s(symbol, "%06d", lNameCode);


			//沪市分数数据写入map
			auto mq_iter_Sh = m_minuteQuoteMapSh.find(symbol);
			pMinuteQuote pMinQuote = nullptr;
			if (mq_iter_Sh == m_minuteQuoteMapSh.end())
			{
				//添加行情到Map
				pMinQuote = new MinuteQuote;
				memset(pMinQuote, 0, sizeof(MinuteQuote));
				strcpy_s(pMinQuote->code, strlen(symbol) + 1, symbol);
				strcpy_s(pMinQuote->name, "");
				strcpy_s(pMinQuote->market, strlen(&cMarketFlag) + 1, &cMarketFlag);

				pMinQuote->minQuote.date = curDateSh;
				pMinQuote->minQuote.time = lCurDbfMinuteSh;
				pMinQuote->minQuote.openPrice = fNewPrice;
				pMinQuote->minQuote.highPrice = fNewPrice;
				pMinQuote->minQuote.lowPrice = fNewPrice;
				pMinQuote->minQuote.closePrice = fNewPrice;
				pMinQuote->minQuote.VolumeToday = fVolumn;
				pMinQuote->minQuote.amountToday = fAmount;

				m_minuteQuoteMapSh[symbol] = pMinQuote;
			}
			else
			{
				pMinQuote = mq_iter_Sh->second;



				//如果分钟切换 新的分钟开始了
				if (isMinChangeSh)
				{
					//分钟数据写csv或入库
					//char cSql[200] = { 0 };
					//sprintf(cSql, "insert into tb_minute values(%d, %d,%s,%f, %f,%f,%f,%d, %d)", pMinQuote->minQuote.date, pMinQuote->minQuote.time, pMinQuote->code, pMinQuote->minQuote.openPrice, pMinQuote->minQuote.highPrice, pMinQuote->minQuote.lowPrice, pMinQuote->minQuote.closePrice, pMinQuote->minQuote.tradeVolume, pMinQuote->minQuote.tradeMoney);
					//execute_Sql(otext(cSql));
					//con.Commit();

					memcpy(&pMinQuote->preMinQuote, &pMinQuote->minQuote, sizeof(MinutedQuoteDetail));
					lMinuteQuoteTimeSh = pMinQuote->minQuote.time;

					//新的分钟已经开始，初始化分钟初始值
					pMinQuote->minQuote.date = curDateSh;
					pMinQuote->minQuote.time = lCurDbfMinuteSh;
					pMinQuote->minQuote.openPrice = fNewPrice;
					pMinQuote->minQuote.highPrice = fNewPrice;
					pMinQuote->minQuote.lowPrice = fNewPrice;
					pMinQuote->minQuote.closePrice = fNewPrice;
					pMinQuote->minQuote.VolumeToday = fVolumn;
					pMinQuote->minQuote.amountToday = fAmount;
				}
				else{
					//计算最高价
					if (fNewPrice > pMinQuote->minQuote.highPrice)
					{
						pMinQuote->minQuote.highPrice = fNewPrice;
					}

					//计算最低价
					if (fNewPrice < pMinQuote->minQuote.lowPrice)
					{
						pMinQuote->minQuote.lowPrice = fNewPrice;
					}

					//分钟收盘价
					pMinQuote->minQuote.closePrice = fNewPrice;

					//计算分钟内成交量和成交额
					pMinQuote->minQuote.volume = (fVolumn - pMinQuote->minQuote.VolumeToday);
					pMinQuote->minQuote.amount = (fAmount - pMinQuote->minQuote.amountToday);
				}
			}
		}
		else if (cMarketFlag == MARKET_SZ)
		{
			//深市股票第一位是1标志
			sprintf_s(symbol, "%06d", lNameCode);

			//深市分数数据写入map
			auto mq_iter_Sz = m_minuteQuoteMapSz.find(symbol);
			pMinuteQuote pMinQuote = nullptr;
			if (mq_iter_Sz == m_minuteQuoteMapSz.end())
			{
				//添加行情到Map
				pMinQuote = new MinuteQuote;
				memset(pMinQuote, 0, sizeof(MinuteQuote));
				strcpy_s(pMinQuote->code, strlen(symbol) + 1, symbol);
				strcpy_s(pMinQuote->name, "");
				strcpy_s(pMinQuote->market, strlen(&cMarketFlag) + 1, &cMarketFlag);

				pMinQuote->minQuote.date = curDateSz;
				pMinQuote->minQuote.time = lCurDbfMinuteSz;
				pMinQuote->minQuote.openPrice = fNewPrice;
				pMinQuote->minQuote.highPrice = fNewPrice;
				pMinQuote->minQuote.lowPrice = fNewPrice;
				pMinQuote->minQuote.closePrice = fNewPrice;
				pMinQuote->minQuote.VolumeToday = fVolumn;
				pMinQuote->minQuote.amountToday = fAmount;

				m_minuteQuoteMapSz[symbol] = pMinQuote;
			}
			else
			{
				pMinQuote = mq_iter_Sz->second;


				//如果分钟切换 新的分钟开始了
				if (isMinChangeSz)
				{
					//分钟数据写csv或入库
					//char cSql[200] = { 0 };
					//sprintf(cSql, "insert into tb_minute values(%d, %d,%s,%f, %f,%f,%f,%d, %d)", pMinQuote->minQuote.date, pMinQuote->minQuote.time, pMinQuote->code, pMinQuote->minQuote.openPrice, pMinQuote->minQuote.highPrice, pMinQuote->minQuote.lowPrice, pMinQuote->minQuote.closePrice, pMinQuote->minQuote.tradeVolume, pMinQuote->minQuote.tradeMoney);
					//execute_Sql(otext(cSql));
					//con.Commit();

					//新的分钟已经开始，初始化分钟初始值
					memcpy(&pMinQuote->preMinQuote, &pMinQuote->minQuote, sizeof(MinutedQuoteDetail));
					lMinuteQuoteTimeSz = pMinQuote->minQuote.time;

					pMinQuote->minQuote.date = curDateSz;
					pMinQuote->minQuote.time = lCurDbfMinuteSz;
					pMinQuote->minQuote.openPrice = fNewPrice;
					pMinQuote->minQuote.highPrice = fNewPrice;
					pMinQuote->minQuote.lowPrice = fNewPrice;
					pMinQuote->minQuote.closePrice = fNewPrice;
					pMinQuote->minQuote.VolumeToday = fVolumn;
					pMinQuote->minQuote.amountToday = fAmount;
				}
				else{

					//计算最高价
					if (fNewPrice > pMinQuote->minQuote.highPrice)
					{
						pMinQuote->minQuote.highPrice = fNewPrice;
					}

					//计算最低价
					if (fNewPrice < pMinQuote->minQuote.lowPrice)
					{
						pMinQuote->minQuote.lowPrice = fNewPrice;
					}

					//分钟收盘价
					pMinQuote->minQuote.closePrice = fNewPrice;

					//计算分钟内成交量和成交额
					pMinQuote->minQuote.volume = (fVolumn - pMinQuote->minQuote.VolumeToday);
					pMinQuote->minQuote.amount = (fAmount - pMinQuote->minQuote.amountToday);
				}
			}
		}
	}

	if (isMinChangeSh)
	{
		//重置标志位
		isMinChangeSh = false;

		//符合时间的分钟数据入库
		if ((lCurDbfMinuteSh >= 931 && lCurDbfMinuteSh <= 1130) || (lCurDbfMinuteSh >= 1301 && lCurDbfMinuteSh <= 1500))
		{
			write_log(_T("InsertToDbShMinuteQuote SH start Time:%d"), lCurDbfMinuteSh);
			InsertToDbShMinuteQuote();
			write_log(_T("InsertToDbShMinuteQuote SH end  Time:%d"), lCurDbfMinuteSh);
		}
	}
	if (isMinChangeSz)
	{
		//重置标志位
		isMinChangeSz = false;

		//符合时间的分钟数据入库
		if ((lCurDbfMinuteSz >= 931 && lCurDbfMinuteSz <= 1130) || (lCurDbfMinuteSz >= 1301 && lCurDbfMinuteSz <= 1500))
		{
			write_log(_T("InsertToDbSzMinuteQuote SZ start Time:%d"), lCurDbfMinuteSz);
			InsertToDbSzMinuteQuote();
			write_log(_T("InsertToDbSzMinuteQuote SZ end Time:%d"), lCurDbfMinuteSz);
		}
	}

	m_gDbfRecordGuard.EndRead();
	//write_log(_T("CalcMinuteQuote end"));
}




//日数据计算沪市
void CDbfProcess::CalcDayQuoteSh()
{
	//操作系统时间15:15以后处理日数据
	if (OS_cur_hms < 151200)
		return;

	//上海行情时间大于15点开始处理
	if (lPreDbfTimeSh < 150000)
		return;


	//上海收盘数据已处理完成不需要在继续操作
	if (isDayQuoteFinishSh)
		return;

	//清理日数据
	ClearMemoryDayData(MARKET_SH);

	//开始处理日数据
	m_gDbfRecordGuard.BeginRead(INFINITE);
	int nDbfRecodeSize = m_aSortDbfRecord.GetSize();
	if (nDbfRecodeSize == 0)
	{
		return;
	}

	for (int i = 0; i < nDbfRecodeSize; i++)
	{
		CDbfRecord* pRecord = (CDbfRecord*)m_aSortDbfRecord[i];
		EnterCriticalSection(&(pRecord->m_rCritical));

		char cMarketFlag = pRecord->m_cSecurityMarket;
		long lNameCode = pRecord->m_lSecurityCode;

		char symbol[20] = { 0 };
		sprintf_s(symbol, "%06d", lNameCode);

		LeaveCriticalSection(&(pRecord->m_rCritical));

		pDayQuote pdayQuote = nullptr;

		BOOL bIndex = pRecord->IsIndex();

		double fOpen = pRecord->m_plRecordValues[OPEN];
		double fHigh = pRecord->m_plRecordValues[HIGH];
		double fLow = pRecord->m_plRecordValues[LOW];
		double fClose = pRecord->m_plRecordValues[PRICE];
		double fVolumn = pRecord->m_plRecordValues[VOLUMN];
		double fAmount = pRecord->m_plRecordValues[AMOUNT];


		if (cMarketFlag == MARKET_SH)
		{
			//添加行情到Map
			pdayQuote = new DayQuote;
			memset(pdayQuote, 0, sizeof(DayQuote));
			strcpy_s(pdayQuote->code, strlen(symbol) + 1, symbol);
			strcpy_s(pdayQuote->name, "");
			strcpy_s(pdayQuote->market, strlen(&cMarketFlag) + 1, &cMarketFlag);

			pdayQuote->dayQuote.date = curDateSh;
			pdayQuote->dayQuote.openPrice = fOpen;
			pdayQuote->dayQuote.highPrice = fHigh;
			pdayQuote->dayQuote.lowPrice = fLow;
			pdayQuote->dayQuote.closePrice = fClose;
			pdayQuote->dayQuote.VolumeToday = fVolumn;
			pdayQuote->dayQuote.amountToday = fAmount;
			m_dayQuoteMapSh[symbol] = pdayQuote;

		}
	}

	if (m_dayQuoteMapSh.size() > 0){
		//沪市日数据入库
		if (InsertToDbShDayQuote()){
			isDayQuoteFinishSh = true;
			//清理内存数据
			ClearMemoryDayData(MARKET_SH);
		}
	}

	m_gDbfRecordGuard.EndRead();
}



//日数据计算深市
void CDbfProcess::CalcDayQuoteSz()
{

	//操作系统时间15:15以后处理日数据
	if (OS_cur_hms < 151200)
		return;

	//深圳行情大于15点开始处理日数据
	if (lPreDbfTimeSz < 150000)
		return;

	//深圳收盘数据已处理完成不需要在继续操作
	if (isDayQuoteFinishSz)
		return;

	//清理日数据
	ClearMemoryDayData(MARKET_SZ);

	//开始处理日数据
	m_gDbfRecordGuard.BeginRead(INFINITE);
	int nDbfRecodeSize = m_aSortDbfRecord.GetSize();
	if (nDbfRecodeSize == 0)
	{
		return;
	}
	for (int i = 0; i < nDbfRecodeSize; i++)
	{
		CDbfRecord* pRecord = (CDbfRecord*)m_aSortDbfRecord[i];
		EnterCriticalSection(&(pRecord->m_rCritical));

		char cMarketFlag = pRecord->m_cSecurityMarket;
		long lNameCode = pRecord->m_lSecurityCode;

		char symbol[20] = { 0 };
		sprintf_s(symbol, "%06d", lNameCode);

		LeaveCriticalSection(&(pRecord->m_rCritical));

		pDayQuote pdayQuote = nullptr;

		BOOL bIndex = pRecord->IsIndex();

		double fOpen = pRecord->m_plRecordValues[OPEN];
		double fHigh = pRecord->m_plRecordValues[HIGH];
		double fLow = pRecord->m_plRecordValues[LOW];
		double fClose = pRecord->m_plRecordValues[PRICE];
		double fVolumn = pRecord->m_plRecordValues[VOLUMN];
		double fAmount = pRecord->m_plRecordValues[AMOUNT];


		if (cMarketFlag == MARKET_SZ)
		{
			//添加行情到Map
			pdayQuote = new DayQuote;
			memset(pdayQuote, 0, sizeof(DayQuote));
			strcpy_s(pdayQuote->code, strlen(symbol) + 1, symbol);
			strcpy_s(pdayQuote->name, "");
			strcpy_s(pdayQuote->market, strlen(&cMarketFlag) + 1, &cMarketFlag);

			pdayQuote->dayQuote.date = curDateSz;
			pdayQuote->dayQuote.openPrice = fOpen;
			pdayQuote->dayQuote.highPrice = fHigh;
			pdayQuote->dayQuote.lowPrice = fLow;
			pdayQuote->dayQuote.closePrice = fClose;
			pdayQuote->dayQuote.VolumeToday = fVolumn;
			pdayQuote->dayQuote.amountToday = fAmount;
			m_dayQuoteMapSz[symbol] = pdayQuote;
		}
	}

	if (m_dayQuoteMapSz.size() > 0){
		//深市日数据入库
		if (InsertToDbSzDayQuote()){
			isDayQuoteFinishSz = true;
			//清理内存数据
			ClearMemoryDayData(MARKET_SZ);
		}
	}

	m_gDbfRecordGuard.EndRead();
}



//盘前数据计算沪市
void CDbfProcess::CalcPreQuoteSh()
{
	//早盘9点以后生成盘前数据 主要是前收价
	if (lPreDbfMinuteSh < 900)
		return;

	int nDbfRecodeSize = m_aSortDbfRecord.GetSize();
	if (nDbfRecodeSize == 0)
	{
		return;
	}

	//沪市场都已处理完成不需要在继续操作
	if (isPreQuoteFinishSh)
		return;

	//清理盘前数据
	ClearMemoryPreData(MARKET_SH);

	//开始处理盘前数据
	m_gDbfRecordGuard.BeginRead(INFINITE);


	for (int i = 0; i < nDbfRecodeSize; i++)
	{
		CDbfRecord* pRecord = (CDbfRecord*)m_aSortDbfRecord[i];
		EnterCriticalSection(&(pRecord->m_rCritical));

		char cMarketFlag = pRecord->m_cSecurityMarket;
		long lNameCode = pRecord->m_lSecurityCode;

		char symbol[20] = { 0 };
		sprintf_s(symbol, "%06d", lNameCode);

		LeaveCriticalSection(&(pRecord->m_rCritical));

		pPreQuote ppreQuote = nullptr;

		BOOL bIndex = pRecord->IsIndex();

		double fPreClose = pRecord->m_plRecordValues[PRECLOSE];


		if (cMarketFlag == MARKET_SH)
		{
			//添加盘前数据到Map
			ppreQuote = new PreQuote;
			memset(ppreQuote, 0, sizeof(PreQuote));
			strcpy_s(ppreQuote->code, strlen(symbol) + 1, symbol);
			ppreQuote->preQuote.date = curDateSh;
			ppreQuote->preQuote.prePrice = fPreClose;

			m_preQuoteMapSh[symbol] = ppreQuote;

		}
	}

	if (m_preQuoteMapSh.size() > 0){
		//沪市盘前数据入库
		if (InsertToDbShPreQuote()){
			isPreQuoteFinishSh = true;
			ClearMemoryPreData(MARKET_SH);
		}
	}


	m_gDbfRecordGuard.EndRead();
}





//盘前数据计算深市
void CDbfProcess::CalcPreQuoteSz()
{
	//早盘9点以后生成盘前数据 主要是前收价
	if (lCurDbfMinuteSz < 900)
		return;


	int nDbfRecodeSize = m_aSortDbfRecord.GetSize();
	if (nDbfRecodeSize == 0)
	{
		return;
	}


	//深市场都已处理完成不需要在继续操作
	if (isPreQuoteFinishSz)
		return;

	//清理盘前数据
	ClearMemoryPreData(MARKET_SZ);

	//开始处理日数据
	m_gDbfRecordGuard.BeginRead(INFINITE);


	for (int i = 0; i < nDbfRecodeSize; i++)
	{
		CDbfRecord* pRecord = (CDbfRecord*)m_aSortDbfRecord[i];
		EnterCriticalSection(&(pRecord->m_rCritical));

		char cMarketFlag = pRecord->m_cSecurityMarket;
		long lNameCode = pRecord->m_lSecurityCode;

		char symbol[20] = { 0 };
		sprintf_s(symbol, "%06d", lNameCode);

		LeaveCriticalSection(&(pRecord->m_rCritical));

		pPreQuote ppreQuote = nullptr;

		BOOL bIndex = pRecord->IsIndex();

		double fPreClose = pRecord->m_plRecordValues[PRECLOSE];


		if (cMarketFlag == MARKET_SZ)
		{
			//添加行情到Map
			ppreQuote = new PreQuote;
			memset(ppreQuote, 0, sizeof(PreQuote));
			strcpy_s(ppreQuote->code, strlen(symbol) + 1, symbol);
			ppreQuote->preQuote.date = curDateSz;
			ppreQuote->preQuote.prePrice = fPreClose;
			m_preQuoteMapSz[symbol] = ppreQuote;
		}
	}

	if (m_preQuoteMapSz.size() > 0){
		//深市盘前数据入库
		if (InsertToDbSzPreQuote()){
			isPreQuoteFinishSz = true;
			ClearMemoryPreData(MARKET_SZ);
		}
	}

	m_gDbfRecordGuard.EndRead();
}

//////////////////////////////////////////////////////////////////////////
//沪市分钟数据入库

bool CDbfProcess::InsertToDbShMinuteQuote(void)
{

	//股票
	std::vector<int>     tab_stock_date;
	std::vector<ostring> tab_stock_time;
	std::vector<ostring> tab_stock_code;
	std::vector<double>   tab_stock_open;
	std::vector<double>   tab_stock_high;
	std::vector<double>   tab_stock_low;
	std::vector<double>   tab_stock_close;
	std::vector<double>   tab_stock_volume;
	std::vector<double>   tab_stock_amount;

	//指数
	std::vector<int>     tab_index_date;
	std::vector<ostring> tab_index_time;
	std::vector<ostring> tab_index_code;
	std::vector<double>   tab_index_open;
	std::vector<double>   tab_index_high;
	std::vector<double>   tab_index_low;
	std::vector<double>   tab_index_close;
	std::vector<double>   tab_index_volume;
	std::vector<double>   tab_index_amount;

	int stockNumber = 0;
	int indexNumber = 0;


	/////////////////////////////csv/////////////////////////////////////////////
	char outputFileName[MAX_PATH] = { 0 };
	ostringstream csvFileBuffer;
	int prec = numeric_limits<double>::digits10;
	csvFileBuffer.precision(prec);
	csvFileBuffer << "type,date,code,time,openPrice,highPrice,lowPrice,closePrice,volume,amount,VolumeToday,amountToday\n";
	/////////////////////////////csv/////////////////////////////////////////////


	for (auto it = begin(m_minuteQuoteMapSh); it != end(m_minuteQuoteMapSh); ++it)
	{
		//股票
		if (CString(it->second->code).Left(1) == "6")
		{
			/////////////////////////////csv/////////////////////////////////////////////
			csvFileBuffer << "STK" << ","
				<< it->second->preMinQuote.date << ","
				<< it->second->code << ","
				<< it->second->minQuote.time << ","
				<< it->second->preMinQuote.openPrice << ","
				<< it->second->preMinQuote.highPrice << ","
				<< it->second->preMinQuote.lowPrice << ","
				<< it->second->preMinQuote.closePrice << ","
				<< it->second->preMinQuote.volume << ","
				<< it->second->preMinQuote.amount << ","
				<< it->second->preMinQuote.VolumeToday << ","
				<< it->second->preMinQuote.amountToday << "\n";
			//////////////////////////////csv////////////////////////////////////////////

			tab_stock_date.push_back(it->second->preMinQuote.date);
			//格式化时间不足4位前面补0
			if (it->second->minQuote.time < 1000)
			{
				tab_stock_time.push_back("0" + to_string(it->second->minQuote.time));
			}
			else
			{
				tab_stock_time.push_back(to_string(it->second->minQuote.time));
			}
			tab_stock_code.push_back(otext(it->second->code));
			tab_stock_open.push_back(it->second->preMinQuote.openPrice);
			tab_stock_high.push_back(it->second->preMinQuote.highPrice);
			tab_stock_low.push_back(it->second->preMinQuote.lowPrice);
			tab_stock_close.push_back(it->second->preMinQuote.closePrice);
			tab_stock_volume.push_back(it->second->preMinQuote.volume);
			tab_stock_amount.push_back(it->second->preMinQuote.amount);
			stockNumber++;
		}

		//指数
		if (CString(it->second->code).Left(3) == "000")
		{
			/////////////////////////////csv/////////////////////////////////////////////
			csvFileBuffer << "IDX" << ","
				<< it->second->preMinQuote.date << ","
				<< it->second->code << ","
				<< it->second->minQuote.time << ","
				<< it->second->preMinQuote.openPrice << ","
				<< it->second->preMinQuote.highPrice << ","
				<< it->second->preMinQuote.lowPrice << ","
				<< it->second->preMinQuote.closePrice << ","
				<< it->second->preMinQuote.volume << ","
				<< it->second->preMinQuote.amount << ","
				<< it->second->preMinQuote.VolumeToday << ","
				<< it->second->preMinQuote.amountToday << "\n";
			//////////////////////////////csv////////////////////////////////////////////

			tab_index_date.push_back(it->second->preMinQuote.date);
			//格式化时间不足4位前面补0
			if (it->second->minQuote.time < 1000)
			{
				tab_index_time.push_back("0" + to_string(it->second->minQuote.time));
			}
			else
			{
				tab_index_time.push_back(to_string(it->second->minQuote.time));
			}
			tab_index_code.push_back(otext(it->second->code));
			tab_index_open.push_back(it->second->preMinQuote.openPrice);
			tab_index_high.push_back(it->second->preMinQuote.highPrice);
			tab_index_low.push_back(it->second->preMinQuote.lowPrice);
			tab_index_close.push_back(it->second->preMinQuote.closePrice);
			tab_index_volume.push_back(it->second->preMinQuote.volume);
			tab_index_amount.push_back(it->second->preMinQuote.amount);
			indexNumber++;
		}

	}
	/////////////////////////////csv/////////////////////////////////////////////
	sprintf_s(outputFileName, ".\\minute\\%d\\%s.sh", tab_stock_date.at(0), tab_stock_time.at(0).c_str());
	Write_csv_to_file(outputFileName, csvFileBuffer.str().c_str(), strlen(csvFileBuffer.str().c_str()));

	///////////////////////////csv///////////////////////////////////////////////

	//股票
	try
	{
		Statement st(con);
		/*
		st.Prepare(otext("insert into tb_minute_stock ")
		otext("( ")
		otext("   data_,time_ , code_, open_, high_, low_, ")
		otext("   close_, volume_ ,amount_")
		otext(") ")
		otext("values ")
		otext("( ")
		otext("   :data_,:time_, :code_, :open_, :high_, :low_, ")
		otext("   :close_, :volume_ ,:amount_")
		otext(") ")
		);
		*/


		st.Prepare(otext("insert into tb_stk_003 ")
			otext("( ")
			otext("   t003_01,t003_02 , t003_03, t003_04, t003_05, t003_06, ")
			otext("   t003_07, t003_08 ,t003_09")
			otext(") ")
			otext("values ")
			otext("( ")
			otext("   to_date(:data_,'yyyymmdd'),:time_, :code_, :open_, :high_, :low_, ")
			otext("   :close_, :volume_ ,:amount_")
			otext(") ")
			);


		st.SetBindArraySize(stockNumber);

		/* bind vectors */
		st.Bind(otext(":data_"), tab_stock_date, BindInfo::In);
		st.Bind(otext(":time_"), tab_stock_time, 10, BindInfo::In);
		st.Bind(otext(":code_"), tab_stock_code, 10, BindInfo::In);
		st.Bind(otext(":open_"), tab_stock_open, BindInfo::In);
		st.Bind(otext(":high_"), tab_stock_high, BindInfo::In);
		st.Bind(otext(":low_"), tab_stock_low, BindInfo::In);
		st.Bind(otext(":close_"), tab_stock_close, BindInfo::In);
		st.Bind(otext(":volume_"), tab_stock_volume, BindInfo::In);
		st.Bind(otext(":amount_"), tab_stock_amount, BindInfo::In);

		st.ExecutePrepared();
		con.Commit();
		ocout << oendl << st.GetAffectedRows() << otext(" row(s) inserted") << oendl;
		write_log(_T("分钟数据入库沪市股票，共%d条记录,%d %hs"), st.GetAffectedRows(), tab_stock_date.at(0), tab_stock_time.at(0).c_str());
		wsprintf(strStaticMinSh, _T("已入库数据时间%hs"), tab_stock_time.at(0).c_str());




		if (st.GetAffectedRows() > 0){
			//return true;
		}
		else{
			//return false;
		}

	}
	catch (Exception &ex)
	{
		switch (ex.GetType().GetValue())
		{
		case Exception::OracleError:
			ocout << otext("Oracle Error => ");
			break;
		case Exception::OracleWarning:
			ocout << otext("Oracle Error => ");
			break;
		case Exception::OcilibError:
			ocout << otext("OCILIB Error => ");
			break;
		default:
			ocout << otext("Unknown Error => ");
			break;
		}

		ocout << ex.what() << oendl;
		write_log(_T("执行沪市分钟数据SQL报错：%hs"), ex.what());
		//return false;
	}


	//指数
	try
	{
		Statement st(con);
		/*
		st.Prepare(otext("insert into tb_minute_index ")
		otext("( ")
		otext("   data_, time_, code_, open_, high_, low_, ")
		otext("   close_, volume_ ,amount_")
		otext(") ")
		otext("values ")
		otext("( ")
		otext("   :data_, :time_,:code_, :open_, :high_, :low_, ")
		otext("   :close_, :volume_ ,:amount_")
		otext(") ")
		);
		*/

		st.Prepare(otext("insert into tb_index_005 ")
			otext("( ")
			otext("   t005_01, t005_02, t005_03, t005_04, t005_05, t005_06, ")
			otext("   t005_07, t005_08 ,t005_09")
			otext(") ")
			otext("values ")
			otext("( ")
			otext("  to_date(:data_,'yyyymmdd'), :time_,:code_, :open_, :high_, :low_, ")
			otext("   :close_, :volume_ ,:amount_")
			otext(") ")
			);


		st.SetBindArraySize(indexNumber);

		/* bind vectors */
		st.Bind(otext(":data_"), tab_index_date, BindInfo::In);
		st.Bind(otext(":time_"), tab_index_time, 10, BindInfo::In);
		st.Bind(otext(":code_"), tab_index_code, 10, BindInfo::In);
		st.Bind(otext(":open_"), tab_index_open, BindInfo::In);
		st.Bind(otext(":high_"), tab_index_high, BindInfo::In);
		st.Bind(otext(":low_"), tab_index_low, BindInfo::In);
		st.Bind(otext(":close_"), tab_index_close, BindInfo::In);
		st.Bind(otext(":volume_"), tab_index_volume, BindInfo::In);
		st.Bind(otext(":amount_"), tab_index_amount, BindInfo::In);

		st.ExecutePrepared();
		con.Commit();
		ocout << oendl << st.GetAffectedRows() << otext(" row(s) inserted") << oendl;
		write_log(_T("分钟数据入库沪市指数，共%d条记录,%d %hs"), st.GetAffectedRows(), tab_index_date.at(0), tab_index_time.at(0).c_str());




		try
		{
			string date = to_string(tab_index_date.at(0));
			string sin = tab_stock_time.at(0);
			string market = "SH";
			st.Prepare("begin p_minute_time(:date,:market, :sin); end;");
			st.Bind(OTEXT(":date"), date, static_cast<unsigned int>(date.size()), BindInfo::In);
			st.Bind(OTEXT(":market"), market, static_cast<unsigned int>(market.size()), BindInfo::In);
			st.Bind(OTEXT(":sin"), sin, static_cast<unsigned int>(sin.size()), BindInfo::In);
			st.ExecutePrepared();
		}
		catch (Exception &ex)
		{
			write_log(_T("沪市分钟数据时间入库失败 p_minute_time，分钟时间 %hs"), st.GetAffectedRows(), tab_stock_time.at(0).c_str());
		}

		if (st.GetAffectedRows() > 0){
			//return true;
		}
		else{
			//return false;
		}

	}
	catch (Exception &ex)
	{
		switch (ex.GetType().GetValue())
		{
		case Exception::OracleError:
			ocout << otext("Oracle Error => ");
			break;
		case Exception::OracleWarning:
			ocout << otext("Oracle Error => ");
			break;
		case Exception::OcilibError:
			ocout << otext("OCILIB Error => ");
			break;
		default:
			ocout << otext("Unknown Error => ");
			break;
		}

		ocout << ex.what() << oendl;
		write_log(_T("执行沪市分钟数据SQL报错：%hs"), ex.what());
		//return false;
	}

	return true;
}



//深市分钟数据入库
bool CDbfProcess::InsertToDbSzMinuteQuote(void)
{

	//股票
	std::vector<int>     tab_stock_date;
	std::vector<ostring> tab_stock_time;
	std::vector<ostring> tab_stock_code;
	std::vector<double>   tab_stock_open;
	std::vector<double>   tab_stock_high;
	std::vector<double>   tab_stock_low;
	std::vector<double>   tab_stock_close;
	std::vector<double>   tab_stock_volume;
	std::vector<double>   tab_stock_amount;

	//指数
	std::vector<int>     tab_index_date;
	std::vector<ostring> tab_index_time;
	std::vector<ostring> tab_index_code;
	std::vector<double>   tab_index_open;
	std::vector<double>   tab_index_high;
	std::vector<double>   tab_index_low;
	std::vector<double>   tab_index_close;
	std::vector<double>   tab_index_volume;
	std::vector<double>   tab_index_amount;

	int stockNumber = 0;
	int indexNumber = 0;

	/////////////////////////////csv/////////////////////////////////////////////
	char outputFileName[MAX_PATH] = { 0 };
	ostringstream csvFileBuffer;
	int prec = numeric_limits<double>::digits10;
	csvFileBuffer.precision(prec);
	csvFileBuffer << "type,date,code,time,openPrice,highPrice,lowPrice,closePrice,volume,amount,VolumeToday,amountToday\n";
	/////////////////////////////csv/////////////////////////////////////////////

	for (auto it = begin(m_minuteQuoteMapSz); it != end(m_minuteQuoteMapSz); ++it)
	{
		//股票
		if (CString(it->second->code).Left(2) == "00" || CString(it->second->code).Left(2) == "30")
		{
			/////////////////////////////csv/////////////////////////////////////////////
			csvFileBuffer << "STK" << ","
				<< it->second->preMinQuote.date << ","
				<< it->second->code << ","
				<< it->second->minQuote.time << ","
				<< it->second->preMinQuote.openPrice << ","
				<< it->second->preMinQuote.highPrice << ","
				<< it->second->preMinQuote.lowPrice << ","
				<< it->second->preMinQuote.closePrice << ","
				<< it->second->preMinQuote.volume << ","
				<< it->second->preMinQuote.amount << ","
				<< it->second->preMinQuote.VolumeToday << ","
				<< it->second->preMinQuote.amountToday << "\n";
			//////////////////////////////csv////////////////////////////////////////////

			tab_stock_date.push_back(it->second->preMinQuote.date);
			//格式化时间不足4位前面补0
			if (it->second->minQuote.time < 1000)
			{
				tab_stock_time.push_back("0" + to_string(it->second->minQuote.time));
			}
			else
			{
				tab_stock_time.push_back(to_string(it->second->minQuote.time));
			}
			tab_stock_code.push_back(otext(it->second->code));
			tab_stock_open.push_back(it->second->preMinQuote.openPrice);
			tab_stock_high.push_back(it->second->preMinQuote.highPrice);
			tab_stock_low.push_back(it->second->preMinQuote.lowPrice);
			tab_stock_close.push_back(it->second->preMinQuote.closePrice);
			tab_stock_volume.push_back(it->second->preMinQuote.volume);
			tab_stock_amount.push_back(it->second->preMinQuote.amount);
			stockNumber++;
		}

		//指数
		if (CString(it->second->code).Left(3) == "399")
		{
			/////////////////////////////csv/////////////////////////////////////////////
			csvFileBuffer << "IDX" << ","
				<< it->second->preMinQuote.date << ","
				<< it->second->code << ","
				<< it->second->minQuote.time << ","
				<< it->second->preMinQuote.openPrice << ","
				<< it->second->preMinQuote.highPrice << ","
				<< it->second->preMinQuote.lowPrice << ","
				<< it->second->preMinQuote.closePrice << ","
				<< it->second->preMinQuote.volume << ","
				<< it->second->preMinQuote.amount / 100 << "," //show2003.dbf 指数成交量手  sjshq.dbf成交量单位是股 分钟数据统一成手为单位
				<< it->second->preMinQuote.VolumeToday << ","
				<< it->second->preMinQuote.amountToday << "\n";
			//////////////////////////////csv////////////////////////////////////////////

			tab_index_date.push_back(it->second->preMinQuote.date);
			//格式化时间不足4位前面补0
			if (it->second->minQuote.time < 1000)
			{
				tab_index_time.push_back("0" + to_string(it->second->minQuote.time));
			}
			else
			{
				tab_index_time.push_back(to_string(it->second->minQuote.time));
			}
			tab_index_code.push_back(otext(it->second->code));
			tab_index_open.push_back(it->second->preMinQuote.openPrice);
			tab_index_high.push_back(it->second->preMinQuote.highPrice);
			tab_index_low.push_back(it->second->preMinQuote.lowPrice);
			tab_index_close.push_back(it->second->preMinQuote.closePrice);
			tab_index_volume.push_back(it->second->preMinQuote.volume / 100);  //show2003.dbf 指数成交量手  sjshq.dbf成交量单位是股 分钟数据统一成手为单位
			tab_index_amount.push_back(it->second->preMinQuote.amount);
			indexNumber++;
		}

	}

	/////////////////////////////csv/////////////////////////////////////////////
	sprintf_s(outputFileName, ".\\minute\\%d\\%s.sz", tab_stock_date.at(0), tab_stock_time.at(0).c_str());
	Write_csv_to_file(outputFileName, csvFileBuffer.str().c_str(), strlen(csvFileBuffer.str().c_str()));

	///////////////////////////csv///////////////////////////////////////////////


	//股票
	try
	{

		Statement st(con);
		/*
		st.Prepare(otext("insert into tb_minute_stock ")
		otext("( ")
		otext("   data_,time_ , code_, open_, high_, low_, ")
		otext("   close_, volume_ ,amount_")
		otext(") ")
		otext("values ")
		otext("( ")
		otext("   :data_,:time_, :code_, :open_, :high_, :low_, ")
		otext("   :close_, :volume_ ,:amount_")
		otext(") ")
		);
		*/
		st.Prepare(otext("insert into tb_stk_003 ")
			otext("( ")
			otext("   t003_01,t003_02 , t003_03, t003_04, t003_05, t003_06, ")
			otext("   t003_07, t003_08 ,t003_09")
			otext(") ")
			otext("values ")
			otext("( ")
			otext("   to_date(:data_,'yyyymmdd'),:time_, :code_, :open_, :high_, :low_, ")
			otext("   :close_, :volume_ ,:amount_")
			otext(") ")
			);

		st.SetBindArraySize(stockNumber);

		/* bind vectors */
		st.Bind(otext(":data_"), tab_stock_date, BindInfo::In);
		st.Bind(otext(":time_"), tab_stock_time, 10, BindInfo::In);
		st.Bind(otext(":code_"), tab_stock_code, 10, BindInfo::In);
		st.Bind(otext(":open_"), tab_stock_open, BindInfo::In);
		st.Bind(otext(":high_"), tab_stock_high, BindInfo::In);
		st.Bind(otext(":low_"), tab_stock_low, BindInfo::In);
		st.Bind(otext(":close_"), tab_stock_close, BindInfo::In);
		st.Bind(otext(":volume_"), tab_stock_volume, BindInfo::In);
		st.Bind(otext(":amount_"), tab_stock_amount, BindInfo::In);

		st.ExecutePrepared();
		con.Commit();
		ocout << oendl << st.GetAffectedRows() << otext(" row(s) inserted") << oendl;
		write_log(_T("分钟数据入库深市股票，共%d条记录,%d %hs"), st.GetAffectedRows(), tab_stock_date.at(0), tab_stock_time.at(0).c_str());
		wsprintf(strStaticMinSz, _T("已入库数据时间%hs"), tab_stock_time.at(0).c_str());





		if (st.GetAffectedRows() > 0){
			//return true;
		}
		else{
			//return false;
		}

	}
	catch (Exception &ex)
	{
		switch (ex.GetType().GetValue())
		{
		case Exception::OracleError:
			ocout << otext("Oracle Error => ");
			break;
		case Exception::OracleWarning:
			ocout << otext("Oracle Error => ");
			break;
		case Exception::OcilibError:
			ocout << otext("OCILIB Error => ");
			break;
		default:
			ocout << otext("Unknown Error => ");
			break;
		}

		ocout << ex.what() << oendl;
		write_log(_T("执行深市分钟数据SQL报错：%hs"), ex.what());
		//return false;
	}


	//指数
	try
	{
		Statement st(con);
		/*
		st.Prepare(otext("insert into tb_minute_index ")
		otext("( ")
		otext("   data_,time_,  code_, open_, high_, low_, ")
		otext("   close_, volume_ ,amount_")
		otext(") ")
		otext("values ")
		otext("( ")
		otext("   :data_,:time_, :code_, :open_, :high_, :low_, ")
		otext("   :close_, :volume_ ,:amount_")
		otext(") ")
		);
		*/

		st.Prepare(otext("insert into tb_index_005 ")
			otext("( ")
			otext("   t005_01, t005_02, t005_03, t005_04, t005_05, t005_06, ")
			otext("   t005_07, t005_08 ,t005_09")
			otext(") ")
			otext("values ")
			otext("( ")
			otext("   to_date(:data_,'yyyymmdd'), :time_,:code_, :open_, :high_, :low_, ")
			otext("   :close_, :volume_ ,:amount_")
			otext(") ")
			);


		st.SetBindArraySize(indexNumber);

		/* bind vectors */
		st.Bind(otext(":data_"), tab_index_date, BindInfo::In);
		st.Bind(otext(":time_"), tab_index_time, 10, BindInfo::In);
		st.Bind(otext(":code_"), tab_index_code, 10, BindInfo::In);
		st.Bind(otext(":open_"), tab_index_open, BindInfo::In);
		st.Bind(otext(":high_"), tab_index_high, BindInfo::In);
		st.Bind(otext(":low_"), tab_index_low, BindInfo::In);
		st.Bind(otext(":close_"), tab_index_close, BindInfo::In);
		st.Bind(otext(":volume_"), tab_index_volume, BindInfo::In);
		st.Bind(otext(":amount_"), tab_index_amount, BindInfo::In);

		st.ExecutePrepared();
		con.Commit();
		ocout << oendl << st.GetAffectedRows() << otext(" row(s) inserted") << oendl;
		write_log(_T("分钟数据入库深市指数，共%d条记录,%d %hs"), st.GetAffectedRows(), tab_index_date.at(0), tab_index_time.at(0).c_str());


		try
		{
			string date = to_string(tab_index_date.at(0));
			string sin = tab_index_time.at(0);
			string market = "SZ";
			st.Prepare("begin p_minute_time(:date,:market, :sin); end;");
			st.Bind(OTEXT(":date"), date, static_cast<unsigned int>(date.size()), BindInfo::In);
			st.Bind(OTEXT(":market"), market, static_cast<unsigned int>(market.size()), BindInfo::In);
			st.Bind(OTEXT(":sin"), sin, static_cast<unsigned int>(sin.size()), BindInfo::In);
			st.ExecutePrepared();
		}
		catch (Exception &ex)
		{
			write_log(_T("深市分钟数据时间入库失败 p_minute_time，分钟时间 %hs"), st.GetAffectedRows(), tab_index_time.at(0).c_str());
		}

		if (st.GetAffectedRows() > 0){
			//return true;
		}
		else{
			//return false;
		}

	}
	catch (Exception &ex)
	{
		switch (ex.GetType().GetValue())
		{
		case Exception::OracleError:
			ocout << otext("Oracle Error => ");
			break;
		case Exception::OracleWarning:
			ocout << otext("Oracle Error => ");
			break;
		case Exception::OcilibError:
			ocout << otext("OCILIB Error => ");
			break;
		default:
			ocout << otext("Unknown Error => ");
			break;
		}

		ocout << ex.what() << oendl;
		write_log(_T("执行深市分钟数据SQL报错：%hs"), ex.what());
		//return false;
	}

	return true;
}


//////////////////////////////////////////////////////////////////////////
//日数据入库

bool CDbfProcess::InsertToDbShDayQuote(void)
{
	/*
	//逐行入库，效率非常低
	char cSql[200] = { 0 };
	for (auto it = begin(m_dayQuoteMapSh); it != end(m_dayQuoteMapSh); ++it){
	sprintf(cSql, "insert into tb_day values(%d,%s,%f, %f,%f,%f,%d, %d)", it->second->dayQuote.date, it->second->code, it->second->dayQuote.openPrice, it->second->dayQuote.highPrice, it->second->dayQuote.lowPrice, it->second->dayQuote.closePrice, it->second->dayQuote.VolumeToday, it->second->dayQuote.MoneyToday);

	*/

	//为了提升入库性能，批量入库
	//股票
	std::vector<int>     tab_stock_date;
	std::vector<ostring> tab_stock_code;
	std::vector<double>   tab_stock_open;
	std::vector<double>   tab_stock_high;
	std::vector<double>   tab_stock_low;
	std::vector<double>   tab_stock_close;
	std::vector<double>   tab_stock_volume;
	std::vector<double>   tab_stock_amount;

	//指数
	std::vector<int>     tab_index_date;
	std::vector<ostring> tab_index_code;
	std::vector<double>   tab_index_open;
	std::vector<double>   tab_index_high;
	std::vector<double>   tab_index_low;
	std::vector<double>   tab_index_close;
	std::vector<double>   tab_index_volume;
	std::vector<double>   tab_index_amount;

	int stockNumber = 0;
	int indexNumber = 0;
	for (auto it = begin(m_dayQuoteMapSh); it != end(m_dayQuoteMapSh); ++it)
	{
		//股票
		if (CString(it->second->code).Left(1) == "6")
		{
			tab_stock_date.push_back(it->second->dayQuote.date);
			ostring str;
			str = otext(it->second->code);
			tab_stock_code.push_back(str);
			tab_stock_open.push_back(it->second->dayQuote.openPrice);
			tab_stock_high.push_back(it->second->dayQuote.highPrice);
			tab_stock_low.push_back(it->second->dayQuote.lowPrice);
			tab_stock_close.push_back(it->second->dayQuote.closePrice);
			tab_stock_volume.push_back(it->second->dayQuote.VolumeToday);
			tab_stock_amount.push_back(it->second->dayQuote.amountToday);
			stockNumber++;
		}

		//指数
		if (CString(it->second->code).Left(3) == "000")
		{
			tab_index_date.push_back(it->second->dayQuote.date);
			ostring str;
			str = otext(it->second->code);
			tab_index_code.push_back(str);
			tab_index_open.push_back(it->second->dayQuote.openPrice);
			tab_index_high.push_back(it->second->dayQuote.highPrice);
			tab_index_low.push_back(it->second->dayQuote.lowPrice);
			tab_index_close.push_back(it->second->dayQuote.closePrice);
			tab_index_volume.push_back(it->second->dayQuote.VolumeToday * 100); //show2003.dbf 指数成交量手  sjshq.dbf成交量单位是股  日数据统一成股
			tab_index_amount.push_back(it->second->dayQuote.amountToday);
			indexNumber++;
		}

	}


	//股票
	try
	{
		Statement st(con);

		//清除数据
		st.Prepare(otext("delete from tb_stk_004 where t004_02 like '6%' and  t004_01=to_date(:data_,'yyyymmdd')"));
		st.Bind(otext(":data_"), tab_stock_date.at(0), BindInfo::In);
		st.ExecutePrepared();

		/*
		st.Prepare(otext("insert into tb_day_stock ")
		otext("( ")
		otext("   data_,  code_, open_, high_, low_, ")
		otext("   close_, volume_ ,amount_")
		otext(") ")
		otext("values ")
		otext("( ")
		otext("   :data_, :code_, :open_, :high_, :low_, ")
		otext("   :close_, :volume_ ,:amount_")
		otext(") ")
		);
		*/

		st.Prepare(otext("insert into tb_stk_004 ")
			otext("( ")
			otext("   t004_01,  t004_02, t004_03, t004_04, t004_05, ")
			otext("   t004_06, t004_07 ,t004_08,t004_09")
			otext(") ")
			otext("values ")
			otext("( ")
			otext("   to_date(:data_,'yyyymmdd'), :code_, :open_, :high_, :low_, ")
			otext("   :close_, :volume_ ,:amount_,0")
			otext(") ")
			);

		st.SetBindArraySize(stockNumber);

		/* bind vectors */
		st.Bind(otext(":data_"), tab_stock_date, BindInfo::In);
		st.Bind(otext(":code_"), tab_stock_code, 10, BindInfo::In);
		st.Bind(otext(":open_"), tab_stock_open, BindInfo::In);
		st.Bind(otext(":high_"), tab_stock_high, BindInfo::In);
		st.Bind(otext(":low_"), tab_stock_low, BindInfo::In);
		st.Bind(otext(":close_"), tab_stock_close, BindInfo::In);
		st.Bind(otext(":volume_"), tab_stock_volume, BindInfo::In);
		st.Bind(otext(":amount_"), tab_stock_amount, BindInfo::In);

		st.ExecutePrepared();
		con.Commit();
		ocout << oendl << st.GetAffectedRows() << otext(" row(s) inserted") << oendl;
		write_log(_T("日数据入库沪市股票，共%d条记录  日期%d"), st.GetAffectedRows(), tab_stock_date.at(0));
		wsprintf(StaticCloseSh, _T("%d 已入库%d条记录"), tab_stock_date.at(0), st.GetAffectedRows());
		if (st.GetAffectedRows() > 0){
			//return true;
		}
		else{
			//return false;
		}

	}
	catch (Exception &ex)
	{
		switch (ex.GetType().GetValue())
		{
		case Exception::OracleError:
			ocout << otext("Oracle Error => ");
			break;
		case Exception::OracleWarning:
			ocout << otext("Oracle Error => ");
			break;
		case Exception::OcilibError:
			ocout << otext("OCILIB Error => ");
			break;
		default:
			ocout << otext("Unknown Error => ");
			break;
		}

		ocout << ex.what() << oendl;
		write_log(_T("执行沪市日数据SQL报错：%hs"), ex.what());
		//return false;
	}


	//指数
	try
	{
		Statement st(con);

		//清除数据
		st.Prepare(otext("delete from tb_index_006 where t006_02 like '000%' and  t006_01=to_date(:data_,'yyyymmdd')"));
		st.Bind(otext(":data_"), tab_index_date.at(0), BindInfo::In);
		st.ExecutePrepared();

		/*
		st.Prepare(otext("insert into tb_day_index ")
		otext("( ")
		otext("   data_,  code_, open_, high_, low_, ")
		otext("   close_, volume_ ,amount_")
		otext(") ")
		otext("values ")
		otext("( ")
		otext("   :data_, :code_, :open_, :high_, :low_, ")
		otext("   :close_, :volume_ ,:amount_")
		otext(") ")
		);
		*/

		st.Prepare(otext("insert into tb_index_006 ")
			otext("( ")
			otext("   t006_01,  t006_02, t006_03, t006_04, t006_05, ")
			otext("   t006_06, t006_07 ,t006_08,t006_09")
			otext(") ")
			otext("values ")
			otext("( ")
			otext("   to_date(:data_,'yyyymmdd'), :code_, :open_, :high_, :low_, ")
			otext("   :close_, :volume_ ,:amount_,0")
			otext(") ")
			);

		st.SetBindArraySize(indexNumber);

		/* bind vectors */
		st.Bind(otext(":data_"), tab_index_date, BindInfo::In);
		st.Bind(otext(":code_"), tab_index_code, 10, BindInfo::In);
		st.Bind(otext(":open_"), tab_index_open, BindInfo::In);
		st.Bind(otext(":high_"), tab_index_high, BindInfo::In);
		st.Bind(otext(":low_"), tab_index_low, BindInfo::In);
		st.Bind(otext(":close_"), tab_index_close, BindInfo::In);
		st.Bind(otext(":volume_"), tab_index_volume, BindInfo::In);
		st.Bind(otext(":amount_"), tab_index_amount, BindInfo::In);

		st.ExecutePrepared();
		con.Commit();
		ocout << oendl << st.GetAffectedRows() << otext(" row(s) inserted") << oendl;
		write_log(_T("日数据入库沪市指数，共%d条记录 日期%d"), st.GetAffectedRows(), tab_index_date.at(0));

		if (st.GetAffectedRows() > 0){
			//return true;
		}
		else{
			//return false;
		}

	}
	catch (Exception &ex)
	{
		switch (ex.GetType().GetValue())
		{
		case Exception::OracleError:
			ocout << otext("Oracle Error => ");
			break;
		case Exception::OracleWarning:
			ocout << otext("Oracle Error => ");
			break;
		case Exception::OcilibError:
			ocout << otext("OCILIB Error => ");
			break;
		default:
			ocout << otext("Unknown Error => ");
			break;
		}

		ocout << ex.what() << oendl;
		write_log(_T("执行沪市日数据SQL报错：%hs"), ex.what());
		//return false;
	}

	return true;
}



bool CDbfProcess::InsertToDbSzDayQuote(void)
{

	/*
	//逐行入库，效率非常低
	char cSql[200] = { 0 };
	for (auto it = begin(m_dayQuoteMapSh); it != end(m_dayQuoteMapSh); ++it){
	sprintf(cSql, "insert into tb_day values(%d,%s,%f, %f,%f,%f,%d, %d)", it->second->dayQuote.date, it->second->code, it->second->dayQuote.openPrice, it->second->dayQuote.highPrice, it->second->dayQuote.lowPrice, it->second->dayQuote.closePrice, it->second->dayQuote.VolumeToday, it->second->dayQuote.MoneyToday);

	*/

	//为了提升入库性能，批量入库
	//股票
	std::vector<int>     tab_stock_date;
	std::vector<ostring> tab_stock_code;
	std::vector<double>   tab_stock_open;
	std::vector<double>   tab_stock_high;
	std::vector<double>   tab_stock_low;
	std::vector<double>   tab_stock_close;
	std::vector<double>   tab_stock_volume;
	std::vector<double>   tab_stock_amount;

	//指数
	std::vector<int>     tab_index_date;
	std::vector<ostring> tab_index_code;
	std::vector<double>   tab_index_open;
	std::vector<double>   tab_index_high;
	std::vector<double>   tab_index_low;
	std::vector<double>   tab_index_close;
	std::vector<double>   tab_index_volume;
	std::vector<double>   tab_index_amount;

	int stockNumber = 0;
	int indexNumber = 0;
	for (auto it = begin(m_dayQuoteMapSz); it != end(m_dayQuoteMapSz); ++it)
	{
		//股票
		if (CString(it->second->code).Left(2) == "00" || CString(it->second->code).Left(2) == "30")
		{
			tab_stock_date.push_back(it->second->dayQuote.date);
			ostring str;
			str = otext(it->second->code);
			tab_stock_code.push_back(str);
			tab_stock_open.push_back(it->second->dayQuote.openPrice);
			tab_stock_high.push_back(it->second->dayQuote.highPrice);
			tab_stock_low.push_back(it->second->dayQuote.lowPrice);
			tab_stock_close.push_back(it->second->dayQuote.closePrice);
			tab_stock_volume.push_back(it->second->dayQuote.VolumeToday);
			tab_stock_amount.push_back(it->second->dayQuote.amountToday);
			stockNumber++;
		}

		//指数
		if (CString(it->second->code).Left(3) == "399")
		{
			tab_index_date.push_back(it->second->dayQuote.date);
			ostring str;
			str = otext(it->second->code);
			tab_index_code.push_back(str);
			tab_index_open.push_back(it->second->dayQuote.openPrice);
			tab_index_high.push_back(it->second->dayQuote.highPrice);
			tab_index_low.push_back(it->second->dayQuote.lowPrice);
			tab_index_close.push_back(it->second->dayQuote.closePrice);
			tab_index_volume.push_back(it->second->dayQuote.VolumeToday);
			tab_index_amount.push_back(it->second->dayQuote.amountToday);
			indexNumber++;
		}

	}


	//股票
	try
	{
		Statement st(con);
		//清除数据
		st.Prepare(otext("delete from tb_stk_004 where t004_02 not like '6%' and  t004_01=to_date(:data_,'yyyymmdd')"));
		st.Bind(otext(":data_"), tab_stock_date.at(0), BindInfo::In);
		st.ExecutePrepared();
		/*
		st.Prepare(otext("insert into tb_day_stock ")
		otext("( ")
		otext("   data_,  code_, open_, high_, low_, ")
		otext("   close_, volume_ ,amount_")
		otext(") ")
		otext("values ")
		otext("( ")
		otext("   :data_, :code_, :open_, :high_, :low_, ")
		otext("   :close_, :volume_ ,:amount_")
		otext(") ")
		);
		*/

		st.Prepare(otext("insert into tb_stk_004 ")
			otext("( ")
			otext("   t004_01,  t004_02, t004_03, t004_04, t004_05, ")
			otext("   t004_06, t004_07 ,t004_08,t004_09")
			otext(") ")
			otext("values ")
			otext("( ")
			otext("   to_date(:data_,'yyyymmdd'), :code_, :open_, :high_, :low_, ")
			otext("   :close_, :volume_ ,:amount_,1")
			otext(") ")
			);

		st.SetBindArraySize(stockNumber);

		/* bind vectors */
		st.Bind(otext(":data_"), tab_stock_date, BindInfo::In);
		st.Bind(otext(":code_"), tab_stock_code, 10, BindInfo::In);
		st.Bind(otext(":open_"), tab_stock_open, BindInfo::In);
		st.Bind(otext(":high_"), tab_stock_high, BindInfo::In);
		st.Bind(otext(":low_"), tab_stock_low, BindInfo::In);
		st.Bind(otext(":close_"), tab_stock_close, BindInfo::In);
		st.Bind(otext(":volume_"), tab_stock_volume, BindInfo::In);
		st.Bind(otext(":amount_"), tab_stock_amount, BindInfo::In);

		st.ExecutePrepared();
		con.Commit();
		ocout << oendl << st.GetAffectedRows() << otext(" row(s) inserted") << oendl;
		write_log(_T("日数据入库深市股票，共%d条记录 日期%d"), st.GetAffectedRows(), tab_stock_date.at(0));
		wsprintf(StaticCloseSz, _T("%d 已入库%d条记录"), tab_stock_date.at(0), st.GetAffectedRows());
		if (st.GetAffectedRows() > 0){
			///return true;
		}
		else{
			//return false;
		}

	}
	catch (Exception &ex)
	{
		switch (ex.GetType().GetValue())
		{
		case Exception::OracleError:
			ocout << otext("Oracle Error => ");
			break;
		case Exception::OracleWarning:
			ocout << otext("Oracle Error => ");
			break;
		case Exception::OcilibError:
			ocout << otext("OCILIB Error => ");
			break;
		default:
			ocout << otext("Unknown Error => ");
			break;
		}

		ocout << ex.what() << oendl;
		write_log(_T("执行深市日数据SQL报错：%hs"), ex.what());
		return false;
	}


	//指数
	try
	{
		Statement st(con);
		//清除数据
		st.Prepare(otext("delete from tb_index_006 where t006_02 like '399%' and  t006_01=to_date(:data_,'yyyymmdd')"));
		st.Bind(otext(":data_"), tab_index_date.at(0), BindInfo::In);
		st.ExecutePrepared();
		/*
		st.Prepare(otext("insert into tb_day_index ")
		otext("( ")
		otext("   data_,  code_, open_, high_, low_, ")
		otext("   close_, volume_ ,amount_")
		otext(") ")
		otext("values ")
		otext("( ")
		otext("   :data_, :code_, :open_, :high_, :low_, ")
		otext("   :close_, :volume_ ,:amount_")
		otext(") ")
		);
		*/

		st.Prepare(otext("insert into tb_index_006 ")
			otext("( ")
			otext("   t006_01,  t006_02, t006_03, t006_04, t006_05, ")
			otext("   t006_06, t006_07 ,t006_08,t006_09")
			otext(") ")
			otext("values ")
			otext("( ")
			otext("   to_date(:data_,'yyyymmdd'), :code_, :open_, :high_, :low_, ")
			otext("   :close_, :volume_ ,:amount_,1")
			otext(") ")
			);

		st.SetBindArraySize(indexNumber);

		/* bind vectors */
		st.Bind(otext(":data_"), tab_index_date, BindInfo::In);
		st.Bind(otext(":code_"), tab_index_code, 10, BindInfo::In);
		st.Bind(otext(":open_"), tab_index_open, BindInfo::In);
		st.Bind(otext(":high_"), tab_index_high, BindInfo::In);
		st.Bind(otext(":low_"), tab_index_low, BindInfo::In);
		st.Bind(otext(":close_"), tab_index_close, BindInfo::In);
		st.Bind(otext(":volume_"), tab_index_volume, BindInfo::In);
		st.Bind(otext(":amount_"), tab_index_amount, BindInfo::In);

		st.ExecutePrepared();
		con.Commit();
		ocout << oendl << st.GetAffectedRows() << otext(" row(s) inserted") << oendl;
		write_log(_T("日数据入库深市指数，共%d条记录 日期%d"), st.GetAffectedRows(), tab_index_date.at(0));
		if (st.GetAffectedRows() > 0){
			//return true;
		}
		else{
			//return false;
		}

	}
	catch (Exception &ex)
	{
		switch (ex.GetType().GetValue())
		{
		case Exception::OracleError:
			ocout << otext("Oracle Error => ");
			break;
		case Exception::OracleWarning:
			ocout << otext("Oracle Error => ");
			break;
		case Exception::OcilibError:
			ocout << otext("OCILIB Error => ");
			break;
		default:
			ocout << otext("Unknown Error => ");
			break;
		}

		ocout << ex.what() << oendl;
		write_log(_T("执行深市日数据SQL报错：%hs"), ex.what());
		//return false;
	}

	return true;
}


//清理盘前数据内存数据
void   CDbfProcess::ClearMemoryPreData(int market){

	if (market == MARKET_SH)
	{
		for (auto itr = m_preQuoteMapSh.begin(); itr != m_preQuoteMapSh.end(); ++itr)
		{
			delete itr->second;
			itr->second = nullptr;
		}
		m_preQuoteMapSh.clear();
	}

	if (market == MARKET_SZ)
	{
		for (auto itr = m_preQuoteMapSz.begin(); itr != m_preQuoteMapSz.end(); ++itr)
		{
			delete itr->second;
			itr->second = nullptr;
		}
		m_preQuoteMapSz.clear();
	}
}


//清理分钟数据内存数据
void   CDbfProcess::ClearMemoryMinData(int market){

	if (market == MARKET_SH)
	{
		for (auto itr = m_minuteQuoteMapSh.begin(); itr != m_minuteQuoteMapSh.end(); ++itr)
		{
			delete itr->second;
			itr->second = nullptr;
		}
		m_minuteQuoteMapSh.clear();
	}

	if (market == MARKET_SZ)
	{
		for (auto itr = m_minuteQuoteMapSz.begin(); itr != m_minuteQuoteMapSz.end(); ++itr)
		{
			delete itr->second;
			itr->second = nullptr;
		}
		m_minuteQuoteMapSz.clear();
	}

	ClearRecords();
}


//清理日数据内存数据
void   CDbfProcess::ClearMemoryDayData(int market){


	if (market == MARKET_SH)
	{
		for (auto itr = m_dayQuoteMapSh.begin(); itr != m_dayQuoteMapSh.end(); ++itr)
		{
			delete itr->second;
			itr->second = nullptr;
		}
		m_dayQuoteMapSh.clear();
	}

	if (market == MARKET_SZ)
	{
		for (auto itr = m_dayQuoteMapSz.begin(); itr != m_dayQuoteMapSz.end(); ++itr)
		{
			delete itr->second;
			itr->second = nullptr;
		}
		m_dayQuoteMapSz.clear();
	}
}


//////////////////////////////////////////////////////////////////////////
//盘前数据入库

bool CDbfProcess::InsertToDbShPreQuote(void)
{
	//股票
	std::vector<int>     tab_stock_date;
	std::vector<ostring> tab_stock_code;
	std::vector<double>   tab_stock_preClose;



	int stockNumber = 0;
	int indexNumber = 0;
	for (auto it = begin(m_preQuoteMapSh); it != end(m_preQuoteMapSh); ++it)
	{
		//股票
		if (CString(it->second->code).Left(1) == "6")
		{
			tab_stock_date.push_back(it->second->preQuote.date);
			ostring str;
			str = otext(it->second->code);
			tab_stock_code.push_back(str);
			tab_stock_preClose.push_back(it->second->preQuote.prePrice);
			stockNumber++;
		}


	}


	//股票
	try
	{
		Statement st(con);

		//清除数据
		st.Prepare(otext("delete from TB_MARKET_PRECLOSE where market=0 and  trade_date=to_date(:data_,'yyyymmdd')"));
		st.Bind(otext(":data_"), tab_stock_date.at(0), BindInfo::In);
		st.ExecutePrepared();


		st.Prepare(otext("insert into TB_MARKET_PRECLOSE ")
			otext("( ")
			otext("   trade_date,  code, pre_close, market")
			otext(") ")
			otext("values ")
			otext("( ")
			otext("   to_date(:data_,'yyyymmdd'), :code_, :open_, 0")
			otext(") ")
			);

		st.SetBindArraySize(stockNumber);

		/* bind vectors */
		st.Bind(otext(":data_"), tab_stock_date, BindInfo::In);
		st.Bind(otext(":code_"), tab_stock_code, 10, BindInfo::In);
		st.Bind(otext(":open_"), tab_stock_preClose, BindInfo::In);


		st.ExecutePrepared();
		con.Commit();
		ocout << oendl << st.GetAffectedRows() << otext(" row(s) inserted") << oendl;
		write_log(_T("盘前数据入库沪市股票，共%d条记录 %d"), st.GetAffectedRows(), tab_stock_date.at(0));
		if (st.GetAffectedRows() > 0){
			//return true;
		}
		else{
			//return false;
		}

	}
	catch (Exception &ex)
	{
		switch (ex.GetType().GetValue())
		{
		case Exception::OracleError:
			ocout << otext("Oracle Error => ");
			break;
		case Exception::OracleWarning:
			ocout << otext("Oracle Error => ");
			break;
		case Exception::OcilibError:
			ocout << otext("OCILIB Error => ");
			break;
		default:
			ocout << otext("Unknown Error => ");
			break;
		}

		ocout << ex.what() << oendl;
		write_log(_T("执行沪市盘前数据SQL报错：%hs"), ex.what());
		//return false;
	}



	return true;
}



bool CDbfProcess::InsertToDbSzPreQuote(void)
{

	//股票
	std::vector<int>     tab_stock_date;
	std::vector<ostring> tab_stock_code;
	std::vector<double>   tab_stock_preClose;

	int stockNumber = 0;
	int indexNumber = 0;
	for (auto it = begin(m_preQuoteMapSz); it != end(m_preQuoteMapSz); ++it)
	{
		//股票
		if (CString(it->second->code).Left(2) == "00" || CString(it->second->code).Left(2) == "30")
		{
			tab_stock_date.push_back(it->second->preQuote.date);
			ostring str;
			str = otext(it->second->code);
			tab_stock_code.push_back(str);
			tab_stock_preClose.push_back(it->second->preQuote.prePrice);
			stockNumber++;
		}



	}


	//股票
	try
	{
		Statement st(con);

		//清除数据
		st.Prepare(otext("delete from TB_MARKET_PRECLOSE where market=1 and  trade_date=to_date(:data_,'yyyymmdd')"));
		st.Bind(otext(":data_"), tab_stock_date.at(0), BindInfo::In);
		st.ExecutePrepared();



		st.Prepare(otext("insert into TB_MARKET_PRECLOSE ")
			otext("( ")
			otext("   trade_date,  code, pre_close, market")
			otext(") ")
			otext("values ")
			otext("( ")
			otext("   to_date(:data_,'yyyymmdd'), :code_, :open_, 1")
			otext(") ")
			);

		st.SetBindArraySize(stockNumber);

		/* bind vectors */
		st.Bind(otext(":data_"), tab_stock_date, BindInfo::In);
		st.Bind(otext(":code_"), tab_stock_code, 10, BindInfo::In);
		st.Bind(otext(":open_"), tab_stock_preClose, BindInfo::In);


		st.ExecutePrepared();
		con.Commit();
		ocout << oendl << st.GetAffectedRows() << otext(" row(s) inserted") << oendl;
		write_log(_T("盘前数据入库深市股票，共%d条记录 %d"), st.GetAffectedRows(), tab_stock_date.at(0));
		if (st.GetAffectedRows() > 0){
			///return true;
		}
		else{
			//return false;
		}

	}
	catch (Exception &ex)
	{
		switch (ex.GetType().GetValue())
		{
		case Exception::OracleError:
			ocout << otext("Oracle Error => ");
			break;
		case Exception::OracleWarning:
			ocout << otext("Oracle Error => ");
			break;
		case Exception::OcilibError:
			ocout << otext("OCILIB Error => ");
			break;
		default:
			ocout << otext("Unknown Error => ");
			break;
		}

		ocout << ex.what() << oendl;
		write_log(_T("执行深市盘前数据SQL报错：%hs"), ex.what());
		return false;
	}



	return true;
}


void CDbfProcess::minute_to_csv(MinutedQuoteDetail& data, ofstream& outFile)
{
	outFile << data.date << "," << data.time << "," << data.openPrice << "," << data.highPrice << "," << data.lowPrice << "," << data.closePrice << "," << data.volume << "," << data.amount << "," << data.VolumeToday << "," << data.amountToday << endl;
}


void CDbfProcess::close_to_csv(DayQuoteDetail& data, ofstream& outFile)
{
	outFile << data.date << "," << data.openPrice << "," << data.highPrice << "," << data.lowPrice << "," << data.closePrice << "," << data.VolumeToday << "," << data.amountToday << endl;
}

void CDbfProcess::PreClose_to_csv(PreQuoteDetail& data, ofstream& outFile)
{
	outFile << data.date << "," << data.prePrice << endl;
}


void CDbfProcess::Write_csv_to_file(char* fileName, const char* fileData, long fileSize){

	cd->createDirectory(fileName);

	ofstream outf;
	outf.open(fileName);
	outf.write(fileData, fileSize);
	outf.close();
}
