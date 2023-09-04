#include "Chat.h"

#include <format>

namespace wc {
	Chat::Chat(std::wstring address, std::wstring screenname) {
		MessageBox(nullptr, std::format(L"Address: {}\nScreen Name: {}", address, screenname).c_str(), L"Alert", MB_OK);
	}

	void Chat::run() {

	}

	Chat::~Chat() {

	}
}