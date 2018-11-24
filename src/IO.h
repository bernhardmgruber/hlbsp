#pragma once

#include <filesystem>
#include <fstream>
#include <istream>
#include <iterator>
#include <vector>

namespace fs = std::filesystem;

template<typename T>
void read(std::istream& is, T& t) {
	is.read(reinterpret_cast<char*>(&t), sizeof(T));
}

template<typename T>
auto read(std::istream& is) {
	T t;
	is.read(reinterpret_cast<char*>(&t), sizeof(T));
	return t;
}

template<typename T>
void readVector(std::istream& is, std::vector<T>& v) {
	is.read(reinterpret_cast<char*>(v.data()), sizeof(T) * v.size());
}

template<typename T>
auto readVector(std::istream& is, std::size_t count) {
	std::vector<T> v(count);
	is.read(reinterpret_cast<char*>(v.data()), sizeof(T) * v.size());
	return v;
}

inline auto readTextFile(const fs::path& filename) {
	std::string content;
	std::ifstream file(filename);
	if (!file)
		throw std::ios::failure("Failed to open file " + filename.string() + " for reading");
	file.seekg(std::ios::end);
	content.reserve(file.tellg());
	file.seekg(std::ios::beg);
	content.assign(std::istreambuf_iterator<char>{file}, std::istreambuf_iterator<char>{});
	return content;
}
