
// QuoteProcessDlg.h : header file
//
#pragma once
#include "InitFile.h"
#include "DbfProcess.h"

// CQuoteProcessDlg dialog
class CQuoteProcessDlg : public CDialogEx
{
// Construction
public:
	CQuoteProcessDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	enum { IDD = IDD_QUOTEPROCESS_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedSetShPath();

	CString m_strShDbfPathMaster;
	CString m_strSzDbfPathMaster;
	int		m_nDBFDelay; //


	afx_msg void OnBnClickedSetSzPath();
	afx_msg void OnTimer(UINT_PTR nIDEvent);


	

	CString m_strSysTime;
	CString m_strShDbfTime;
	CString m_strSzDbfTime;

	InitFile init;
	void LoadSet();
	void SaveSet();

	void m_isTradeDate();



	CRITICAL_SECTION m_rCritical;
	CStringArray m_aLog;
	void AddLog(CString);
	void StartDBFThread();
	void StopDBFThread();
	CDbfProcess* m_pDbfProcess;
	afx_msg void OnBnClickedButtonServiceStart();
	afx_msg void OnBnClickedButtonServiceStop();
	afx_msg void OnBnClickedCancel();
	CString m_strShDbfPathSlave;
	CString m_strSzDbfPathSlave;

	afx_msg void OnBnClickedButtonShDbf2();
	afx_msg void OnBnClickedButtonSzDbf2();
	CString m_strShDbfTimeSlave;
	CString m_strSzDbfTimeSlave;

	CString m_strDbStatus;
	CString m_strStaticMin;
	CString m_strStaticClose;
	CString m_strStaticMinSz;
	CString m_strStaticCloseSz;
	CString m_strErrorInfo;
};
