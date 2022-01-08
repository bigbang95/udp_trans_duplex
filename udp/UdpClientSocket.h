#pragma once
#include "UdpSocket.h"
#include<WS2tcpip.h>

class CUdpClientSocket :public CUdpSocket
{
private:
	DWORD m_dwClientSeq; // 当前 客户端 发送 序号
	DWORD m_dwServerSeq; // 当前 服务端 发送 序号

public:
	CUdpClientSocket(USHORT port, LPCWSTR ip);
	~CUdpClientSocket();

	// 发送 一个 UDP 数据报文
	int send1(SOCKET s, const char* buf, int len, bool bSyn = false);

	// 理论上 任意长度 发送
	// 返回 成功(0)、失败
	int send(SOCKET s, std::string buf);
	int send(std::string buf);

	// 理论上 任意长度 接收
	// 返回 成功(0)、失败
	int recv(SOCKET s, std::string& buf, size_t buf_len);
	int recv(std::string& buf, size_t buf_len);
};
