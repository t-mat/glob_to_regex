#include <stdio.h>
#include "glob_to_regex.hpp"

int main() {
    const std::vector<std::string> specimens = { "at", "bat", "cat", "tab", "tac", };

    const std::string globPattern = "?at";

    const std::string regexStr = GlobToRegex::translateGlobPatternToRegex(globPattern);
    if(regexStr.empty()) {
        printf("error\n");
        exit(EXIT_FAILURE);
    }

    const std::regex::flag_type regexFlags = std::regex::ECMAScript;
    const std::regex r = std::regex(regexStr, regexFlags);

    for(const auto& s : specimens) {
        const bool b = std::regex_match(s, r);
        printf("'%s' %-13s '%s'\n", globPattern.c_str(), b ? "matches" : "doesn't match", s.c_str());
    }

    return 0;
}
