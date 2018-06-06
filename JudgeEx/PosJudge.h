
// PosJudge.h : main header file for the PROJECT_NAME application
//

#pragma once

#ifndef __AFXWIN_H__
	#error "include 'stdafx.h' before including this file for PCH"
#endif

#include "resource.h"		// main symbols
#include "MySqlHPE.h"

// CPosJudgeApp:
// See PosJudge.cpp for the implementation of this class
//

class CPosJudgeApp : public CWinAppEx
{
public:
	CPosJudgeApp();

// Overrides
public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();

	// Implementation
	UINT  m_nAppLook;
	BOOL  m_bHiColorIcons;
	
	afx_msg void OnAppAbout();
	DECLARE_MESSAGE_MAP()

public:
	CMySqlHPE	sql;
};

extern CPosJudgeApp theApp;