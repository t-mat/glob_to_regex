// Process pseudo gitattributes (myConfig).  For C++17.  Very slow.
#include <map>
#include <sstream>
#include <stdio.h>
#include "glob_to_regex.hpp"

using Config = std::vector<std::vector<std::string>>;
namespace Fs = std::filesystem;
using Path = Fs::path;

static const std::string myConfig = R"(
# Default
# * text auto

*.md text auto
Makefile text auto

# Source code
*.c text eol=lf
*.cpp text eol=lf
*.h text eol=lf
*.hpp text eol=lf

tests/*.cpp eol=lf

# Object
*.o binary
*.obj binary

# Binary
*.png binary
*.jpg binary

# Windows
*.bat text eol=crlf
*.cmd text eol=crlf
)";


static std::vector<std::string> splitLine(const char* line) {
    std::vector<std::string> elements;

    const auto isBlank = [](int x) -> bool { return x == '\t' || x == ' '; };
    const auto isEol = [](int x) -> bool { return x == 0 || x == '#' || x == '\r' || x == '\n'; };
    const auto isEscape = [](int x) -> bool { return x == '\\'; };

    enum class State { SkipBlank, InString };
    State s = State::SkipBlank;
    std::string t;
    bool escape = false;
    for(int p = 0; line[p]; ++p) {
        const char c = line[p];
        if(escape) {
            t.push_back(c);
            escape = false;
            continue;
        }
        if(isEscape(c)) {
            escape = true;
            continue;
        }
        if(isEol(c)) {
            break;
        }
        if(s == State::SkipBlank) {
            if(isBlank(c)) {
                continue;
            }
            t.push_back(c);
            s = State::InString;
        } else {
            if(! isBlank(c)) {
                t.push_back(c);
                continue;
            }
            elements.push_back(t);
            t.clear();
            s = State::SkipBlank;
        }
    }
    if(s == State::InString && !t.empty()) {
        elements.push_back(t);
    }
    return elements;
}


static Config readConfigStream(std::istream& istr) {
    Config config;
    std::string line;
    while(std::getline(istr, line)) {
        std::vector<std::string> elements = splitLine(line.c_str());
        if(! elements.empty()) {
            config.push_back(elements);
        }
    }
    return config;
}


static void procConfig(
      const Config& config
    , const Path& basePath_
    , bool caseSensitivity
    , bool followSimlink
    , const std::function<void(const Path&, int)>& setPathPatternIndex
) {
    const Path basePath = Fs::weakly_canonical(basePath_);

    std::regex::flag_type regexFlags = std::regex::ECMAScript;
    if(! caseSensitivity) {
        regexFlags |= std::regex::icase;
    }

    std::filesystem::directory_options directoryOptions = std::filesystem::directory_options::skip_permission_denied;
    if(followSimlink) {
        directoryOptions |= std::filesystem::directory_options::follow_directory_symlink;
    }

    struct Rule {
        int         index;
        std::string globPattern;
    };

    std::vector<Rule> rules;
    for(int i = 0; i < (int) config.size(); ++i) {
        std::string pattern = config[i][0];
        if(pattern.find('/') != std::string::npos) {
            if(pattern[0] == '/') {
                // "/*.txt" -> "*.txt"
                pattern = &pattern[1];
            }
        } else {
            // "*.txt" -> "**/*.txt"
            pattern = std::string("**/") + pattern;
        }

        Rule rule;
        rule.index          = i;
        rule.globPattern    = pattern;
        rules.push_back(rule);
    }

    const auto isMatch = [&](const Rule& rule, const Path& path) -> bool {
        const Path relPath = Fs::relative(path, basePath);
        const std::string relPathStr = relPath.generic_u8string().c_str();
        const std::string regexStr = GlobToRegex::translateGlobPatternToRegex(rule.globPattern);
        const std::regex r = std::regex(regexStr, regexFlags);
        const bool b = std::regex_match(relPathStr, r);
        return b;
    };

    for(const Fs::directory_entry& e : Fs::recursive_directory_iterator(basePath, directoryOptions)) {
        if(! e.is_regular_file()) {
            continue;
        }
        int lastMatchedRuleIndex = -1;
        for(int i = 0; i < (int) rules.size(); ++i) {
            if(isMatch(rules[i], e.path())) {
                lastMatchedRuleIndex = i;
            }
        }
        setPathPatternIndex(e, lastMatchedRuleIndex);
    }
}


int main() {
    std::stringstream ss(myConfig);
    const Config config = readConfigStream(ss);

    {   // Show all rules
        printf("Rules:\n");
        for(const auto& elements : config) {
            printf("rule=%-16s", elements[0].c_str());
            printf(" => { ");
            for(int i = 1; i < (int) elements.size(); ++i) {
                if(i != 1) { printf(", "); }
                printf("%s", elements[i].c_str());
            }
            printf(" }\n");
        }
        printf("\n");
    }

#if _WIN32
    const bool caseSensitivity = false;
#else
    const bool caseSensitivity = true;
#endif
    const bool followSimlink = true;

    const auto basePath = Path(".");

    const auto setPathPatternIndex = [&](const Path& path, int index) {
        const Path relPath = Fs::relative(path, Fs::weakly_canonical(basePath));
        printf("%-48s", (const char*) relPath.generic_u8string().c_str());

        if(index < 0) {
            printf(", no rules\n");
        } else {
            const std::vector<std::string>& elements = config[index];
            printf(", rule=%-16s", elements[0].c_str());
            printf(" => { ");
            for(int i = 1; i < (int) elements.size(); ++i) {
                if(i != 1) { printf(", "); }
                printf("%s", elements[i].c_str());
            }
            printf(" }\n");
        }
    };

    printf("Matching paths:\n");

    procConfig(config, basePath, caseSensitivity, followSimlink, setPathPatternIndex);
}
