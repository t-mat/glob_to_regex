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


static std::vector<std::string> splitLine(const std::string& line) {
    std::vector<std::string> elements;

    const auto isBlank = [](int x) -> bool { return x == '\t' || x == ' '; };
    const auto isEol = [](int x) -> bool { return x == 0 || x == '#' || x == '\r' || x == '\n'; };
    const auto isEscape = [](int x) -> bool { return x == '\\'; };

    enum class State { SkipBlank, InString };
    State s = State::SkipBlank;
    std::string t;
    bool escape = false;
    for(int p = 0; line[p]; ++p) {
        if(escape) {
            t.push_back(line[p]);
            escape = false;
            continue;
        }
        if(isEscape(line[p])) {
            escape = true;
            continue;
        }
        if(isEol(line[p])) {
            break;
        }
        if(s == State::SkipBlank) {
            if(isBlank(line[p])) {
                continue;
            }
            t.push_back(line[p]);
            s = State::InString;
        } else {
            if(! isBlank(line[p])) {
                t.push_back(line[p]);
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
        std::vector<std::string> elements = splitLine(line);
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

    struct Rule {
        int         index;
        std::string pattern;
    };

    std::vector<Rule> rules;
    for(int i = 0; i < (int) config.size(); ++i) {
        Rule rule;
        rule.index      = i;
        rule.pattern    = config[i][0];
        rules.push_back(rule);
    }

    std::map<const Rule*, std::vector<Path>> rulePathMap;

    for(const Rule& rule : rules) {
        Path rp;
        if(rule.pattern.find('/') != std::string::npos) {
            if(rule.pattern[0] == '/') {
                // "/*.txt" -> "*.txt"
                rp = basePath / &rule.pattern[1];
            } else {
                rp = basePath / rule.pattern;
            }
        } else {
            // "*.txt" -> "**/*.txt"
            rp = basePath / "**" / rule.pattern;
        }
        std::vector<Path> paths;
        GlobToRegex::dirWalk(
              caseSensitivity
            , followSimlink
            , rp
            , [&](const Path& path) -> bool {
                paths.push_back(path);
                return true;
            }
        );
        for(const Path& path : paths) {
            const Path cp = Fs::weakly_canonical(path);
            rulePathMap[&rule].push_back(cp);
        }
    }

    for(const Fs::directory_entry& e : Fs::recursive_directory_iterator(basePath)) {
        const Path path = Fs::weakly_canonical(e.path());
        if(! Fs::is_regular_file(path)) {
            continue;
        }
        const Rule* lastMatchedRule = nullptr;
        for(const Rule& rule : rules) {
            const Rule* pRule = &rule;
            bool match = false;
            for(const Path& rp : rulePathMap[pRule]) {
                if(rp.compare(path) == 0) {
                    match = true;
                    break;
                }
            }
            if(match) {
                lastMatchedRule = pRule;
            }
        }

        const Path relPath = Fs::relative(path, basePath);
        if(lastMatchedRule) {
            setPathPatternIndex(relPath, lastMatchedRule->index);
        } else {
            setPathPatternIndex(relPath, -1);
        }
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
        printf("%-48s", (const char*) path.generic_u8string().c_str());

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
