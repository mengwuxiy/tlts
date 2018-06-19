
// tlts.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "afxwinappex.h"
#include "afxdialogex.h"
#include "tlts.h"
#include "MainFrm.h"

#include "tltsDoc.h"
#include "tltsView.h"

#include <afxsock.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CtltsApp

BEGIN_MESSAGE_MAP(CtltsApp, CWinAppEx)
	ON_COMMAND(ID_APP_ABOUT, &CtltsApp::OnAppAbout)
	// Standard file based document commands
	ON_COMMAND(ID_FILE_NEW, &CWinAppEx::OnFileNew)
	ON_COMMAND(ID_FILE_OPEN, &CWinAppEx::OnFileOpen)
END_MESSAGE_MAP()


// CtltsApp construction

CtltsApp::CtltsApp()
{
	m_bHiColorIcons = TRUE;

	// TODO: replace application ID string below with unique ID string; recommended
	// format for string is CompanyName.ProductName.SubProduct.VersionInformation
	SetAppID(_T("tlts.AppID.NoVersion"));

	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}

// The one and only CtltsApp object

CtltsApp theApp;


// CtltsApp initialization

BOOL CtltsApp::InitInstance()
{
	// InitCommonControlsEx() is required on Windows XP if an application
	// manifest specifies use of ComCtl32.dll version 6 or later to enable
	// visual styles.  Otherwise, any window creation will fail.
	INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof(InitCtrls);
	// Set this to include all the common control classes you want to use
	// in your application.
	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&InitCtrls);

	CWinAppEx::InitInstance();

	if (!AfxSocketInit())
	{
		AfxMessageBox("Netword Fail");
		return FALSE;
	}

	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData)) //µ÷ÓÃWindows Sockets DLL
	{
		WSACleanup();
	}

	// Initialize OLE libraries
	if (!AfxOleInit())
	{
		AfxMessageBox(IDP_OLE_INIT_FAILED);
		return FALSE;
	}

	TCHAR szPath[MAX_PATH];
	GetModuleFileName(NULL, szPath, MAX_PATH);
	g_strModuleFolder.Format("%s", szPath);
	g_strModuleFolder = g_strModuleFolder.Left(g_strModuleFolder.ReverseFind('\\'));	


	CString strDir = g_strModuleFolder + "\\mysql.cfg";
	CStdioFile file;
	file.Open(strDir, CFile::modeRead);
	CString strServer, strUser, strPwd, strDB;
	file.ReadString(strServer);
	file.ReadString(strUser);
	file.ReadString(strPwd);
	file.ReadString(strDB);
	//CString strCmd; 
	//CString swMode;
	//file.ReadString(strCmd);
	//file.ReadString(swMode);
	file.Close();
	if (theApp.sql.IsConnected())
	{
		theApp.sql.CloseConn();
	}
	if (theApp.sql.ConnectDatabase(strServer, strUser, strPwd, strDB) == false)
	{		
		//CString strError;
		AfxMessageBox(theApp.sql.GetErrorMsg());
		return FALSE;
	}
	//ShellExecute(NULL, "Open", strExe, NULL, NULL, SW_MAXIMIZE);
	
	//WinExec(strCmd, StrToInt(swMode));

	AfxEnableControlContainer();

	EnableTaskbarInteraction(FALSE);

	// AfxInitRichEdit2() is required to use RichEdit control	
	// AfxInitRichEdit2();

	// Standard initialization
	// If you are not using these features and wish to reduce the size
	// of your final executable, you should remove from the following
	// the specific initialization routines you do not need
	// Change the registry key under which our settings are stored
	// TODO: You should modify this string to be something appropriate
	// such as the name of your company or organization
	SetRegistryKey(_T("Local AppWizard-Generated Applications"));
	LoadStdProfileSettings(4);  // Load standard INI file options (including MRU)

	InitContextMenuManager();

	InitKeyboardManager();

	InitTooltipManager();
	CMFCToolTipInfo ttParams;
	ttParams.m_bVislManagerTheme = TRUE;
	theApp.GetTooltipManager()->SetTooltipParams(AFX_TOOLTIP_TYPE_ALL,
		RUNTIME_CLASS(CMFCToolTipCtrl), &ttParams);

	// Register the application's document templates.  Document templates
	//  serve as the connection between documents, frame windows and views
	CSingleDocTemplate* pDocTemplate;
	pDocTemplate = new CSingleDocTemplate(
		IDR_MAINFRAME,
		RUNTIME_CLASS(CtltsDoc),
		RUNTIME_CLASS(CMainFrame),       // main SDI frame window
		RUNTIME_CLASS(CtltsView));
	if (!pDocTemplate)
		return FALSE;

	AddDocTemplate(pDocTemplate);


	// Parse command line for standard shell commands, DDE, file open
	CCommandLineInfo cmdInfo;
	ParseCommandLine(cmdInfo);



	// Dispatch commands specified on the command line.  Will return FALSE if
	// app was launched with /RegServer, /Register, /Unregserver or /Unregister.

	if (!ProcessShellCommand(cmdInfo))
		return FALSE;


	// The one and only window has been initialized, so show and update it
	m_pMainWnd->ShowWindow(SW_SHOW);
	m_pMainWnd->UpdateWindow();
	return TRUE;
}

int CtltsApp::ExitInstance()
{
	//TODO: handle additional resources you may have added
	AfxOleTerm(FALSE); 

	WSACleanup();
	return CWinAppEx::ExitInstance();
}

// CtltsApp message handlers


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

// App command to run the dialog
void CtltsApp::OnAppAbout()
{
	CString strFile = _T("");
	CFileDialog    dlgFile(TRUE, NULL, NULL, OFN_HIDEREADONLY, _T("tpB Files (*.tpB)|*.tpB|All Files (*.*)|*.*||"), NULL);

	if (dlgFile.DoModal())
	{
		strFile = dlgFile.GetPathName();
	}

	strFile.Replace('\\', '/');

	CMainFrame* pframe = (CMainFrame*)AfxGetApp()->m_pMainWnd;
	pframe->JudgeTPB(strFile, NULL);
	//CAboutDlg aboutDlg;
	//aboutDlg.DoModal();
}

// CtltsApp customization load/save methods

void CtltsApp::PreLoadState()
{
	BOOL bNameValid;
	CString strName;
	bNameValid = strName.LoadString(IDS_EDIT_MENU);
	ASSERT(bNameValid);
	GetContextMenuManager()->AddMenu(strName, IDR_POPUP_EDIT);
}

void CtltsApp::LoadCustomState()
{
}

void CtltsApp::SaveCustomState()
{
}

// CtltsApp message handlers



