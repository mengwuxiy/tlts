
// MainFrm.h : interface of the CMainFrame class
//

#pragma once
#include "SocketEx.h"
#include "ExcelHPE.h"
#include "Judge.h"


class CMainFrame : public CFrameWndEx
{
	
protected: // create from serialization only
	CMainFrame();
	DECLARE_DYNCREATE(CMainFrame)

// Attributes
public:

// Operations
public:

// Overrides
public:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual BOOL LoadFrame(UINT nIDResource, DWORD dwDefaultStyle = WS_OVERLAPPEDWINDOW | FWS_ADDTOTITLE, CWnd* pParentWnd = NULL, CCreateContext* pContext = NULL);

// Implementation
public:
	virtual ~CMainFrame();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:  // control bar embedded members
	CMFCMenuBar       m_wndMenuBar;
	CMFCToolBar       m_wndToolBar;
	CMFCStatusBar     m_wndStatusBar;
	CMFCToolBarImages m_UserImages;

// Generated message map functions
protected:
	afx_msg int		OnCreate(LPCREATESTRUCT lpCreateStruct); 
	afx_msg void	OnClose();
	afx_msg void	OnViewCustomize();
	afx_msg LRESULT OnToolbarCreateNew(WPARAM wp, LPARAM lp);
	afx_msg void	OnApplicationLook(UINT id);
	afx_msg void	OnUpdateApplicationLook(CCmdUI* pCmdUI);
	DECLARE_MESSAGE_MAP()

public://网络通信
	CSocketEx		m_ts;//服务器
	BOOL			m_blnConnectMCS;	//是否连接到主控
	unsigned int	m_threadID;
	uintptr_t		m_MainControlThread;//与主控进行通讯
	BOOL			CreateWebCom();

	afx_msg	LRESULT OnMessageReCreateWebCom(WPARAM wParam, LPARAM lParam);
	
	//0:空闲，1：正在接收，2：正在发送
	int				m_iCommunicateState;
	BOOL			m_bExit;

	void			OnAccept(CSocketEx* sock);
	void			OnReceive(CSocketEx* sock);
	void			OnSend(CSocketEx* sock);
	void			OnSocketClose(CSocketEx* sock);
	void			OnConnect(CSocketEx* sock);
	void			OnOutOfBandData(CSocketEx* sock);

	map<CString,uint8_t>		m_vList;
	map<CSocketEx*, uint8_t>	m_vSocks;
	

public:
	BOOL		OpenFileAB();
	uint32_t	ReadABData(int iBlock);
	void		CloseFileAB();
	BOOL		m_bLoaded;
	BOOL		m_bOpen;
	BOOL		m_bSolving;
	BOOL		m_bReadingB;
	int			m_iCurrentBlock;

	FILE*		m_pFileA;
	uint32_t	m_iSizeA;
	BlockData_A		m_DataA;
	VRI			m_vInfoA;


	F_HEAD		m_Head;

	CString		m_szFileB;
	FILE*		m_pFileB;
	uint32_t	m_iSizeB;
	vector<BLOCK>	m_vBlockHeads;
	BlockData_A			m_vABlocksToShow;
	vector<BlockData_B> m_vBBlocksToShow;
	vector<B_Step>		m_vSteps;
	VRI			 m_vInfoB;


	void 		SendWounds(vector<Wound_Judged>& vWounds, CAsyncSocket* sock);
	BOOL		m_bExported;

	void		CombineWound();
	void		ExportWounds(vector<Wound_Judged>& vWounds, int count, CString strPath);
	void		ExportWoundsToExcel(vector<Wound_Judged>& vWounds, int count, CString strPath);

	int			m_iStepIndex;

	afx_msg LRESULT ONMessageStep(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT ONMessageFinish(WPARAM wParam, LPARAM lParam);

public:
	vector<Position_Mark>	m_vPMs;


public:
	CExcelHPE			m_oExcel;
	afx_msg void		OnBnClickedExport();

	void				TravelDir(CString strPath);
	void				JudgeTPB(CString strrtpB, CAsyncSocket* sock);
	uintptr_t			m_threadJudge;//与主控进行通讯
	void				StopJudge();
	bool				m_bJudging;

public:
	void				CombinePM();
	HANDLE				m_jugedHandle;

public:
	vector<Wound_Judged> m_vWounds;
	int					m_iWDBlock;
	int					m_iWDStep;
	int32_t				m_iCurrentWound;
	void				ShowWound(int iWound);
	uint32_t			ReadABData2(CString strFileA, CString strFileB, vector<BLOCK_SQL>&blocks, int iBlock, F_HEAD& head, BlockData_A& vASteps, vector<BlockData_B>& datas);
	void				GetAString(vector<A_Step>& vASteps, CString& strA);
	void				GetBString(vector<BlockData_B>& datas, CString& strB);

	CString				GetWoundData(Wound_Judged& wd);
	CString				GetMultiCycleWoundData(uint64_t iWoundID);

public:
	void				ToUTF8(CString& data, uint16_t code, char* pOut, int* len);
	bool				SendUTF8(CString& data, uint16_t code, CAsyncSocket* sock);
};


