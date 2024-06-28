#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <regex>
#include <unordered_map>

// Define structure to hold function metadata
struct FunctionInfo {
    std::vector<std::string> outputs;
    std::vector<std::string> inputs;
    std::vector<std::string> body;

    FunctionInfo() = default;
    FunctionInfo(const std::vector<std::string>& outs, const std::vector<std::string>& ins, const std::vector<std::string>& bdy)
        : outputs(outs), inputs(ins), body(bdy) {}
};

// Function to trim whitespace from a string
std::string trim(const std::string& str) {
    size_t first = str.find_first_not_of(' ');
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(' ');
    return str.substr(first, last - first + 1);
}

// Function to split a string by a delimiter
std::vector<std::string> splitString(const std::string& str, const std::string& delimiter) {
    std::vector<std::string> tokens;
    size_t start = 0, end = 0;
    while ((end = str.find(delimiter, start)) != std::string::npos) {
        tokens.push_back(str.substr(start, end - start));
        start = end + delimiter.length();
    }
    tokens.push_back(str.substr(start));
    return tokens;
}

// Function to parse function definitions
void parseFunctionDefinitions(const std::vector<std::string>& lines, std::unordered_map<std::string, FunctionInfo>& functions) {
    std::regex functionPattern(R"(function\s+\[(.*)\]:=\s*(\w+)\((.*)\))");
    std::regex endPattern(R"(end)");
    bool inFunction = false;
    FunctionInfo currentFunction;
    std::string currentFunctionName;

    for (const std::string& line : lines) {
        std::string trimmedLine = trim(line);
        std::smatch match;

        if (!inFunction && std::regex_match(trimmedLine, match, functionPattern)) {
            inFunction = true;
            currentFunctionName = match[2];
            currentFunction.outputs = splitString(match[1], ",");
            currentFunction.inputs = splitString(match[3], ",");
        } else if (inFunction && std::regex_match(trimmedLine, endPattern)) {
            inFunction = false;
            functions[currentFunctionName] = currentFunction;
            currentFunction = FunctionInfo();
        } else if (inFunction) {
            currentFunction.body.push_back(trimmedLine);
        }
    }
}

// Function to inline function calls
void inlineFunctionCalls(std::vector<std::string>& lines, const std::unordered_map<std::string, FunctionInfo>& functions, int& dummyCounter) {
    std::regex callPattern(R"(\[(.*)\]:=\s*(\w+)\((.*)\))");

    for (auto it = lines.begin(); it != lines.end(); ) {
        std::string line = *it;
        std::smatch match;

        if (std::regex_match(line, match, callPattern)) {
            std::string callOutputs = match[1];
            std::string functionName = match[2];
            std::string callInputs = match[3];

            if (functions.find(functionName) != functions.end()) {
                const FunctionInfo& func = functions.at(functionName);
                std::vector<std::string> outputVars = splitString(callOutputs, ",");
                std::vector<std::string> inputVars = splitString(callInputs, ",");

                std::unordered_map<std::string, std::string> variableMapping;

                // Map input variables
                for (size_t i = 0; i < func.inputs.size(); ++i) {
                    variableMapping[func.inputs[i]] = inputVars[i];
                }

                // Map output variables
                for (size_t i = 0; i < func.outputs.size(); ++i) {
                    variableMapping[func.outputs[i]] = outputVars[i];
                }

                // Replace the function call line with the inlined body
                it = lines.erase(it);
                for (const std::string& funcLine : func.body) {
                    std::string processedLine = funcLine;
                    for (const auto& pair : variableMapping) {
                        std::regex varPattern("\\b" + pair.first + "\\b");
                        processedLine = std::regex_replace(processedLine, varPattern, pair.second);
                    }

                    // Handle dummy variables for intermediate variables
                    std::regex dummyVarPattern(R"((\w+):=)");
                    std::smatch dummyVarMatch;
                    if (std::regex_search(processedLine, dummyVarMatch, dummyVarPattern)) {
                        std::string dummyVar = dummyVarMatch[1];
                        if (variableMapping.find(dummyVar) == variableMapping.end() && 
                            std::find(outputVars.begin(), outputVars.end(), dummyVar) == outputVars.end() && 
                            std::find(inputVars.begin(), inputVars.end(), dummyVar) == inputVars.end()) {
                            std::string newDummyVar = dummyVar + std::to_string(dummyCounter);
                            variableMapping[dummyVar] = newDummyVar;
                            std::regex varPattern("\\b" + dummyVar + "\\b");
                            processedLine = std::regex_replace(processedLine, varPattern, newDummyVar);
                        }
                    }

                    it = lines.insert(it, processedLine) + 1;
                }
                dummyCounter++;
            } else {
                ++it;
            }
        } else {
            ++it;
        }
    }
}

// Main function for processing
int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <input file> <output file>\n";
        return 1;
    }

    std::ifstream infile(argv[1]);
    if (!infile.is_open()) {
        std::cerr << "Error opening input file: " << argv[1] << "\n";
        return 1;
    }

    std::ofstream outfile(argv[2]);
    if (!outfile.is_open()) {
        std::cerr << "Error opening output file: " << argv[2] << "\n";
        return 1;
    }

    std::stringstream buffer;
    buffer << infile.rdbuf();
    std::string code = buffer.str();

    // Split the code into lines
    std::istringstream codeStream(code);
    std::vector<std::string> lines;
    std::string tempLine;
    while (std::getline(codeStream, tempLine)) {
        lines.push_back(tempLine);
    }

    // Parse function definitions
    std::unordered_map<std::string, FunctionInfo> functions;
    parseFunctionDefinitions(lines, functions);

    // Remove function definitions from the original lines
    std::vector<std::string> codeLines;
    std::regex functionPattern(R"(function\s+\[(.*)\]:=\s*(\w+)\((.*)\))");
    std::regex endPattern(R"(end)");
    bool inFunction = false;

    for (const std::string& line : lines) {
        std::string trimmedLine = trim(line);
        if (std::regex_match(trimmedLine, functionPattern)) {
            inFunction = true;
        } else if (inFunction && std::regex_match(trimmedLine, endPattern)) {
            inFunction = false;
        } else if (!inFunction) {
            codeLines.push_back(line);
        }
    }

    // Inline function calls
    int dummyCounter = 1;
    inlineFunctionCalls(codeLines, functions, dummyCounter);

    // Output the processed lines
    for (const auto& line : codeLines) {
        outfile << line << "\n";
    }

    return 0;
}




