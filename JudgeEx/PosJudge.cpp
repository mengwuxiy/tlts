
// PosJudge.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "PosJudge.h"
#include "PosJudgeDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CPosJudgeApp

BEGIN_MESSAGE_MAP(CPosJudgeApp, CWinApp)
	ON_COMMAND(ID_HELP, &CWinApp::OnHelp)
END_MESSAGE_MAP()


// CPosJudgeApp construction

CPosJudgeApp::CPosJudgeApp()
{
	m_bHiColorIcons = TRUE;

	// TODO: replace application ID string below with unique ID string; recommended
	// format for string is CompanyName.ProductName.SubProduct.VersionInformation
	SetAppID(_T("tlts.AppID.NoVersion"));

	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}


// The one and only CPosJudgeApp object

CPosJudgeApp theApp;


// CPosJudgeApp initialization

BOOL CPosJudgeApp::InitInstance()
{
	//CoInitialize(NULL);
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
	if (!AfxOleInit())
	{		
		return FALSE;
	}

	//AfxEnableControlContainer();

	// Create the shell manager, in case the dialog contains
	// any shell tree view or shell list view controls.
	//CShellManager *pShellManager = new CShellManager;

	// Activate "Windows Native" visual manager for enabling themes in MFC controls
	CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerWindows));

	// Standard initialization
	// If you are not using these features and wish to reduce the size
	// of your final executable, you should remove from the following
	// the specific initialization routines you do not need
	// Change the registry key under which our settings are stored
	// TODO: You should modify this string to be something appropriate
	// such as the name of your company or organization
	SetRegistryKey(_T("Local AppWizard-Generated Applications"));


	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData)) //µ÷ÓÃWindows Sockets DLL
	{
		WSACleanup();
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
	if (theApp.sql.IsConnected())
	{
		theApp.sql.CloseConn();
	}
	if (theApp.sql.ConnectDatabase(strServer, strUser, strPwd, strDB) == false)
	{
		file.Close();
		return FALSE;
	}
	file.Close();


	CPosJudgeDlg dlg;
	m_pMainWnd = &dlg;
	INT_PTR nResponse = dlg.DoModal();
	if (nResponse == IDOK)
	{
		// TODO: Place code here to handle when the dialog is
		//  dismissed with OK
	}
	else if (nResponse == IDCANCEL)
	{
		// TODO: Place code here to handle when the dialog is
		//  dismissed with Cancel
	}
	else if (nResponse == -1)
	{
		TRACE(traceAppMsg, 0, "Warning: dialog creation failed, so application is terminating unexpectedly.\n");
		TRACE(traceAppMsg, 0, "Warning: if you are using MFC controls on the dialog, you cannot #define _AFX_NO_MFC_CONTROLS_IN_DIALOGS.\n");
	}

	// Delete the shell manager created above.
	//if (pShellManager != NULL)
	//{
	//	delete pShellManager;
	//}

	// Since the dialog has been closed, return FALSE so that we exit the
	//  application, rather than start the application's message pump.
	return FALSE;
}

int CPosJudgeApp::ExitInstance()
{
	//TODO: handle additional resources you may have added
	AfxOleTerm(FALSE);

	WSACleanup();
	return CWinAppEx::ExitInstance();
}