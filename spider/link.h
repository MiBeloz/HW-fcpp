#pragma once 
#include <string>
#include <ostream>

enum class ProtocolType {
	HTTP = 0,
	HTTPS = 1
};

struct Link {
	ProtocolType protocol = ProtocolType::HTTP;
	std::string hostName;
	std::string query;

	bool operator==(const Link& l) const {
		return protocol == l.protocol && hostName == l.hostName && query == l.query;
	}

	bool operator<(const Link& r) const {
		return std::to_string(static_cast<int>(protocol)) + hostName + query < std::to_string(static_cast<int>(r.protocol)) + r.hostName + r.query;
	}

	std::string to_string() const {
		std::string out;
		if (protocol == ProtocolType::HTTP) {
			out = "HTTP";
		}
		else {
			out = "HTTPS";
		}
		out += "://" + hostName + query;
		return out;
	}
};
