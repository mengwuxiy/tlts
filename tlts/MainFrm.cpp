
// MainFrm.cpp : implementation of the CMainFrame class
//

#include "stdafx.h"
#include "tlts.h"

#include "MainFrm.h"
#include <io.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif



struct ASYNC_PARAM
{
	CMainFrame*		pFrame;
	CAsyncSocket*	pSock;
};
// CMainFrame

IMPLEMENT_DYNCREATE(CMainFrame, CFrameWndEx)

const int  iMaxUserToolbars = 10;
const UINT uiFirstUserToolBarId = AFX_IDW_CONTROLBAR_FIRST + 40;
const UINT uiLastUserToolBarId = uiFirstUserToolBarId + iMaxUserToolbars - 1;

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWndEx)
	ON_WM_CREATE()
	ON_COMMAND(ID_VIEW_CUSTOMIZE, &CMainFrame::OnViewCustomize)
	ON_REGISTERED_MESSAGE(AFX_WM_CREATETOOLBAR, &CMainFrame::OnToolbarCreateNew)
	ON_COMMAND_RANGE(ID_VIEW_APPLOOK_WIN_2000, ID_VIEW_APPLOOK_WINDOWS_7, &CMainFrame::OnApplicationLook)
	ON_UPDATE_COMMAND_UI_RANGE(ID_VIEW_APPLOOK_WIN_2000, ID_VIEW_APPLOOK_WINDOWS_7, &CMainFrame::OnUpdateApplicationLook)

	ON_MESSAGE(WM_WEBINFO_RECREATE, OnMessageReCreateWebCom)

	ON_MESSAGE(WM_FINISH, ONMessageFinish)
	ON_MESSAGE(WM_STEP, ONMessageStep)
	ON_WM_CLOSE()
END_MESSAGE_MAP()

static UINT indicators[] =
{
	ID_SEPARATOR,           // status line indicator
	ID_INDICATOR_CAPS,
	ID_INDICATOR_NUM,
	ID_INDICATOR_SCRL,
};

// CMainFrame construction/destruction

CMainFrame::CMainFrame()
{
	// TODO: add member initialization code here
	theApp.m_nAppLook = theApp.GetInt(_T("ApplicationLook"), ID_VIEW_APPLOOK_VS_2008);

	m_pFileA = NULL;
	m_pFileB = NULL;

	m_bSolving = FALSE;
	m_bOpen = FALSE;
	m_bLoaded = FALSE;
	m_bReadingB = FALSE;

	m_jugedHandle = CreateEvent(NULL, TRUE, TRUE, "Judge");

	m_bJudging = false;

	m_bExit = FALSE;
}

CMainFrame::~CMainFrame()
{
	m_ts.Close();
	CloseHandle(m_jugedHandle);
}

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{	
	if (CFrameWndEx::OnCreate(lpCreateStruct) == -1)
		return -1;

	if (!m_wndMenuBar.Create(this))
	{
		TRACE0("Failed to create menubar\n");
		return -1;      // fail to create
	}

	m_wndMenuBar.SetPaneStyle(m_wndMenuBar.GetPaneStyle() | CBRS_SIZE_DYNAMIC | CBRS_TOOLTIPS | CBRS_FLYBY);

	// prevent the menu bar from taking the focus on activation
	CMFCPopupMenu::SetForceMenuFocus(FALSE);

	if (!m_wndToolBar.CreateEx(this, TBSTYLE_FLAT, WS_CHILD | WS_VISIBLE | CBRS_TOP | CBRS_GRIPPER | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC) ||
		!m_wndToolBar.LoadToolBar(theApp.m_bHiColorIcons ? IDR_MAINFRAME_256 : IDR_MAINFRAME))
	{
		TRACE0("Failed to create toolbar\n");
		return -1;      // fail to create
	}

	
	CString strToolBarName;
	BOOL bNameValid = strToolBarName.LoadString(IDS_TOOLBAR_STANDARD);
	ASSERT(bNameValid);
	m_wndToolBar.SetWindowText(strToolBarName);

	CString strCustomize;
	bNameValid = strCustomize.LoadString(IDS_TOOLBAR_CUSTOMIZE);
	ASSERT(bNameValid);
	m_wndToolBar.EnableCustomizeButton(TRUE, ID_VIEW_CUSTOMIZE, strCustomize);

	// Allow user-defined toolbars operations:
	InitUserToolbars(NULL, uiFirstUserToolBarId, uiLastUserToolBarId);

	if (!m_wndStatusBar.Create(this))
	{
		TRACE0("Failed to create status bar\n");
		return -1;      // fail to create
	}
	m_wndStatusBar.SetIndicators(indicators, sizeof(indicators) / sizeof(UINT));
	

	// TODO: Delete these five lines if you don't want the toolbar and menubar to be dockable
	m_wndMenuBar.EnableDocking(CBRS_ALIGN_ANY);
	m_wndToolBar.EnableDocking(CBRS_ALIGN_ANY);
	EnableDocking(CBRS_ALIGN_ANY);
	DockPane(&m_wndMenuBar);
	DockPane(&m_wndToolBar);


	// enable Visual Studio 2005 style docking window behavior
	CDockingManager::SetDockingMode(DT_SMART);
	// enable Visual Studio 2005 style docking window auto-hide behavior
	EnableAutoHidePanes(CBRS_ALIGN_ANY);
	// set the visual manager and style based on persisted value
	OnApplicationLook(theApp.m_nAppLook);

	// Enable toolbar and docking window menu replacement
	EnablePaneMenu(TRUE, ID_VIEW_CUSTOMIZE, strCustomize, ID_VIEW_TOOLBAR);

	// enable quick (Alt+drag) toolbar customization
	CMFCToolBar::EnableQuickCustomization();

	if (CMFCToolBar::GetUserImages() == NULL)
	{
		// load user-defined toolbar images
		if (m_UserImages.Load(_T(".\\UserImages.bmp")))
		{
			CMFCToolBar::SetUserImages(&m_UserImages);
		}
	}

	

	// enable menu personalization (most-recently used commands)
	// TODO: define your own basic commands, ensuring that each pulldown menu has at least one basic command.
	CList<UINT, UINT> lstBasicCommands;

	lstBasicCommands.AddTail(ID_FILE_NEW);
	lstBasicCommands.AddTail(ID_FILE_OPEN);
	lstBasicCommands.AddTail(ID_FILE_SAVE);
	lstBasicCommands.AddTail(ID_FILE_PRINT);
	lstBasicCommands.AddTail(ID_APP_EXIT);
	lstBasicCommands.AddTail(ID_EDIT_CUT);
	lstBasicCommands.AddTail(ID_EDIT_PASTE);
	lstBasicCommands.AddTail(ID_EDIT_UNDO);
	lstBasicCommands.AddTail(ID_APP_ABOUT);
	lstBasicCommands.AddTail(ID_VIEW_STATUS_BAR);
	lstBasicCommands.AddTail(ID_VIEW_TOOLBAR);
	lstBasicCommands.AddTail(ID_VIEW_APPLOOK_OFF_2003);
	lstBasicCommands.AddTail(ID_VIEW_APPLOOK_VS_2005);
	lstBasicCommands.AddTail(ID_VIEW_APPLOOK_OFF_2007_BLUE);
	lstBasicCommands.AddTail(ID_VIEW_APPLOOK_OFF_2007_SILVER);
	lstBasicCommands.AddTail(ID_VIEW_APPLOOK_OFF_2007_BLACK);
	lstBasicCommands.AddTail(ID_VIEW_APPLOOK_OFF_2007_AQUA);
	lstBasicCommands.AddTail(ID_VIEW_APPLOOK_WINDOWS_7);

	CMFCToolBar::SetBasicCommands(lstBasicCommands);
	if (!CreateWebCom())
	{
		return -1;
	}

	//AfxMessageBox("Getwo");
	//Wound_Judged wd;
	//theApp.sql.GetWound(1, wd);
	//AfxMessageBox("Getwo ok");
	//CString str = GetWoundData(wd);
	//AfxMessageBox("Getwo data ok");
	return 0;
}

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	if (!CFrameWndEx::PreCreateWindow(cs))
		return FALSE;
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	return TRUE;
}

// CMainFrame diagnostics

#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
	CFrameWndEx::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const
{
	CFrameWndEx::Dump(dc);
}
#endif //_DEBUG


// CMainFrame message handlers

void CMainFrame::OnViewCustomize()
{
	CMFCToolBarsCustomizeDialog* pDlgCust = new CMFCToolBarsCustomizeDialog(this, TRUE /* scan menus */);
	pDlgCust->EnableUserDefinedToolbars();
	pDlgCust->Create();
}

LRESULT CMainFrame::OnToolbarCreateNew(WPARAM wp, LPARAM lp)
{
	LRESULT lres = CFrameWndEx::OnToolbarCreateNew(wp, lp);
	if (lres == 0)
	{
		return 0;
	}

	CMFCToolBar* pUserToolbar = (CMFCToolBar*)lres;
	ASSERT_VALID(pUserToolbar);

	BOOL bNameValid;
	CString strCustomize;
	bNameValid = strCustomize.LoadString(IDS_TOOLBAR_CUSTOMIZE);
	ASSERT(bNameValid);

	pUserToolbar->EnableCustomizeButton(TRUE, ID_VIEW_CUSTOMIZE, strCustomize);
	return lres;
}

void CMainFrame::OnApplicationLook(UINT id)
{
	CWaitCursor wait;

	theApp.m_nAppLook = id;

	switch (theApp.m_nAppLook)
	{
	case ID_VIEW_APPLOOK_WIN_2000:
		CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManager));
		break;

	case ID_VIEW_APPLOOK_OFF_XP:
		CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerOfficeXP));
		break;

	case ID_VIEW_APPLOOK_WIN_XP:
		CMFCVisualManagerWindows::m_b3DTabsXPTheme = TRUE;
		CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerWindows));
		break;

	case ID_VIEW_APPLOOK_OFF_2003:
		CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerOffice2003));
		CDockingManager::SetDockingMode(DT_SMART);
		break;

	case ID_VIEW_APPLOOK_VS_2005:
		CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerVS2005));
		CDockingManager::SetDockingMode(DT_SMART);
		break;

	case ID_VIEW_APPLOOK_VS_2008:
		CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerVS2008));
		CDockingManager::SetDockingMode(DT_SMART);
		break;

	case ID_VIEW_APPLOOK_WINDOWS_7:
		CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerWindows7));
		CDockingManager::SetDockingMode(DT_SMART);
		break;

	default:
		switch (theApp.m_nAppLook)
		{
		case ID_VIEW_APPLOOK_OFF_2007_BLUE:
			CMFCVisualManagerOffice2007::SetStyle(CMFCVisualManagerOffice2007::Office2007_LunaBlue);
			break;

		case ID_VIEW_APPLOOK_OFF_2007_BLACK:
			CMFCVisualManagerOffice2007::SetStyle(CMFCVisualManagerOffice2007::Office2007_ObsidianBlack);
			break;

		case ID_VIEW_APPLOOK_OFF_2007_SILVER:
			CMFCVisualManagerOffice2007::SetStyle(CMFCVisualManagerOffice2007::Office2007_Silver);
			break;

		case ID_VIEW_APPLOOK_OFF_2007_AQUA:
			CMFCVisualManagerOffice2007::SetStyle(CMFCVisualManagerOffice2007::Office2007_Aqua);
			break;
		}

		CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerOffice2007));
		CDockingManager::SetDockingMode(DT_SMART);
	}

	RedrawWindow(NULL, NULL, RDW_ALLCHILDREN | RDW_INVALIDATE | RDW_UPDATENOW | RDW_FRAME | RDW_ERASE);

	theApp.WriteInt(_T("ApplicationLook"), theApp.m_nAppLook);
}

void CMainFrame::OnUpdateApplicationLook(CCmdUI* pCmdUI)
{
	pCmdUI->SetRadio(theApp.m_nAppLook == pCmdUI->m_nID);
}


BOOL CMainFrame::LoadFrame(UINT nIDResource, DWORD dwDefaultStyle, CWnd* pParentWnd, CCreateContext* pContext)
{
	// base class does the real work
	if (!CFrameWndEx::LoadFrame(nIDResource, dwDefaultStyle, pParentWnd, pContext))
	{
		return FALSE;
	}

	// enable customization button for all user toolbars
	BOOL bNameValid;
	CString strCustomize;
	bNameValid = strCustomize.LoadString(IDS_TOOLBAR_CUSTOMIZE);
	ASSERT(bNameValid);

	for (int i = 0; i < iMaxUserToolbars; i++)
	{
		CMFCToolBar* pUserToolbar = GetUserToolBarByIndex(i);
		if (pUserToolbar != NULL)
		{
			pUserToolbar->EnableCustomizeButton(TRUE, ID_VIEW_CUSTOMIZE, strCustomize);
		}
	}

	return TRUE;
}


void	CMainFrame::OnAccept(CSocketEx* sock)
{
	CSocketEx *sock2 = new CSocketEx;
	SOCKADDR sar;
	int sockaddr_in_len = sizeof(sockaddr_in);
	if (m_ts.Accept(*sock2, &sar, &sockaddr_in_len))
	{
		CString rPeerAddress; 
		UINT rPeerPort;
		sock2->GetPeerName(rPeerAddress, rPeerPort);
		CString str;
		str.Format("%s:%d", rPeerAddress, rPeerPort);
		m_vList.insert( make_pair(str, 1));
		m_vSocks.insert(make_pair(sock2, 1));
		sock2->SetParent((DWORD_PTR)this);
		sock2->AsyncSelect(FD_READ | FD_WRITE | FD_CLOSE);
		//sock2->Send("1234", 5);
	}
}

void	CMainFrame::OnReceive(CSocketEx* sock)
{
	m_iCommunicateState = 1;//正在处理信息
	char  temp[1000] = {0};
	int nRecv = sock->Receive(temp, 1000);
	if (nRecv < 15)
	{
		return;
	}

	//只处理来自主控软件的信息
	CString label(temp);
	CString strBytes = label.Left(11);
	CString strCode = label.Mid(11, 4);
	CString strData = label.Mid(15);
	if (strCode == "1000")
	{
		strData.Replace('\\', '/');
		this->JudgeTPB(strData, sock);
	}
	else if (strCode == "1001")
	{
		uint64_t wID = strtoull(temp + 15, NULL, 10);
		CString strRet = GetMultiCycleWoundData(wID);
		SendUTF8(strRet, 1003, sock);
	}
	m_iCommunicateState = 0;//处理完成
}

void	CMainFrame::OnSend(CSocketEx* sock)
{

}

void	CMainFrame::OnSocketClose(CSocketEx* sock)
{
	//char ip[20] = { 0 };
	//int port = 0;
	//sockaddr addr;
	//int namelen = 0;
	//getpeername(sock->m_hSocket, &addr, &namelen);
	//sockaddr_in sin;
	//memcpy(&sin, &addr, sizeof(sin));
	//inet_ntop(AF_INET, &sin.sin_addr, ip, 20);
	//port = sin.sin_port;
	//CString str;
	//str.Format("%s:%d", ip, port);
	//m_vList.erase(m_vList.find(str));

	map<CSocketEx*, uint8_t>::iterator itr = m_vSocks.find(sock);
	if (itr!=m_vSocks.end())
	{
		m_vSocks.erase(itr);
	}
	delete sock;
	sock = NULL;
}


void	CMainFrame::OnOutOfBandData(CSocketEx* sock)
{

}

LRESULT CMainFrame::OnMessageReCreateWebCom(WPARAM wParam, LPARAM lParam)
{
	CreateWebCom();
	return 0;
}

BOOL CMainFrame::CreateWebCom()
{
	CString strDir = g_strModuleFolder + "\\mysql.cfg";
	CStdioFile file;
	file.Open(strDir, CFile::modeRead);
	CString strServer, strUser, strPwd, strDB;
	file.ReadString(strServer);
	file.ReadString(strUser);
	file.ReadString(strPwd);
	file.ReadString(strDB);


	BOOL bSuccess = m_ts.Create(4666, SOCK_STREAM, 63);
	if (!bSuccess)
	{
		return FALSE;
	}
	m_ts.SetParent((DWORD_PTR)this);
	return	m_ts.Listen();
}


void CMainFrame::OnClose()
{
	// TODO: Add your message handler code here and/or call default
	m_bExit = TRUE;
	CFrameWndEx::OnClose();
}


void HeapAdjust(vector<Wound_Judged>& wounds, int count, int index, bool direction = true)
{
	Wound_Judged pm = wounds[index];
	for (int k = index * 2 + 1; k < count; k = k * 2 + 1)
	{
		if (k + 1 < count && (wounds[k].Block < wounds[k + 1].Block || (wounds[k].Block == wounds[k + 1].Block && wounds[k].Step < wounds[k + 1].Step)))
		{
			k++;
		}
		if (wounds[k].Block > pm.Block || (wounds[k].Block == pm.Block && wounds[k].Step > pm.Step))
		{
			wounds[index] = wounds[k];
			index = k;
		}
		else
		{
			break;
		}
	}
	wounds[index] = pm;
}

void HeapSort(vector<Wound_Judged> &wounds, int count, bool direction = true)
{
	for (int i = count / 2 - 1; i >= 0; i--)  // 对每一个非叶结点进行堆调整(从最后一个非叶结点开始)
	{
		HeapAdjust(wounds, count, i, direction);
	}
	for (int i = count - 1; i >= 1; i--)
	{
		swap(wounds[0], wounds[i]);           // 将当前最大的放置到数组末尾
		HeapAdjust(wounds, i, 0, direction);              // 将未完成排序的部分继续进行堆排序
	}
}

void CMainFrame::ExportWounds(vector<Wound_Judged>& vWounds, int count, CString strPath)
{
	int nRet = _access((char*)(LPCTSTR)strPath, 0);
	bool bExist = (0 == nRet || EACCES == nRet);
	if (bExist)
	{
		DeleteFile(strPath);
		Sleep(1);
	}

	FILE* pFileOut = fopen((char*)(LPCTSTR)strPath, "w+");
	if (pFileOut == NULL)
	{
		AfxMessageBox(_T("结果打开失败！！"), MB_OK | MB_ICONWARNING);
		return;
	}

	const char CH_STR[16][3] = { "A1", "A2", "a1", "a2", "B1", "B2", "b1", "b2", "C", "c", "D", "d", "E", "e", "F", "G" };
	const char Wave_STR[2][5] = { "失波", "出波" };
	int k = 1;
	for (int i = 0; i < count; ++i)
	{
		if (vWounds[i].Flag == 1)
		{
			continue;
		}
		fprintf(pFileOut, "%d %.3lf\t%s\t%s\t%d\t%d\n", k++, vWounds[i].Walk, g_strTypeDefines[vWounds[i].Type], g_strTypeDefines[vWounds[i].Type2], vWounds[i].Block + 1, vWounds[i].Step);
	}
	fclose(pFileOut);
}

void CMainFrame::ExportWoundsToExcel(vector<Wound_Judged>& vWounds, int count, CString strPath)
{
	int nRet = _access((char*)(LPCTSTR)strPath, 0);
	bool bExist = (0 == nRet || EACCES == nRet);
	if (bExist)
	{
		DeleteFile(strPath);
		Sleep(1);
	}

	try
	{
		CString strTitles[] = { "里程", "推荐判定类型", "人工判定类型", "米块", "步进" };
		m_oExcel.CreateExcelFile();

		for (int i = 0; i < 5; ++i)
		{
			m_oExcel.SetCellString(1, i + 1, strTitles[i]);
		}

		for (int i = 0; i < vWounds.size(); ++i)
		{
			CString strTemp;
			strTemp.Format("%.3lf", vWounds[i].Walk);
			m_oExcel.SetCellString(i + 2, 1, strTemp);

			m_oExcel.SetCellString(i + 2, 2, g_strTypeDefines[vWounds[i].Type]);

			m_oExcel.SetCellString(i + 2, 3, g_strTypeDefines[vWounds[i].Type2]);

			strTemp.Format("%d", vWounds[i].Block);
			m_oExcel.SetCellString(i + 2, 4, strTemp);

			strTemp.Format("%d", vWounds[i].Step);
			m_oExcel.SetCellString(i + 2, 5, strTemp);
		}

		m_oExcel.SaveasXSLFile(strPath);
		m_oExcel.ReleaseExcel();
	}
	catch (_com_error & e)
	{
		CString strMsg;
		strMsg.Format("位置: 文件：%s,行：%d\n错误：%s\n源：%s\n其他：%s", __FILE__, __LINE__,
			e.ErrorMessage(),
			CString(e.Source().copy()),
			CString(e.Description().copy()));

		AfxMessageBox(strMsg);
	}
	catch (...)
	{
		AfxMessageBox("未知错误！", MB_OK | MB_ICONWARNING);
	}
}


void HeapAdjust(vector<Position_Mark>& vPMs, int count, int index, bool direction = true)
{
	Position_Mark pm = vPMs[index];
	for (int k = index * 2 + 1; k < count; k = k * 2 + 1)
	{
		if (k + 1 < count && (vPMs[k].Block < vPMs[k + 1].Block || (vPMs[k].Block == vPMs[k + 1].Block && vPMs[k].Step < vPMs[k + 1].Step)))
		{
			k++;
		}
		if (vPMs[k].Block > pm.Block || (vPMs[k].Block == pm.Block && vPMs[k].Step > pm.Step))
		{
			vPMs[index] = vPMs[k];
			index = k;
		}
		else
		{
			break;
		}
	}
	vPMs[index] = pm;
}

void HeapSort(vector<Position_Mark> &vPMs, int count, bool direction = true)
{
	for (int i = count / 2 - 1; i >= 0; i--)  // 对每一个非叶结点进行堆调整(从最后一个非叶结点开始)
	{
		HeapAdjust(vPMs, count, i, direction);
	}
	for (int i = count - 1; i >= 1; i--)
	{
		swap(vPMs[0], vPMs[i]);           // 将当前最大的放置到数组末尾
		HeapAdjust(vPMs, i, 0, direction);              // 将未完成排序的部分继续进行堆排序
	}
}

void CMainFrame::SendWounds(vector<Wound_Judged>& vWounds, CAsyncSocket* sock)
{
	CString strWounds;
	if (vWounds.size() > 0)
	{
		//strWounds.Format("{data:{rail:{name:'%s',xingbie:'%s',gubie:'%s',s_mil:%.5lf, e_mil:%.5lf},detail:[", g_strRailName, g_strXingBieDefines[g_xingbie], g_strGuBieDefines[g_gubie], g_startPos, g_endPos);
		strWounds.Format("{data:{rail:{block:%d,name:'%s',xingbie:'%s',gubie:'%s',s_mil:%.5lf, e_mil:%.5lf},detail:[", vWounds[0].Block, g_strRailName, g_strXingBieDefines[g_xingbie], g_strGuBieDefines[g_gubie], g_startPos, g_endPos);
		CString strTemp;
		for (int i = 0; i < vWounds.size(); ++i)
		{
			strTemp.Format("{According:'%s',ana_time:'%s',gps_lat:%lf,gps_long:%lf,index:%03d, re_state:%d,rew_address:%lf,rew_degree:%d,rew_des:'%s',rew_place:%d,rew_type:%d,rewx_size:%d,rewy_size:%d,w_address:%lf,w_degree:%d,w_id:0,w_place:%d,w_type:%d,wx_size:%d,wy_size:%d},",
				vWounds[i].According, vWounds[i].ana_time, vWounds[i].gps_lat, vWounds[i].gps_log, i, vWounds[i].Checked, vWounds[i].Walk2, vWounds[i].Degree2, vWounds[i].Desc, vWounds[i].Place2, vWounds[i].Type2, vWounds[i].SizeX2, vWounds[i].SizeY2,
				vWounds[i].Walk, vWounds[i].Degree, vWounds[i].Place, vWounds[i].Type, vWounds[i].SizeX, vWounds[i].SizeY);
			strWounds += strTemp;
		}
		strWounds = strWounds.Left(strWounds.GetLength() - 1);
		
		strTemp.Format("],wtype_count_detail:{info:{w_address:%.3lf,count:%d}, header:{index:'序号',w_address:'里程',w_type:'伤损类型',w_place:'伤损位置',w_degree:'伤损程度'}}}}",
			m_Head.distance * m_Head.step / 1000000, vWounds.size());
		strWounds += strTemp;

		SendUTF8(strWounds, 1001, sock);
	}
}

bool CMainFrame::SendUTF8(CString& data, uint16_t code, CAsyncSocket* sock)
{
	const char* pSource = (const char*)(LPCTSTR)data;
	int nLength = MultiByteToWideChar(CP_ACP, 0, pSource, -1, NULL, NULL);   // 获取缓冲区长度，再分配内存
	WCHAR *tch = new WCHAR[nLength];
	nLength = MultiByteToWideChar(CP_ACP, 0, pSource, -1, tch, nLength);     // 将MBCS转换成Unicode

	int nUTF8len = WideCharToMultiByte(CP_UTF8, 0, tch, nLength, 0, 0, 0, 0);   // 获取UTF-8编码长度
	char *utf8_string = new char[nUTF8len];
	WideCharToMultiByte(CP_UTF8, 0, tch, nLength, utf8_string, nUTF8len, 0, 0); //转换成UTF-8编码

	char* pSend = new char[nUTF8len + 15];
	sprintf_s(pSend, nUTF8len + 15, "%011d%04d%s", nUTF8len + 4, code, utf8_string);
	//bool bOK = m_ts.Send(pSend, nUTF8len + 15);
	bool bOK = sock->Send(pSend, nUTF8len + 15);

	delete pSend;
	delete utf8_string;
	delete tch;
	return bOK;
}

void DoJudge(void* param)
{
	ASYNC_PARAM* pData= (ASYNC_PARAM*)param;
	CMainFrame*	pFrame = pData->pFrame;

	BlockData_A vAFrames;
	vector<BlockData_B> vBdatas;

	pFrame->m_vInfoB.clear();
	pFrame->m_vPMs.clear();
	pFrame->m_vWounds.clear();
	pFrame->m_vBlockHeads.clear();
	int iBeginFrame = 0;
	uint32_t use_Size_B = 0, read_Size_B = 0;

	int32_t readL_A = 0, useL_A = 0;

	time_t timec;
	tm* p;
	int ibeginstep = 0;
	vector<Wound_Judged> vWounds;
	while (pFrame->m_bJudging)
	{
		PostMessage(pFrame->m_hWnd, WM_STEP, 0, 0);
		vAFrames.vAStepDatas.clear();
		vBdatas.clear();
		vWounds.clear();

		uint32_t iReadBlockCount = Analyse_Bchao(pFrame->m_pFileB, vBdatas, use_Size_B, read_Size_B, pFrame->m_iSizeB, pFrame->m_iCurrentBlock, ibeginstep, pFrame->m_vInfoB, N_BLOCKREAD);
		if (iReadBlockCount == 0)
		{
			break;
		}

		Analyse_Achao(pFrame->m_pFileA, vAFrames, vBdatas, readL_A, useL_A, pFrame->m_iSizeA, pFrame->m_iCurrentBlock, iBeginFrame, pFrame->m_vInfoA, N_BLOCKREAD);
		for (int i = 0; i < vBdatas.size(); ++i)
		{
			pFrame->m_vBlockHeads.push_back(vBdatas[i].BlockHead);
		}

		Analyse(pFrame->m_Head, vAFrames, vBdatas, vWounds, pFrame->m_vPMs);
		for (int i = 0; i < vWounds.size(); ++i)
		{
			pFrame->m_vWounds.push_back(vWounds[i]);
		}
		if (vWounds.size() > 0)
		{
			pFrame->SendWounds(vWounds, pData->pSock);
		}
		pFrame->m_iCurrentBlock += N_BLOCKREAD;
		if (pFrame->m_vInfoB[pFrame->m_vInfoB.size() - 1].UsedL >= pFrame->m_iSizeB )
		{
			break;
		}
	} 

	if (!pFrame->m_bJudging)
	{
		pFrame->CloseFileAB();
		return ;
	}

	HeapSort(pFrame->m_vPMs, pFrame->m_vPMs.size(), false);

	//S5 伤损根据里程排序
	HeapSort(pFrame->m_vWounds, pFrame->m_vWounds.size(), false);

	pFrame->CloseFileAB();

	::PostMessage(pFrame->m_hWnd, WM_FINISH, 0, (LPARAM)pData);
}

void CMainFrame::JudgeTPB(CString strTpB, CAsyncSocket* sock)
{
	StopJudge();
	Sleep(1000);

	if (m_pFileA != NULL)
	{
		fclose(m_pFileA);	m_pFileA = NULL; m_iSizeA = 0;
	}

	if (m_pFileB != NULL)
	{
		fclose(m_pFileB);	m_pFileB = NULL; m_iSizeB = 0;
	}
	
	time_t time2;
	time(&time2);
	tm* p = localtime(&time2);
	m_bSolving = TRUE;
	m_szFileB = strTpB;
	CurrentYear = p->tm_year + 1900;
	CurrentMonth = p->tm_mon + 1;
	CurrentDay = p->tm_mday;

	UpdateData(FALSE);
	Invalidate();

	double gps_log = 0, gps_lat = 0;
	if (!AddWork((char*)(LPCTSTR)m_szFileB, gps_log, gps_lat))
	{
		m_bSolving = FALSE;
		SetEvent(m_jugedHandle);
		CString str;
		str.Format("%I64d", g_FileID);
		SendUTF8(str, 1002, sock);
		return;
	}

	//AfxMessageBox("Begin");
	CString strSend;
	strSend.Format("{rail:{rail_name:'%s',xingbie:'%s',gubie:'%s',s_mil:%.0lf,e_mil:%.0lf,log:%lf,lat:%lf}}", g_strRailName, g_strXingbie, g_strGubie, g_startPos, g_endPos, gps_log, gps_lat);
	
	SendUTF8(strSend, 1000, sock);

	CString strNewTpbPath = "D:/Files/" + g_strNewFileName + ".tpB";
	//AfxMessageBox(strNewTpbPath);
	DeleteFile(strNewTpbPath);
	DeleteFile("D:/Files/" + g_strNewFileName + ".tpA");
	CopyFile(m_szFileB, strNewTpbPath, FALSE);
	CopyFile(m_szFileB.Left(m_szFileB.GetLength() - 1) + "A", "D:/Files/" + g_strNewFileName + ".tpA", FALSE);
	m_szFileB = strNewTpbPath;

	int idx = m_szFileB.ReverseFind('/');
	g_strtpBFolder = m_szFileB.Left(idx);
	g_strtpBFileName = m_szFileB.Mid(idx + 1);
	int idx2 = g_strtpBFileName.ReverseFind('.');
	g_strtpBFileName = g_strtpBFileName.Left(idx2);

	if (FALSE == OpenFileAB())
	{
		m_bSolving = FALSE; 
		SetEvent(m_jugedHandle);
		CString str;
		str.Format("Fail to Open File");
		SendUTF8(str, 4444, sock);
		return;
	}
	m_iCurrentBlock = 0;

	uint32_t addr = 0;
	m_bExported = FALSE;
	m_bJudging = true;
	ASYNC_PARAM* para = new ASYNC_PARAM;
	para->pFrame = this;
	para->pSock = sock;
	m_threadJudge = _beginthreadex(NULL, 0, (unsigned int(__stdcall *)(void *))DoJudge, para, 0, &addr);
	/*
	while (m_bSolving == TRUE)
	{
		DWORD result;
		MSG msg;

		result = MsgWaitForMultipleObjects(1, &m_jugedHandle, FALSE, INFINITE, QS_ALLINPUT);

		if (result == (WAIT_OBJECT_0))
		{
			break;
		}
		else
		{
			PeekMessage(&msg, NULL, 0, 0, PM_REMOVE);
			DispatchMessage(&msg);
		}
	}*/
}


void CMainFrame::TravelDir(CString strPath)
{
	CString fdPath;
	CString strDataPath = strPath;
	CString strTmp;

	CFileFind find;//MFC的文件查找类
	BOOL bf = find.FindFile(strDataPath);
	while (bf)//循环遍历文件夹下的文件
	{
		bf = find.FindNextFile();
		if (!find.IsDots() && !find.IsDirectory())
		{
			fdPath = find.GetFilePath();
			strTmp = fdPath.Right(4);//CString很方便
			strTmp.MakeLower();
			if (".tpb" == strTmp)//指定格式后缀
			{
				m_bLoaded = FALSE;
				ResetEvent(m_jugedHandle);
				JudgeTPB(fdPath, NULL);//针对文件的操作

				while (m_bSolving == TRUE)
				{
					DWORD result;
					MSG msg;

					result = MsgWaitForMultipleObjects(1, &m_jugedHandle, FALSE, INFINITE, QS_ALLINPUT);

					if (result == (WAIT_OBJECT_0))
					{
						break;
					}
					else
					{
						PeekMessage(&msg, NULL, 0, 0, PM_REMOVE);
						DispatchMessage(&msg);
					}
				}
			}
		}
		else if (find.IsDirectory() && !find.IsDots())
		{
			fdPath = find.GetFilePath() + "\\*.*";
			TravelDir(fdPath);
		}
	}
}


void CMainFrame::StopJudge()
{
	m_bJudging = false;
}

void CMainFrame::CombinePM()
{
	Position_Mark pmLastJoint;
	memset(&pmLastJoint, 0, sizeof(Position_Mark));
	for (int i = 0; i < m_vPMs.size(); ++i)
	{
		if (m_vPMs[i].Flag == 1)
		{
			continue;
		}
		double wd = m_vPMs[i].Walk;
		Position_Mark& pm = m_vPMs[i];
		uint8_t iLeftS = 0, iRightS = 0, iLeftG = 0, iRightG = 0;
		if (m_vPMs[i].Mark == PM_JOINT || m_vPMs[i].Mark == PM_SEW_CH || m_vPMs[i].Mark == PM_SEW_LRH)
		{
			if (m_vPMs[i].Mark == PM_JOINT)
			{
				pmLastJoint = m_vPMs[i];
			}
			for (int j = i - 1; j >= 0; --j)
			{
				if (m_vPMs[j].Flag == 1)
				{
					continue;
				}
				double wdj = m_vPMs[j].Walk;
				if (wd - wdj >= 0.001 || m_vPMs[i].Block - m_vPMs[j].Block > 1)
				{
					break;
				}
				if (m_vPMs[j].Mark == PM_SCREWHOLE)
				{
					++iRightS;
					m_vPMs[j].Flag = 1;
				}
				if (m_vPMs[j].Mark == PM_GUIDEHOLE)
				{
					++iRightG;
					m_vPMs[j].Flag = 1;
				}
			}
			for (int j = i + 1; j < m_vPMs.size(); ++j)
			{
				if (m_vPMs[j].Flag == 1)
				{
					continue;
				}
				float wdj = m_vPMs[j].Walk;
				if (wdj - wd >= 0.001 || m_vPMs[j].Block - m_vPMs[i].Block > 1)
				{
					break;
				}
				else if (m_vPMs[i].Mark == PM_GUIDEHOLE && wd - pmLastJoint.Walk < 0.0002 && wd - pmLastJoint.Walk >= -0.0002)
				{
					m_vPMs[i].Flag = 1;
					continue;
				}

				if (m_vPMs[j].Mark == m_vPMs[i].Mark)
				{
					m_vPMs[j].Flag = 1;
				}
				if (m_vPMs[j].Mark == PM_SCREWHOLE)
				{
					++iLeftS;
					m_vPMs[j].Flag = 1;
				}
				if (m_vPMs[j].Mark == PM_GUIDEHOLE)
				{
					++iLeftG;
					m_vPMs[j].Flag = 1;
				}
			}
		}
		m_vPMs[i].Data2 = (iLeftG << 12) + (iLeftS << 8) + (iRightS << 4) + iRightG;
	}
}

void CMainFrame::CombineWound()
{
	for (int i = 0; i < m_vWounds.size(); ++i)
	{
		if (m_vWounds[i].Flag == 1)
		{
			continue;
		}
		double wd = m_vWounds[i].Walk;
		for (int j = i + 1; j < m_vWounds.size(); ++j)
		{
			double wdj = m_vWounds[j].Walk;
			if (wdj - wd >= 0.0003 || wdj - wd <= -0.0003)//30cm
			{
				break;
			}
			m_vWounds[j].Flag = 1;
			wd = m_vWounds[j].Walk;
		}
	}
}


BOOL CMainFrame::OpenFileAB()
{
	CString strFileA = g_strtpBFolder + "/" + g_strtpBFileName + ".tpA";
	m_pFileA = fopen((char*)(LPCTSTR)strFileA, "rb");
	if (m_pFileA == NULL)
	{
		//AfxMessageBox(_T("A打开失败！！"));
		return FALSE;
	}

	fseek(m_pFileA, 0L, SEEK_END);
	m_iSizeA = ftell(m_pFileA);

	m_pFileB = fopen((char*)(LPCTSTR)m_szFileB, "rb");
	if (m_pFileB == NULL)
	{
		//AfxMessageBox(_T("B打开失败！！"));
		return FALSE;
	}
	fseek(m_pFileB, 0L, SEEK_END);
	m_iSizeB = ftell(m_pFileB);
	fseek(m_pFileB, 0L, SEEK_SET);
	fread(&m_Head, sizeof(F_HEAD), 1, m_pFileB);
	m_bOpen = TRUE;
	return TRUE;
}

void CMainFrame::CloseFileAB()
{
	fclose(m_pFileA); m_pFileA = NULL; m_iSizeA = 0;
	fclose(m_pFileB); m_pFileB = NULL; m_iSizeB = 0;
	m_bOpen = FALSE;
}

uint32_t CMainFrame::ReadABData(int iBlock)
{
	if (m_bReadingB == TRUE)
	{
		return 0;
	}
	m_bReadingB = TRUE;
	int iBeginBlock = iBlock - 2, iEndBlock = iBlock + 2;
	if (iBlock < 2)
	{
		iBeginBlock = 0;
	}
	if (iBlock >= m_vBlockHeads.size() - 1)
	{
		iBeginBlock = m_vBlockHeads.size() - 2;
		iEndBlock = m_vBlockHeads.size() - 1;
	}

	int i = iBeginBlock;
	int iBeginFrame = 0;
	while (i < iEndBlock)
	{
		Analyse_Achao2(m_pFileA, m_vABlocksToShow, m_vInfoA[i].UsedL, m_iSizeA, i, m_vBlockHeads[i].indexL, iBeginFrame, 1);
		Analyse_Bchao2(m_pFileB, m_vBBlocksToShow, m_vInfoB[i].UsedL, m_iSizeB, i, m_vBlockHeads[i].indexL, 1);
		++i;
	}

	int iStepCount = 0;
	for (int i = 0; i < m_vBBlocksToShow.size(); ++i)
	{
		BlockData_B& block = m_vBBlocksToShow[i];
		for (int j = 0; j < m_vBBlocksToShow[i].vBStepDatas.size(); ++j)
		{
			B_Step& step = m_vBBlocksToShow[i].vBStepDatas[j];
			m_vSteps.push_back(step);
		}
		iStepCount += m_vBBlocksToShow[i].BlockHead.row;
	}
	m_bReadingB = FALSE;
	return m_vSteps.size();
}

uint32_t CMainFrame::ReadABData2(CString strFileA, CString strFileB, vector<BLOCK_SQL>&blocks, int iBlock, F_HEAD& head, BlockData_A& vASteps, vector<BlockData_B>& datas)
{
	FILE* pFileA = fopen((char*)(LPCTSTR)strFileA, "rb");
	if (pFileA == NULL)
	{		
		return 0;
	}

	fseek(pFileA, 0L, SEEK_END);
	uint32_t iSizeA = ftell(pFileA);

	FILE*  pFileB = fopen((char*)(LPCTSTR)strFileB, "rb");
	if (pFileB == NULL)
	{
		return 0;
	}
	fseek(pFileB, 0L, SEEK_END);
	uint32_t iSizeB = ftell(pFileB);
	fseek(pFileB, 0L, SEEK_SET);
	fread(&head, sizeof(F_HEAD), 1, pFileB);
	if (pFileA == NULL || pFileB == NULL)
	{
		return 0;
	}

	//int iBeginBlock = iBlock - 1, iEndBlock = iBlock + 1;
	int iBeginBlock = iBlock - 1, iEndBlock = iBlock + 1;
	if (iBlock < 1)
	{
		iBeginBlock = 0;
	}
	if (iBlock >= blocks.size() - 1)
	{
		iBeginBlock = blocks.size() - 1;
		iEndBlock = blocks.size() - 1;
	}

	int i = iBeginBlock;
	int t = blocks[i].Step;
	int iBeginFrame = 0;
	while (i < iEndBlock)
	{
		Analyse_Achao2(pFileA, vASteps, blocks[i].StartA, iSizeA, i, blocks[i].Step - t, iBeginFrame, 1);
		Analyse_Bchao2(pFileB, datas, blocks[i].StartB, iSizeB, i, blocks[i].Step - t, 1);
		++i;
	}

	fclose(pFileA); 
	fclose(pFileB);
	return vASteps.vAStepDatas.size();
}

LRESULT CMainFrame::ONMessageStep(WPARAM wParam, LPARAM lParam)
{
	//if (m_iCurrentBlock < m_Head.block)
	//{
	//	if (m_vInfoB.size() > 0)
	//	{
	//		m_strProgress.Format(_T("%d/%d"), m_iCurrentBlock, m_Head.block);
	//	}
	//}
	//else
	//{
	//	m_Head.block = m_vInfoB.size();
	//	m_strProgress.Format(_T("%d/%d"), m_vInfoB.size(), m_Head.block);
	//}
	//UpdateData(FALSE);
	return 0L;
}

LRESULT CMainFrame::ONMessageFinish(WPARAM wParam, LPARAM lParam)
{
	m_bSolving = FALSE;
	m_bJudging = false;
	ASYNC_PARAM *p = (ASYNC_PARAM*)lParam;
	CombineOutputData(m_vPMs, m_vWounds, m_vBlockHeads);

	AddBlocks(m_vBlockHeads, m_vInfoA, m_vInfoB);
	AddWounds(m_vWounds);
	AddPMs(m_vPMs);

	m_bLoaded = m_vInfoB.size() > 0;
	SetEvent(m_jugedHandle);

	CString str;
	str.Format("%I64d", g_FileID);
	SendUTF8(str, 1002, p->pSock);
	delete p;
	//AfxMessageBox("Finish");
	return 0L;
}


void CMainFrame::GetAString(vector<A_Step>& vASteps, CString& strA)
{
	CString strTemp;
	CString strFrame, strCh;
	uint16_t count = 0, ichannelCount = 0;
	int iFrameIndex = 0;
	for (int i = 0; i < vASteps.size(); ++i)
	{
		vASteps[i].Index2 = iFrameIndex++;
	}
	for (int i = 0; i < vASteps.size(); ++i)
	{
		strFrame = "";
		ichannelCount = 0;
		A_Step& step = vASteps[i];
		for (int k = 0; k < CH_N; ++k)
		{
			count = 0;
			strCh.Format("%c:[", ChannelNames[k]);
			for (int j = 0; j < step.Frames.size(); ++j)
			{
				if (step.Frames[j].F[k] > 0)
				{
					++count;
					strTemp.Format("[%d,%d],", step.Frames[j].Horizon, step.Frames[j].F[k]);
					strCh += strTemp;
				}
			}
			if (count > 0)
			{
				++ichannelCount;
				strCh = strCh.Left(strCh.GetLength() - 1) + "],";
				strFrame += strCh;
			}
		}
		strFrame = strFrame.Left(strFrame.GetLength() - 1);
		strTemp.Format(_T("%d:[%d,{%s}],"), step.Step, step.Index2, strFrame);
		strA += strTemp;
	}	
	if (vASteps.size() > 1)
	{
		strA = strA.Left(strA.GetLength() - 1);
	}
	strA = "{" + strA + "}";
}

void CMainFrame::GetBString(vector<BlockData_B>& datas, CString& strB)
{
	//C, c, D, d, E, e, F, G
	vector<WaveData> wdata[16];	//16个通道的连通域
	const uint16_t bits[16] = { BIT0, BIT1, BIT2, BIT3, BIT4, BIT5, BIT6, BIT7, BIT8, BIT9, BIT10, BIT11, BIT12, BIT13, BIT14, BIT15 };

	//建立A1, A2, a1, a2, B1, B2, b1, b2
	WaveData wd;
	for (int j = 0; j < datas.size(); ++j)
	{
		for (int step = 0; step < datas[j].vBStepDatas.size(); ++step)
		{
			int railType = datas[j].BlockHead.railType & 0x03;
			for (int row = 0; row < datas[j].vBStepDatas[step].vRowDatas.size(); ++row)
			{
				for (int m = 0; m < 16; ++m)
				{
					if (datas[j].vBStepDatas[step].vRowDatas[row].Point.Draw1 & bits[m])
					{
						wd.block = datas[j].Index;
						wd.step = datas[j].vBStepDatas[step].Step;
						wd.row = datas[j].vBStepDatas[step].vRowDatas[row].Row;
						wdata[m].push_back(wd);
					}
				}
			}
		}
	}

	CString strTemp, strChannel[16];
	strB = "{";
	for (int m = 0; m < 16; ++m)
	{
		strTemp.Format("%s:[", ChannelNamesB[m]);
		strChannel[m] = strTemp;
		for (int i = 0; i < wdata[m].size(); ++i)
		{
			strTemp.Format("[%d,%d],", wdata[m][i].step, wdata[m][i].row);
			strChannel[m] += strTemp;
		}
		if (wdata[m].size() > 0)
		{
			strChannel[m] = strChannel[m].Left(strChannel[m].GetLength() - 1) + "],";
			strB += strChannel[m];
		}
	}
	if (strB.Right(1) == ",")
	{
		strB = strB.Left(strB.GetLength() - 1) + "}";
	}
}

CString	CMainFrame::GetWoundData(Wound_Judged& wd)
{
	vector<BLOCK_SQL> blocks;
	if (!theApp.sql.GetBlocks(wd.FileID, blocks))
	{
		return "";
	}

	CString strSql;
	strSql.Format("select data_path from f_head where id = %I64d", wd.FileID);
	MYSQL_RES *rs;
	if (!theApp.sql.Query(strSql, &rs))
	{
		return "";
	}
	MYSQL_ROW row = mysql_fetch_row(rs);
	if (row == NULL)
	{
		return "";
	}
	CString strFilePath(row[0]);
	F_HEAD fHead;
	BlockData_A dataA;
	vector<BlockData_B> dataB;
	CString strFileA = "D:/Files/" + strFilePath + ".tpA";
	CString strFileB = "D:/Files/" + strFilePath + ".tpB";
	if (ReadABData2(strFileA, strFileB, blocks, wd.Block, fHead, dataA, dataB) == 0)
	{
		return "";
	}

	CString strAngle = "{", strBigGate,strSmallGate;
	CString strTemp;
	for (int i = 0; i < CH_N; ++i)
	{
		strTemp.Format("%c:%d,", ChannelNames[i], fHead.deviceP2.Angle[i].Refrac);
		strAngle += strTemp;
	}
	strAngle = strAngle.Left(strAngle.GetLength() - 1) + "}";
	CString strColor = "{A1:'#FF66FF',A2:'#CC33CC',a1:'#FF0000',a2:'#990000',B1:'#00FFFF',B2:'#00CCCC',b1:'#0066FF',b2:'#003399',C:'#00FF00',c:'#CD853F',D:'#C71585',E:'#7FFF00',F:'#7FFFD4',G:'#6B8E23'}";

	int idx = -1;
	for (int i = 0; i < dataB.size(); ++i)
	{
		if (dataB[i].Index == wd.Block)
		{
			idx = i;
			break;
		}
	}

	CString strOffset = "{";
	for (int i = 0; i < CH_N; ++i)
	{
		strTemp.Format("%c:%d,", ChannelNames[i], fHead.deviceP2.Place[i] + dataB[idx].BlockHead.probOff[i]);
		strOffset += strTemp;
	}
	strOffset = strOffset.Left(strOffset.GetLength() - 1) + "}";

	CString strA, strB;	
	GetAString(dataA.vAStepDatas, strA);
	GetBString(dataB, strB);

	CString strData;
	strData.Format("{step_len:%f,color:%s,angle:%s,offset:%s,A_data:%s,B_data:%s}", fHead.step, strColor, strAngle, strOffset, strA, strB);
	return strData;
}

CString CMainFrame::GetMultiCycleWoundData(uint64_t iWoundID)
{
	Wound_Judged wd, wd2, wd3;
	theApp.sql.GetWound(iWoundID, wd);
	CString strData, strData2, strData3;
	strData = GetWoundData(wd);

	if (wd.LastCycleID > 0)
	{	
		theApp.sql.GetWound(wd.LastCycleID, wd2);
		strData2 = GetWoundData(wd2);

		if (wd2.LastCycleID > 0)
		{			
			theApp.sql.GetWound(wd2.LastCycleID, wd3);
			strData3 = GetWoundData(wd3);
		}
	}	

	CString str;
	str.Format("[%s,%s,%s]", strData, strData2, strData3);
	return str;
}