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

	};
}