#include "http_parser.h"

std::vector<std::regex> HTTP_Parser::m_tagsPatterns{ 
    std::regex(R"((\<h[1-6](.*?)\>)(.*?)(\<\/h[1-6]\>))"),
    std::regex(R"((\<p(.*?)\>)(.*?)(\<\/p\>))"),
    std::regex(R"((\<title(.*?)\>)(.*?)(\<\/title\>))")
};
std::regex HTTP_Parser::m_wordPattern(R"([A-Za-zР-пр-џ]{3,})");
std::regex HTTP_Parser::m_linkPattern(R"(\<a\shref\=\"(.*?)\")");

HTTP_Parser::HTTP_Parser(Link link, std::string html) : m_link(link), m_html(html) {
    removeNewlines();
}

std::map<std::string, int> HTTP_Parser::get_words() const {
    std::map<std::string, int> foundWorlds;

    for (const auto& el : m_tagsPatterns) {
        auto itBegin = std::sregex_iterator(m_html.begin(), m_html.end(), el);
        auto itEnd = std::sregex_iterator();
        std::string match_str;
        for (std::sregex_iterator i = itBegin; i != itEnd; ++i) {
            std::smatch match = *i;
            match_str += match.str();
        }
        deleteTags(match_str, '<', '>');
        
        auto words_begin = std::sregex_iterator(match_str.begin(), match_str.end(), m_wordPattern);
        auto words_end = std::sregex_iterator();
        for (std::sregex_iterator i = words_begin; i != words_end; ++i) {
            std::smatch match = *i;
            std::string match_string = match.str();
            match_string = toLower(match_string);
            auto it = foundWorlds.find(match_string);
            if (it != foundWorlds.end()) {
                int key = foundWorlds[match_string];
                foundWorlds[match_string] = ++key;
            }
            else {
                foundWorlds[match_string] = 1;
            }
        }
    }
    return foundWorlds;
}

std::set<Link> HTTP_Parser::get_links() const {
    auto linkBegin = std::sregex_iterator(m_html.begin(), m_html.end(), m_linkPattern);
    auto linkEnd = std::sregex_iterator();
    std::vector<std::string> foundLinks;
    std::set<Link> resultLinks;

    for (std::sregex_iterator i = linkBegin; i != linkEnd; ++i) {
        std::smatch match = *i;
        foundLinks.push_back(match.str());
    }

    for (auto& it : foundLinks) {
        Link link;

        deleteTags(it, '<', '\"');
        deleteTags(it, '\"', '\"');

        size_t n = it.find("http://");
        if (n != std::string::npos) {
            link.protocol = ProtocolType::HTTP;
            it.erase(0, 7);
            link.hostName = getHostName(it);
            it.erase(0, link.hostName.size() + 1);
            link.query = it;
        }
        else {
            n = it.find("https://");
            if (n != std::string::npos) {
                link.protocol = ProtocolType::HTTPS;
                it.erase(0, 8);
                link.hostName = getHostName(it);
                it.erase(0, link.hostName.size() + 1);
                link.query = it;
            }
            else {
                link.protocol = m_link.protocol;
                link.hostName = m_link.hostName;
                link.query = it;
            }
        }

        if (link.query.empty()) {
            link.query = "/";
        }
        else {
            if (link.query[0] != '/') {
                link.query = "/" + link.query;
            }
        }
        
        resultLinks.insert(link);
    }
    return resultLinks;
}

void HTTP_Parser::removeNewlines() {
    if (m_html.empty()) {
        return;
    }

    std::string result_html;
    for (int i = 0; i < m_html.size(); ++i) {
        if (m_html[i] != '\n' && m_html[i] != '\r') {
            result_html.push_back(m_html[i]);
        }
    }
    m_html = std::move(result_html);
}

void HTTP_Parser::deleteTags(std::string& str, const char startTag, const char endTag) const {
    if (str.empty()) {
        return;
    }

    auto itStart = str.begin();
    auto itEnd = str.begin();
    while (itStart != str.end() && itEnd != str.end()) {
        itStart = std::find(str.begin(), str.end(), startTag);
        itEnd = std::find(itStart, str.end(), endTag);

        if (itStart != str.end() && itEnd != str.end()) {
            str.erase(itStart, ++itEnd);
        }
    }
}

std::string HTTP_Parser::getHostName(std::string str) const {
    if (str.empty()) {
        return {};
    }

    auto it = std::find(str.begin(), str.end(), '/');
    if (it != str.end()) {
        str.erase(it, str.end());
    }
    return str;
}

std::string HTTP_Parser::toLower(std::string str) const {
    boost::locale::generator gen;
    std::locale loc = gen("");
    std::locale::global(loc);
    std::cout.imbue(loc);

    return boost::locale::to_lower(str);
}
