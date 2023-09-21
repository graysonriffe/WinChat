#pragma once
#include <string>

#include <Windows.h>

namespace wc {
	class Chat {
	public:
		Chat(std::wstring address, std::wstring screenname);

		void run();

		~Chat();
	private:
		const std::wstring m_address;
		const std::wstring m_screenname;
	};
}