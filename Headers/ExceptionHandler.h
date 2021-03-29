#pragma once

#include <string>
#include <sstream>

class ExceptionHandler
{
public:
	ExceptionHandler(int line, std::string file, std::string message);
	~ExceptionHandler();

	std::string getType(void);
	std::string getErrorDescription(void);
private:
	int line;
	std::string file;
protected:
	std::string type; // changes based on exception source
	std::string errorDescription; // windows translated description of HR
};