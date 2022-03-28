#pragma once
#include <stdexcept>
#include <vector>
#include <string>
#include <sstream>
#include <windows.h>

class ArgumentParser : public std::vector<std::wstring>
{
public:
	ArgumentParser();
	ArgumentParser(int argc, wchar_t** argv);
	~ArgumentParser();

	size_t find(const std::wstring& arg_name) const;

	template<typename T> T  get              (const std::wstring& arg_mame);
	template<> std::wstring get<std::wstring>(const std::wstring& arg_mame);
	template<typename T> T  get              (const std::wstring& arg_name, const T& default_data);

private:
};

#include "ArgumentParser.inl"