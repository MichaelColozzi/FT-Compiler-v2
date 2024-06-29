#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <regex>
#include <unordered_map>
#include <map>

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
void parseFunctionDefinitions(const std::vector<std::string>& lines, std::map<std::string, FunctionInfo>& functions) {
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

// Function to generate a new dummy variable name
std::string getNewDummyVariable(int& counter) {
    return "dummy" + std::to_string(counter++);
}

// Function to unwrap nested function calls
std::string unwrapNestedCalls(const std::string& expr, const std::map<std::string, FunctionInfo>& functions, std::vector<std::string>& newLines, int& dummyCounter) {
    std::string modifiedExpr = expr;
    std::vector<std::pair<int, std::string>> functionCalls;
    std::regex callPattern(R"((\w+)\(([^()]*)\))");
    std::smatch match;

    // Find all function calls
    for (auto it = functions.rbegin(); it != functions.rend(); ++it) {
        const std::string& functionName = it->first;
        std::regex callPattern(functionName + R"(\(([^()]*)\))");

        auto start = std::sregex_iterator(modifiedExpr.begin(), modifiedExpr.end(), callPattern);
        auto end = std::sregex_iterator();

        for (std::sregex_iterator i = start; i != end; ++i) {
            std::smatch match = *i;
            functionCalls.push_back({match.position(), match.str()});
        }
    }

    // Sort function calls by their position in the string
    std::sort(functionCalls.begin(), functionCalls.end(), [](const auto& a, const auto& b) {
        return a.first > b.first; // Sort in reverse order
    });

    // Replace function calls with dummy variables
    for (const auto& call : functionCalls) {
        std::string functionCall = call.second;
        std::regex callPattern(R"((\w+)\(([^()]*)\))");
        std::smatch match;

        if (std::regex_match(functionCall, match, callPattern)) {
            std::string functionName = match[1];
            std::string functionArgs = match[2];

            if (functions.find(functionName) != functions.end()) {
                std::string newDummyVar = getNewDummyVariable(dummyCounter);
                newLines.push_back(newDummyVar + ":=" + functionName + "(" + functionArgs + ")");
                modifiedExpr.replace(call.first, functionCall.length(), newDummyVar);
            }
        }
    }

    return modifiedExpr;
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
    std::map<std::string, FunctionInfo> functions;
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

    // Handle nested function compositions and regular function calls
    int dummyCounter = 1;
    std::vector<std::string> newLines;
    for (const std::string& line : codeLines) {
        std::regex assignmentPattern(R"((\w+)\s*:=\s*(.*))");
        std::smatch match;
        if (std::regex_match(line, match, assignmentPattern)) {
            std::string lhs = match[1];
            std::string rhs = match[2];

            // Unwrap nested function calls
            std::string newRhs = unwrapNestedCalls(rhs, functions, newLines, dummyCounter);
            newLines.push_back(lhs + ":=" + newRhs);
        } else {
            newLines.push_back(line);
        }
    }

    // Output the processed lines
    for (const auto& line : newLines) {
        outfile << line << "\n";
    }

    return 0;
}






