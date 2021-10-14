# GlobToRegex

- Glob pattern to regex translator in C++11.
- Directory traversal with glob pattern in C++17.

`GlobToRegex` supports the following glob notations:

- `?`
- `*`
- `**`
- `[a-z]`, `[!a-z]`


## String matching test with glob patterns (C++11)

```c++
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
```


## Directory traversal with glob pattern matching (C++17)

```c++
#include <stdio.h>
#include "glob_to_regex.hpp"

int main(int argc, const char** argv) {
#if _WIN32
    const bool caseSensitivity = false;
    const auto home = std::string(getenv_("USERPROFILE"));
#else
    const bool caseSensitivity = true;
    const auto home = std::string(getenv("HOME"));
#endif
    const bool followSimlink = true;

    std::filesystem::path globPattern = home + "/**/*.txt";
    printf("globPattern=%s\n", (const char*) globPattern.generic_u8string().c_str());

    GlobToRegex::dirWalk(
          caseSensitivity
        , followSimlink
        , globPattern.generic_u8string().c_str()
        , [&](const std::filesystem::path& path) -> bool {
            printf("  %s\n", (const char*) path.generic_u8string().c_str());
            return true;
        }
    );
}
```



## .gitattributes like glob pattern matching rules (C++17)

```c++
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
```
