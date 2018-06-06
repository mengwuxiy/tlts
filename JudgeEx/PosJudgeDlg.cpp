
// TLTSDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "PosJudge.h"
#include "PosJudgeDlg.h"
#include "afxdialogex.h"

#include "Judge.h"

#include <process.h>
#include <fstream>
#include <io.h>

#include "DlgLocate.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define TIMER_SOLVINGDATA 1

// CPosJudgeDlg 对话框

const uint32_t bits[32] =
{ BIT0, BIT1, BIT2, BIT3, BIT4, BIT5, BIT6, BIT7, BIT8, BIT9,
BIT10, BIT11, BIT12, BIT13, BIT14, BIT15, BIT16, BIT17, BIT18, BIT19,
BIT20, BIT21, BIT22, BIT23, BIT24, BIT25, BIT26, BIT27, BIT28, BIT29,
BIT30, BIT31
};

const int clrChannels[][4] = {
	{ 255, 105, 180, 1 },
	{ 176, 48, 96, 1 },
	{ 255, 0, 0, 1 },
	{ 205, 38, 38, 1 },

	{ 0, 255, 255, 1 },
	{ 95, 158, 160, 1 },
	{ 0, 191, 255, 1 },
	{ 0, 0, 255, 1 },

	{ 0, 255, 0, 1 },//C
	{ 255, 193, 371, 1 },//c

	{ 255, 105, 180, 1 },
	{ 0, 0, 0, 1 },

	{ 0, 255, 0, 1 },
	{ 0, 0, 0, 1 },
	{ 0, 255, 255, 1 },
	{ 205, 186, 150, 1 }
};

const uint8_t clrBK[] = { 205, 205, 205 };

const CString strPMTypes[] = { "", "厂焊", "铝热焊", "端面", "螺孔", "导孔", "其他", "zz" };

uint32_t pmCounts[7] = { 0 };


uint32_t wdCount[10] = { 0 };


// 四种轨型轨头内的内外70度反射点的高度mm）
UINT16 rail_hDC[4] = { 33, 35, 38, 46 };

// 轨底高度
UINT16 rail_uDC[4] = { 140, 152, 176, 192 };

float g_A_L1[4] = { 78.780945, 82.996033, 90.973236, 109.27811 };

//四种轨型的70度角的三角函数值
TRIANGLE g_a[4] = { { 0.89503974, 0.44598642, 2.0068767 },
{ 0.90143627, 0.43291181, 2.082263 },
{ 0.89503974, 0.44598642, 2.0068767 },
{ 0.90143627, 0.43291181, 2.082263 } };


CPosJudgeDlg::CPosJudgeDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CPosJudgeDlg::IDD, pParent)
	, m_szFileB(_T(""))
	, m_strBeginTime(_T(""))
	, m_strEndTime(_T(""))
	, m_strProgress(_T(""))
	, m_strTime(_T(""))
	, m_strRetOld(_T(""))
	, m_strRetNew(_T(""))
	, m_strProgress2(_T(""))
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);

	m_pFileA = NULL;
	m_pFileB = NULL;

	m_bSolving = FALSE;
	m_bOpen = FALSE;
	m_bLoaded = FALSE;
	m_bReadingB = FALSE;

	m_iCurrentMark = -1;
	m_iBlock = -1;
	m_iStep = -1;


	m_iCurrentWound = -1;	
	m_iCurrentBlock = -1;

	m_BrushBK = new CBrush(RGB(0, 0, 0));

	m_bExported = FALSE;

	m_iMode = 0;


	m_jugedHandle = CreateEvent(NULL, TRUE, TRUE, "Judge");
	m_bJudging = false;
	m_bExit = FALSE;
}

void CPosJudgeDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EDIT_PATH, m_strPath);
	DDX_Text(pDX, IDC_EDIT_TPB, m_szFileB);
	DDX_Text(pDX, IDC_EDIT3, m_strBeginTime);
	DDX_Text(pDX, IDC_EDIT4, m_strEndTime);
	DDX_Text(pDX, IDC_CURRENT, m_strProgress);
	DDX_Control(pDX, IDC_LIST1, m_oList);
	DDX_Control(pDX, IDC_WAVE, m_oPlot);
	DDX_Control(pDX, IDC_WAVE2, m_oPlot2);
	DDX_Control(pDX, IDC_TIPS, m_oTips);
	DDX_Text(pDX, IDC_STATIC_TIME, m_strTime);
	DDX_Text(pDX, IDC_R1, m_strRetOld);
	DDX_Text(pDX, IDC_R2, m_strRetNew);

	DDX_Text(pDX, IDC_EDIT_BLOCK, m_iBlock);
	DDX_Text(pDX, IDC_EDIT_STEP, m_iStep);
	DDX_Text(pDX, IDC_CURRENT2, m_strProgress2);
}

BEGIN_MESSAGE_MAP(CPosJudgeDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDB_Judge, &CPosJudgeDlg::OnBnClickedJudge)
	ON_BN_CLICKED(IDC_BUTTON2, &CPosJudgeDlg::OnBnClickedBrowse)

	ON_MESSAGE(WM_WEBINFO_IN, OnMessageNet)
	ON_MESSAGE(WM_WEBINFO_RECREATE, OnMessageReCreateWebCom)
	ON_MESSAGE(WM_FINISH, ONMessageFinish)
	ON_MESSAGE(WM_STEP, ONMessageStep)

	ON_BN_CLICKED(IDB_EXPORT, &CPosJudgeDlg::OnBnClickedExport)
	ON_WM_CTLCOLOR()
	ON_WM_TIMER()
	ON_BN_CLICKED(IDB_LOCATE, &CPosJudgeDlg::OnBnClickedLocate)

	ON_WM_INITMENUPOPUP()

	ON_WM_CONTEXTMENU()

	ON_COMMAND(ID_WD_CHECK, &CPosJudgeDlg::OnWdCheck)
	ON_COMMAND(ID_POS_CHECK, &CPosJudgeDlg::OnPosCheck)
	ON_UPDATE_COMMAND_UI(ID_WD_CHECK, &CPosJudgeDlg::OnUpdateWdCheck)
	ON_UPDATE_COMMAND_UI(ID_POS_CHECK, &CPosJudgeDlg::OnUpdatePosCheck)
	ON_WM_WINDOWPOSCHANGING()
	ON_WM_CLOSE()
END_MESSAGE_MAP()

void CPosJudgeDlg::OnInitMenuPopup(CMenu *pPopupMenu, UINT nIndex, BOOL bSysMenu)
{
	ASSERT(pPopupMenu != NULL);
	// Check the enabled state of various menu items.

	CCmdUI state;
	state.m_pMenu = pPopupMenu;
	ASSERT(state.m_pOther == NULL);
	ASSERT(state.m_pParentMenu == NULL);

	// Determine if menu is popup in top-level menu and set m_pOther to
	// it if so (m_pParentMenu == NULL indicates that it is secondary popup).
	HMENU hParentMenu;
	if (AfxGetThreadState()->m_hTrackingMenu == pPopupMenu->m_hMenu)
		state.m_pParentMenu = pPopupMenu;    // Parent == child for tracking popup.
	else if ((hParentMenu = ::GetMenu(m_hWnd)) != NULL)
	{
		CWnd* pParent = this;
		// Child windows don't have menus--need to go to the top!
		if (pParent != NULL &&
			(hParentMenu = ::GetMenu(pParent->m_hWnd)) != NULL)
		{
			int nIndexMax = ::GetMenuItemCount(hParentMenu);
			for (int nIndex = 0; nIndex < nIndexMax; nIndex++)
			{
				if (::GetSubMenu(hParentMenu, nIndex) == pPopupMenu->m_hMenu)
				{
					// When popup is found, m_pParentMenu is containing menu.
					state.m_pParentMenu = CMenu::FromHandle(hParentMenu);
					break;
				}
			}
		}
	}

	state.m_nIndexMax = pPopupMenu->GetMenuItemCount();
	for (state.m_nIndex = 0; state.m_nIndex < state.m_nIndexMax;
		state.m_nIndex++)
	{
		state.m_nID = pPopupMenu->GetMenuItemID(state.m_nIndex);
		if (state.m_nID == 0)
			continue; // Menu separator or invalid cmd - ignore it.

		ASSERT(state.m_pOther == NULL);
		ASSERT(state.m_pMenu != NULL);
		if (state.m_nID == (UINT)-1)
		{
			// Possibly a popup menu, route to first item of that popup.
			state.m_pSubMenu = pPopupMenu->GetSubMenu(state.m_nIndex);
			if (state.m_pSubMenu == NULL ||
				(state.m_nID = state.m_pSubMenu->GetMenuItemID(0)) == 0 ||
				state.m_nID == (UINT)-1)
			{
				continue;       // First item of popup can't be routed to.
			}
			state.DoUpdate(this, TRUE);   // Popups are never auto disabled.
		}
		else
		{
			// Normal menu item.
			// Auto enable/disable if frame window has m_bAutoMenuEnable
			// set and command is _not_ a system command.
			state.m_pSubMenu = NULL;
			state.DoUpdate(this, FALSE);
		}

		// Adjust for menu deletions and additions.
		UINT nCount = pPopupMenu->GetMenuItemCount();
		if (nCount < state.m_nIndexMax)
		{
			state.m_nIndex -= (state.m_nIndexMax - nCount);
			while (state.m_nIndex < nCount &&
				pPopupMenu->GetMenuItemID(state.m_nIndex) == state.m_nID)
			{
				state.m_nIndex++;
			}
		}
		state.m_nIndexMax = nCount;
	}
}

// CPosJudgeDlg 消息处理程序

BOOL CPosJudgeDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 设置此对话框的图标。当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	LoadConfig();

	m_pen.CreatePen(PS_DASHDOT, 1, RGB(0x2F, 0x2F, 0x2F));
	m_pen2.CreatePen(PS_DASHDOT, 1, RGB(0x7F, 0x7F, 0x7F));

	// TODO: 在此添加额外的初始化代码

	LOGFONT logFont;
	m_fontTip.CreateFont(-24, 0, 0, 0, 400, FALSE, FALSE,
		0, GB2312_CHARSET, OUT_DEFAULT_PRECIS,//ANSI_CHARSET
		CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
		VARIABLE_PITCH | FF_SCRIPT, "楷体_GB2312");
	m_oTips.SetFont(&m_fontTip, 1);
	GetDlgItem(IDC_RET_OLD)->SetFont(&m_fontTip, 1);
	GetDlgItem(IDC_RET_NEW)->SetFont(&m_fontTip, 1);

	GetDlgItem(IDC_R1)->SetFont(&m_fontTip, 1);
	GetDlgItem(IDC_R2)->SetFont(&m_fontTip, 1);

	m_oList.SetExtendedStyle(m_oList.GetExtendedStyle() | LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_SHOWSELALWAYS);
	m_oList.SetTextBkColor(RGB(202, 202, 202));

	SetMode(m_iMode);

	CreateWebCom();

	//Wound_Judged wd;
	//theApp.sql.GetWound(134, wd);
	//CString str = GetWoundData(wd);
	return 0;

	//m_oSplit.SetDropDownMenu(IDR_MENU1, 0);
	return FALSE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CPosJudgeDlg::SetMode(int iMode)
{
	m_iMode = iMode;

	m_oList.DeleteAllItems();
	m_oList.DeleteColumn(1);
	m_oList.DeleteColumn(0);
	if (m_iMode == 0)
	{

		m_oList.InsertColumn(0, "位置类型", 0, 150);
		m_oList.InsertColumn(1, "标记数量", 0, 70);
		for (int i = 1; i < 7; ++i)
		{
			m_oList.InsertItem(i - 1, strPMTypes[i]);
			CString strT;
			strT.Format("%d", pmCounts[i]);
			m_oList.SetItemText(i - 1, 1, strT);
		}
		m_oTips.SetWindowText(_T("各种标记含义：\r\n\r\n1: 厂焊\r\n2: 铝热焊\r\n3: 端面\r\n4: 螺孔\r\n5: 导孔\r\n6: 其他\r\n"));
	}
	else if (m_iMode == 1)
	{
		m_oList.InsertColumn(0, "伤损类型", 0, 150);
		m_oList.InsertColumn(1, "伤损数量", 0, 70);
		for (int i = 0; i < 10; ++i)
		{
			m_oList.InsertItem(i, g_strTypeDefines[i]);
			CString strT;
			strT.Format("%d", wdCount[i]);
			m_oList.SetItemText(i, 1, strT);
		}
		m_oTips.SetWindowText(_T("各种伤损含义：\r\n\r\n1: 厂焊焊缝\r\n2: 铝热焊缝\r\n3: 轨头核伤\r\n4: 鱼鳞伤\r\n5: 螺孔斜裂纹\r\n6: 螺孔水平裂纹\r\n7: 轨腰（轨颚）水平裂纹\r\n8: 轨腰斜裂纹\r\n9: 轨底横向裂纹\r\n0: 其他"));
	}
	m_bExported = FALSE;
	m_oList.UpdateWindow();
}

void CPosJudgeDlg::LoadConfig()
{
	TCHAR sz[MAX_PATH];
	::GetModuleFileName(NULL, sz, MAX_PATH);
	CString strPath(sz);
	m_strExeFolder = strPath.Left(strPath.ReverseFind('\\'));

	m_strRetFolder = m_strExeFolder.Mid(m_strExeFolder.ReverseFind('\\') + 1);

	//ifstream file;
	//file.open(m_strExeFolder + "\\config.ini");
	//if (!file.is_open())
	//{
	for (int i = 0; i < 16; ++i)
	{
		m_R[i] = clrChannels[i][0];
		m_G[i] = clrChannels[i][1];
		m_B[i] = clrChannels[i][2];
		m_fWidth[i] = clrChannels[i][3];
	}

	m_vBbrush.clear();
	for (int i = 0; i < 16; ++i)
	{
		m_vBbrush.push_back(new CBrush(RGB(m_R[i], m_G[i], m_B[i])));
	}
	this->SetBackgroundColor(RGB(clrBK[0], clrBK[1], clrBK[2]), TRUE);

	/*
	FILE* pFileConfig = fopen(m_strExeFolder + "\\config.ini", "w");
	if (pFileConfig)
	{
	for (int i = 0; i < 16; ++i)
	{
	fprintf(pFileConfig, "%d\t%d\t%d\t%d\n", m_R[i], m_G[i], m_B[i], m_fWidth[i]);
	}
	fprintf(pFileConfig, "%d\t%d\t%d", clrBK[0], clrBK[1], clrBK[2]);
	fclose(pFileConfig);
	}
	*/
	/*}
	else
	{
	for (int i = 0; i < 16; ++i)
	{
	file >> m_R[i] >> m_G[i] >> m_B[i] >> m_fWidth[i];
	}

	m_vBbrush.clear();
	for (int i = 0; i < 16; ++i)
	{
	m_vBbrush.push_back(new CBrush(RGB(m_R[i], m_G[i], m_B[i])));
	}

	int r, g, b;
	file >> r >> g >> b;
	this->SetBackgroundColor(RGB(r, g, b), TRUE);
	}*/
	ShowMark(m_iCurrentMark);
}

void CPosJudgeDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	CDialogEx::OnSysCommand(nID, lParam);
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CPosJudgeDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
	//ShowMark(m_iCurrentMark);
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CPosJudgeDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
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

void CPosJudgeDlg::ExportWounds(vector<Wound_Judged>& vWounds, int count, CString strPath)
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

	for (int i = 0; i < 10; ++i)
	{
		if (wdCount[i] > 0)
		{
			fprintf(pFileOut, "%s:\t%d\n", (char*)(LPCTSTR)g_strTypeDefines[i], wdCount[i]);
		}
	}

	//CombineWound();


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

		/*
		for (int j = 0; j < vWounds[i].vCRs.size(); ++j)
		{
		fprintf(pFileOut, "%s%s，%6d %4d %6d %4d", CH_STR[vWounds[i].vCRs[j].Channel], Wave_STR[vWounds[i].vCRs[j].Region[0].find & BIT0], vWounds[i].vCRs[j].Step1, vWounds[i].vCRs[j].Row1, vWounds[i].vCRs[j].Step2, vWounds[i].vCRs[j].Row2);
		fprintf(pFileOut, "\t%6d米块%d步进 %4d\t[", vWounds[i].vCRs[j].Region[0].block + 1, vWounds[i].vCRs[j].Region[0].step, vWounds[i].vCRs[j].Step);
		fprintf(pFileOut, "]\n");
		}
		fprintf(pFileOut, "\n");
		*/
	}
	fclose(pFileOut);
}

void CPosJudgeDlg::ExportWoundsToExcel(vector<Wound_Judged>& vWounds, int count, CString strPath)
{
	bool bExist = IsFileExist(strPath);
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
			strTemp.Format("%.6lf", vWounds[i].Walk);
			m_oExcel.SetCellString(i + 2, 1, strTemp);

			//strTemp.Format("%s", strWoundTypes[vWounds[i].Type]);
			//AfxMessageBox(strTemp);
			m_oExcel.SetCellString(i + 2, 2, g_strTypeDefines[vWounds[i].Type]);

			strTemp.Format("%d", vWounds[i].Block);
			m_oExcel.SetCellString(i + 2, 4, strTemp);

			strTemp.Format("%d", vWounds[i].Step);
			m_oExcel.SetCellString(i + 2, 5, strTemp);

			strTemp.Format("%s", vWounds[i].According);
			m_oExcel.SetCellString(i + 2, 6, strTemp);
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

//
//void HeapAdjust(vector<Position_Mark>& vPMs, int count, int index, bool direction = true)
//{
//	int left = 2 * index + 1;
//	int right = 2 * index + 2;
//	int maxIdx = index;
//	if (direction)
//	{
//		if (left < count && vPMs[left].Walk >= vPMs[maxIdx].Walk)
//		{
//			maxIdx = left;
//		}
//		if (right < count && vPMs[right].Walk >= vPMs[maxIdx].Walk)
//		{
//			maxIdx = right;  // maxIdx是3个数中最大数的下标
//		}
//	}
//	else
//	{
//		if (left < count && vPMs[left].Walk <= vPMs[maxIdx].Walk)
//		{
//			maxIdx = left;
//		}
//		if (right < count && vPMs[right].Walk <= vPMs[maxIdx].Walk)
//		{
//			maxIdx = right;  // maxIdx是3个数中最大数的下标
//		}
//	}
//
//	if (maxIdx != index)                 // 如果maxIdx的值有更新
//	{
//		swap(vPMs[maxIdx], vPMs[index]);
//		HeapAdjust(vPMs, count, maxIdx, direction);       // 递归调整其他不满足堆性质的部分
//	}
//}


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

void CPosJudgeDlg::ExportPMs(vector<Position_Mark>& vPMs, int count, CString strPath)
{
	bool bExist = IsFileExist(strPath);
	if (bExist)
	{
		DeleteFile(strPath);
		Sleep(1);
	}
	//bool bFindJ = false, bFindS = false;
	//W_D w1, w2;
	//W_D wP, wP2;
	//wP.Km = 0; wP.m = 1; wP.mm = 500;
	//for (int i = 0; i < vPMs.size(); ++i)
	//{
	//	if (vPMs[i].Mark == PM_JOINT)
	//	{
	//		if (!bFindJ)
	//		{
	//			w1 = vPMs[i].Walk;
	//			bFindJ = true;
	//		}
	//		else
	//		{
	//			if (vPMs[i].Walk - w1 <= wP && w1 - vPMs[i].Walk <= wP)
	//			{
	//				vPMs[i].Data = 0x0F;
	//			}
	//			else
	//			{
	//				w1 = vPMs[i].Walk;
	//			}
	//		}
	//	}
	//
	//		
	//	if (vPMs[i].Mark == PM_SEW)
	//	{
	//		if (!bFindS)
	//		{
	//			w2 = vPMs[i].Walk;
	//			bFindS = true;
	//		}
	//		else
	//		{
	//			if (vPMs[i].Walk - w2 <= wP && w2 - vPMs[i].Walk <= wP)
	//			{
	//				vPMs[i].Data = 0x0F;
	//			}
	//			else
	//			{
	//				w2 = vPMs[i].Walk;
	//			}
	//		}
	//	}
	//}	

	FILE* pFileOut = fopen((char*)(LPCTSTR)strPath, "w+");
	if (pFileOut == NULL)
	{
		AfxMessageBox(_T("结果打开失败！！"));
		return;
	}

	for (int i = 0; i < 7; ++i)
	{
		if (pmCounts[i] > 0)
		{
			fprintf(pFileOut, "%s:\t%3d\n", strPMTypes[i], pmCounts[i]);
		}
	}

	//CombinePM();

	time_t timec;
	tm* p;
	time(&timec);
	p = localtime(&timec);
	fprintf(pFileOut, "%4d-%02d-%02d %02d:%02d:%02d\n", p->tm_year + 1900, 1 + p->tm_mon, p->tm_mday, p->tm_hour, p->tm_min, p->tm_sec);

	for (int i = 0; i < count; ++i)
	{
		if (vPMs[i].Flag == 1)
		{
			continue;
		}

		CString strTpye1, strType2;
		if (vPMs[i].Mark >= 1 && vPMs[i].Mark <= 5)
		{
			strTpye1 = strPMTypes[vPMs[i].Mark];
		}
		else
		{
			strTpye1 = strPMTypes[6];
		}

		if (vPMs[i].Mark2 >= 1 && vPMs[i].Mark2 <= 5)
		{
			strType2 = strPMTypes[vPMs[i].Mark2];
		}
		else
		{
			strType2 = strPMTypes[6];
		}
		if (vPMs[i].Mark == PM_JOINT || vPMs[i].Mark == PM_SEW_CH || vPMs[i].Mark == PM_SEW_LRH)
		{
			fprintf(pFileOut, "%.3lf\t%s\t%s\t%4d\t%3d\t", vPMs[i].Walk, strTpye1, strType2, vPMs[i].Block, vPMs[i].Step);
			fprintf(pFileOut, "左导(%d),左螺(%d),右螺(%d),右导(%d)\n", vPMs[i].Data2 >> 12, (vPMs[i].Data2 & 0x0F00) >> 8, (vPMs[i].Data2 & 0x00F0) >> 4, vPMs[i].Data2 & 0x000F);
		}
		else
		{
			fprintf(pFileOut, "%.3lf\t%s\t%s\t%4d\t%3d\n", vPMs[i].Walk, strTpye1, strType2, vPMs[i].Block, vPMs[i].Step);
		}
	}
	fclose(pFileOut);



#ifdef EXPORT_EXCEL
	try
	{
		const char CH_STR[16][5] = { "", "焊缝", "接头", "道岔", "高轨", "低轨", "螺孔", "导孔", "C", "c", "D", "d", "E", "e", "F", "G" };


		CString strTitles[] = { "里程", "类型", "米块", "步进", "步进差", "行差", "出波通道数", "A1", "A2", "a1", "a2", "B1", "B2", "b1", "b2", "C", "c", "D", "d", "E", "e", "F", "G",
			"面积", "平均步进", "平均行", "方差" };
		long width[] = { 100, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20 };
		m_oExcel.CreateExcelFile();


		for (int i = 0; i < 27; ++i)
		{
			m_oExcel.SetCellString(1, i + 1, strTitles[i]);

		}

		for (int i = 0; i < vPMs.size(); ++i)
		{
			CString strTemp;
			strTemp.Format("%4dKm%dm%dmm", vPMs[i].Walk.Km, vPMs[i].Walk.m, vPMs[i].Walk.mm);
			m_oExcel.SetCellString(i + 2, 1, strTemp);

			m_oExcel.SetCellString(i + 2, 2, CH_STR[vPMs[i].Mark]);

			strTemp.Format("%d", vPMs[i].Block);
			m_oExcel.SetCellString(i + 2, 3, strTemp);

			strTemp.Format("%d", vPMs[i].Step);
			m_oExcel.SetCellString(i + 2, 4, strTemp);

			strTemp.Format("%d", vPMs[i].Length);
			m_oExcel.SetCellString(i + 2, 5, strTemp);

			strTemp.Format("%d", vPMs[i].Height);
			m_oExcel.SetCellString(i + 2, 6, strTemp);

			strTemp.Format("%d", vPMs[i].ChannelNum);
			m_oExcel.SetCellString(i + 2, 7, strTemp);

			for (int j = 0; j < 16; ++j)
			{
				strTemp.Format("%d", vPMs[i].Num[j]);
				m_oExcel.SetCellString(i + 2, 8 + j, strTemp);
			}

			strTemp.Format("%d", vPMs[i].Size);
			m_oExcel.SetCellString(i + 2, 24, strTemp);

			strTemp.Format("%d", vPMs[i].AStep);
			m_oExcel.SetCellString(i + 2, 25, strTemp);

			strTemp.Format("%d", vPMs[i].ARow);
			m_oExcel.SetCellString(i + 2, 26, strTemp);

			strTemp.Format("%.1lf", vPMs[i].Fangcha);
			m_oExcel.SetCellString(i + 2, 27, strTemp);
		}



		for (int i = 0; i < 4; ++i)
		{
			m_oExcel.SetCellString(1, i + 1, strTitles[i]);
		}

		for (int i = 0; i < vPMs.size(); ++i)
		{
			CString strTemp;
			strTemp.Format("%4dKm%dm%dmm", vPMs[i].Walk.Km, vPMs[i].Walk.m, vPMs[i].Walk.mm);
			m_oExcel.SetCellString(i + 2, 1, strTemp);

			m_oExcel.SetCellString(i + 2, 2, strTypes[vPMs[i].Mark]);

			strTemp.Format("%d", vPMs[i].Block);
			m_oExcel.SetCellString(i + 2, 3, strTemp);

			strTemp.Format("%d", vPMs[i].Step);
			m_oExcel.SetCellString(i + 2, 4, strTemp);
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
#endif // EXPORT_EXCEL	
}

void CPosJudgeDlg::ExportPMsToExcel(vector<Position_Mark>& vPMs, int count, CString strPath)
{
	bool bExist = IsFileExist(strPath);
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

		int k = 2;
		for (int i = 0; i < vPMs.size(); ++i)
		{
			if (vPMs[i].Mark != PM_SEW_LRH)
			{
				continue;
			}

			CString strTemp;
			strTemp.Format("%.6lf", vPMs[i].Walk);
			m_oExcel.SetCellString(k, 1, strTemp);

			if (vPMs[i].Mark >= 1 && vPMs[i].Mark <= 5)
			{
				m_oExcel.SetCellString(k, 2, strPMTypes[vPMs[i].Mark]);
			}
			else
			{
				m_oExcel.SetCellString(k, 2, "其他");
			}

			/*
			if (vPMs[i].Mark2 >= 1 && vPMs[i].Mark2 <= 5)
			{
			m_oExcel.SetCellString(i + 2, 3, strPMTypes[vPMs[i].Mark2]);
			}
			else
			{
			m_oExcel.SetCellString(i + 2, 3, "其他");
			}
			*/
			strTemp.Format("%d", vPMs[i].Block);
			m_oExcel.SetCellString(k, 3, strTemp);

			strTemp.Format("%d", vPMs[i].Step);
			m_oExcel.SetCellString(k, 4, strTemp);

			strTemp.Format("%lf", vPMs[i].gps_log);
			m_oExcel.SetCellString(k, 5, strTemp);

			strTemp.Format("%lf", vPMs[i].gps_lat);
			m_oExcel.SetCellString(k, 6, strTemp);
			k++;
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



void DoJudge(void* param)
{
	CPosJudgeDlg* pFrame = (CPosJudgeDlg*)param;

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
			pFrame->SendWounds(vWounds);
		}
		pFrame->m_iCurrentBlock += N_BLOCKREAD;
		if (pFrame->m_vInfoB[pFrame->m_vInfoB.size() - 1].UsedL >= pFrame->m_iSizeB)
		{
			break;
		}
	}

	//AfxMessageBox("Finished");
	if (!pFrame->m_bJudging)
	{
		return;
	}

	HeapSort(pFrame->m_vPMs, pFrame->m_vPMs.size(), false);

	//S5 伤损根据里程排序
	HeapSort(pFrame->m_vWounds, pFrame->m_vWounds.size(), false);

	pFrame->CloseFileAB();

	::PostMessage(pFrame->m_hWnd, WM_FINISH, 0, 0);
}

void CPosJudgeDlg::JudgeTPB(CString strTpB)
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
		SendUTF8(str, 1002);
		return;
	}

	//AfxMessageBox("Begin");
	CString strSend;
	strSend.Format("{rail:{rail_name:'%s',xingbie:'%s',gubie:'%s',s_mil:%.0lf,e_mil:%.0lf,log:%lf,lat:%lf}}", g_strRailName, g_strXingbie, g_strGubie, g_startPos, g_endPos, gps_log, gps_lat);
	SendUTF8(strSend, 1000);
	/*
	const char* pSource = (const char*)(LPCTSTR)strSend;
	int nLength = MultiByteToWideChar(CP_ACP, 0, pSource, -1, NULL, NULL);   // 获取缓冲区长度，再分配内存
	WCHAR *tch = new WCHAR[nLength];
	nLength = MultiByteToWideChar(CP_ACP, 0, pSource, -1, tch, nLength);     // 将MBCS转换成Unicode

	int nUTF8len = WideCharToMultiByte(CP_UTF8, 0, tch, nLength, 0, 0, 0, 0);   // 获取UTF-8编码长度
	char *utf8_string = new char[nUTF8len];
	WideCharToMultiByte(CP_UTF8, 0, tch, nLength, utf8_string, nUTF8len, 0, 0); //转换成UTF-8编码

	char * pSend = new char[nUTF8len + 15];
	sprintf_s(pSend, nUTF8len + 15, "%04d%011d%s", 1000, nUTF8len, utf8_string);
	m_ts.Write(pSend, nUTF8len + 15);
	//send(m_curSocket, utf8_string, nUTF8len, 0);
	delete pSend;
	delete utf8_string;
	delete tch;
	*/
	//m_ts.Write((char*)(LPCTSTR)strSend, strSend.GetLength());

	//CString strNewTpbPath = "D:\\Files\\" + g_strNewFileName + ".tpB";
	//rename(m_szFileB, strNewTpbPath);
	//rename(m_szFileB.Left(m_szFileB.GetLength() - 1) + "A", "D:\\Files\\" + g_strNewFileName + ".tpA");
	//m_szFileB = strNewTpbPath;

	int idx = m_szFileB.ReverseFind('/');
	g_strtpBFolder = m_szFileB.Left(idx);
	g_strtpBFileName = m_szFileB.Mid(idx + 1);
	int idx2 = g_strtpBFileName.ReverseFind('.');
	g_strtpBFileName = g_strtpBFileName.Left(idx2);

	if (FALSE == OpenFileAB())
	{
		m_bSolving = FALSE;
		SetEvent(m_jugedHandle);
		return;
	}
	m_iCurrentBlock = 0;

	uint32_t addr = 0;
	m_bExported = FALSE;
	m_bJudging = true;
	m_threadJudge = _beginthreadex(NULL, 0, (unsigned int(__stdcall *)(void *))DoJudge, this, 0, &addr);
}

void CPosJudgeDlg::StopJudge()
{
	m_bJudging = false;
}


BOOL CPosJudgeDlg::OpenFileAB()
{
	int idx = m_szFileB.ReverseFind('\\');
	m_strFolder = m_szFileB.Left(idx);
	m_strFileName = m_szFileB.Mid(idx + 1);

	int idx2 = m_strFileName.ReverseFind('.');
	m_strFileName = m_strFileName.Left(idx2);
	CString strFileA = m_strFolder + "\\" + m_strFileName + ".tpA";

	m_pFileA = fopen((char*)(LPCTSTR)strFileA, "rb");
	if (m_pFileA == NULL)
	{
		AfxMessageBox(_T("A打开失败！！"));
		return FALSE;
	}

	fseek(m_pFileA, 0L, SEEK_END);
	m_iSizeA = ftell(m_pFileA);

	m_pFileB = fopen((char*)(LPCTSTR)m_szFileB, "rb");
	if (m_pFileB == NULL)
	{
		AfxMessageBox(_T("B打开失败！！"));
		return FALSE;
	}
	fseek(m_pFileB, 0L, SEEK_END);
	m_iSizeB = ftell(m_pFileB);
	fseek(m_pFileB, 0L, SEEK_SET);
	fread(&m_Head, sizeof(F_HEAD), 1, m_pFileB);
	m_bOpen = TRUE;

	// 计算A超探头覆盖范围
	PROBE_COVERAGE_AREA ProbeCovArea[4][CH_N];          // A超探头覆盖范围数组[轨型][通道]
	double HOR_POINT_FOR_LENTH = 2.7;

	//float ForeheadLine[4] = {42, 42, 48.5, 55.3};

	int k;
	// 每种轨型分别计算
	for (int i = 0; i < 4; i++)
	{
		// 每个通道分别计算
		for (int j = 0; j < CH_N; j++)
		{
			// 得到探头方向
			if (m_Head.deviceP2.Angle[j].Refrac > 0)
			{
				ProbeCovArea[i][j].Direction = 1;
				k = 1;
			}
			else if (m_Head.deviceP2.Angle[j].Refrac < 0)
			{
				ProbeCovArea[i][j].Direction = -1;
				k = -1;
			}
			else
			{
				ProbeCovArea[i][j].Direction = 0;
				k = 0;
			}
			// 斜探头
			if (m_Head.deviceP2.Angle[j].Angle != 0)
			{
				// 第一部分
				ProbeCovArea[i][j].Range1 = g_A_L1[i] / HOR_POINT_FOR_LENTH;
				// 第二部分
				ProbeCovArea[i][j].Range2 = rail_hDC[i] * g_a[i].tan / HOR_POINT_FOR_LENTH;
			}
			// 直探头
			else
			{
				if (j == ACH_C || j == ACH_c)            // 直70°时
				{
					ProbeCovArea[i][j].Range1 = tan(k*m_Head.deviceP2.Angle[j].Refrac*3.14 / (10.0 * 180))*rail_uDC[i] / 2 / HOR_POINT_FOR_LENTH;
				}
				else
				{
					ProbeCovArea[i][j].Range1 = tan(k*m_Head.deviceP2.Angle[j].Refrac*3.14 / (10.0 * 180))*rail_uDC[i] / HOR_POINT_FOR_LENTH;
				}
				ProbeCovArea[i][j].Range2 = 0;
			}
		}
	}
	return TRUE;
}

void CPosJudgeDlg::CloseFileAB()
{
	fclose(m_pFileA); m_pFileA = NULL; m_iSizeA = 0;
	fclose(m_pFileB); m_pFileB = NULL; m_iSizeB = 0;
	m_bOpen = FALSE;
}


void CPosJudgeDlg::ShowBlock(int iBlock)
{
	if (m_bLoaded == FALSE)
	{
		return;
	}
	if (!m_bOpen)
	{
		OpenFileAB();
	}

	m_iStep = 0;
	UpdateData(FALSE);

	m_vBlocksToShow.clear();
	m_DataA.vAStepDatas.clear();
	m_vSteps.clear();

	if (ReadABData(iBlock) > 0)
	{
		DrawData(); DrawData2();
	}
}

void CPosJudgeDlg::ShowMark(int iPM)
{
	if (iPM < 0 || iPM >= m_vPMs.size())
	{
		return;
	}
	if (m_bLoaded == FALSE)
	{
		return;
	}
	m_iMode = 0;
	if (!m_bOpen)
	{
		OpenFileAB();
	}

	if (m_vPMs[iPM].Mark >= 1 && m_vPMs[iPM].Mark <= 5)
	{
		m_strRetOld = strPMTypes[m_vPMs[iPM].Mark];
	}
	else if (m_vPMs[iPM].Mark >= 6)
	{
		m_strRetOld = strPMTypes[6];
	}

	if (m_vPMs[iPM].Mark2 >= 1 && m_vPMs[iPM].Mark2 <= 5)
	{
		m_strRetNew = strPMTypes[m_vPMs[iPM].Mark2];
	}
	else if (m_vPMs[iPM].Mark2 >= 6)
	{
		m_strRetNew = strPMTypes[6];
	}

	m_iBlock = m_vPMs[iPM].Block + 1;
	m_iStep = m_vPMs[iPM].Step;
	UpdateData(FALSE);

	m_vBlocksToShow.clear();
	m_DataA.vAStepDatas.clear();
	m_vSteps.clear();
	int iBlock = m_vPMs[iPM].Block;
	m_iCurrentMark = iPM;

	if (ReadABData(iBlock) > 0)
	{
		DrawData(); DrawData2();
	}
}

void CPosJudgeDlg::ShowWound(int iWound)
{
	if (m_bLoaded == FALSE)
	{
		return;
	}
	if (iWound < 0 || iWound >= m_vWounds.size())
	{
		return;
	}

	if (!m_bOpen)
	{
		OpenFileAB();
	}

	m_strRetOld = g_strTypeDefines[m_vWounds[iWound].Type];
	m_strRetNew = g_strTypeDefines[m_vWounds[iWound].Type2];

	m_iBlock = m_vWounds[iWound].Block + 1;
	m_iStep = m_vWounds[iWound].Step;
	UpdateData(FALSE);

	m_vBlocksToShow.clear();
	m_DataA.vAStepDatas.clear();
	m_vSteps.clear();
	int iBlock = m_vWounds[iWound].Block;
	m_iCurrentWound = iWound;

	if (ReadABData(iBlock) > 0)
	{
		DrawData();
		DrawData2();
	}

	m_oPlot.SetFocus();
}

uint32_t CPosJudgeDlg::ReadABData(int iBlock)
{
	/*
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
	while (i < iEndBlock)
	{
	Analyse_Achao2(m_pFileA, m_vABlocksToShow, m_vInfoA[i].UsedL, m_iSizeA, i, m_vBlockHeads[i].indexL, 1);
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
	*/
	return 0;
}

LRESULT CPosJudgeDlg::ONMessageFinish(WPARAM wParam, LPARAM lParam)
{
	m_bSolving = FALSE;
	m_bJudging = false;

	CombineOutputData(m_vPMs, m_vWounds, m_vBlockHeads);

	AddBlocks(m_vBlockHeads, m_vInfoA, m_vInfoB);
	AddWounds(m_vWounds);
	AddPMs(m_vPMs);

	m_bLoaded = m_vInfoB.size() > 0;
	SetEvent(m_jugedHandle);

	CString str;
	str.Format("%I64d", g_FileID);
	SendUTF8(str, 1002);
	//AfxMessageBox("Finish");
	return 0L;

	m_bSolving = FALSE;
	KillTimer(TIMER_SOLVINGDATA);

	for (int i = 0; i < 7; ++i)
	{
		pmCounts[i] = 0;
	}

	for (int i = 0; i < m_vPMs.size(); ++i)
	{
		m_vPMs[i].Mark2 = m_vPMs[i].Mark;
		if (m_vPMs[i].Mark >= 1 && m_vPMs[i].Mark <= 5)
		{
			pmCounts[m_vPMs[i].Mark] ++;
		}
		else
		{
			pmCounts[6] ++;
		}
	}

	vector<Wound_Judged> vT;
	for (int i = 0; i < m_vWounds.size(); ++i)
	{
		m_vWounds[i].Type2 = m_vWounds[i].Type;
		if (m_vWounds[i].Flag == 0)
		{
			vT.push_back(m_vWounds[i]);
		}
	}
	m_vWounds = vT;
	m_iCurrentWound = -1;

	for (int i = 0; i < 10; ++i)
	{
		wdCount[i] = 0;
	}
	for (int i = 0; i < m_vWounds.size(); ++i)
	{
		wdCount[m_vWounds[i].Type] ++;
	}


	if (m_iMode == 0)
	{
		m_iCurrentMark = -1;
		for (int i = 0; i < 7; ++i)
		{
			CString strT;
			strT.Format(_T("%d"), pmCounts[i + 1]);
			m_oList.SetItemText(i, 1, strT);
		}
		m_strProgress2.Format("0/%d", m_vPMs.size());
	}
	else if (m_iMode == 1)
	{
		//CombineWound();		
		for (int i = 0; i < 10; ++i)
		{
			CString strT;
			strT.Format(_T("%d"), wdCount[i]);
			m_oList.SetItemText(i, 1, strT);
		}
		m_strProgress2.Format("0/%d", m_vWounds.size());
	}

	ExportPMsToExcel(m_vPMs, m_vPMs.size(), m_strExeFolder + _T("\\") + m_strFileName + _T("_pm.xlsx"));
	//ExportPMs(m_vPMs, m_vPMs.size(), m_strExeFolder + _T("\\") + m_strFileName + _T("_pm.txt"));
	ExportWoundsToExcel(m_vWounds, m_vWounds.size(), m_strExeFolder + _T("\\") + m_strFileName + _T("_wd.xlsx"));
	m_bLoaded = m_vInfoB.size() > 0;
	this->Invalidate();
	AfxMessageBox(_T("解析完成！"));
	return 0L;
}

LRESULT CPosJudgeDlg::ONMessageStep(WPARAM wParam, LPARAM lParam)
{
	if (m_iCurrentBlock < m_Head.block)
	{
		if (m_vInfoB.size() > 0)
		{
			m_strProgress.Format(_T("%d/%d"), m_iCurrentBlock, m_Head.block);
		}
	}
	else
	{
		m_Head.block = m_vInfoB.size();
		m_strProgress.Format(_T("%d/%d"), m_vInfoB.size(), m_Head.block);
	}
	UpdateData(FALSE);
	return 0L;
}

void CPosJudgeDlg::OnBnClickedJudge()
{
	// TODO: 在此添加控件通知处理程序代码
	if (m_bSolving)
	{
		AfxMessageBox(_T("当前正在解析文件，请稍后再试！", MB_OK | MB_ICONWARNING));
		return;
	}

	TravelDir(m_strPath + "\\*.*");

	m_bSolving = FALSE;
	AfxMessageBox(_T("解析完成！", MB_OK | MB_ICONINFORMATION));
}


void CPosJudgeDlg::OnBnClickedBrowse()
{
	// TODO: 在此添加控件通知处理程序代码
	OPENFILENAME ofn;
	TCHAR szPath[MAX_PATH];
	ZeroMemory(&ofn, sizeof(ofn));

	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = NULL;
	ofn.lpstrFile = szPath;
	ofn.lpstrFile[0] = '\0';
	ofn.nMaxFile = MAX_PATH;
	ofn.lpstrFilter = _T("tpB文件(*.tpB)\0*.tpB");
	ofn.nFilterIndex = 0;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

	if (GetOpenFileName(&ofn))
	{
		m_szFileB = ofn.lpstrFile;
		UpdateData(FALSE);
	}
}

void CPosJudgeDlg::DrawData()
{
	CRect rc;
	m_oPlot.GetClientRect(&rc);
	int iStepsToDrawCount = m_vSteps.size();

	int iBeginStep = 0, iEndStep = iStepsToDrawCount - 1;
	/*if (m_iStepIndex > 400)
	{
	iBeginStep = m_iStepIndex - 370;
	}

	if (m_iStepIndex + 370 < iStepsToDrawCount)
	{
	iEndStep = m_iStepIndex + 370;
	}
	iStepsToDrawCount = (iEndStep - iBeginStep);*/

	CDC *pDC = m_oPlot.GetDC();
	//pDC->SetBkMode(TRANSPARENT);
	//pDC->SetBkColor(RGB(0, 0, 0));
	pDC->FillRect(&rc, m_BrushBK);

	double dx = 1.0 * (rc.right - rc.left) / (iStepsToDrawCount - 1);
	double dy = 1.0 * (rc.bottom - rc.top - 10) / (65);

	double tx = rc.right - dx * (m_iStepIndex - iBeginStep);
	if (m_iMode == 2)
	{
		tx = 0;
	}

	int p = 0;
	for (int j = iBeginStep; j < iEndStep; ++j)
	{
		B_Step & s = m_vSteps[j];
		int x = rc.right - dx * p;
		for (int k = 0; k < s.vRowDatas.size(); ++k)
		{
			int y = rc.top + dy * s.vRowDatas[k].Row;
			CRect rc2(x, y, x + 1, y + dy);
			for (int m = 0; m < 14; ++m)
			{
				if (s.vRowDatas[k].Point.Draw1 & bits[m])
				{
					pDC->FillRect(&rc2, m_vBbrush[m]);
				}
			}

			if (s.vRowDatas[k].Point.Draw1 & bits[15])
			{
				pDC->FillRect(&rc2, m_vBbrush[15]);
			}

			if (s.vRowDatas[k].Point.Draw1 & bits[14])
			{
				pDC->FillRect(&rc2, m_vBbrush[14]);
			}
		}
		++p;
	}


	if (m_iMode == 0 && m_iCurrentMark >= 0 || m_iMode == 1 && m_iCurrentWound >= 0)
	{
		CPen * pPen = pDC->SelectObject(&m_pen);
		pDC->MoveTo(tx, 0);
		pDC->LineTo(tx, rc.bottom);
		pDC->SelectObject(pPen);
	}


	int dStep = 0;
	for (int i = 0; i < m_vBlocksToShow.size(); ++i)
	{
		dStep += m_vBlocksToShow[i].BlockHead.row;
		tx = rc.right - dx * dStep;
		CString str;
		str.Format(_T("%d"), m_vBlocksToShow[i].Index + 1);
		CRect rc2(tx + 5, rc.bottom - 20, tx + 65, rc.bottom);
		pDC->DrawText(str, rc2, DT_LEFT);
		CPen * pPen = pDC->SelectObject(&m_pen2);
		pDC->MoveTo(tx, rc.bottom - 10);
		if (tx > rc.left)
		{
			pDC->LineTo(tx, rc.bottom);
			pDC->SelectObject(pPen);
		}
	}
}


void CPosJudgeDlg::DrawData2()
{
	CRect rc;
	m_oPlot2.GetClientRect(&rc);
	int iStepsToDrawCount = m_vSteps.size();

	int iBeginStep = 0, iEndStep = iStepsToDrawCount - 1;
	if (m_iStepIndex > 50)
	{
		iBeginStep = m_iStepIndex - 50;
	}

	if (m_iStepIndex + 50 < iStepsToDrawCount)
	{
		iEndStep = m_iStepIndex + 50;
	}
	iStepsToDrawCount = (iEndStep - iBeginStep);

	CDC *pDC = m_oPlot2.GetDC();
	pDC->FillRect(&rc, m_BrushBK);

	double dx = 1.0 * (rc.right - rc.left) / (iStepsToDrawCount - 1);
	double dy = 1.0 * (rc.bottom - rc.top - 10) / (65);

	double tx = rc.right - dx * (m_iStepIndex - iBeginStep);

	int p = 0;
	for (int j = iBeginStep; j < iEndStep; ++j)
	{
		B_Step & s = m_vSteps[j];
		int x = rc.right - dx * p;
		for (int k = 0; k < s.vRowDatas.size(); ++k)
		{
			int y = rc.top + dy * s.vRowDatas[k].Row;
			CRect rc2(x, y, x + dx, y + dy);
			for (int m = 0; m < 14; ++m)
			{
				if (s.vRowDatas[k].Point.Draw1 & bits[m])
				{
					pDC->FillRect(&rc2, m_vBbrush[m]);
				}
			}

			if (s.vRowDatas[k].Point.Draw1 & bits[15])
			{
				pDC->FillRect(&rc2, m_vBbrush[15]);
			}

			if (s.vRowDatas[k].Point.Draw1 & bits[14])
			{
				pDC->FillRect(&rc2, m_vBbrush[14]);
			}
		}
		++p;
	}
	//if (m_iCurrentMark >= 0)
	//{
	//	CRect rcT(tx - 10, rc.bottom - 10, tx + 10, rc.bottom);
	//	pDC->Rectangle(&rcT);
	//}
}

void CPosJudgeDlg::OnBnClickedExport()
{


}


void CPosJudgeDlg::OnBnClickedLocate()
{
	// TODO: Add your control notification handler code here
	CDlgLocate dlg;
	if (dlg.DoModal() == IDOK)
	{
		if (m_iMode == 0)
		{
			for (int i = 0; i < m_vPMs.size(); ++i)
			{
				if (m_vPMs[i].Block + 1 >= dlg.m_iBlockToLocate)
				{
					m_iCurrentMark = i;
					break;
				}
			}
			ShowMark(m_iCurrentMark);
		}
		else if (m_iMode == 1)
		{
			for (int i = 0; i < m_vWounds.size(); ++i)
			{
				if (m_vWounds[i].Block + 1 >= dlg.m_iBlockToLocate)
				{
					m_iCurrentWound = i;
					break;
				}
			}
			ShowWound(m_iCurrentWound);
		}
	}
	m_oPlot.SetFocus();
}

bool CPosJudgeDlg::IsFileExist(CString strFilePath)
{
	int nRet = _access((char*)(LPCTSTR)strFilePath, 0);
	bool bExist = (0 == nRet || EACCES == nRet);
	return bExist;

}

BOOL CPosJudgeDlg::PreTranslateMessage(MSG* pMsg)
{
	// TODO: Add your specialized code here and/or call the base class
	if (pMsg->message == WM_KEYDOWN)
	{
		//CString str;
		//str.Format("%d", pMsg->wParam);
		//AfxMessageBox(str);
		if (m_iMode == 0 && pMsg->wParam >= VK_NUMPAD1 && pMsg->wParam <= VK_NUMPAD6)
		{
			if (m_bReadingB)
			{
				AfxMessageBox(_T("正在读取上一个位置的数据，请稍侯！"), MB_OK | MB_ICONWARNING);
				return TRUE;
			}

			if (m_iCurrentMark >= 0 || m_iCurrentMark < m_vPMs.size())
			{
				m_vPMs[m_iCurrentMark].Mark2 = pMsg->wParam - VK_NUMPAD0;
				m_strRetNew = strPMTypes[m_vPMs[m_iCurrentMark].Mark2];
				UpdateData(FALSE);
			}
			return TRUE;
		}
		else if (m_iMode == 1 && pMsg->wParam >= VK_NUMPAD0 && pMsg->wParam <= VK_NUMPAD9)
		{
			if (m_bReadingB)
			{
				AfxMessageBox(_T("正在读取上一个位置的数据，请稍侯！"), MB_OK | MB_ICONWARNING);
				return TRUE;
			}
			if (m_iCurrentWound >= 0 || m_iCurrentWound < m_vWounds.size())
			{
				m_vWounds[m_iCurrentWound].Type2 = pMsg->wParam - VK_NUMPAD0;
				m_strRetNew = g_strTypeDefines[m_vWounds[m_iCurrentWound].Type2];
				UpdateData(FALSE);
			}
			return TRUE;
		}
		else if (m_iMode == 0 && pMsg->wParam >= '1' && pMsg->wParam <= '6')//数字1~6
		{
			if (m_bReadingB)
			{
				AfxMessageBox(_T("正在读取上一个位置的数据，请稍侯！"), MB_OK | MB_ICONWARNING);
				return TRUE;
			}

			if (m_iCurrentMark >= 0 || m_iCurrentMark < m_vPMs.size())
			{
				m_vPMs[m_iCurrentMark].Mark2 = pMsg->wParam - 48;
				m_strRetNew = strPMTypes[m_vPMs[m_iCurrentMark].Mark2];
				UpdateData(FALSE);
			}
			return TRUE;
		}
		else if (m_iMode == 1 && pMsg->wParam >= '0' && pMsg->wParam <= '9')//数字1~6
		{
			if (m_bReadingB)
			{
				AfxMessageBox(_T("正在读取上一个位置的数据，请稍侯！"), MB_OK | MB_ICONWARNING);
				return TRUE;
			}

			if (m_iCurrentWound >= 0 || m_iCurrentWound < m_vWounds.size())
			{
				m_vWounds[m_iCurrentWound].Type2 = pMsg->wParam - '0';
				m_strRetNew = g_strTypeDefines[m_vWounds[m_iCurrentWound].Type2];
				UpdateData(FALSE);
			}
			return TRUE;
		}
		else if (pMsg->wParam == _T('L') && (::GetKeyState(VK_CONTROL) & 0x8000))
		{
			LoadConfig();
			return TRUE;
		}
		else if (pMsg->wParam == _T('S') && (::GetKeyState(VK_CONTROL) & 0x8000))
		{
			if (m_bLoaded == FALSE)
			{
				return CDialogEx::PreTranslateMessage(pMsg);
			}

			if (m_iMode == 0)
			{
				CString strPath = m_strExeFolder + _T("\\") + m_strFileName + _T("_") + m_strRetFolder + _T(".pt");
				int idx = 1;
				while (IsFileExist(strPath))
				{
					CString strIndex;
					strIndex.Format(_T("(%d)"), idx++);
					strPath = m_strExeFolder + "\\" + m_strFileName + "_" + m_strRetFolder + strIndex + ".pt";
				}
				ExportPMs(m_vPMs, m_iCurrentMark, strPath);
			}
			else if (m_iMode == 1)
			{
				CString strIndex;
				strIndex.Format(_T("(%d)"), m_iCurrentWound);
				CString strPath = m_strExeFolder + "\\" + m_strFileName + "_" + m_strRetFolder + strIndex + ".wd";
				ExportWounds(m_vWounds, m_iCurrentWound, strPath);
			}
			return TRUE;
		}
	}
	if (pMsg->message == WM_KEYUP)
	{
		if (m_bReadingB)
		{
			AfxMessageBox(_T("正在读取上一个位置的数据，请稍侯！"), MB_OK | MB_ICONWARNING);
			return TRUE;
		}
		else if (pMsg->wParam == VK_SPACE || pMsg->wParam == VK_LEFT)
		{
			if (m_bLoaded == FALSE)
			{
				return CDialogEx::PreTranslateMessage(pMsg);
			}
			else if (m_iMode == 0)
			{
				++m_iCurrentMark;
				if (m_iCurrentMark == m_vPMs.size())
				{
					--m_iCurrentMark;
					//CloseFileAB();
					GetDlgItem(IDC_BTN_BROWSE)->EnableWindow();
					GetDlgItem(IDB_Judge)->EnableWindow();
					if (m_bExported == FALSE)	//导出
					{
						CString strExcel = m_strExeFolder + _T("\\") + m_strFileName + _T("_") + m_strRetFolder + _T("位置.xlsx");
						ExportPMsToExcel(m_vPMs, m_vPMs.size(), strExcel);
						m_bExported = TRUE;
						AfxMessageBox("标定完成！", MB_OK | MB_ICONINFORMATION);
					}
				}
				else
				{
					ShowMark(m_iCurrentMark);
				}
				m_strProgress2.Format(_T("%d/%d"), m_iCurrentMark + 1, m_vPMs.size());
			}
			else if (m_iMode == 1)//wound
			{
				++m_iCurrentWound;
				if (m_iCurrentWound == m_vWounds.size())
				{
					--m_iCurrentWound;
					//CloseFileAB();
					GetDlgItem(IDC_BTN_BROWSE)->EnableWindow();
					GetDlgItem(IDB_Judge)->EnableWindow();
					if (m_bExported == FALSE)	//导出
					{
						CString strExcel = m_strExeFolder + _T("\\") + m_strFileName + _T("_") + m_strRetFolder + _T("伤损.xlsx");
						ExportWoundsToExcel(m_vWounds, m_vWounds.size(), strExcel);
						m_bExported = TRUE;
						AfxMessageBox("标定完成！", MB_OK | MB_ICONINFORMATION);
					}
				}
				else
				{
					ShowWound(m_iCurrentWound);
				}
				m_strProgress2.Format(_T("%d/%d"), m_iCurrentWound + 1, m_vWounds.size());
			}
			UpdateData(FALSE);
			m_oPlot.SetFocus();
			return TRUE;
		}
		else if (pMsg->wParam == VK_RIGHT)
		{
			if (m_iMode == 0)
			{
				--m_iCurrentMark;
				if (m_iCurrentMark < 0)
				{
					m_iCurrentMark = 0;
				}
				ShowMark(m_iCurrentMark);
				m_strProgress2.Format(_T("%d/%d"), m_iCurrentMark + 1, m_vPMs.size());
				UpdateData(FALSE);
			}
			else if (m_iMode == 1)
			{
				--m_iCurrentWound;
				if (m_iCurrentWound < 0)
				{
					m_iCurrentWound = 0;
				}
				ShowWound(m_iCurrentWound);
				m_strProgress2.Format(_T("%d/%d"), m_iCurrentWound + 1, m_vWounds.size());
				UpdateData(FALSE);
			}
			return TRUE;
		}
	}
	return CDialogEx::PreTranslateMessage(pMsg);
}


HBRUSH CPosJudgeDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	HBRUSH hbr = CDialogEx::OnCtlColor(pDC, pWnd, nCtlColor);

	// TODO:  Change any attributes of the DC here

	// TODO:  Return a different brush if the default is not desired
	if (pWnd->GetDlgCtrlID() == IDC_WAVE || pWnd->GetDlgCtrlID() == IDC_WAVE2)
	{
		return  (HBRUSH)::GetStockObject(BLACK_BRUSH);
	}
	else if (pWnd->GetDlgCtrlID() == IDC_R2)
	{
		pDC->SetTextColor(RGB(0xFF, 0, 0));
	}
	return hbr;
}


void CPosJudgeDlg::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: Add your message handler code here and/or call default
	if (nIDEvent == TIMER_SOLVINGDATA)
	{
		if (m_bSolving)
		{
			time_t time2;
			time(&time2);
			int d = difftime(time2, m_tBegin);
			m_strTime.Format(_T("%02d:%02d:%02d"), d / 3600, (d / 60) % 60, d % 60);
			UpdateData(FALSE);
		}
		else
		{

		}
	}
	CDialogEx::OnTimer(nIDEvent);
}


void CPosJudgeDlg::OnOK()
{
	// TODO: Add your specialized code here and/or call the base class

	//CDialogEx::OnOK();
}


void CPosJudgeDlg::CombinePM()
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
				if (wd - wdj >= 1 || m_vPMs[i].Block - m_vPMs[j].Block > 1)
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
				if (wdj - wd >= 1 || m_vPMs[j].Block - m_vPMs[i].Block > 1)
				{
					break;
				}
				else if (m_vPMs[i].Mark == PM_GUIDEHOLE && wd - pmLastJoint.Walk < 0.2 && wd - pmLastJoint.Walk >= -0.2)
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

void CPosJudgeDlg::CombineWound()
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
			if (wdj - wd >= 0.3 || wdj - wd <= -0.3)
			{
				break;
			}
			m_vWounds[j].Flag = 1;
			wd = m_vWounds[j].Walk;
		}
	}
}


void CPosJudgeDlg::OnContextMenu(CWnd* pWnd, CPoint point)
{
	// TODO: Add your message handler code here
	CPoint pt = point;
	CMenu menu;
	CMenu* PopupMenu = NULL;

	ScreenToClient(&pt);

	//加载菜单
	menu.LoadMenu(IDR_MENU1);

	//子菜单项
	//右键点击 弹出此子菜单项
	PopupMenu = menu.GetSubMenu(0);
	PopupMenu->TrackPopupMenu(TPM_RIGHTBUTTON | TPM_LEFTALIGN, point.x, point.y, this);
	if (m_iMode == 0)
	{
		PopupMenu->CheckMenuItem(ID_WD_CHECK, MF_BYCOMMAND | MF_UNCHECKED);
		PopupMenu->CheckMenuItem(ID_POS_CHECK, MF_BYCOMMAND | MF_CHECKED);
	}
	else if (m_iMode == 1)
	{
		PopupMenu->CheckMenuItem(ID_POS_CHECK, MF_BYCOMMAND | MF_UNCHECKED);
		PopupMenu->CheckMenuItem(ID_WD_CHECK, MF_BYCOMMAND | MF_CHECKED);
	}
}


void CPosJudgeDlg::OnWdCheck()
{
	// TODO: Add your command handler code here
	SetMode(1);
}


void CPosJudgeDlg::OnPosCheck()
{
	// TODO: Add your command handler code here
	SetMode(0);
}




void CPosJudgeDlg::OnUpdateWdCheck(CCmdUI *pCmdUI)
{
	// TODO: Add your command update UI handler code here
	pCmdUI->SetCheck(m_iMode == 1);
}


void CPosJudgeDlg::OnUpdatePosCheck(CCmdUI *pCmdUI)
{
	// TODO: Add your command update UI handler code here
	pCmdUI->SetCheck(m_iMode == 0);
}


void CPosJudgeDlg::OnWindowPosChanging(WINDOWPOS* lpwndpos)
{
	//lpwndpos->flags &= ~SWP_SHOWWINDOW;
	CDialogEx::OnWindowPosChanging(lpwndpos);

	// TODO: Add your message handler code here
}

LRESULT CPosJudgeDlg::OnMessageNet(WPARAM wParam, LPARAM lParam)
{
	m_iCommunicateState = 1;//正在处理信息
	char * temp = (char*)wParam;
	m_curSocket = (SOCKET)lParam;
	//send(m_curSocket, "123", 4, 0);

	//只处理来自主控软件的信息
	CString label(temp);
	CString strBytes = label.Left(11);
	CString strCode = label.Mid(11, 4);
	CString strData = label.Mid(15);
	if (strCode == "1000")
	{
		//AfxMessageBox("Socket Begin");
		this->JudgeTPB(strData);
	}
	else if (strCode == "1001")
	{
		uint64_t wID = strtoull(temp + 15, NULL, 10);
		SendWoundData(wID);
	}
	m_iCommunicateState = 0;//处理完成
	return 0;
}

LRESULT CPosJudgeDlg::OnMessageReCreateWebCom(WPARAM wParam, LPARAM lParam)
{
	CreateWebCom();
	return 0;
}

unsigned int __stdcall WebCommunicate(LPVOID lParam)
{
	CPosJudgeDlg *pMainFrm = (CPosJudgeDlg *)lParam;
	while (!pMainFrm->m_bExit)
	{
		Sleep(1);
		if (AfxGetApp()->m_pMainWnd->GetSafeHwnd() == NULL)
		{
			continue;
		}
		char recv[1000] = { 0 };
		if (pMainFrm->m_ts.Read(recv, sizeof(recv)) == FALSE)
		{
			break;
		}
	}
	return 0;
}

void CPosJudgeDlg::CreateWebCom()
{
	CString strDir = g_strModuleFolder + "\\mysql.cfg";
	CStdioFile file;
	file.Open(strDir, CFile::modeRead);
	CString strServer, strUser, strPwd, strDB;
	file.ReadString(strServer);
	file.ReadString(strUser);
	file.ReadString(strPwd);
	file.ReadString(strDB);

	CString strIP, strPort;
	file.ReadString(strIP);
	file.ReadString(strPort);
	int iport = StrToInt(strPort);
	BOOL bSuccess = m_ts.InitialServerSocket(5666);
	if (!bSuccess)
	{
		return;
	}
	//bSuccess = m_tsClient.InitialClientSocket(iport, (char*)(LPCTSTR)strIP);
	//if (!bSuccess)
	//{
	//	return;
	//}
	//BOOL bsend = m_tsClient.Write("123", 4);
	//unsigned int	threadID = 101;
	
	//第五个参数为0表示创建后立即执行
	m_MainControlThread = _beginthreadex(NULL, 20000000, (unsigned int(__stdcall*)(void*))WebCommunicate, this, 0, &m_threadID);//20M内存空间
}

void CPosJudgeDlg::OnClose()
{
	// TODO: Add your message handler code here and/or call default
	m_bExit = TRUE;
	m_ts.Clear();
	m_tsClient.Clear();
	CloseHandle(m_jugedHandle);
	CDialogEx::OnClose();
}

void CPosJudgeDlg::TravelDir(CString strPath)
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
				JudgeTPB(fdPath);//针对文件的操作

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



uint32_t CPosJudgeDlg::ReadABData2(CString strFileA, CString strFileB, vector<BLOCK_SQL>&blocks, int iBlock, F_HEAD& head, BlockData_A& vASteps, vector<BlockData_B>& datas)
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

void CPosJudgeDlg::GetAString(vector<A_Step>& vASteps, CString& strA)
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

void CPosJudgeDlg::GetBString(vector<BlockData_B>& datas, CString& strB)
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

CString	CPosJudgeDlg::GetWoundData(Wound_Judged& wd)
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
	ReadABData2("D:\\Files\\" + strFilePath + ".tpA", "D:\\Files\\" + strFilePath + ".tpB", blocks, wd.Block, fHead, dataA, dataB);

	CString strAngle = "{", strBigGate, strSmallGate;
	CString strTemp;
	for (int i = 0; i < CH_N; ++i)
	{
		strTemp.Format("%c:%d,", ChannelNames[i], fHead.deviceP2.Angle[i].Refrac);
		strAngle += strTemp;
	}
	strAngle = strAngle.Left(strAngle.GetLength() - 1) + "}";

	/*
	int iRailType = ;
	strBigGate.Format("{A:[[%d,%d],[%d,%d]],a:[[%d,%d],[%d,%d]],B:[[%d,%d],[%d,%d]],b:[[%d,%d],[%d,%d]],C:[[%d,%d],[%d,%d]],D:[[%d,%d],[%d,%d]],d:[[%d,%d],[%d,%d]],F:[[%d,%d],[%d,%d]],c:[[%d,%d],[%d,%d]],e:[[%d,%d],[%d,%d]],E:[[%d,%d],[%d,%d]],G:[[%d,%d],[%d,%d]]}",
	fHead.deviceP2.Gate[wd.railType][0].start, fHead.deviceP2.Gate[wd.railType][0].end,);
	*/
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


void CPosJudgeDlg::SendWounds(vector<Wound_Judged>& vWounds)
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

		/*
		strTemp.Format("],wtype_count_detail:{info:{licheng:%.3lf,count :%d},header : {index:'序号',w_address :%lf,w_degree:%s,wdpostion:%s,wdtype:%s},data:[",
		m_Head.distance * m_Head.step / 1000000, vWounds.size() );
		strWounds += strTemp;
		for (int i = 0; i < vWounds.size(); ++i)
		{
		strTemp.Format("index:%03d,w_address:%.5lf,w_degree:%s,status :%s,w_place:%s,w_type:%s},",
		i + 1, vWounds[i].Walk, g_strWoundPlaceDefines[vWounds[i].Place], g_strTypeDefines[vWounds[i].Type], g_strDegreeDefines[vWounds[i].Degree], g_strCheckStateDefines[vWounds[i].Checked]);
		strWounds += strTemp;
		}
		strWounds = strWounds.Left(strWounds.GetLength() - 1) + "]}}}}>>>";
		*/

		strTemp.Format("],wtype_count_detail:{info:{w_address:%.3lf,count:%d}, header:{index:'序号',w_address:'里程',w_type:'伤损类型',w_place:'伤损位置',w_degree:'伤损程度'}}}}",
			m_Head.distance * m_Head.step / 1000000, vWounds.size());
		strWounds += strTemp;

		SendUTF8(strWounds, 1001);
	}
}

bool CPosJudgeDlg::SendWoundData(uint64_t iWoundID)
{
	Wound_Judged wd, wd2, wd3;
	theApp.sql.GetWound(iWoundID, wd);
	CString strData = GetWoundData(wd);

	wd.LastCycleID = wd.id;
	if (wd.LastCycleID > 0)
	{
		theApp.sql.GetWound(wd.LastCycleID, wd2);
		CString strData2 = GetWoundData(wd2);
		strData += "," + strData2;

		wd2.LastCycleID = wd.id;
		if (wd2.LastCycleID > 0)
		{
			theApp.sql.GetWound(wd2.LastCycleID, wd3);
			CString strData3 = GetWoundData(wd3);
			strData += "," + strData3;
		}
	}
	return SendUTF8(strData, 1003);
}



bool CPosJudgeDlg::SendUTF8(CString& data, uint16_t code)
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
	bool bOK = m_ts.Write(pSend, nUTF8len + 15);

	delete pSend;
	delete utf8_string;
	delete tch;
	return bOK;
}

