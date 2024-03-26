#include "http_connection.h"

#include <sstream>
#include <iomanip>
#include <locale>
#include <codecvt>
#include <iostream>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = boost::asio::ip::tcp;

Database* HttpConnection::mp_database = nullptr;

std::string url_decode(const std::string& encoded) {
	std::string res;
	std::istringstream iss(encoded);
	char ch;

	while (iss.get(ch)) {
		if (ch == '%') {
			int hex;
			iss >> std::hex >> hex;
			res += static_cast<char>(hex);
		}
		else {
			res += ch;
		}
	}

	return res;
}

std::string convert_to_utf8(const std::string& str) {
	std::string url_decoded = url_decode(str.c_str());
	return url_decoded;
}

HttpConnection::HttpConnection(tcp::socket socket) : socket_(std::move(socket))
{}

void HttpConnection::setDatabase(Database* database) {
	mp_database = database;
}


void HttpConnection::start() {
	if (mp_database) {
		readRequest();
		checkDeadline();
	}
	else {
		throw std::exception("No connection to database.");
	}
}


void HttpConnection::readRequest() {
	auto self = shared_from_this();

	http::async_read(
		socket_,
		buffer_,
		request_,
		[self](beast::error_code ec,
			std::size_t bytes_transferred)
		{
			boost::ignore_unused(bytes_transferred);
			if (!ec)
				self->processRequest();
		});
}

void HttpConnection::processRequest() {
	response_.version(request_.version());
	response_.keep_alive(false);

	switch (request_.method()) {
	case http::verb::get:
		response_.result(http::status::ok);
		response_.set(http::field::server, "Beast");
		createResponseGet();
		break;
	case http::verb::post:
		response_.result(http::status::ok);
		response_.set(http::field::server, "Beast");
		createResponsePost();
		break;

	default:
		response_.result(http::status::bad_request);
		response_.set(http::field::content_type, "text/plain");
		beast::ostream(response_.body())
			<< "Invalid request-method '"
			<< std::string(request_.method_string())
			<< "'";
		break;
	}

	writeResponse();
}


void HttpConnection::createResponseGet() {
	if (request_.target() == "/")
	{
		response_.set(http::field::content_type, "text/html");
		beast::ostream(response_.body())
			<< "<html>\n"
			<< "<head><meta charset=\"UTF-8\"><title>Search Engine</title></head>\n"
			<< "<body>\n"
			<< "<h1>Search Engine</h1>\n"
			<< "<p>Welcome!<p>\n"
			<< "<form action=\"/\" method=\"post\">\n"
			<< "    <label for=\"search\">Search:</label><br>\n"
			<< "    <input type=\"text\" id=\"search\" name=\"search\"><br>\n"
			<< "    <input type=\"submit\" value=\"Search\">\n"
			<< "</form>\n"
			<< "</body>\n"
			<< "</html>\n";
	}
	else {
		response_.result(http::status::not_found);
		response_.set(http::field::content_type, "text/plain");
		beast::ostream(response_.body()) << "File not found\r\n";
	}
}

void HttpConnection::createResponsePost() {
	if (request_.target() == "/") {
		std::string s = buffers_to_string(request_.body().data());

		std::cout << "POST data: " << s << std::endl;

		size_t pos = s.find('=');
		if (pos == std::string::npos) {
			response_.result(http::status::not_found);
			response_.set(http::field::content_type, "text/plain");
			beast::ostream(response_.body()) << "File not found\r\n";
			return;
		}

		std::string key = s.substr(0, pos);
		std::string value = s.substr(pos + 1);

		std::string utf8value = convert_to_utf8(value);

		if (key != "search") {
			response_.result(http::status::not_found);
			response_.set(http::field::content_type, "text/plain");
			beast::ostream(response_.body()) << "File not found\r\n";
			return;
		}

		std::vector<std::string> searchWord = findWords(utf8value);
		std::map<std::string, int> searchResult;
		mp_database->findLinks(searchWord, searchResult);
		std::multimap<int, std::string> multiSearchResult;
		for (const auto& el : searchResult) {
			multiSearchResult.insert(std::pair<int, std::string>(el.second, el.first));
		}

		response_.set(http::field::content_type, "text/html");
		beast::ostream(response_.body())
			<< "<html>\n"
			<< "<head><meta charset=\"UTF-8\"><title>Search Engine</title></head>\n"
			<< "<body>\n"
			<< "<h1>Search Engine</h1>\n"
			<< "<p>Response:<p>\n"
			<< "<ul>\n";

		int i = 0;
		for (auto& url = multiSearchResult.rbegin(); url != multiSearchResult.rend(); ++url) {
			beast::ostream(response_.body())
				<< "<li><a href=\""
				<< url->second << "\">"
				<< url->second << "</a>"
				<< "<p> Count: " << url->first << "<p></li>";
			++i;
			if (i > 10) {
				break;
			}
		}

		beast::ostream(response_.body())
			<< "</ul>\n"
			<< "</body>\n"
			<< "</html>\n";
	}
	else {
		response_.result(http::status::not_found);
		response_.set(http::field::content_type, "text/plain");
		beast::ostream(response_.body()) << "File not found\r\n";
	}
}

void HttpConnection::writeResponse() {
	auto self = shared_from_this();

	response_.content_length(response_.body().size());

	http::async_write(
		socket_,
		response_,
		[self](beast::error_code ec, std::size_t)
		{
			self->socket_.shutdown(tcp::socket::shutdown_send, ec);
			self->deadline_.cancel();
		});
}

void HttpConnection::checkDeadline() {
	auto self = shared_from_this();

	deadline_.async_wait(
		[self](beast::error_code ec) {
			if (!ec) {
				self->socket_.close(ec);
			}
		});
}

std::vector<std::string> HttpConnection::findWords(const std::string& value) {
	std::vector<std::string> result;
	bool beginValue = true;
	bool space = false;
	std::string word;
	for (const auto& el : value) {
		if ((el >= 'A' && el <= 'Z') || (el >= 'a' || el <= 'z') || el == '+') {
			if (el == '+') {
				space = true;
			}
			else {
				word.push_back(el);
				beginValue = false;
				space = false;
			}
		}

		if (space) {
			if (!beginValue) {
				result.push_back(toLower(word));
				word.clear();
				beginValue = true;
			}
		}
	}
	if (!word.empty())
		result.push_back(toLower(word));

	return result;
}

std::string HttpConnection::toLower(std::string str) {
	boost::locale::generator gen;
	std::locale loc = gen("");
	std::locale::global(loc);
	std::cout.imbue(loc);

	return boost::locale::to_lower(str);
}

