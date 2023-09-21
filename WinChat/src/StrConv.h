#pragma once
#include <string>

namespace wc {
	std::string toStr(const std::wstring& in);
	std::wstring toWideStr(const std::string& in);
}