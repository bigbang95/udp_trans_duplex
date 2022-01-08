#include "UdpClientSocket.h"
#include "..\log\wLog.h"

CUdpClientSocket::CUdpClientSocket(USHORT port, LPCWSTR ip)
{
	m_sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (m_sock == INVALID_SOCKET)
	{
		WriteLog(L"socket error code: " + std::to_wstring(WSAGetLastError()), __FILEW__, __LINE__);
		return;
	}

	m_event = WSACreateEvent(); // 创建一个 初始状态 为 失信的 事件对象

	if (m_event == WSA_INVALID_EVENT)
	{
		WriteLog(L"WSACreateEvent error code: " + std::to_wstring(WSAGetLastError()), __FILEW__, __LINE__);
		return;
	}

	if (SOCKET_ERROR == WSAEventSelect(m_sock, m_event, FD_READ))
	{
		WriteLog(L"WSAEventSelect error code: " + std::to_wstring(WSAGetLastError()), __FILEW__, __LINE__);
		return;
	}

	in_addr in_addr1;
	int ret = InetPtonW(AF_INET, ip, &in_addr1);
	if (ret != 1)
	{
		// 失败
		WriteLog(L"ip 地址无效", __FILEW__, __LINE__);
		return;
	}

	m_addr.sin_addr = in_addr1;
	m_addr.sin_family = AF_INET;
	m_addr.sin_port = htons(port);


	printf("UDP client start success.\n");


	m_dwClientSeq = 0;
	m_dwServerSeq = 0;
}


CUdpClientSocket::~CUdpClientSocket()
{
	if (m_sock != INVALID_SOCKET)
		closesocket(m_sock);

	if (m_event != WSA_INVALID_EVENT)
		WSACloseEvent(m_event);
}


//////////////////////////////////////////////////////////////


// 返回 0、5、ret
int CUdpClientSocket::send1(SOCKET s, const char* buf, int len, bool bSyn)
{
	char sz1[2048], sz2[2048];
	UDP_HEAD h1, h2;
	h1.c_seq = m_dwClientSeq;
	h1.s_seq = 0;

	if (bSyn)
	{
		h1.flags = UDP_SYN;
	}
	else
	{
		h1.flags = UDP_PSH;
	}

	memcpy(sz1, &h1, sizeof(h1));
	memcpy(sz1 + sizeof(h1), buf, len);

	DWORD dwTimeout = 1000;

	int j = 5; // 执行 5 次 循环
	while (j--)
	{
		if (sendto(s, sz1, len + sizeof(h1), (sockaddr*)&m_addr, sizeof(sockaddr)) == SOCKET_ERROR)
		{
			return WSAGetLastError();
		}

		int len2, addr_len = sizeof(sockaddr);
		int ret = recvfrom(s, sz2, &len2, dwTimeout, (sockaddr*)&m_addr, &addr_len);
		if (ret == WSA_WAIT_TIMEOUT)
		{

		}
		else if (ret == ERROR_SUCCESS)
		{
			if (len2 == sizeof(h2))
			{
				h2 = *(UDP_HEAD*)sz2;

				if ((h2.c_seq == m_dwClientSeq + 1)
					&& (h2.flags == UDP_ACK))
				{
					m_dwClientSeq++;
					break;
				}
			}
		}
		else
		{
			return ret;
		}

		dwTimeout *= 2;

		WriteLog(L"丢包 " + std::to_wstring(5 - j) + L" 次", __FILEW__, __LINE__);
	}

	if (j == -1)
	{
		// 5 次 数据包 全 发送失败
		return 5;
	}

	return ERROR_SUCCESS;
}

int CUdpClientSocket::send(SOCKET s, std::string buf)
{
	m_dwClientSeq = GetRandomDword();

	int ret;
	if ((ret = send1(s, "", 0, true)) != ERROR_SUCCESS)
	{
		return ret;
	}

	const int one_len = 1390;

	while (buf.length() != 0)
	{
		std::string str = buf.substr(0, one_len);
		int ret;
		if ((ret = send1(s, str.c_str(), str.length())) != ERROR_SUCCESS)
		{
			return ret;
		}

		if (str.length() < one_len)
		{
			break;
		}

		buf = buf.substr(one_len);
	}

	return ERROR_SUCCESS;
}


////////////////////////////////////////////////////////////////////////////////


// 返回 0、5、ret
int CUdpClientSocket::recv(SOCKET s, std::string& buf, size_t buf_len)
{
	buf = "";

	char sz1[2048];

	UDP_HEAD h1, h2;

	const DWORD dwTimeout = INFINITE;

	int j = 5; // 执行 5 次 循环
	while (j--)
	{
		int len1, addr_len = sizeof(sockaddr);
		int ret = recvfrom(s, sz1, &len1, dwTimeout, (sockaddr*)&m_addr, &addr_len);
		if (ret == WSA_WAIT_TIMEOUT)
		{
			// 不可能事件
		}
		else if (ret != ERROR_SUCCESS)
		{
			return ret;
		}
		else
		{
			if (len1 == sizeof(h1))
			{
				h1 = *(UDP_HEAD*)sz1;

				if (h1.flags == UDP_SYN)
				{
					m_dwServerSeq = h1.s_seq;
				}
			}
			else if (len1 > sizeof(h1))
			{
				h1 = *(UDP_HEAD*)sz1;

				if ((h1.s_seq == m_dwServerSeq + 1)
					&& (h1.flags == UDP_PSH))
				{
					// 一次 recv 成功
					buf += std::string(sz1 + sizeof(UDP_HEAD), len1 - sizeof(UDP_HEAD));

					m_dwServerSeq++;

					// 重置 while 参数
					j = 5;
				}
			}
			else
			{
				continue;
			}
		}


		h2.c_seq = 0;
		h2.s_seq = m_dwServerSeq + 1;
		h2.flags = UDP_ACK;

		if (sendto(s, (char*)&h2, sizeof(h2), (sockaddr*)&m_addr, sizeof(sockaddr)) == SOCKET_ERROR)
		{
			return WSAGetLastError();
		}

		if (buf.length() == buf_len)
		{
			break;
		}
	}

	if (j == -1)
	{
		// 5 次 数据包 全 接收失败
		return 5;
	}

	return ERROR_SUCCESS;
}


////////////////////////////////////////////////////////////////////////


int CUdpClientSocket::send(std::string buf)
{
	return send(m_sock, buf);
}

int CUdpClientSocket::recv(std::string& buf, size_t buf_len)
{
	return recv(m_sock, buf, buf_len);
}
