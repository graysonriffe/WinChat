#include "Application.h"

namespace wc {
	Application::Application(std::string& appName, std::string& appVersion)
		: m_appName(appName)
		, m_appVersion(appVersion)
	{
		std::printf("%s %s\n", m_appName.c_str(), m_appVersion.c_str());
	}

	Application::~Application() {

	}
}