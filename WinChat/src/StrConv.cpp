#include "pch.h"
#include "StrConv.h"

namespace wc {
	std::string toStr(const std::wstring& in) {
		std::string out;
		out.resize(in.size() + 1);
		size_t tempSize = 0;
		wcstombs_s(&tempSize, out.data(), out.size(), in.c_str(), out.size());
		return out;
	}

	std::wstring toWideStr(const std::string& in){
		return std::wstring(in.begin(), in.end());
	}
}