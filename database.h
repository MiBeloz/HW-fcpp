#pragma once

#include <memory>
#include <pqxx/pqxx>

#include "spider/link.h"

class Database final {
public:
	Database(const Database&) = delete;
	Database& operator=(const Database&) = delete;
	Database(Database&&) = delete;
	Database& operator=(Database&&) = delete;

	explicit Database(const std::string& host, const std::string& port, const std::string& db_name, const std::string& username, const std::string& password);

	void addLink(std::string link) const;
	void addWord(std::string word) const;
	void addLinkWord(std::string link, std::string word, int count) const;
	void updateLinkWord(std::string link, std::string word, int count) const;

	void findLinks(const std::vector<std::string>& words, std::map<std::string, int>& result) const;

private:
	std::string m_host;
	std::string m_port;
	std::string m_db_name;
	std::string m_username;
	std::string m_password;
	std::unique_ptr<pqxx::connection> m_con;

	void makeDatabase() const;

	int get_id_link(std::string link) const;
	int get_id_word(std::string word) const;

	void exec(std::string str) const;
	void exec_prepared_add_link(std::string link) const;
	void exec_prepared_add_word(std::string word) const;
	void exec_prepared_add_LinkWord(int link, int word, int count) const;
	void exec_prepared_update_LinkWord(int link, int word, int count) const;
};
