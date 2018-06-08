// SocketEx.cpp : implementation file
//

#include "stdafx.h"
#include "tlts.h"
#include "SocketEx.h"

#include "MainFrm.h"
// CSocketEx

CSocketEx::CSocketEx()
{
	m_pFrame = NULL;
}

CSocketEx::~CSocketEx()
{

}


// CSocketEx member functions


void CSocketEx::OnAccept(int nErrorCode)
{
	// TODO: Add your specialized code here and/or call the base class
	if (nErrorCode == 0 && m_pFrame != NULL)
	{
		((CMainFrame*)m_pFrame)->OnAccept(this);
	}
	//CAsyncSocket::OnAccept(nErrorCode);
}


void CSocketEx::OnReceive(int nErrorCode)
{
	// TODO: Add your specialized code here and/or call the base class

	if (nErrorCode == 0 && m_pFrame != NULL)
	{
		((CMainFrame*)m_pFrame)->OnReceive(this);
	}
	//CAsyncSocket::OnReceive(nErrorCode);
}


void CSocketEx::OnSend(int nErrorCode)
{
	// TODO: Add your specialized code here and/or call the base class

	if (nErrorCode == 0 && m_pFrame != NULL)
	{
		((CMainFrame*)m_pFrame)->OnSend(this);
	}
	CAsyncSocket::OnSend(nErrorCode);
}


void CSocketEx::OnClose(int nErrorCode)
{
	// TODO: Add your specialized code here and/or call the base class

	if (nErrorCode == 0 && m_pFrame != NULL)
	{
		((CMainFrame*)m_pFrame)->OnSocketClose(this);
	}
	CAsyncSocket::OnClose(nErrorCode);
}


void CSocketEx::OnConnect(int nErrorCode)
{
	// TODO: Add your specialized code here and/or call the base class
	if (nErrorCode == 0 && m_pFrame != NULL)
	{
	//	((CMainFrame*)m_pFrame)->OnConnect(this);
	}
	CAsyncSocket::OnConnect(nErrorCode);
}

void CSocketEx::OnOutOfBandData(int nErrorCode)
{
	// TODO: Add your specialized code here and/or call the base class

	if (nErrorCode == 0 && m_pFrame != NULL)
	{
		((CMainFrame*)m_pFrame)->OnOutOfBandData(this);
	}
	CAsyncSocket::OnOutOfBandData(nErrorCode);
}


void CSocketEx::SetParent(DWORD_PTR frame)
{
	m_pFrame = frame;
}