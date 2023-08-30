#pragma once
#include <string>

namespace wc {
	class Application {
	public:
		Application(std::string& appName, std::string& appVersion);

		~Application();
	private:
		const std::string m_appName;
		const std::string m_appVersion;
	};
}