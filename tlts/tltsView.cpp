
// tltsView.cpp : implementation of the CtltsView class
//

#include "stdafx.h"
// SHARED_HANDLERS can be defined in an ATL project implementing preview, thumbnail
// and search filter handlers and allows sharing of document code with that project.
#ifndef SHARED_HANDLERS
#include "tlts.h"
#endif

#include "tltsDoc.h"
#include "tltsView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CtltsView

IMPLEMENT_DYNCREATE(CtltsView, CView)

BEGIN_MESSAGE_MAP(CtltsView, CView)
	ON_WM_CONTEXTMENU()
	ON_WM_RBUTTONUP()
END_MESSAGE_MAP()

// CtltsView construction/destruction

CtltsView::CtltsView()
{
	// TODO: add construction code here

}

CtltsView::~CtltsView()
{
}

BOOL CtltsView::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	return CView::PreCreateWindow(cs);
}

// CtltsView drawing

void CtltsView::OnDraw(CDC* /*pDC*/)
{
	CtltsDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	if (!pDoc)
		return;

	// TODO: add draw code for native data here
}

void CtltsView::OnRButtonUp(UINT /* nFlags */, CPoint point)
{
	ClientToScreen(&point);
	OnContextMenu(this, point);
}

void CtltsView::OnContextMenu(CWnd* /* pWnd */, CPoint point)
{
#ifndef SHARED_HANDLERS
	theApp.GetContextMenuManager()->ShowPopupMenu(IDR_POPUP_EDIT, point.x, point.y, this, TRUE);
#endif
}


// CtltsView diagnostics

#ifdef _DEBUG
void CtltsView::AssertValid() const
{
	CView::AssertValid();
}

void CtltsView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

CtltsDoc* CtltsView::GetDocument() const // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CtltsDoc)));
	return (CtltsDoc*)m_pDocument;
}
#endif //_DEBUG


// CtltsView message handlers


void CtltsView::OnInitialUpdate()
{
	CView::OnInitialUpdate();

	// TODO: Add your specialized code here and/or call the base class
}
