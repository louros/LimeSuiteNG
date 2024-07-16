#ifndef COEFFICIENT_FILE_PARSER_H
#define COEFFICIENT_FILE_PARSER_H

/**
@file	CoefficientFileParser.h
@author	Lime Microsystems
@brief	The FIR coefficient file parser.
*/

#include "limesuiteng/config.h"

#include <fstream>
#include <filesystem>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace lime {

class CoefficientFileParser
{
  public:
    CoefficientFileParser(const std::filesystem::path& filename);

    int getCoefficients(std::vector<double>& coefficients, int max);
    void saveToFile(const std::vector<double>& coefficients);

  private:
    enum class ErrorCodes : int8_t {
        SUCCESS = 0,
        END_OF_FILE = -1,
        SYNTAX_ERROR = -2,
        EMPTY_FILENAME = -3,
        UNOPENABLE_FILE = -4,
        TOO_MANY_COEFFS = -5,
    };

    ErrorCodes getValue(std::ifstream& file, double& value);
    void parseMultilineComments(std::ifstream& file, std::string& token);

    std::filesystem::path filename;
    std::stringstream tokenBuffer;
};

} // namespace lime

#endif // COEFFICIENT_FILE_PARSER_H
