
// TLTSDlg.h : 头文件
//

#pragma once
#include <afxcmn.h>

#include "TSocket.h"
#include "ExcelHPE.h"
#include "Judge.h"

#include <vector>
#include "afxwin.h"

using namespace std;

#define WM_STEP (WM_USER+1)

#define WM_FINISH (WM_USER+10)
#define WM_BEGIN  (WM_USER+100)

// CPosJudgeDlg 对话框
class CPosJudgeDlg : public CDialogEx
{
	// 构造
public:
	CPosJudgeDlg(CWnd* pParent = NULL);	// 标准构造函数

	// 对话框数据
	enum { IDD = IDD_POSJUDGE_DIALOG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持


	// 实现
protected:
	HICON m_hIcon;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg LRESULT ONMessageStep(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT ONMessageFinish(WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedJudge();
	afx_msg void OnBnClickedBrowse();

	CListCtrl		m_oList;
	CString			m_strBeginTime;
	CString			m_strEndTime;
	CString			m_strProgress;

	CStatic			m_oPlot;
	CStatic			m_oPlot2;

public:
	bool			IsFileExist(CString strFilePath);

public:
	CString			m_strRetFolder;//Exe路径
	CString			m_strExeFolder;//tpB上一层目录

//解析文件
public:
	CString				m_strPath;
	void				TravelDir(CString strPath);
	void				JudgeTPB(CString strrtpB);
	uintptr_t			m_threadJudge;//与主控进行通讯
	HANDLE				m_jugedHandle;
	void				StopJudge();
	bool				m_bJudging;
	BOOL				OpenFileAB();
	void				CloseFileAB();
	BOOL				m_bLoaded;
	BOOL				m_bOpen;
	BOOL				m_bSolving;
	int					m_iCurrentBlock;

	CString				m_strFolder;
	CString				m_strFileName;

	FILE*				m_pFileA;
	uint32_t			m_iSizeA;
	BlockData_A			m_DataA;
	VRI					m_vInfoA;

	CString				m_szFileB; 
	FILE*				m_pFileB;
	uint32_t			m_iSizeB;
	F_HEAD				m_Head;
	VRI					m_vInfoB;
	vector<BLOCK>		m_vBlockHeads;

	//显示
public:
	vector<BlockData_B>	m_vBlocksToShow;
	vector<B_Step>		m_vSteps;

	//导出
public:
	BOOL				m_bExported;
	void				ExportPMs(vector<Position_Mark>& vPMs, int count, CString strPath);
	void				ExportPMsToExcel(vector<Position_Mark>& vPMs, int count, CString strPath);

	void				ExportWounds(vector<Wound_Judged>& vWounds, int count, CString strPath);
	void				ExportWoundsToExcel(vector<Wound_Judged>& vWounds, int count, CString strPath);

	time_t				m_tBegin;
	CString				m_strTime;
	afx_msg void		OnTimer(UINT_PTR nIDEvent);


public:
	int					m_iStepIndex;

	CString				m_strRetOld;
	CString				m_strRetNew;

	BOOL				m_bReadingB;
	uint32_t			ReadABData(int iBeginBlock);
	
	void				DrawData();
	void				DrawData2();//画放大波形
	CString				m_strProgress2;

	/*
	0 : 位置标
	1 : 伤损
	2 : 米块
	*/
	BOOL					m_iMode;
	int						m_iBlock;
	int						m_iStep;

public:
	vector<Position_Mark>	m_vPMs;
	int						m_iCurrentMark;
	void					ShowMark(int iPM);

public:
	vector<Wound_Judged> m_vWounds;
	int					m_iWDBlock;
	int					m_iWDStep;
	int32_t				m_iCurrentWound;
	void				ShowWound(int iWound);
	void				CombineWound();

public:
	void				ShowBlock(int iBlock);
	afx_msg void		OnBnClickedLocate();

public:
	CExcelHPE			m_oExcel;
	afx_msg void		OnBnClickedExport();

public:
	CFont				m_fontTip;
	COLORREF			m_clrBK;
	int					m_R[16];
	int					m_G[16];
	int					m_B[16];
	float				m_fWidth[16];
	void				LoadConfig();	

	vector<CBrush*>		m_vBbrush;
	CBrush*				m_BrushBK;

	CPen				m_pen;//画虚线用的
	CPen				m_pen2;
	CStatic				m_oTips;
	virtual BOOL		PreTranslateMessage(MSG* pMsg); 


	virtual void		OnOK();

public:
	void				CombinePM();

	void				SetMode(int iMode);
public:

	//CSplitButton m_oSplit;
	afx_msg HBRUSH	OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	afx_msg void	OnInitMenuPopup(CMenu *pPopupMenu, UINT nIndex, BOOL bSysMenu);
	afx_msg void	OnWdCheck();
	afx_msg void	OnPosCheck();
	afx_msg void	OnContextMenu(CWnd* /*pWnd*/, CPoint /*point*/);
	afx_msg void	OnUpdateWdCheck(CCmdUI *pCmdUI);
	afx_msg void	OnUpdatePosCheck(CCmdUI *pCmdUI);
	afx_msg void	OnWindowPosChanging(WINDOWPOS* lpwndpos);
	afx_msg void	OnClose();

public://网络通信
	CTSocket		m_ts;
	CTSocket		m_tsClient;
	SOCKET			m_curSocket;
	BOOL			m_blnConnectMCS;	//是否连接到主控
	unsigned int	m_threadID;
	uintptr_t		m_MainControlThread;//与主控进行通讯
	void			CreateWebCom();

	afx_msg	LRESULT OnMessageReCreateWebCom(WPARAM wParam, LPARAM lParam);
	afx_msg	LRESULT OnMessageNet(WPARAM wParam, LPARAM lParam);

	//0:空闲，1：正在接收，2：正在发送
	int				m_iCommunicateState;
	BOOL			m_bExit;

	void 			SendWounds(vector<Wound_Judged>& vWounds);

	uint32_t		ReadABData2(CString strFileA, CString strFileB, vector<BLOCK_SQL>&blocks, int iBlock, F_HEAD& head, BlockData_A& vASteps, vector<BlockData_B>& datas);
	void			GetAString(vector<A_Step>& vASteps, CString& strA);
	void			GetBString(vector<BlockData_B>& datas, CString& strB);
	
	CString			GetWoundData(Wound_Judged& wd);
	bool			SendWoundData(uint64_t iWoundID);

	bool			SendUTF8(CString& data, uint16_t code);
};


