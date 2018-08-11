#pragma once

#include <string>
#include <vector>

class Hud {
public:
	void print(std::string text);

	const auto& console() const { return m_console; }

private:
	std::vector<std::string> m_console;
};
