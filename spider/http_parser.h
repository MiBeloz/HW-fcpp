#pragma once

#include <map>
#include <regex>
#include <iostream>
#include <boost/locale.hpp>

#include "link.h"

class HTTP_Parser final {
public:
	HTTP_Parser(Link link, std::string html);
	std::map<std::string, int> get_words() const;
	std::set<Link> get_links() const;

private:
	Link m_link;
	std::string m_html;
	static std::vector<std::regex> m_tagsPatterns;
	static std::regex m_wordPattern;
	static std::regex m_linkPattern;

	void removeNewlines();
	void deleteTags(std::string& str, const char startTag, const char endTag) const;
	std::string getHostName(std::string str) const;
	std::string toLower(std::string str) const;
};