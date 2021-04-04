#include "ExceptionHandler.h"

ExceptionHandler::ExceptionHandler(int l, std::string f, std::string description)
{
    line = l;
    file = f;
    errorDescription = description;
}

ExceptionHandler::~ExceptionHandler(void)
{
}

std::string ExceptionHandler::getType(void)
{
    return type;
}

std::string ExceptionHandler::getErrorDescription(void)
{
    std::ostringstream oss;
    oss << std::endl << errorDescription << std::endl
        << "Line: " << line << std::endl
        << "File : " << file << std::endl;
    errorDescription = oss.str();
    return errorDescription;
}
