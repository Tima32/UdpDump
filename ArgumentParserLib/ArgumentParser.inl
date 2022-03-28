#pragma once
#include "ArgumentParser.hpp"

inline ArgumentParser::ArgumentParser()
{
	int argc{ -1 };
	wchar_t** argv{ nullptr };

	argv = CommandLineToArgvW(GetCommandLineW(), &argc);
	if (argv == nullptr)
		throw std::runtime_error("CommandLineToArgvW return nullptr");

	for (int i = 0; i < argc; ++i)
		emplace_back(argv[i]);

	LocalFree(argv);
}
inline ArgumentParser::ArgumentParser(int argc, wchar_t** argv)
{
	for (int i = 0; i < argc; ++i)
		emplace_back(argv[i]);
}
inline ArgumentParser::~ArgumentParser()
{
}

inline size_t ArgumentParser::find(const std::wstring& arg_name) const
{
	for (size_t i = 0; i < size(); ++i)
		if (operator[](i) == arg_name)
			return i;
	return size_t(-1);
}

template<typename T>
inline T ArgumentParser::get(const std::wstring& arg_mame)
{
	size_t pos = find(arg_mame);
	if (pos == -1)
		throw std::logic_error("Argument not found.");
	if (pos + 1 == size())
		throw std::logic_error("There is no data behind the argument.");

	std::wstringstream ss;
	ss << operator[](pos + 1);
	T data;
	ss >> data;

	return data;
}
template<>
inline std::wstring ArgumentParser::get<std::wstring>(const std::wstring& arg_name)
{
	size_t pos = find(arg_name);
	if (pos == -1)
		throw std::logic_error("Argument not found.");
	if (pos + 1 == size())
		throw std::logic_error("There is no data behind the argument.");

	return operator[](pos + 1);
}
template<typename T>
inline T ArgumentParser::get(const std::wstring& arg_name, const T& default_data)
{
	try
	{
		T data = get<T>(arg_name);
		return data;
	}
	catch (const std::logic_error&)
	{
		return default_data;
	}
}