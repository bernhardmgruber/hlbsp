#include "hud.h"

void Hud::print(std::string text) {
	m_console.push_back(std::move(text));
}
