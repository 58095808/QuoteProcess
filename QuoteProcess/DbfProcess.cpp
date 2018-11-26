
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
//����oracle
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

	lMinuteQuoteTimeSh = 0;//��ǰ�������ݵ�ʱ��
	lMinuteQuoteTimeSz = 0;//��ǰ�������ݵ�ʱ��
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
	lPreDbfTimeSh = 0; //�ϴλ���dbf����ʱ�� 
	lPreDbfTimeSz = 0; //�ϴλ���dbf����ʱ�� 
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
		write_log(_T("��ʼ�����ݿ�ʧ�ܣ�%hs"), ex.what());
		wsprintf(dbStatus, _T("%hs"), ex.what());
	}

	//�Ѿ���ʼ�������
	isInitDb = true;

	if (con)
	{
		wsprintf(dbStatus, _T("��ʼ�����ݿ�ɹ�"));
		write_log(_T("��ʼ�����ݿ�ɹ�"));
		return true;
	}
	else
	{
		write_log(_T("��ʼ�����ݿ�ʧ�ܣ���鿴��־"));
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
		write_log(_T("ִ��SQL����%hs"), ex.what());
	}
}



void CDbfProcess::dbReconnect()
{
	int count = 0;
	try
	{
		//�������û�г�ʼ�����ݿ⣬�Ȳ���Ҫ�������ݿ���������
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
			rFile.Read(&m_sShDbfRecords, 2); //�ļ��еļ�¼������
			rFile.Seek(8, CFile::begin);
			rFile.Read(&headSize, 2);  //�ļ�ͷ�е��ֽ���
			rFile.Close();
		}
		catch (...)
		{
			rFile.Close();
			write_log(_T("ReadShDbfHead  ��ȡDBFʧ��"));
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
			rFile.Read(&m_sSzDbfRecords, 2); //�ļ��еļ�¼������
			rFile.Seek(8, CFile::begin);
			rFile.Read(&headSize, 2); //�ļ�ͷ�е��ֽ���
			rFile.Close();
		}
		catch (...)
		{
			rFile.Close();
			write_log(_T("ReadSzDbfHead  ��ȡDBFʧ��"));
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
	//ֻ����һ��
	int rowNum = 1;
	CFile rFile;
	try{
		//��ȡ����DBF�ļ�
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
							//dbf��һ����¼
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
						write_log(_T("��ȡ����DBFʧ��:%s"), dbfPaht);
					}
					//delete[] pcDbfRecords;
					//pcDbfRecords = nullptr;
				}
			}
			rFile.Close();
		}
		else{
			wsprintf(errInfo, _T("����DBF�ļ��򲻿�,����·��"));
			write_log(_T("����DBF�ļ��򲻿�:%s"), dbfPaht);
		}
	}
	catch (...)
	{
		write_log(_T("process sz dbf Exception"));
	}
}



void CDbfProcess::ReadDbfTimeSh(CString dbfPaht, int* dbfDate, int* dbfTime)
{

	//ֻ����һ��
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
							//dbf��һ����¼
							if (i == 0)
							{
								char pcBuf[16];
								memset(pcBuf, NULL, sizeof(pcBuf));
								long lDbfLastTime, lDbfLastDate;

								//dbf��������ʱ�� ʱ����
								memset(pcBuf, NULL, sizeof(pcBuf));
								memcpy(pcBuf, pCurrent + 7, 8);
								lDbfLastTime = atol(pcBuf);


								//dbf������������
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
						write_log(_T("��ȡ�Ϻ�DBFʧ��:%s"), dbfPaht);
					}
					//delete[] pcDbfRecords;
					//pcDbfRecords = nullptr;
				}
			}
			rFile.Close();
		}
		else{
			wsprintf(errInfo, _T("�Ϻ�DBF�ļ��򲻿�,����·��"));
			write_log(_T("�Ϻ�DBF�ļ��򲻿�:%s"), dbfPaht);
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
							//dbf��һ����¼
							if (i == 0)
							{
								char pcBuf[16];
								memset(pcBuf, NULL, sizeof(pcBuf));
								long lDbfLastTime, lDbfLastDate;

								//dbf��������ʱ�� ʱ����
								memset(pcBuf, NULL, sizeof(pcBuf));
								memcpy(pcBuf, pCurrent + 7, 8);
								lDbfLastTime = atol(pcBuf);


								//dbf������������
								memset(pcBuf, NULL, sizeof(pcBuf));
								memcpy(pcBuf, pCurrent + 43, 8);
								lDbfLastDate = atol(pcBuf);
								curDateSh = lDbfLastDate;


								if (curDateSz != OS_cur_date || curDateSh != OS_cur_date)
								{
									write_log(_T("��������û���л�SH:curDateSz %d curDateSh %d OS_cur_date %d"), curDateSz, curDateSh, OS_cur_date);
									break;
								}
									


								write_log(_T("CurDbfQuoteTime-SH = %d"), lDbfLastTime);

								if (lDbfLastTime > lPreDbfTimeSh){
									lPreDbfTimeSh = lDbfLastTime;
									isDbfTimeChangeSh = true;
								}
								else if (lDbfLastTime < lPreDbfTimeSh)
								{
									write_log(_T("��������ʱ����� lDbfLastTime %d lDbfLastTimeShPre %d"), lDbfLastTime, lPreDbfTimeSh);
									break;
								}
								else if (lDbfLastTime == lPreDbfTimeSh)
								{
									break;
								}


								//dbf��������ʱ�� ʱ�� ���������л���
								lCurDbfMinuteSh = (int)(lDbfLastTime / 100);


								if (0 == lPreDbfMinuteSh)
								{
									//��һ��������ʱ����Ҫ
									lPreDbfMinuteSh = lCurDbfMinuteSh;
								}




								if (lPreDbfMinuteSh != lCurDbfMinuteSh)
								{
									write_log(_T("��ǰ�Ϻ���������л� lPreDbfMinuteSh  %d lCurDbfMinuteSh %d"), lPreDbfMinuteSh, lCurDbfMinuteSh);
									lPreDbfMinuteSh = lCurDbfMinuteSh;
									isMinChangeSh = true;
								}
							}
							//�ڶ�����¼��ʼ
							else
							{
								long lNameCode;
								char pcBuf[7];
								memcpy(pcBuf, pCurrent + 1, 6);	// ��һ��byte��ɾ����ǣ�
								pcBuf[6] = 0;
								lNameCode = atol(pcBuf);


								//A��֤ȯ �ۺϻ�ɷ�ָ  �������������Ʒ�־ͼ�����һ��
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
						write_log(_T("��ȡ�Ϻ�DBFʧ��:%s"), sh_dbf);
					}
					//delete[] pcDbfRecordsSH;
					//pcDbfRecordsSH = nullptr;
				}
			}
			rFile.Close();
		}
		else{
			wsprintf(errInfo, _T("�Ϻ�DBF�ļ��򲻿�,����·��"));
			write_log(_T("�Ϻ�DBF�ļ��򲻿�:%s"), sh_dbf);
		}

	}
	catch (...)
	{
		write_log(_T("process sh dbf Exception "));
	}


	try{
		//��ȡ����DBF�ļ�
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
							//dbf��һ����¼
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
									write_log(_T("��������û���л�SZ:curDateSz %d curDateSh %d OS_cur_date %d"), curDateSz, curDateSh, OS_cur_date);
									break;
								}

								write_log(_T("CurDbfQuoteTime-SZ = %d"), lDbfLastTime);


								if (lDbfLastTime > lPreDbfTimeSz){
									lPreDbfTimeSz = lDbfLastTime;
									isDbfTimeChangeSz = true;
								}
								else if (lDbfLastTime < lPreDbfTimeSz)
								{
									write_log(_T("��������ʱ����� lDbfLastTime %d lDbfLastTimeSzPre %d"), lDbfLastTime, lPreDbfTimeSz);
									break;
								}
								else if (lDbfLastTime == lPreDbfTimeSz)
								{
									break;
								}

								//dbf��������ʱ�� ʱ�� ���������л���
								lCurDbfMinuteSz = (int)(lDbfLastTime / 100);
								if (0 == lPreDbfMinuteSz)
								{
									//��һ��������ʱ����Ҫ
									lPreDbfMinuteSz = lCurDbfMinuteSz;
								}

								


								if (lPreDbfMinuteSz != lCurDbfMinuteSz)
								{
									write_log(_T("��ǰ������������л� lPreDbfMinuteSz  %d lCurDbfMinuteSz %d"), lPreDbfMinuteSz, lCurDbfMinuteSz);
									lPreDbfMinuteSz = lCurDbfMinuteSz;
									isMinChangeSz = true;
								}
							}
							//dbf�ڶ�����¼��ʼ
							else
							{
								long lSecurityCode;
								char pcBuf[7];
								memcpy(pcBuf, pCurrent + 1, 6);	// ��һ��byte��ɾ����ǣ�
								pcBuf[6] = 0;


								//A��֤ȯ �ۺϻ�ɷ�ָ  �������������Ʒ�־ͼ�����һ��
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
						write_log(_T("��ȡ����DBFʧ��:%s"), sh_dbf);
					}
					//delete[] pcDbfRecordsSZ;
					//pcDbfRecordsSZ = nullptr;
				}
			}
			rFile.Close();
		}
		else{
			wsprintf(errInfo, _T("����DBF�ļ��򲻿�,����·��"));
			write_log(_T("����DBF�ļ��򲻿�:"), sz_dbf);
		}
	}
	catch (...)
	{
		write_log(_T("process sz dbf Exception"));
	}


}


void CDbfProcess::ProcessData(){

	write_log(_T("calculate start......"));


	//������ǰ����
	if (isPreQuoteFinishSh == false)
	{
		CalcPreQuoteSh();
	}

	if (isPreQuoteFinishSz == false)
	{
		CalcPreQuoteSz();
	}



	//�����������
	//������������ڴ�
	CalcMinuteQuote();


	//����������
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



//�������ݼ���
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
			//���й�Ʊ��һλ��0��־
			sprintf_s(symbol, "%06d", lNameCode);


			//���з�������д��map
			auto mq_iter_Sh = m_minuteQuoteMapSh.find(symbol);
			pMinuteQuote pMinQuote = nullptr;
			if (mq_iter_Sh == m_minuteQuoteMapSh.end())
			{
				//������鵽Map
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



				//��������л� �µķ��ӿ�ʼ��
				if (isMinChangeSh)
				{
					//��������дcsv�����
					//char cSql[200] = { 0 };
					//sprintf(cSql, "insert into tb_minute values(%d, %d,%s,%f, %f,%f,%f,%d, %d)", pMinQuote->minQuote.date, pMinQuote->minQuote.time, pMinQuote->code, pMinQuote->minQuote.openPrice, pMinQuote->minQuote.highPrice, pMinQuote->minQuote.lowPrice, pMinQuote->minQuote.closePrice, pMinQuote->minQuote.tradeVolume, pMinQuote->minQuote.tradeMoney);
					//execute_Sql(otext(cSql));
					//con.Commit();

					memcpy(&pMinQuote->preMinQuote, &pMinQuote->minQuote, sizeof(MinutedQuoteDetail));
					lMinuteQuoteTimeSh = pMinQuote->minQuote.time;

					//�µķ����Ѿ���ʼ����ʼ�����ӳ�ʼֵ
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
					//������߼�
					if (fNewPrice > pMinQuote->minQuote.highPrice)
					{
						pMinQuote->minQuote.highPrice = fNewPrice;
					}

					//������ͼ�
					if (fNewPrice < pMinQuote->minQuote.lowPrice)
					{
						pMinQuote->minQuote.lowPrice = fNewPrice;
					}

					//�������̼�
					pMinQuote->minQuote.closePrice = fNewPrice;

					//��������ڳɽ����ͳɽ���
					pMinQuote->minQuote.volume = (fVolumn - pMinQuote->minQuote.VolumeToday);
					pMinQuote->minQuote.amount = (fAmount - pMinQuote->minQuote.amountToday);
				}
			}
		}
		else if (cMarketFlag == MARKET_SZ)
		{
			//���й�Ʊ��һλ��1��־
			sprintf_s(symbol, "%06d", lNameCode);

			//���з�������д��map
			auto mq_iter_Sz = m_minuteQuoteMapSz.find(symbol);
			pMinuteQuote pMinQuote = nullptr;
			if (mq_iter_Sz == m_minuteQuoteMapSz.end())
			{
				//������鵽Map
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


				//��������л� �µķ��ӿ�ʼ��
				if (isMinChangeSz)
				{
					//��������дcsv�����
					//char cSql[200] = { 0 };
					//sprintf(cSql, "insert into tb_minute values(%d, %d,%s,%f, %f,%f,%f,%d, %d)", pMinQuote->minQuote.date, pMinQuote->minQuote.time, pMinQuote->code, pMinQuote->minQuote.openPrice, pMinQuote->minQuote.highPrice, pMinQuote->minQuote.lowPrice, pMinQuote->minQuote.closePrice, pMinQuote->minQuote.tradeVolume, pMinQuote->minQuote.tradeMoney);
					//execute_Sql(otext(cSql));
					//con.Commit();

					//�µķ����Ѿ���ʼ����ʼ�����ӳ�ʼֵ
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

					//������߼�
					if (fNewPrice > pMinQuote->minQuote.highPrice)
					{
						pMinQuote->minQuote.highPrice = fNewPrice;
					}

					//������ͼ�
					if (fNewPrice < pMinQuote->minQuote.lowPrice)
					{
						pMinQuote->minQuote.lowPrice = fNewPrice;
					}

					//�������̼�
					pMinQuote->minQuote.closePrice = fNewPrice;

					//��������ڳɽ����ͳɽ���
					pMinQuote->minQuote.volume = (fVolumn - pMinQuote->minQuote.VolumeToday);
					pMinQuote->minQuote.amount = (fAmount - pMinQuote->minQuote.amountToday);
				}
			}
		}
	}

	if (isMinChangeSh)
	{
		//���ñ�־λ
		isMinChangeSh = false;

		//����ʱ��ķ����������
		if ((lCurDbfMinuteSh >= 931 && lCurDbfMinuteSh <= 1130) || (lCurDbfMinuteSh >= 1301 && lCurDbfMinuteSh <= 1500))
		{
			write_log(_T("InsertToDbShMinuteQuote SH start Time:%d"), lCurDbfMinuteSh);
			InsertToDbShMinuteQuote();
			write_log(_T("InsertToDbShMinuteQuote SH end  Time:%d"), lCurDbfMinuteSh);
		}
	}
	if (isMinChangeSz)
	{
		//���ñ�־λ
		isMinChangeSz = false;

		//����ʱ��ķ����������
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




//�����ݼ��㻦��
void CDbfProcess::CalcDayQuoteSh()
{
	//����ϵͳʱ��15:15�Ժ���������
	if (OS_cur_hms < 151200)
		return;

	//�Ϻ�����ʱ�����15�㿪ʼ����
	if (lPreDbfTimeSh < 150000)
		return;


	//�Ϻ����������Ѵ�����ɲ���Ҫ�ڼ�������
	if (isDayQuoteFinishSh)
		return;

	//����������
	ClearMemoryDayData(MARKET_SH);

	//��ʼ����������
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
			//������鵽Map
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
		//�������������
		if (InsertToDbShDayQuote()){
			isDayQuoteFinishSh = true;
			//�����ڴ�����
			ClearMemoryDayData(MARKET_SH);
		}
	}

	m_gDbfRecordGuard.EndRead();
}



//�����ݼ�������
void CDbfProcess::CalcDayQuoteSz()
{

	//����ϵͳʱ��15:15�Ժ���������
	if (OS_cur_hms < 151200)
		return;

	//�����������15�㿪ʼ����������
	if (lPreDbfTimeSz < 150000)
		return;

	//�������������Ѵ�����ɲ���Ҫ�ڼ�������
	if (isDayQuoteFinishSz)
		return;

	//����������
	ClearMemoryDayData(MARKET_SZ);

	//��ʼ����������
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
			//������鵽Map
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
		//�������������
		if (InsertToDbSzDayQuote()){
			isDayQuoteFinishSz = true;
			//�����ڴ�����
			ClearMemoryDayData(MARKET_SZ);
		}
	}

	m_gDbfRecordGuard.EndRead();
}



//��ǰ���ݼ��㻦��
void CDbfProcess::CalcPreQuoteSh()
{
	//����9���Ժ�������ǰ���� ��Ҫ��ǰ�ռ�
	if (lPreDbfMinuteSh < 900)
		return;

	int nDbfRecodeSize = m_aSortDbfRecord.GetSize();
	if (nDbfRecodeSize == 0)
	{
		return;
	}

	//���г����Ѵ�����ɲ���Ҫ�ڼ�������
	if (isPreQuoteFinishSh)
		return;

	//������ǰ����
	ClearMemoryPreData(MARKET_SH);

	//��ʼ������ǰ����
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
			//�����ǰ���ݵ�Map
			ppreQuote = new PreQuote;
			memset(ppreQuote, 0, sizeof(PreQuote));
			strcpy_s(ppreQuote->code, strlen(symbol) + 1, symbol);
			ppreQuote->preQuote.date = curDateSh;
			ppreQuote->preQuote.prePrice = fPreClose;

			m_preQuoteMapSh[symbol] = ppreQuote;

		}
	}

	if (m_preQuoteMapSh.size() > 0){
		//������ǰ�������
		if (InsertToDbShPreQuote()){
			isPreQuoteFinishSh = true;
			ClearMemoryPreData(MARKET_SH);
		}
	}


	m_gDbfRecordGuard.EndRead();
}





//��ǰ���ݼ�������
void CDbfProcess::CalcPreQuoteSz()
{
	//����9���Ժ�������ǰ���� ��Ҫ��ǰ�ռ�
	if (lCurDbfMinuteSz < 900)
		return;


	int nDbfRecodeSize = m_aSortDbfRecord.GetSize();
	if (nDbfRecodeSize == 0)
	{
		return;
	}


	//���г����Ѵ�����ɲ���Ҫ�ڼ�������
	if (isPreQuoteFinishSz)
		return;

	//������ǰ����
	ClearMemoryPreData(MARKET_SZ);

	//��ʼ����������
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
			//������鵽Map
			ppreQuote = new PreQuote;
			memset(ppreQuote, 0, sizeof(PreQuote));
			strcpy_s(ppreQuote->code, strlen(symbol) + 1, symbol);
			ppreQuote->preQuote.date = curDateSz;
			ppreQuote->preQuote.prePrice = fPreClose;
			m_preQuoteMapSz[symbol] = ppreQuote;
		}
	}

	if (m_preQuoteMapSz.size() > 0){
		//������ǰ�������
		if (InsertToDbSzPreQuote()){
			isPreQuoteFinishSz = true;
			ClearMemoryPreData(MARKET_SZ);
		}
	}

	m_gDbfRecordGuard.EndRead();
}

//////////////////////////////////////////////////////////////////////////
//���з����������

bool CDbfProcess::InsertToDbShMinuteQuote(void)
{

	//��Ʊ
	std::vector<int>     tab_stock_date;
	std::vector<ostring> tab_stock_time;
	std::vector<ostring> tab_stock_code;
	std::vector<double>   tab_stock_open;
	std::vector<double>   tab_stock_high;
	std::vector<double>   tab_stock_low;
	std::vector<double>   tab_stock_close;
	std::vector<double>   tab_stock_volume;
	std::vector<double>   tab_stock_amount;

	//ָ��
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
		//��Ʊ
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
			//��ʽ��ʱ�䲻��4λǰ�油0
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

		//ָ��
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
			//��ʽ��ʱ�䲻��4λǰ�油0
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

	//��Ʊ
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
		write_log(_T("����������⻦�й�Ʊ����%d����¼,%d %hs"), st.GetAffectedRows(), tab_stock_date.at(0), tab_stock_time.at(0).c_str());
		wsprintf(strStaticMinSh, _T("���������ʱ��%hs"), tab_stock_time.at(0).c_str());




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
		write_log(_T("ִ�л��з�������SQL����%hs"), ex.what());
		//return false;
	}


	//ָ��
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
		write_log(_T("����������⻦��ָ������%d����¼,%d %hs"), st.GetAffectedRows(), tab_index_date.at(0), tab_index_time.at(0).c_str());




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
			write_log(_T("���з�������ʱ�����ʧ�� p_minute_time������ʱ�� %hs"), st.GetAffectedRows(), tab_stock_time.at(0).c_str());
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
		write_log(_T("ִ�л��з�������SQL����%hs"), ex.what());
		//return false;
	}

	return true;
}



//���з����������
bool CDbfProcess::InsertToDbSzMinuteQuote(void)
{

	//��Ʊ
	std::vector<int>     tab_stock_date;
	std::vector<ostring> tab_stock_time;
	std::vector<ostring> tab_stock_code;
	std::vector<double>   tab_stock_open;
	std::vector<double>   tab_stock_high;
	std::vector<double>   tab_stock_low;
	std::vector<double>   tab_stock_close;
	std::vector<double>   tab_stock_volume;
	std::vector<double>   tab_stock_amount;

	//ָ��
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
		//��Ʊ
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
			//��ʽ��ʱ�䲻��4λǰ�油0
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

		//ָ��
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
				<< it->second->preMinQuote.amount / 100 << "," //show2003.dbf ָ���ɽ�����  sjshq.dbf�ɽ�����λ�ǹ� ��������ͳһ����Ϊ��λ
				<< it->second->preMinQuote.VolumeToday << ","
				<< it->second->preMinQuote.amountToday << "\n";
			//////////////////////////////csv////////////////////////////////////////////

			tab_index_date.push_back(it->second->preMinQuote.date);
			//��ʽ��ʱ�䲻��4λǰ�油0
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
			tab_index_volume.push_back(it->second->preMinQuote.volume / 100);  //show2003.dbf ָ���ɽ�����  sjshq.dbf�ɽ�����λ�ǹ� ��������ͳһ����Ϊ��λ
			tab_index_amount.push_back(it->second->preMinQuote.amount);
			indexNumber++;
		}

	}

	/////////////////////////////csv/////////////////////////////////////////////
	sprintf_s(outputFileName, ".\\minute\\%d\\%s.sz", tab_stock_date.at(0), tab_stock_time.at(0).c_str());
	Write_csv_to_file(outputFileName, csvFileBuffer.str().c_str(), strlen(csvFileBuffer.str().c_str()));

	///////////////////////////csv///////////////////////////////////////////////


	//��Ʊ
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
		write_log(_T("��������������й�Ʊ����%d����¼,%d %hs"), st.GetAffectedRows(), tab_stock_date.at(0), tab_stock_time.at(0).c_str());
		wsprintf(strStaticMinSz, _T("���������ʱ��%hs"), tab_stock_time.at(0).c_str());





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
		write_log(_T("ִ�����з�������SQL����%hs"), ex.what());
		//return false;
	}


	//ָ��
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
		write_log(_T("���������������ָ������%d����¼,%d %hs"), st.GetAffectedRows(), tab_index_date.at(0), tab_index_time.at(0).c_str());


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
			write_log(_T("���з�������ʱ�����ʧ�� p_minute_time������ʱ�� %hs"), st.GetAffectedRows(), tab_index_time.at(0).c_str());
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
		write_log(_T("ִ�����з�������SQL����%hs"), ex.what());
		//return false;
	}

	return true;
}


//////////////////////////////////////////////////////////////////////////
//���������

bool CDbfProcess::InsertToDbShDayQuote(void)
{
	/*
	//������⣬Ч�ʷǳ���
	char cSql[200] = { 0 };
	for (auto it = begin(m_dayQuoteMapSh); it != end(m_dayQuoteMapSh); ++it){
	sprintf(cSql, "insert into tb_day values(%d,%s,%f, %f,%f,%f,%d, %d)", it->second->dayQuote.date, it->second->code, it->second->dayQuote.openPrice, it->second->dayQuote.highPrice, it->second->dayQuote.lowPrice, it->second->dayQuote.closePrice, it->second->dayQuote.VolumeToday, it->second->dayQuote.MoneyToday);

	*/

	//Ϊ������������ܣ��������
	//��Ʊ
	std::vector<int>     tab_stock_date;
	std::vector<ostring> tab_stock_code;
	std::vector<double>   tab_stock_open;
	std::vector<double>   tab_stock_high;
	std::vector<double>   tab_stock_low;
	std::vector<double>   tab_stock_close;
	std::vector<double>   tab_stock_volume;
	std::vector<double>   tab_stock_amount;

	//ָ��
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
		//��Ʊ
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

		//ָ��
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
			tab_index_volume.push_back(it->second->dayQuote.VolumeToday * 100); //show2003.dbf ָ���ɽ�����  sjshq.dbf�ɽ�����λ�ǹ�  ������ͳһ�ɹ�
			tab_index_amount.push_back(it->second->dayQuote.amountToday);
			indexNumber++;
		}

	}


	//��Ʊ
	try
	{
		Statement st(con);

		//�������
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
		write_log(_T("��������⻦�й�Ʊ����%d����¼  ����%d"), st.GetAffectedRows(), tab_stock_date.at(0));
		wsprintf(StaticCloseSh, _T("%d �����%d����¼"), tab_stock_date.at(0), st.GetAffectedRows());
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
		write_log(_T("ִ�л���������SQL����%hs"), ex.what());
		//return false;
	}


	//ָ��
	try
	{
		Statement st(con);

		//�������
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
		write_log(_T("��������⻦��ָ������%d����¼ ����%d"), st.GetAffectedRows(), tab_index_date.at(0));

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
		write_log(_T("ִ�л���������SQL����%hs"), ex.what());
		//return false;
	}

	return true;
}



bool CDbfProcess::InsertToDbSzDayQuote(void)
{

	/*
	//������⣬Ч�ʷǳ���
	char cSql[200] = { 0 };
	for (auto it = begin(m_dayQuoteMapSh); it != end(m_dayQuoteMapSh); ++it){
	sprintf(cSql, "insert into tb_day values(%d,%s,%f, %f,%f,%f,%d, %d)", it->second->dayQuote.date, it->second->code, it->second->dayQuote.openPrice, it->second->dayQuote.highPrice, it->second->dayQuote.lowPrice, it->second->dayQuote.closePrice, it->second->dayQuote.VolumeToday, it->second->dayQuote.MoneyToday);

	*/

	//Ϊ������������ܣ��������
	//��Ʊ
	std::vector<int>     tab_stock_date;
	std::vector<ostring> tab_stock_code;
	std::vector<double>   tab_stock_open;
	std::vector<double>   tab_stock_high;
	std::vector<double>   tab_stock_low;
	std::vector<double>   tab_stock_close;
	std::vector<double>   tab_stock_volume;
	std::vector<double>   tab_stock_amount;

	//ָ��
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
		//��Ʊ
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

		//ָ��
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


	//��Ʊ
	try
	{
		Statement st(con);
		//�������
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
		write_log(_T("������������й�Ʊ����%d����¼ ����%d"), st.GetAffectedRows(), tab_stock_date.at(0));
		wsprintf(StaticCloseSz, _T("%d �����%d����¼"), tab_stock_date.at(0), st.GetAffectedRows());
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
		write_log(_T("ִ������������SQL����%hs"), ex.what());
		return false;
	}


	//ָ��
	try
	{
		Statement st(con);
		//�������
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
		write_log(_T("�������������ָ������%d����¼ ����%d"), st.GetAffectedRows(), tab_index_date.at(0));
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
		write_log(_T("ִ������������SQL����%hs"), ex.what());
		//return false;
	}

	return true;
}


//������ǰ�����ڴ�����
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


//������������ڴ�����
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


//�����������ڴ�����
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
//��ǰ�������

bool CDbfProcess::InsertToDbShPreQuote(void)
{
	//��Ʊ
	std::vector<int>     tab_stock_date;
	std::vector<ostring> tab_stock_code;
	std::vector<double>   tab_stock_preClose;



	int stockNumber = 0;
	int indexNumber = 0;
	for (auto it = begin(m_preQuoteMapSh); it != end(m_preQuoteMapSh); ++it)
	{
		//��Ʊ
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


	//��Ʊ
	try
	{
		Statement st(con);

		//�������
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
		write_log(_T("��ǰ������⻦�й�Ʊ����%d����¼ %d"), st.GetAffectedRows(), tab_stock_date.at(0));
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
		write_log(_T("ִ�л�����ǰ����SQL����%hs"), ex.what());
		//return false;
	}



	return true;
}



bool CDbfProcess::InsertToDbSzPreQuote(void)
{

	//��Ʊ
	std::vector<int>     tab_stock_date;
	std::vector<ostring> tab_stock_code;
	std::vector<double>   tab_stock_preClose;

	int stockNumber = 0;
	int indexNumber = 0;
	for (auto it = begin(m_preQuoteMapSz); it != end(m_preQuoteMapSz); ++it)
	{
		//��Ʊ
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


	//��Ʊ
	try
	{
		Statement st(con);

		//�������
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
		write_log(_T("��ǰ����������й�Ʊ����%d����¼ %d"), st.GetAffectedRows(), tab_stock_date.at(0));
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
		write_log(_T("ִ��������ǰ����SQL����%hs"), ex.what());
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
