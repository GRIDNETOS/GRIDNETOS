#pragma once
#include "enums.h"
#include <mutex>
#include <vector>

/// <summary>
/// Defines the Command-Line command.
/// Note:Command-Line command can CONTAIN GridScript instructions. (not the other way around)
/// </summary>
class CCLCommand
{
private:
	std::string mCommand;// command name ex. 'bye'
	std::vector<std::string> mParams;// type-independant string representation of cmd params
public:
};
