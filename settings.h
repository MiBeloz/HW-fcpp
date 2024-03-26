#pragma once

#include <string>
#include <vector>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>


enum eSettings {
	Spider_StartContentProtocol,
	Spider_StartContentHostName,
	Spider_StartContentQuery,
	Spider_Depth,
	HTTPServer_Address,
	HTTPServer_Port,
	DB_Host,
	DB_Port,
	DB_DBName,
	DB_Username,
	DB_Password
};

class Settings final {
public:
	Settings(const Settings&) = delete;
	Settings& operator=(const Settings&) = delete;

	static Settings& init();
	void setFileName(const std::string &fileName);
	std::vector<std::string> readSettings();

protected:
	Settings() {};

private:
	static Settings* m_ptr;
	static std::vector<std::pair<std::string, std::string>> m_settings;
	std::string m_fileName;
	boost::property_tree::ptree m_propertyTree;
};