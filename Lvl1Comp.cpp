#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <regex>
#include <unordered_map>
#include <vector>

// Function to trim leading and trailing whitespace from a string
std::string trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t");
    if (first == std::string::npos) return ""; // no content
    size_t last = str.find_last_not_of(" \t");
    return str.substr(first, last - first + 1);
}

// Function to split a string by a delimiter
std::vector<std::string> split(const std::string& str, char delimiter) {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(str);
    while (std::getline(tokenStream, token, delimiter)) {
        tokens.push_back(trim(token));
    }
    return tokens;
}

// Structure to hold function details
struct Function {
    std::vector<std::string> outputs;
    std::vector<std::string> inputs;
    std::vector<std::string> body;
};

// Function to parse functions from Level 1 code
void parseFunctions(std::ifstream& inFile, std::unordered_map<std::string, Function>& functions) {
    std::string line;
    std::regex functionPattern(R"(function\s+\[(.*?)\]:=\s*(\w+)\((.*?)\))");
    std::regex endPattern(R"(end)");
    bool inFunction = false;
    Function currentFunction;
    std::string currentFunctionName;

    while (std::getline(inFile, line)) {
        std::smatch matches;
        if (!inFunction && std::regex_search(line, matches, functionPattern)) {
            inFunction = true;
            currentFunctionName = matches[2];
            currentFunction.outputs = split(matches[1], ',');
            currentFunction.inputs = split(matches[3], ',');
        } else if (inFunction && std::regex_search(line, endPattern)) {
            inFunction = false;
            functions[currentFunctionName] = currentFunction;
            currentFunction = Function();
        } else if (inFunction) {
            currentFunction.body.push_back(trim(line));
        }
    }
}

// Function to process a line of Level 1 code and convert to Level 0 syntax
std::vector<std::string> processFunctionCall(const std::string& line, const std::unordered_map<std::string, Function>& functions) {
    std::vector<std::string> outputLines;
    std::string trimmedLine = trim(line);
    
    if (trimmedLine.empty()) {
        return outputLines; // Ignore empty or whitespace-only lines
    }

    std::regex callPattern(R"(\[(.*?)\]:=\s*(\w+)\((.*?)\))");
    std::smatch matches;

    if (std::regex_search(trimmedLine, matches, callPattern)) {
        std::string callOutputs = matches[1];
        std::string functionName = matches[2];
        std::string callInputs = matches[3];

        if (functions.find(functionName) != functions.end()) {
            const Function& func = functions.at(functionName);
            std::vector<std::string> outputVars = split(callOutputs, ',');
            std::vector<std::string> inputVars = split(callInputs, ',');

            // Inline the function body with substituted variables
            for (const std::string& funcLine : func.body) {
                std::string processedLine = funcLine;
                for (size_t i = 0; i < func.inputs.size(); ++i) {
                    std::regex inputPattern("\\b" + func.inputs[i] + "\\b");
                    processedLine = std::regex_replace(processedLine, inputPattern, inputVars[i]);
                }
                for (size_t i = 0; i < func.outputs.size(); ++i) {
                    std::regex outputPattern("\\b" + func.outputs[i] + "\\b");
                    processedLine = std::regex_replace(processedLine, outputPattern, outputVars[i]);
                }
                outputLines.push_back(processedLine);
            }
        }
    } else {
        outputLines.push_back(trimmedLine);
    }

    return outputLines;
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

    // Parse functions from the Level 1 input file
    std::unordered_map<std::string, Function> functions;
    parseFunctions(inFile, functions);

    // Reset the input file stream to process the lines again
    inFile.clear();
    inFile.seekg(0, std::ios::beg);

    // Process lines and convert to Level 0 syntax
    std::string line;

    while (std::getline(inFile, line)) {
        if (line.find("function") == std::string::npos && line.find("end") == std::string::npos) {
            std::vector<std::string> lines = processFunctionCall(line, functions);
            for (const std::string& processedLine : lines) {
                if (!processedLine.empty()) {
                    outFile << processedLine << std::endl;
                }
            }
        }
    }

    inFile.close();
    outFile.close();

    std::cout << "Conversion complete. Check " << outputFileName << " for the result." << std::endl;

    return 0;
}
