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
		addrinfo hints = { };
		addrinfo* destInfo = nullptr;
		hints.ai_family = AF_UNSPEC;
		hints.ai_socktype = SOCK_STREAM;
		getaddrinfo(toStr(m_address).c_str(), "9430", &hints, &destInfo);

		SOCKET sock = socket(destInfo->ai_family, destInfo->ai_socktype, destInfo->ai_protocol);
		if (sock == INVALID_SOCKET) {
			std::cerr << std::format("Error: could not create network socket!\n");
			std::exit(1);
		}

		if (connect(sock, destInfo->ai_addr, static_cast<int>(destInfo->ai_addrlen)) != 0) {
			std::cerr << std::format("Error: could not connect to destination!\n");
			std::exit(1);
		}

		freeaddrinfo(destInfo);

		std::string data;
		while (data != "end") {
			std::getline(std::cin, data);
			send(sock, data.data(), static_cast<int>(data.size()), 0);
		}

		closesocket(sock);
	}

	Chat::~Chat() {

	}
}