#pragma once

#include "InitFile.h"
#include "DbfProcess.h"

// CBackupQuoteFile dialog

class CBackupQuoteFile : public CDialogEx
{
	DECLARE_DYNAMIC(CBackupQuoteFile)

public:
	CBackupQuoteFile(CWnd* pParent = NULL);   // standard constructor
	virtual ~CBackupQuoteFile();

// Dialog Data
	enum { IDD = IDD_DIALOG_COPYFILE };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()

public:
	CDbfProcess* m_pDbfProcess;
	CString m_strShDbf;
	CString m_strSzDbf;
	CString m_strShDbfDest;
	CString m_strSzDbfDest;
};
