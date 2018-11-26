
// QuoteProcessDlg.cpp : implementation file
//






#include "stdafx.h"
#include "QuoteProcess.h"
#include "QuoteProcessDlg.h"
#include "afxdialogex.h"
#include "DbfProcess.h"



#include "minicsv.h"
#include <iostream>

#pragma warning(disable:4996)

struct QuoteDayClose
{
	QuoteDayClose() : code(""), price(0.0f) {}
	QuoteDayClose(std::string code_, float price_) : code(code_), price(price_) {}
	std::string code;
	float price;
};

#ifdef _DEBUG
#define new DEBUG_NEW
#endif






// CAboutDlg dialog used for App About

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

	// Dialog Data
	enum { IDD = IDD_ABOUTBOX };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	// Implementation
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CQuoteProcessDlg dialog



CQuoteProcessDlg::CQuoteProcessDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CQuoteProcessDlg::IDD, pParent)
	, m_strShDbfPathMaster(_T(""))
	, m_strSzDbfPathMaster(_T(""))
	, m_strSysTime(_T(""))
	, m_strShDbfTime(_T(""))
	, m_strSzDbfTime(_T(""))
	, m_strShDbfPathSlave(_T(""))
	, m_strSzDbfPathSlave(_T(""))
	, m_strShDbfTimeSlave(_T(""))
	, m_strSzDbfTimeSlave(_T(""))
	, m_strDbStatus(_T(""))
	, m_strStaticMin(_T(""))
	, m_strStaticClose(_T(""))
	, m_strStaticMinSz(_T(""))
	, m_strStaticCloseSz(_T(""))
	, m_strErrorInfo(_T(""))
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	::InitializeCriticalSection(&__cs_logfile1__);


}

void CQuoteProcessDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EDIT1, m_strShDbfPathMaster);
	DDX_Text(pDX, IDC_EDIT2, m_strSzDbfPathMaster);
	DDX_Text(pDX, IDC_STATIC_SYSTEM_TIME, m_strSysTime);
	DDX_Text(pDX, IDC_STATIC_SH_DBF_TIME, m_strShDbfTime);
	DDX_Text(pDX, IDC_STATIC_SZ_DBF_TIME, m_strSzDbfTime);
	DDX_Text(pDX, IDC_EDIT3, m_strShDbfPathSlave);
	DDX_Text(pDX, IDC_EDIT4, m_strSzDbfPathSlave);
	DDX_Text(pDX, IDC_STATIC_SH_DBF_TIME_SLAVE, m_strShDbfTimeSlave);
	DDX_Text(pDX, IDC_STATIC_SZ_DBF_TIME_SLAVE, m_strSzDbfTimeSlave);
	DDX_Text(pDX, IDC_STATIC_DB, m_strDbStatus);
	DDX_Text(pDX, IDC_STATIC_MIN, m_strStaticMin);
	DDX_Text(pDX, IDC_STATIC_CLOSE, m_strStaticClose);
	DDX_Text(pDX, IDC_STATIC_MIN_SZ, m_strStaticMinSz);
	DDX_Text(pDX, IDC_STATIC_CLOSE_SZ, m_strStaticCloseSz);
	DDX_Text(pDX, IDC_STATIC_ERR_INFO, m_strErrorInfo);
}

BEGIN_MESSAGE_MAP(CQuoteProcessDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON_SH_DBF, &CQuoteProcessDlg::OnBnClickedSetShPath)
	ON_BN_CLICKED(IDC_BUTTON_SZ_DBF, &CQuoteProcessDlg::OnBnClickedSetSzPath)
	ON_WM_TIMER()
	ON_BN_CLICKED(IDC_BUTTON_SERVICE_START, &CQuoteProcessDlg::OnBnClickedButtonServiceStart)
	ON_BN_CLICKED(IDC_BUTTON_SERVICE_STOP, &CQuoteProcessDlg::OnBnClickedButtonServiceStop)
	ON_BN_CLICKED(IDCANCEL, &CQuoteProcessDlg::OnBnClickedCancel)
	ON_BN_CLICKED(IDC_BUTTON_SH_DBF2, &CQuoteProcessDlg::OnBnClickedButtonShDbf2)
	ON_BN_CLICKED(IDC_BUTTON_SZ_DBF2, &CQuoteProcessDlg::OnBnClickedButtonSzDbf2)
END_MESSAGE_MAP()


// CQuoteProcessDlg message handlers

BOOL CQuoteProcessDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	//ShowWindow(SW_MINIMIZE);

	// TODO: Add extra initialization here

	GetModuleFileName(GetModuleHandle(NULL), __main_home__, MAX_PATH);

	for (int l = lstrlen(__main_home__); l > 0 && __main_home__[l] != _T('\\'); __main_home__[l--] = 0);
	wsprintf(__iniSysConfig__, TEXT("%sconfig\\%s.conf"), __main_home__, TEXT("QuoteProcess"));

	//得到非交易日表
	loadNontradingDay();
	m_isTradeDate();

	m_pDbfProcess = new CDbfProcess;

	LoadSet();




	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CQuoteProcessDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CQuoteProcessDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CQuoteProcessDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



void CQuoteProcessDlg::OnBnClickedSetShPath()
{
	// TODO: Add your control notification handler code here

	if (GetDlgItem(IDC_BUTTON_SERVICE_START)->IsWindowEnabled() == FALSE)
	{
		MessageBox(_T("先停止服务"), NULL, MB_OK);
		return;
	}

	UpdateData();
	CString filter = L"DBF(*.dbf)|*.dbf|所有文件 (*.*)|*.*||";
	CFileDialog FileDialog(TRUE, m_strShDbfPathMaster, NULL, OFN_FILEMUSTEXIST | OFN_HIDEREADONLY, filter, this);
	if (FileDialog.DoModal() == IDOK)
	{
		m_strShDbfPathMaster = FileDialog.GetPathName();
		UpdateData(FALSE);
		SaveSet();
	}
}


void CQuoteProcessDlg::OnBnClickedSetSzPath()
{
	// TODO: Add your control notification handler code here

	if (GetDlgItem(IDC_BUTTON_SERVICE_START)->IsWindowEnabled() == FALSE)
	{
		MessageBox(_T("先停止服务"), NULL, MB_OK);
		return;
	}

	UpdateData();
	CString filter = L"DBF(*.dbf)|*.dbf|所有文件 (*.*)|*.*||";
	CFileDialog FileDialog(TRUE, m_strSzDbfPathMaster, NULL, OFN_FILEMUSTEXIST | OFN_HIDEREADONLY, filter, this);
	if (FileDialog.DoModal() == IDOK)
	{
		m_strSzDbfPathMaster = FileDialog.GetPathName();
		UpdateData(FALSE);
		SaveSet();
	}
}





void CQuoteProcessDlg::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: Add your message handler code here and/or call default

	if (nIDEvent == 1) //1秒钟Timer
	{
		SYSTEMTIME rTime;
		GetLocalTime(&rTime);

		InterlockedExchange(&OS_cur_date, rTime.wYear * 10000L + rTime.wMonth * 100 + rTime.wDay);
		InterlockedExchange(&OS_cur_hms, rTime.wHour * 10000L + rTime.wMinute * 100 + rTime.wSecond);
		InterlockedExchange(&OS_cur_hms_mis, rTime.wHour * 10000000L + rTime.wMinute * 100000L + rTime.wSecond * 1000 + rTime.wMilliseconds);

		//write_log(_T("OS_cur_date %d, OS_cur_hms %d, OS_cur_hms_mis %d"), OS_cur_date, OS_cur_hms, OS_cur_hms_mis);

		m_strSysTime.Format(_T("%04d-%02d-%02d %02d:%02d:%02d"),
			OS_cur_date / 10000, (OS_cur_date % 10000) / 100, (OS_cur_date % 10000) % 100,
			(OS_cur_hms_mis / 1000) / 10000, ((OS_cur_hms_mis / 1000) % 10000) / 100, ((OS_cur_hms_mis / 1000) % 10000) % 100);


		m_strShDbfTime.Format(_T("主：%04d-%02d-%02d %02d:%02d:%02d"),
			shMasterDate / 10000, (shMasterDate % 10000) / 100, (shMasterDate % 10000) % 100,
			(shMasterTime / 10000), ((shMasterTime % 10000) / 100), shMasterTime % 100);


		m_strSzDbfTime.Format(_T("主：%04d-%02d-%02d %02d:%02d:%02d"),
			szMasterDate / 10000, (szMasterDate % 10000) / 100, (szMasterDate % 10000) % 100,
			(szMasterTime / 10000), ((szMasterTime % 10000) / 100), szMasterTime % 100);



		m_strShDbfTimeSlave.Format(_T("从：%04d-%02d-%02d %02d:%02d:%02d"),
			shSlaveDate / 10000, (shSlaveDate % 10000) / 100, (shSlaveDate % 10000) % 100,
			(shSlaveTime / 10000), ((shSlaveTime % 10000) / 100), shSlaveTime % 100);


		m_strSzDbfTimeSlave.Format(_T("从：%04d-%02d-%02d %02d:%02d:%02d"),
			szSlaveDate / 10000, (szSlaveDate % 10000) / 100, (szSlaveDate % 10000) % 100,
			(szSlaveTime / 10000), ((szSlaveTime % 10000) / 100), szSlaveTime % 100);

		m_strDbStatus.Format(_T("%s"), dbStatus); 
		m_strErrorInfo.Format(_T("%s"), errInfo);
		m_strStaticMin.Format(_T("%s"), strStaticMinSh);
		m_strStaticClose.Format(_T("%s"), StaticCloseSh);
		m_strStaticMinSz.Format(_T("%s"), strStaticMinSz);
		m_strStaticCloseSz.Format(_T("%s"), StaticCloseSz);

		if (OS_cur_hms%5==0)
		{
			m_pDbfProcess->dbReconnect();
		}
		
		
		UpdateData(FALSE);


	}


	//初始化数据
	if (nIDEvent == 2)  //1分钟Timer
	{

		//更新是否交易日状态
		m_isTradeDate();

		//每天8点变量初始化盘前操作
		if (8 == (OS_cur_hms / 10000) && !isIniFinish)
		{
			isPreQuoteFinishSh = false;
			isPreQuoteFinishSz = false;
			isDayQuoteFinishSh = false; 
			isDayQuoteFinishSz = false;

			m_pDbfProcess->ClearMemoryMinData(MARKET_SH);
			m_pDbfProcess->ClearMemoryMinData(MARKET_SZ);
			m_pDbfProcess->init();

			write_log(_T("早盘8点初始化完成"));
			isIniFinish = true;
			
		}

		//每天16点开始收盘初始化，为了第二天盘前做准备
		if (16 == (OS_cur_hms / 10000) && isIniFinish)
		{
			write_log(_T("收盘初始化完成"));
			isIniFinish = false;
			
		}

		//清理前一天分钟数据缓存

		//清理前一天日数据缓存

	}
	CDialogEx::OnTimer(nIDEvent);
}


void CQuoteProcessDlg::m_isTradeDate()
{
	//判断是否为交易日

	char tmp[10] = { 0 };
	sprintf(tmp, "%d", OS_cur_date);
	string key(tmp);

	map<string, string>::const_iterator map_itr_nontrading_day = __nontrading_day_map__.find(tmp);
	if (map_itr_nontrading_day == __nontrading_day_map__.end())
	{
		isTradeDate = true;
	}
	else
	{
		wsprintf(errInfo, _T("非交易日-不处理"));
		isTradeDate = false;
	}
}


void CQuoteProcessDlg::LoadSet()
{
	init.LoadIniString();
	m_strShDbfPathMaster = init.GetIniString(_T("SH_DBF_PATH_MASTER"), _T(""));
	m_strSzDbfPathMaster = init.GetIniString(_T("SZ_DBF_PATH_MASTER"), _T(""));

	m_strShDbfPathSlave = init.GetIniString(_T("SH_DBF_PATH_SLAVE"), _T(""));
	m_strSzDbfPathSlave = init.GetIniString(_T("SZ_DBF_PATH_SLAVE"), _T(""));


	m_nDBFDelay = init.GetIniInt(_T("DBF_DELAY"), 2000);

	m_strDbUser = init.GetIniString(_T("DB_USER"), _T(""));
	m_strDbPwd = init.GetIniString(_T("DB_PWD"), _T(""));
	m_strDbName = init.GetIniString(_T("DB_NAME"), _T(""));

	UpdateData(FALSE);
}

void CQuoteProcessDlg::SaveSet()
{
	init.WriteIniString(_T("SH_DBF_PATH_MASTER"), m_strShDbfPathMaster);
	init.WriteIniString(_T("SZ_DBF_PATH_MASTER"), m_strSzDbfPathMaster);

	init.WriteIniString(_T("SH_DBF_PATH_SLAVE"), m_strShDbfPathSlave);
	init.WriteIniString(_T("SZ_DBF_PATH_SLAVE"), m_strSzDbfPathSlave);

	init.WriteIniInt(_T("DBF_DELAY"), m_nDBFDelay);
	init.SaveIniString();
}




void CQuoteProcessDlg::AddLog(CString strLog)
{
	EnterCriticalSection(&m_rCritical);

	m_aLog.Add(strLog);

	LeaveCriticalSection(&m_rCritical);
}




INT32 himmss_to_timestamp(INT32 himmss)
{
	INT32 timestamp = himmss / 10000 * 3600 + (himmss % 10000) / 100 * 60 + (himmss % 10000) % 100;
	return timestamp;
}


//dbf处理线程
UINT QuoteProcessThread(LPVOID pParam)
{


 	CQuoteProcessDlg* dlg = (CQuoteProcessDlg*)pParam;
	dlg->m_pDbfProcess->InitDB();
	
	CString m_strPreShDbfPath;
	CString m_strPreSzDbfPath;

	CString m_strShDbfPath;
	CString m_strSzDbfPath;


	//程序启动的时候默认master文件
	m_strShDbfPath = dlg->m_strShDbfPathMaster;
	m_strSzDbfPath = dlg->m_strSzDbfPathMaster;
	//获取每种dbf文件的数据头大小
	dlg->m_pDbfProcess->ReadShDbfHead(dlg->m_strShDbfPathMaster, dlg->m_pDbfProcess->m_sSHHeadLength);
	dlg->m_pDbfProcess->ReadSzDbfHead(dlg->m_strSzDbfPathMaster, dlg->m_pDbfProcess->m_sSZHeadLength);




	while (TRUE)
	{

		//write_log(_T("QuoteProcessThread  OS_cur_date %d, OS_cur_hms %02d:%02d:%02d, OS_cur_hms_mis %02d:%02d:%02d.%d"), OS_cur_date, OS_cur_hms / 10000, (OS_cur_hms / 100) % 100, OS_cur_hms % 100, OS_cur_hms_mis / 10000000, (OS_cur_hms_mis / 100000) % 100, (OS_cur_hms_mis / 1000) % 100,OS_cur_hms_mis % 1000);


		if (WaitForSingleObject(dlg->m_pDbfProcess->m_hEventKillThread, dlg->m_pDbfProcess->m_nDBFDelay) == WAIT_OBJECT_0)
			break;
		
		//过滤非交易日，非交易日不处理，直接返回
		if (!isTradeDate)
		{
			if (OS_cur_hms % 10 == 0)
				write_log(_T("非交易日,不处理行情......"));
			continue;
		}

		//处理时间限制，9点20前不处理，直接返回
		int time = 92000;
		if (OS_cur_hms < time)
		{
			if (OS_cur_hms % 10 == 0)
				write_log(_T("操作系统时间<%d(hms)不进行业务处理."), time);

			continue;
		}

		if (OS_cur_hms%10==0)
		{

			//读两路dbf文件，为了取最新的一个
			//主dbf最新时间
			if (dlg->m_strShDbfPathMaster.GetLength() >0)
			{
				dlg->m_pDbfProcess->ReadDbfTimeSh(dlg->m_strShDbfPathMaster, &shMasterDate, &shMasterTime);
				//write_log(_T("QuoteTimeSH = %d"), shMasterTime);
			}
			if (dlg->m_strSzDbfPathMaster.GetLength() > 0)
			{
				dlg->m_pDbfProcess->ReadDbfTimeSz(dlg->m_strSzDbfPathMaster, &szMasterDate, &szMasterTime);
			}


			//辅dbf
			if (dlg->m_strShDbfPathSlave.GetLength() > 0)
			{
				dlg->m_pDbfProcess->ReadDbfTimeSh(dlg->m_strShDbfPathSlave, &shSlaveDate, &shSlaveTime);
			}

			if (dlg->m_strSzDbfPathSlave.GetLength() > 0)
			{
				dlg->m_pDbfProcess->ReadDbfTimeSz(dlg->m_strSzDbfPathSlave, &szSlaveDate, &szSlaveTime);
			}




			//取最新日期的上海dbf
			if (shMasterDate > shSlaveDate)
			{
				m_strShDbfPath = dlg->m_strShDbfPathMaster;
			}
			else if (shMasterDate < shSlaveDate){
				m_strShDbfPath = dlg->m_strShDbfPathSlave;
			}
			else if (shMasterDate == shSlaveDate){
				if (himmss_to_timestamp(shSlaveTime) - himmss_to_timestamp(shMasterTime) >= 50)
				{
					m_strShDbfPath = dlg->m_strShDbfPathSlave;
				}
				else
				{
					m_strShDbfPath = dlg->m_strShDbfPathMaster;
				}
			}

			//取最新日期的深圳dbf
			if (szMasterDate > szSlaveDate)
			{
				m_strSzDbfPath = dlg->m_strSzDbfPathMaster;
			}
			else if (szMasterDate < szSlaveDate){
				m_strSzDbfPath = dlg->m_strSzDbfPathSlave;
			}
			else if (szMasterDate == szSlaveDate){
				if (himmss_to_timestamp(szSlaveTime) - himmss_to_timestamp(szMasterTime) >= 50)
				{
					m_strSzDbfPath = dlg->m_strSzDbfPathSlave;
				}
				else
				{
					m_strSzDbfPath = dlg->m_strSzDbfPathMaster;
				}
			}


			//不同的行情源，记录行数可能不一样，需要重新获取

			if (m_strPreShDbfPath != m_strShDbfPath){
				dlg->m_pDbfProcess->ReadShDbfHead(m_strShDbfPath, dlg->m_pDbfProcess->m_sSHHeadLength);
				m_strPreShDbfPath = m_strShDbfPath;
			}


			if (m_strPreSzDbfPath != m_strSzDbfPath){
				dlg->m_pDbfProcess->ReadSzDbfHead(m_strSzDbfPath, dlg->m_pDbfProcess->m_sSZHeadLength);
				m_strPreSzDbfPath = m_strSzDbfPath;
			}

		}


		write_log(_T("ReadDbfRecords start OS_cur_hms_mis %02d:%02d:%02d.%d"), OS_cur_hms_mis / 10000000, (OS_cur_hms_mis / 100000) % 100, (OS_cur_hms_mis / 1000) % 100, OS_cur_hms_mis % 1000);
		dlg->m_pDbfProcess->ReadMarketDbf(m_strShDbfPath, m_strSzDbfPath);
		write_log(_T("ReadDbfRecords end   OS_cur_hms_mis %02d:%02d:%02d.%d"), OS_cur_hms_mis / 10000000, (OS_cur_hms_mis / 100000) % 100, (OS_cur_hms_mis / 1000) % 100, OS_cur_hms_mis % 1000);
		dlg->m_pDbfProcess->ProcessData();

	}

	//pDbfThread->m_pDlg->AddLog("DBFThread结束.");
	SetEvent(dlg->m_pDbfProcess->m_hEventThreadKilled);

	return 0;
}



void CQuoteProcessDlg::StartDBFThread()
{

	//StopDBFThread();

	
	if (m_pDbfProcess)
	{
		//m_pDBFThread->m_pDlg = this;
		m_pDbfProcess->m_nDBFDelay = m_nDBFDelay;

		AfxBeginThread(QuoteProcessThread, this, THREAD_PRIORITY_BELOW_NORMAL);
	}
	else
		MessageBox(_T("内存异常错(new CDBFThread)!"), _T("警告"), MB_OK | MB_ICONERROR);
}

void CQuoteProcessDlg::StopDBFThread()
{
	if (m_pDbfProcess)
	{
        //跳出dbf分析循环
		SetEvent(m_pDbfProcess->m_hEventKillThread);
		WaitForSingleObject(m_pDbfProcess->m_hEventThreadKilled, INFINITE);
		//delete m_pDbfProcess;
		//m_pDbfProcess = nullptr;
	}
}


void CQuoteProcessDlg::OnBnClickedButtonServiceStart()
{
	// TODO: Add your control notification handler code here


	//启动定时器
	SetTimer(1, 1000, NULL);        //一秒
	SetTimer(2, 1000 * 60, NULL);   //一分

	//保存dbf路径
	UpdateData(TRUE);
	SaveSet();


	StartDBFThread();
	GetDlgItem(IDC_BUTTON_SERVICE_START)->EnableWindow(FALSE);
	GetDlgItem(IDC_BUTTON_SERVICE_STOP)->EnableWindow(TRUE);
}


void CQuoteProcessDlg::OnBnClickedButtonServiceStop()
{
	// TODO: Add your control notification handler code here
	KillTimer(1);
	KillTimer(2);


	StopDBFThread();
	GetDlgItem(IDC_BUTTON_SERVICE_START)->EnableWindow(TRUE);
	GetDlgItem(IDC_BUTTON_SERVICE_STOP)->EnableWindow(FALSE);
}


void CQuoteProcessDlg::OnBnClickedCancel()
{
	// TODO: Add your control notification handler code here
	CDialogEx::OnCancel();
}


void CQuoteProcessDlg::OnBnClickedButtonShDbf2()
{
	// TODO: Add your control notification handler code here

	if (GetDlgItem(IDC_BUTTON_SERVICE_START)->IsWindowEnabled() == FALSE)
	{
		MessageBox(_T("先停止服务"), NULL, MB_OK);
		return;
	}

	UpdateData();
	CString filter = L"DBF(*.dbf)|*.dbf|所有文件 (*.*)|*.*||";
	CFileDialog FileDialog(TRUE, m_strShDbfPathSlave, NULL, OFN_FILEMUSTEXIST | OFN_HIDEREADONLY, filter, this);
	if (FileDialog.DoModal() == IDOK)
	{
		m_strShDbfPathSlave = FileDialog.GetPathName();
		UpdateData(FALSE);
		SaveSet();
	}
}


void CQuoteProcessDlg::OnBnClickedButtonSzDbf2()
{
	// TODO: Add your control notification handler code here

	if (GetDlgItem(IDC_BUTTON_SERVICE_START)->IsWindowEnabled() == FALSE)
	{
		MessageBox(_T("先停止服务"), NULL, MB_OK);
		return;
	}

	UpdateData();
	CString filter = L"DBF(*.dbf)|*.dbf|所有文件 (*.*)|*.*||";
	CFileDialog FileDialog(TRUE, m_strSzDbfPathSlave, NULL, OFN_FILEMUSTEXIST | OFN_HIDEREADONLY, filter, this);
	if (FileDialog.DoModal() == IDOK)
	{
		m_strSzDbfPathSlave = FileDialog.GetPathName();
		UpdateData(FALSE);
		SaveSet();
	}
}
