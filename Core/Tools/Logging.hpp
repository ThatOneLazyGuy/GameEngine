#pragma once

#include <format>
#include <string>
#include <chrono>
#include <iostream>

namespace Log
{
	using namespace std::chrono;

	// Print logs buffered to stderr.
	template <typename... Types>
	void Log(const std::string& format, const Types&... args)
	{
        const auto now = time_point_cast<seconds>(system_clock::now());
		try
		{
			std::clog << std::vformat("[{:%F, %T}] " + format, std::make_format_args(now, args...)) << '\n';
		}
		catch (...)
		{
			// If the printed format string is invalid it'll throw an error, we catch it to simply print the format string raw.
			std::clog << std::vformat("[{:%F, %T}] ", std::make_format_args(now)) << format << '\n';
		}
	}

	// Print errors non-buffered to stderr.
	template <typename... Types>
	void Error(const std::string& format, const Types&... args)
	{
        const auto now = time_point_cast<seconds>(system_clock::now());
		try
		{
			std::cerr << std::vformat("[{:%F, %T}] " + format, std::make_format_args(now, args...)) << '\n';
		}
		catch (...)
		{
			// If the printed format string is invalid it'll throw an error, we catch it to simply print the format string raw.
			std::cerr << std::vformat("[{:%F, %T}] ", std::make_format_args(now)) << format << '\n';
		}
	}
}