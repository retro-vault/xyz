#include <iostream>
#include <iomanip>
#include <fstream>
#include <string>
#include <vector>
#include <stdexcept>
#include <sstream>

#include "token.h"
#include "lexer.h"

// Read entire file into a string
std::string read_file(const std::string &filename)
{
    std::ifstream file(filename);
    if (!file.is_open())
    {
        throw std::runtime_error("Could not open input file: " + filename);
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

int main(int argc, char *argv[])
{

    try
    {
        // Read input file
        // std::string source = read_file(input_file);
        /*
        // Scan source into tokens
        scanner s(source);
        auto tokens = s.scan_tokens();

        // Assemble tokens into object
        assembler a(tokens);
        object obj = a.assemble();

        // Write REL output

        // Print label table (optional)
        std::cout << "Assembly successful. Label table:" << std::endl;
        for (const auto &[label, addr] : obj.labels)
        {
            std::cout << "  " << label << ": 0x" << std::hex << std::setw(4)
                      << std::setfill('0') << addr << std::dec << std::endl;
        }

        std::cout << "Output written to " << output_file << " (" << obj.code.size()
                  << " bytes)" << std::endl;
        */
        return 0;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    catch (...)
    {
        std::cerr << "Error: Unknown error during assembly" << std::endl;
        return 1;
    }
}