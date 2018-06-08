
// tltsView.h : interface of the CtltsView class
//

#pragma once


class CtltsView : public CView
{
protected: // create from serialization only
	CtltsView();
	DECLARE_DYNCREATE(CtltsView)

// Attributes
public:
	CtltsDoc* GetDocument() const;

// Operations
public:

// Overrides
public:
	virtual void OnDraw(CDC* pDC);  // overridden to draw this view
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
protected:

// Implementation
public:
	virtual ~CtltsView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
	afx_msg void OnFilePrintPreview();
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	DECLARE_MESSAGE_MAP()
public:
	virtual void OnInitialUpdate();
};

#ifndef _DEBUG  // debug version in tltsView.cpp
inline CtltsDoc* CtltsView::GetDocument() const
   { return reinterpret_cast<CtltsDoc*>(m_pDocument); }
#endif

