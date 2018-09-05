#pragma once

#include <string>
#include <unordered_map>

class Entity {
public:
	explicit Entity(const std::string& propertiesString);
	auto findProperty(const std::string& name) const -> const std::string*;

private:
	std::unordered_map<std::string, std::string> properties;
};
