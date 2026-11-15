#pragma once
#include<string>
namespace FileUtils {
	//Using it to read and then return the latest ID
	//Will return "" if file is empty or missing or doesn't have a seperator
	std::string readLastId(const std::string& filename);
}