// DlgLocate.cpp : implementation file
//

#include "stdafx.h"
#include "PosJudge.h"
#include "DlgLocate.h"
#include "afxdialogex.h"


// CDlgLocate dialog

IMPLEMENT_DYNAMIC(CDlgLocate, CDialogEx)

CDlgLocate::CDlgLocate(CWnd* pParent /*=NULL*/)
: CDialogEx(CDlgLocate::IDD, pParent)
, m_iBlockToLocate(0)
{
	m_strData = _T("");
}

CDlgLocate::~CDlgLocate()
{
}

void CDlgLocate::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EDIT1, m_strData);
}


BEGIN_MESSAGE_MAP(CDlgLocate, CDialogEx)
	ON_BN_CLICKED(IDOK, &CDlgLocate::OnBnClickedOk)
END_MESSAGE_MAP()


// CDlgLocate message handlers


void CDlgLocate::OnBnClickedOk()
{
	// TODO: Add your control notification handler code here
	UpdateData();
	if (m_strData.GetLength() > 0)
	{
		m_iBlockToLocate = atoi(m_strData);
	}
	else
	{
		return;
	}
	CDialogEx::OnOK();
}


BOOL CDlgLocate::PreTranslateMessage(MSG* pMsg)
{
	// TODO: Add your specialized code here and/or call the base class
	if (pMsg->message == WM_KEYUP && pMsg->wParam == VK_RETURN && GetFocus() == GetDlgItem(IDC_EDIT1))
	{
		OnBnClickedOk();
	}
	return CDialogEx::PreTranslateMessage(pMsg);
}


BOOL CDlgLocate::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  Add extra initialization here
	GetDlgItem(IDC_EDIT1)->SetWindowText("");
	GetDlgItem(IDC_EDIT1)->SetFocus();
	return FALSE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}
