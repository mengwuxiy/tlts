#pragma once


// CDlgLocate dialog

class CDlgLocate : public CDialogEx
{
	DECLARE_DYNAMIC(CDlgLocate)

public:
	CDlgLocate(CWnd* pParent = NULL);   // standard constructor
	virtual ~CDlgLocate();

	// Dialog Data
	enum { IDD = IDD_DIALOG1 };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	int			m_iBlockToLocate;
	CString		m_strData;
	afx_msg void OnBnClickedOk();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual BOOL OnInitDialog();
};
