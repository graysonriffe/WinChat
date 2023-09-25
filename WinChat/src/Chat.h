#pragma once
#include <string>

#include <Windows.h>

namespace wc {
	class Chat {
	public:
		Chat(std::wstring address, std::wstring screenname);

		void run(int xPos, int yPos);

		~Chat();
	private:
		void runNetThread();
		std::wstring getErrorString();
		static BOOL CALLBACK connDlgProc(HWND dlg, UINT msg, WPARAM wParam, LPARAM lParam);

		const std::wstring m_address;
		const std::wstring m_screenname;
		bool m_connected;
		bool m_connectionError;
	};
}