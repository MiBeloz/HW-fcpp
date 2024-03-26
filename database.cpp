#include "database.h"

Database::Database(const std::string& host, const std::string& port, const std::string& db_name, const std::string& username, const std::string& password) :
	m_host(host), m_port(port), m_db_name(db_name), m_username(username), m_password(password) {
	m_con = std::make_unique<pqxx::connection>(
		"host=" + m_host + " "
		"port=" + m_port + " "
		"dbname=" + m_db_name + " "
		"user=" + m_username + " "
		"password=" + m_password
	);

	makeDatabase();

	m_con->prepare("add_Link", "INSERT INTO Links(link) VALUES ($1)");
	m_con->prepare("add_Word", "INSERT INTO Words(word) VALUES ($1)");
	m_con->prepare("add_LinkWord", "INSERT INTO LinksWords(link_id, word_id, count) VALUES ($1, $2, $3)");
	m_con->prepare("update_LinkWord", "UPDATE LinksWords SET count = $3 WHERE link_id = $1 AND word_id = $2;");
}

void Database::addLink(std::string link) const {
	exec_prepared_add_link(link);
}

void Database::addWord(std::string word) const {
	exec_prepared_add_word(word);
}

void Database::addLinkWord(std::string link, std::string word, int count) const {
	int id_link = get_id_link(link);
	int id_word = get_id_word(word);
	exec_prepared_add_LinkWord(id_link, id_word, count);
}

void Database::updateLinkWord(std::string link, std::string word, int count) const {
	int id_link = get_id_link(link);
	int id_word = get_id_word(word);
	exec_prepared_update_LinkWord(id_link, id_word, count);
}

void Database::findLinks(const std::vector<std::string>& words, std::map<std::string, int>& result) const {
	pqxx::work tx(*m_con);

	bool firstWord = true;
	for (const auto& word : words) {
		std::map<std::string, int> tempResult;
		for (const auto& [link, count] : tx.query<std::string, int>(
			"SELECT l.link, lw.count FROM Links l "
			"LEFT JOIN LinksWords lw ON l.id = lw.link_id "
			"LEFT JOIN Words w ON lw.word_id = w.id "
			"WHERE word = '" + word + "'")) {
			auto it = tempResult.find(link);
			if (it != tempResult.end()) {
				tempResult[link] = tempResult[link] + count;
			}
			else {
				tempResult.insert(std::pair<std::string, int>(link, count));
			}
		}

		if (firstWord) {
			result = std::move(tempResult);
		}
		else {
			std::map<std::string, int> uniqueTempResult;
			for (const auto& el : tempResult) {
				auto it = result.find(el.first);
				if (it != result.end()) {
					uniqueTempResult.insert(std::pair<std::string, int>(el.first, el.second + it->second));
				}
			}
			result = std::move(uniqueTempResult);
		}

		firstWord = false;
	}

	tx.abort();
}

void Database::makeDatabase() const {
	exec("CREATE TABLE IF NOT EXISTS Links ("
		 "id SERIAL PRIMARY KEY, "
		 "link text UNIQUE NOT NULL)");

	exec("CREATE TABLE IF NOT EXISTS Words ("
		 "id SERIAL PRIMARY KEY, "
		 "word text UNIQUE NOT NULL)");

	exec("CREATE TABLE IF NOT EXISTS LinksWords ("
		 "link_id INTEGER REFERENCES Links(id), "
		 "word_id INTEGER REFERENCES Words(id), "
		 "count INTEGER, "
		 "CONSTRAINT pk_LinksWords PRIMARY KEY(link_id, word_id))");
}

int Database::get_id_link(std::string link) const {
	pqxx::work tx(*m_con);
	int id_link = tx.query_value<int>("SELECT id FROM Links WHERE link = '" + link + "'");
	tx.abort();

	return id_link;
}

int Database::get_id_word(std::string word) const {
	pqxx::work tx(*m_con);
	int id_word = tx.query_value<int>("SELECT id FROM Words WHERE word = '" + word + "'");
	tx.abort();

	return id_word;
}

void Database::exec(std::string str) const {
	pqxx::work tx(*m_con);
	tx.exec(str);
	tx.commit();
}

void Database::exec_prepared_add_link(std::string link) const {
	pqxx::work tx(*m_con);
	tx.exec_prepared("add_Link", link);
	tx.commit();
}

void Database::exec_prepared_add_word(std::string word) const {
	pqxx::work tx(*m_con);
	tx.exec_prepared("add_Word", word);
	tx.commit();
}

void Database::exec_prepared_add_LinkWord(int link, int word, int count) const {
	pqxx::work tx(*m_con);
	tx.exec_prepared("add_LinkWord", link, word, count);
	tx.commit();
}

void Database::exec_prepared_update_LinkWord(int link, int word, int count) const {
	pqxx::work tx(*m_con);
	tx.exec_prepared("update_LinkWord", link, word, count);
	tx.commit();
}
