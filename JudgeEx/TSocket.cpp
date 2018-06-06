// TSocket.cpp: implementation of the CTSocket class.
//
//////////////////////////////////////////////////////////////////////

#define  _WINSOCK_DEPRECATED_NO_WARNINGS

#include "stdafx.h"
#include "TSocket.h"
#include <process.h>
#include <Ws2tcpip.h>
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////



CTSocket::CTSocket()
{
	Clear();
}

CTSocket::~CTSocket()
{
	closesocket(m_socketClient);
	closesocket(m_socketServer);
}


void CTSocket::Clear(int t)
{
	if (t < 0)
	{
		m_blnIsConnect = FALSE;
		if (m_socketTemp != INVALID_SOCKET)
			closesocket(m_socketTemp);
		if (m_socketClient != INVALID_SOCKET)
			closesocket(m_socketClient);
		if (m_socketServer != INVALID_SOCKET)
			closesocket(m_socketServer);
		m_socketClient = m_socketServer = m_socketTemp = INVALID_SOCKET;
		m_blnIsFinish = TRUE;
		m_blnIsServerInitialed = FALSE;
		m_blnIsClientInitialed = FALSE;
		m_IsServer = FALSE;
	}
	else if (t == 0)//Client
	{
		m_blnIsConnect = FALSE;
		if (m_socketTemp != INVALID_SOCKET)
			closesocket(m_socketTemp);
		if (m_socketClient != INVALID_SOCKET)
			closesocket(m_socketClient);
		m_socketClient = m_socketTemp = INVALID_SOCKET;
		m_blnIsFinish = TRUE;
		m_blnIsClientInitialed = FALSE;
		m_IsServer = FALSE;
	}
	else//Server
	{
		if (m_socketTemp != INVALID_SOCKET)
			closesocket(m_socketTemp);
		if (m_socketServer != INVALID_SOCKET)
			closesocket(m_socketServer);
		m_blnIsConnect = FALSE;
		m_socketServer = m_socketTemp = INVALID_SOCKET;
		m_blnIsFinish = TRUE;
		m_blnIsServerInitialed = FALSE;
		m_IsServer = FALSE;
	}
	m_iState = 0;
}


CString CTSocket::GetErrorMsg()
{
	return m_strErrMsg;
}


BOOL CTSocket::InitialClientSocket(int toPort, char * ipAddress, int recvTimeout, int sendTimeout)
{
	m_socketClient = socket(PF_INET, SOCK_STREAM, 0);    //创建服务器端Socket，类型为SOCK_STREAM，面向连接的通信

	if (m_socketClient == INVALID_SOCKET)
	{
		return FALSE;
	}

	if (::setsockopt(m_socketClient, SOL_SOCKET, SO_RCVTIMEO, (char*)&recvTimeout, sizeof(recvTimeout)) == SOCKET_ERROR)
	{
		m_strErrMsg.Format(_T("设置接收超时时间失败\n%s"), GetError());
	}
	if (::setsockopt(m_socketClient, SOL_SOCKET, SO_SNDTIMEO, (char*)&sendTimeout, sizeof(sendTimeout)) == SOCKET_ERROR)
	{
		m_strErrMsg.Format(_T("设置发送超时时间失败\n%s"), GetError());
	}
	m_AddrClient.sin_family = AF_INET; //使用TCP/IP协议
	m_AddrClient.sin_port = htons(toPort);
	//m_AddrClient.sin_addr.S_un.S_addr = inet_addr(ipAddress);
	InetPton(AF_INET, ipAddress, &m_AddrClient.sin_addr.s_addr);
	if (connect(m_socketClient, (LPSOCKADDR)&m_AddrClient, sizeof(m_AddrClient)) == SOCKET_ERROR)
	{
		m_strErrMsg.Format(_T("建立连接失败!\n%s"), GetError());
	}
	else
	{
		//给服务器发信息，成功
	}
	m_blnIsClientInitialed = TRUE;
	m_IsServer = FALSE;
	return TRUE;
}

BOOL CTSocket::InitialServerSocket(int listenPort)
{
	//创建服务器端Socket，类型为SOCK_STREAM，面向连接的通信
	m_nConnected = 0;
	
	m_socketServer = socket(PF_INET, SOCK_STREAM, 0);
	if (m_socketServer == INVALID_SOCKET)
	{
		m_strErrMsg.Format(_T("ServerSocket无法初始化!\n%s"), GetError());
		return FALSE;
	}


	m_AddrServer.sin_family = AF_INET;
	m_AddrServer.sin_addr.s_addr = INADDR_ANY;   //向所有的IP地址发送消息
	m_AddrServer.sin_port = htons(listenPort);

	//与选定的端口绑定
	if (bind(m_socketServer, (LPSOCKADDR)&m_AddrServer, sizeof(m_AddrServer)) == SOCKET_ERROR)
	{
		m_strErrMsg.Format(_T("无法绑定服务器!\n%s"), GetError());
		return FALSE;
	}

	if (listen(m_socketServer, 10) == SOCKET_ERROR) //开始监听客户连接请求
	{
		m_strErrMsg.Format(_T("服务器监听失败!\n%s"), GetError());
		return FALSE;
	}
	m_blnIsServerInitialed = TRUE;
	m_IsServer = TRUE;
	m_iState = 1;
	return TRUE;
}

BOOL CTSocket::Write(char* data, int len)
{
	int length = -1;
	if (m_IsServer == FALSE)
	{
		length = send(m_socketClient, data, len, 0);
		if (length <= 0)
		{
			int err = WSAGetLastError();
			m_strErrMsg.Format("%s", GetError());
			return FALSE;
		}
		return TRUE;
	}
	else
	{
		length = send(m_preSocket, data, len, 0);
		if (length <= 0)
		{
			return FALSE;
		}
		return TRUE;
	}
}

unsigned int __stdcall AnswerThread(LPVOID lParam)
{

	SOCKET clientSocket = (SOCKET)lParam;
	int recvCount = -1;
	char recvBuff[400] = { 0 };
	while (1)
	{
		recvCount = recv(clientSocket, recvBuff, 400, 0);
		if (recvCount < 0)
		{
			DWORD nError = GetLastError();
			if (nError == EINTR)
			{
				continue;
			}
			else
			{
				closesocket(clientSocket);
				return 0;
			}
		}
		//接收成功		
		::SendMessage(AfxGetApp()->m_pMainWnd->GetSafeHwnd(), WM_WEBINFO_IN, (WPARAM)recvBuff, (LPARAM)clientSocket);
		memset(recvBuff, 0, 400);


		//if (pMainFrm->m_ts.m_iState == 0)
		//{
		//	closesocket(clientSocket);	
		//	Sleep(10);

		//	if ( !pMainFrm->m_bExit )
		//	{
		//		::PostMessage(pMainFrm->m_hWnd, WM_WEBINFO_RECREATE ,0 ,0);
		//	}
		//	return 0;
		//}		

		//memset(recvBuff, 0, sizeof(char)*strlen(recvBuff));
	}
	closesocket(clientSocket);
	return 0;
}


BOOL CTSocket::Read(char *temp, int len)
{
	if (m_blnIsFinish == FALSE)
		return FALSE;

	if (m_IsServer == FALSE)
	{
		m_blnIsFinish = FALSE;
		if (recv(m_socketClient, temp, len, 0) < 0)
		{
			m_blnIsFinish = TRUE;
			return FALSE;
		}
		m_blnIsFinish = TRUE;
		return TRUE;
	}
	else
	{
		while (1)
		{
			m_socketTemp = SOCKET_ERROR;
			while (m_socketTemp == SOCKET_ERROR)
			{
				m_socketTemp = accept(m_socketServer, (LPSOCKADDR)&m_AddrServer, 0);
				if (!m_iState)
					return FALSE;
			}
			m_nConnected += 1;
			unsigned int	dwThreadID;
			HANDLE			hThread;
			m_preSocket = m_socketTemp;
			hThread = (HANDLE)_beginthreadex(NULL, 0, (unsigned int(__stdcall*)(void*))AnswerThread, (LPVOID)m_socketTemp, 0, &dwThreadID);
		}

		return TRUE;
	}
}

//#define _CRT_SECURE_NO_WARNINGS
CString CTSocket::GetError(int nErrorCode /* = -1 */)
{
	if (nErrorCode == -1)
	{
		nErrorCode = WSAGetLastError();
	}

	CString str;
	str.Format(_T("错误代码：%d\n错误信息：%s"), nErrorCode, strerror(nErrorCode));
	return str;
}