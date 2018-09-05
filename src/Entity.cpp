#include "Entity.h"

Entity::Entity(const std::string& propertiesString) {
	std::size_t pos = 0;
	while (true) {
		auto readQuotedString = [&] {
			pos++;
			auto end = propertiesString.find('"', pos);
			auto s = propertiesString.substr(pos, end - pos);
			pos = end + 1;
			return s;
		};

		pos = propertiesString.find('"', pos);
		if (pos == std::string::npos)
			break;

		auto name = readQuotedString();
		pos = propertiesString.find('"', pos);
		auto value = readQuotedString();
		properties.emplace(std::move(name), std::move(value));
	}
}

// TODO This will need to be modified later when there are more than one of the same type
auto Entity::findProperty(const std::string& name) const -> const std::string* {
	const auto it = properties.find(name);
	if (it != end(properties))
		return &it->second;
	return nullptr;
}
