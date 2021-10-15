// String matching test with glob patterns (C++11)
//
// gcc  : g++     -std=c++11 -I .. 0-glob-to-regex.cpp && ./a.out
// clang: clang++ -std=c++11 -I .. 0-glob-to-regex.cpp && ./a.out
// MSVC : cl.exe /I .. 0-glob-to-regex.cpp && .\0-glob-to-regex.exe

#include <stdio.h>
#include "glob_to_regex.hpp"

int main() {
    const std::vector<std::string> specimens = { "at", "bat", "cat", "tab", "tac", };

    const std::string globPattern = "?at";

    GlobToRegex::GlobToRegexErrc error;
    const std::string regexStr = GlobToRegex::translateGlobPatternToRegex(globPattern, error);
    if(GlobToRegex::any(error)) {
        printf("error\n");
        exit(EXIT_FAILURE);
    }

    const std::regex::flag_type regexFlags = std::regex::ECMAScript;
    const std::regex r = std::regex(regexStr, regexFlags);

    for(const auto& s : specimens) {
        const bool b = std::regex_match(s, r);
        printf("'%s' %-13s '%s'\n", globPattern.c_str(), b ? "matches" : "doesn't match", s.c_str());
    }
}
