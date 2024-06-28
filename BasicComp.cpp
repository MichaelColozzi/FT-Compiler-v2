#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <regex>
#include <algorithm>

// Function to trim leading and trailing whitespace from a string
std::string trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t");
    if (first == std::string::npos) return ""; // no content
    size_t last = str.find_last_not_of(" \t");
    return str.substr(first, last - first + 1);
}

void processLine(const std::string& line, std::ofstream& outFile) {
    std::regex expressionPattern(R"((\w+)\s*:=\s*(.+?)\s*(?:\$\s*(\S+))?$)");
    std::smatch matches;

    std::string trimmedLine = trim(line);

    if (trimmedLine.empty()) {
        return; // Ignore empty or whitespace-only lines
    }

    if (std::regex_search(trimmedLine, matches, expressionPattern)) {
        std::string variable = matches[1];
        std::string function = matches[2];
        std::string activator = matches[3].matched ? matches[3].str() : "1";

        outFile << "  <Setter variable=\"" << variable 
                << "\" function=\"" << function 
                << "\" activator=\"" << activator 
                << "\" priority=\"0\" />" << std::endl;
    }
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <input_file> <output_file>" << std::endl;
        return 1;
    }

    std::string inputFileName = argv[1];
    std::string outputFileName = argv[2];

    std::ifstream inFile(inputFileName);
    std::ofstream outFile(outputFileName);

    if (!inFile.is_open()) {
        std::cerr << "Error opening input file!" << std::endl;
        return 1;
    }

    if (!outFile.is_open()) {
        std::cerr << "Error opening output file!" << std::endl;
        return 1;
    }

    outFile << "<Variables>" << std::endl;

    std::string line;
    while (std::getline(inFile, line)) {
        processLine(line, outFile);
    }

    outFile << "</Variables>" << std::endl;

    inFile.close();
    outFile.close();

    std::cout << "Conversion complete. Check " << outputFileName << " for the result." << std::endl;

    return 0;
}





