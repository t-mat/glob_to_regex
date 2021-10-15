// Multiple glob pattern matching.  For C++17.  Very slow.
//
// gcc  : g++     -std=c++17 -I .. 2-glob-rules.cpp && pushd .. && examples/a.out && popd
// clang: clang++ -std=c++17 -I .. 2-glob-rules.cpp && pushd .. && examples/a.out && popd
// MSVC : cl.exe /std:c++17 /Zc:__cplusplus /EHsc /I .. 2-glob-rules.cpp && pushd .. && examples\2-glob-rules.exe && popd

#include <stdio.h>
#include "glob_to_regex.hpp"

int main() {
    std::vector<std::vector<std::string>> rules = {
        { "Makefile",       "Makefile (text)",      },
        { "*.cpp",          "C++ source code",      },
        { "tests/*.cpp",    "C++ source for test",  },  // later rule has higher priority
        { "*.hpp",          "C++ header",           },
        { "*.o",            "Object file",          },
    };

    for(const auto& de : std::filesystem::recursive_directory_iterator(".")) {
        if(! de.is_regular_file()) {
            continue;
        }
        const std::string pathStr = de.path().generic_u8string();
        int lastIndex = -1;
        for(int i = 0; i < (int) rules.size(); ++i) {
            std::string globPattern = std::string("**/") + rules[i][0];
            const std::string regexStr = GlobToRegex::translateGlobPatternToRegex(globPattern);
            const std::regex r = std::regex(regexStr, std::regex::ECMAScript);
            if(std::regex_match(pathStr, r)) {
                lastIndex = i;
            }
        }
        if(lastIndex >= 0) {
            printf("%-40s", pathStr.c_str());
            for(const auto& e : rules[lastIndex]) {
                printf(", %s", e.c_str());
            }
            printf("\n");
        }
    }
}
