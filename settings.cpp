#include "settings.h"

Settings* Settings::m_ptr = nullptr;
std::vector<std::pair<std::string, std::string>> Settings::m_settings{
	{ "Spider", "StartContentProtocol" },
	{ "Spider", "StartContentHostName" },
	{ "Spider", "StartContentQuery" },
	{ "Spider", "Depth" },
	{ "HTTPServer", "Address" },
	{ "HTTPServer", "Port" },
	{ "Database", "Host" },
	{ "Database", "Port" },
	{ "Database", "DBName" },
	{ "Database", "Username" },
	{ "Database", "Password" }
};

Settings& Settings::init() {
	if (!m_ptr) {
		m_ptr = new Settings;
	}
	return *m_ptr;
}

void Settings::setFileName(const std::string &fileName) {
	m_fileName = fileName;
}

std::vector<std::string> Settings::readSettings() {
	std::vector<std::string> settings;
	boost::property_tree::ini_parser::read_ini(m_fileName, m_propertyTree);
	for (auto& it : m_settings) {
		settings.push_back(m_propertyTree.get<std::string>(it.first + '.' + it.second, "null"));
	}
	return settings;
}
