#include "pch.h"
#include "Chat.h"

#include "StrConv.h"

namespace wc {
	Chat::Chat(std::wstring address, std::wstring screenname)
		: m_address(address)
		, m_screenname(screenname)
	{

	}

	void Chat::run() {
		addrinfo* destInfo = nullptr;
		if (getaddrinfo(toStr(m_address).c_str(), "9430", nullptr, &destInfo) != 0) {
			MessageBox(nullptr, L"Error: Could not resolve host or invalid address!", L"Error", MB_ICONERROR);
			return;
		}

		SOCKET sock = socket(destInfo->ai_family, SOCK_STREAM, NULL);
		if (sock == INVALID_SOCKET) {
			MessageBox(nullptr, L"Error: could not create network socket!", L"Error", MB_ICONERROR);
			freeaddrinfo(destInfo);
			return;
		}

		if (connect(sock, destInfo->ai_addr, static_cast<int>(destInfo->ai_addrlen)) != 0) {
			int errorCode = WSAGetLastError();
			std::wstring errorStr;
			switch (errorCode) {
				case WSAETIMEDOUT:
					errorStr = L"Timed out";
					break;

				case WSAECONNREFUSED:
					errorStr = L"Connection refused (WinChat may not be running on remote host)";
					break;

				case WSAENETUNREACH:
				case WSAEHOSTUNREACH:
					errorStr = L"Remote host unreachable (You may not be connected to the internet)";
					break;

				case WSAEADDRNOTAVAIL:
					errorStr = L"Not a valid address to connect to";
					break;

				default:
					errorStr = std::format(L"Error Code {}", errorCode);
					break;
			}

			MessageBox(nullptr, std::format(L"Error: Could not connect - {}", errorStr).c_str(), L"Error", MB_ICONERROR);
			freeaddrinfo(destInfo);
			closesocket(sock);
			return;
		}

		freeaddrinfo(destInfo);

		std::string data;
		while (data != "end") {
			std::getline(std::cin, data);
			send(sock, data.data(), static_cast<int>(data.size()), NULL);
		}

		closesocket(sock);
	}

	Chat::~Chat() {

	}
}