// TSocket.h: interface for the CTSocket class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_TSOCKET_H__136B182C_65C2_404A_818E_FE0108AE149A__INCLUDED_)
#define AFX_TSOCKET_H__136B182C_65C2_404A_818E_FE0108AE149A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <winsock2.h>
#pragma comment(lib, "WS2_32")	// 链接到WS2_32.lib


struct NET_PARAM
{
	SOCKET	sock;	//通讯socket
	char*	data;	//字节地址
	int		len;	//字节长度
};

class CTSocket
{

public:
	CTSocket();
	~CTSocket();

	void Clear(int r = -1);
	//发送数据
	BOOL Write(char* message, int len);

	//接收数据
	BOOL Read(char *data, int len = 1024);

	BOOL InitialServerSocket(int port);

	BOOL InitialClientSocket(int toPort, char* ipAddress, int recvTimeout = 3000, int sendTimeout = 3000);

	//获取错误信息
	CString GetErrorMsg();

public:
	CString		GetError(int nErrorCode = -1);
	SOCKET		m_socketClient, m_socketServer, m_socketTemp;
	SOCKET		m_preSocket;
	SOCKADDR_IN m_AddrClient, m_AddrServer;
	CString		m_strErrMsg;
	int			m_iErrorCode;

private:
	BOOL	m_blnIsConnect;
	BOOL	m_blnIsFinish;
	BOOL	m_blnIsServerInitialed, m_blnIsClientInitialed;
	BOOL	m_IsServer;

	//当前链接的客户数
	int		m_nConnected;

public:
	//当前状态: 0:无效， 1：有效
	//只对服务器设置有效
	BOOL	m_iState;
};


#endif // !defined(AFX_TSOCKET_H__136B182C_65C2_404A_818E_FE0108AE149A__INCLUDED_)
