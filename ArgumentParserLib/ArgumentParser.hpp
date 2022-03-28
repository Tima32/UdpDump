#pragma once
#include <stdexcept>
#include <vector>
#include <string>
#include <sstream>

class ArgumentParser : public std::vector<std::string>
{
public:
	//ArgumentParser();
	ArgumentParser(int argc, const char** argv);
	~ArgumentParser();

	size_t find(const std::string& arg_name) const;

	template<typename T> T  get              (const std::string& arg_mame);
	//template<> std::string  get(const std::string& arg_mame);
	template<typename T> T  get              (const std::string& arg_name, const T& default_data);

private:
};

#include "ArgumentParser.inl"