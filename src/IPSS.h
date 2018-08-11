#pragma once

#include <string>
#include <sstream>

class IPSS {
public:
	IPSS() = default;
	IPSS(const IPSS&) = delete;
	IPSS(IPSS&&) = default;

	auto str() const -> std::string {
		return oss.str();
	}

	operator std::string() const {
		return oss.str();
	}

	template<typename T>
	friend auto operator<<(IPSS&& ipss, T&& t) -> IPSS {
		ipss.oss << std::forward<T>(t);
		return std::move(ipss);
	}

private:
	std::ostringstream oss;
};
