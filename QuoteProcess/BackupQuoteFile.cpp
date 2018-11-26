// BackupQuoteFile.cpp : implementation file
//

#include "stdafx.h"
#include "QuoteProcess.h"
#include "BackupQuoteFile.h"
#include "afxdialogex.h"


// CBackupQuoteFile dialog

IMPLEMENT_DYNAMIC(CBackupQuoteFile, CDialogEx)

CBackupQuoteFile::CBackupQuoteFile(CWnd* pParent /*=NULL*/)
	: CDialogEx(CBackupQuoteFile::IDD, pParent)
	, m_strShDbf(_T(""))
	, m_strSzDbf(_T(""))
	, m_strShDbfDest(_T(""))
	, m_strSzDbfDest(_T(""))
{

}

CBackupQuoteFile::~CBackupQuoteFile()
{
}

void CBackupQuoteFile::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EDIT_SH_SOURCE, m_strShDbf);
	DDX_Text(pDX, IDC_EDIT_SZ_SOURCE, m_strSzDbf);
	DDX_Text(pDX, IDC_EDIT_SH_BACKUP, m_strShDbfDest);
	DDX_Text(pDX, IDC_EDIT_SZ_BACKUP, m_strSzDbfDest);
}


BEGIN_MESSAGE_MAP(CBackupQuoteFile, CDialogEx)
END_MESSAGE_MAP()


// CBackupQuoteFile message handlers


//dbf处理线程
UINT QuoteProcessThread2(LPVOID pParam)
{

	CBackupQuoteFile* dlg = (CBackupQuoteFile*)pParam;
	dlg->m_pDbfProcess->ReadShDbfHead(dlg->m_strShDbf);
	dlg->m_pDbfProcess->ReadSzDbfHead(dlg->m_strSzDbf);

	while (TRUE)
	{
		dlg->m_pDbfProcess->ReadMarketDbf(dlg->m_strShDbf, dlg->m_strSzDbf);


		if (WaitForSingleObject(dlg->m_pDbfProcess->m_hEventKillThread, dlg->m_pDbfProcess->m_nDBFDelay) == WAIT_OBJECT_0)
			break;
	}

	//pDbfThread->m_pDlg->AddLog("DBFThread结束.");
	SetEvent(dlg->m_pDbfProcess->m_hEventThreadKilled);

	return 0;
}