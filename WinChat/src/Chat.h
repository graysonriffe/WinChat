#pragma once
#include <string>

#include <Windows.h>

#include "TSQueue.h"

namespace wc {
	class Chat {
	public:
		Chat(std::wstring address, std::wstring port, std::wstring screenname);

		void run(int xPos, int yPos, SOCKET socket);

		~Chat();
	private:
		void runNetThread(SOCKET inSocket);
		std::wstring getErrorString();
		static BOOL CALLBACK connDlgProc(HWND dlg, UINT msg, WPARAM wParam, LPARAM lParam);
		static BOOL CALLBACK chatDlgProc(HWND dlg, UINT msg, WPARAM wParam, LPARAM lParam);
		void addMessage(HWND dlg, std::wstring str);

		const std::wstring m_address;
		const std::wstring m_port;
		const std::wstring m_screenname;
		std::wstring m_remoteScreenname;
		bool m_connected;
		bool m_connectionError;

		TSQueue<std::wstring> m_sendQueue;
		TSQueue<std::wstring> m_recvQueue;
	};
}