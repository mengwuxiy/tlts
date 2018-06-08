#pragma once
// CSocketEx command target

class CSocketEx : public CAsyncSocket
{
public:
	CSocketEx();
	virtual ~CSocketEx();
	virtual void OnAccept(int nErrorCode);
	virtual void OnReceive(int nErrorCode);
	virtual void OnSend(int nErrorCode);

	//���ͻ��˶Ͽ�socket����server�˵�CClientSock�е�OnClose()�ᱻ���ã�
	virtual void OnClose(int nErrorCode);
	virtual void OnConnect(int nErrorCode);
	virtual void OnOutOfBandData(int nErrorCode);

private:
	DWORD_PTR	m_pFrame;
public:
	void		SetParent(DWORD_PTR frame);
};


