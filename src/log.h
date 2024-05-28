#pragma once

#include <string>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>

const std::string log_filename = "/tmp/test.log";

template<typename ... Args>
inline std::string string_format( const std::string& format, Args ... args )
{
	int size_s = std::snprintf( nullptr, 0, format.c_str(), args ... ) + 1; // Extra space for '\0'
	if( size_s <= 0 ){ throw std::runtime_error( "Error during formatting." ); }
	auto size = static_cast<size_t>( size_s );
	std::unique_ptr<char[]> buf( new char[ size ] );
	std::snprintf( buf.get(), size, format.c_str(), args ... );
	return std::string( buf.get(), buf.get() + size - 1 ); // We don't want the '\0' inside
}

template<typename ... Args>
void write_log(const std::string& format, Args ... args) {
	std::ofstream file(log_filename.c_str(), std::ios::in | std::ios::out | std::ios::app);

	if (file.is_open()) {
		file << string_format(format, args ...);
		file.close();
	}
}

template<typename ... Args>
void Log(const std::string& format, Args ... args) {
	write_log(std::string("[Log] ") + format, args ...);
}

template<typename ... Args>
void Error(const std::string& format, Args ... args) {
	write_log(std::string("[Error] ") + format, args ...);
}

template<typename ... Args>
void Warning(const std::string& format, Args ... args) {
	write_log(std::string("[Warning] ") + format, args ...);
}