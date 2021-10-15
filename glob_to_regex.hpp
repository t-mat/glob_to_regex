//  Glob pattern to regex translator in C++11.
//  Directory traversal with glob pattern in C++17.
//
//  This code supports the following glob notations:
//
//      - ?
//      - *
//      - **
//      - [a-z], [!a-z]
//
//
//  Usage (C++11, string matching test with glob patterns):
//      std::vector<std::string> specimens = { "bat", "cat", "tab", "tac", };
//
//      std::string globPattern = "?at";
//
//      const std::string regexStr = GlobToRegex::translateGlobPatternToRegex(globPattern);
//      if(regexStr.empty()) {
//          printf("error\n");
//          exit(EXIT_FAILURE);
//      }
//
//      const std::regex::flag_type regexFlags = std::regex::ECMAScript;
//  //  const std::regex::flag_type regexFlags = std::regex::ECMAScript | std::regex::icase; // for case insensitive filesystems
//      const std::regex r = std::regex(regexStr, regexFlags);
//
//      for(const auto& s : specimens) {
//          const bool b = std::regex_match(s, r);
//          printf("'%s' %-13s '%s'\n", globPattern.c_str(), b ? "matches" : "doesn't match", s.c_str());
//      }
//
//
//  Usage (C++17, directory traversal with glob pattern matching):
//      const bool caseSensitivity = true;
//      const bool followSimlink = true;
//
//      const auto home = [&]() -> std::string {
//          const char* s = getenv("HOME");
//          return std::string(s ? s : ".");
//      }();
//
//      std::filesystem::path globPattern = home + "/**/*.txt";
//      printf("globPattern=%s\n", (const char*) globPattern.generic_u8string().c_str());
//
//      GlobToRegex::dirWalk(
//            caseSensitivity
//          , followSimlink
//          , globPattern
//          , [&](const std::filesystem::path& path) -> bool {
//              printf("  %s\n", (const char*) path.generic_u8string().c_str());
//              return true;
//          }
//      );
//
//
// License
//      SPDX-FileCopyrightText: Copyright (c) Takayuki Matsuoka
//      SPDX-License-Identifier: CC0-1.0
//      See http://creativecommons.org/publicdomain/zero/1.0/
#ifndef glob_to_regex_hpp_included
#define glob_to_regex_hpp_included

#include <string>
#include <regex>

namespace GlobToRegex {
    enum class GlobToRegexErrc : int {
        Ok              = 0,
        Empty           = -1,
        BadTerm         = -2,
        BadEscape       = -3,
        BadDoubleStar   = -4,
        BadBracket      = -5,
        BadBasePath     = -6,
    };

    inline bool any(GlobToRegexErrc e) {
        return e != GlobToRegexErrc::Ok;
    }

    namespace {
        inline bool isSpecialRegexChar(int c) {
            static const char specialChars[] = "$()*+.?[]^{|}\\";
            for(const auto s : specialChars) {
                if(s == c) {
                    return true;
                }
            }
            return false;
        }

        inline bool isBadEscapeChar(int c) {
            return (c < 0)
                || (c >= 0 && c < 32)           // control char
                || (c == 0x7f)                  // del
                || (c >= 'A' && c <= 'Z')       // normal char
                || (c >= 'a' && c <= 'z')       // normal char
                || (c >= '0' && c <= '9')       // normal char
                || (c >= 0x80 && c <= 0xff);    // normal char (Multi-byte in UTF-8)
        }
    }

    inline std::string translateGlobPatternToRegex(const std::string& globPattern, GlobToRegexErrc& outputErrorCode) {
        std::string result = "";

        const auto error = [&](GlobToRegexErrc errorCode) -> std::string {
            outputErrorCode = errorCode;
            return {};
        };

        if(globPattern.empty()) {
            return error(GlobToRegexErrc::Empty);
        }

        const auto n = static_cast<int>(globPattern.size());
        for(int i = 0; i < n; ) {
            const int eolChar = -256;

            const auto putRawChar = [&](int c) {
                result += static_cast<char>(c);
            };

            const auto putRawStr = [&](const char* s) {
                while(*s) {
                    putRawChar(*s++);
                }
            };

            const auto putCharWithRegexEscape = [&](int c) {
                if(isSpecialRegexChar(c)) {
                    putRawChar('\\');
                    putRawChar(c);
                } else {
                    putRawChar(c);
                }
            };

            const auto advance = [&]() {
                ++i;
            };

            const auto peekNextChar = [&]() -> int {
                if(i < n) {
                    return static_cast<uint8_t>(globPattern[i]);
                }
                return eolChar;
            };

            const auto getNextChar = [&]() -> int {
                const int x = peekNextChar();
                if(x != eolChar) {
                    advance();
                }
                return x;
            };

            const int c0 = getNextChar();
            if(c0 == eolChar) {
                break;
            }

            const int c1 = peekNextChar();

            if(c0 == '\\') {
                if(isBadEscapeChar(c1)) {
                    return error(GlobToRegexErrc::BadEscape);
                }
                advance();
                putCharWithRegexEscape(c1);
                continue;
            }

            if(c0 == '?') {
                // '?' matches any character except '/'.
                putRawStr("[^/]");
                continue;
            }

            if(c0 == '*' && c1 != '*') {
                // '*' matches any length of characters except '/'.
                putRawStr("[^/]*");
                continue;
            }

            if(c0 == '*' && c1 == '*') {
                advance();
                const int c2 = getNextChar();
                if(c2 == eolChar) {
                    // '*', '*', EOL matches everything including '/'.
                    putRawStr(".*");
                    break;
                }
                if(c2 != '/') {             // '*', '*', not('/')
                    // '*', '*', not('/') is invalid glob pattern.
                    return error(GlobToRegexErrc::BadDoubleStar);
                }
                // '*', '*', '/' matches any times (including 0 times) of directory names.
                putRawStr("([^/]+/)*");
                continue;
            }

            if(c0 == '[') {
                // '[', ... , ']'
                putRawChar('[');
                if(c1 == '!') {
                    putRawChar('^');
                    advance();
                }
                for(;;) {
                    const int d = getNextChar();
                    if(d == eolChar) {
                        return error(GlobToRegexErrc::BadBracket);
                    }
                    if(d == ']') {
                        break;
                    }
                    if(d == '-') {
                        putRawChar('-');
                        continue;
                    }
                    if(d == '\\') {
                        const int e = getNextChar();
                        if(isBadEscapeChar(e)) {
                            return error(GlobToRegexErrc::BadEscape);
                        }
                        putCharWithRegexEscape(e);
                        continue;
                    }
                    putCharWithRegexEscape(d);
                }
                putRawChar(']');
                continue;
            }

            putCharWithRegexEscape(c0);
        }

        outputErrorCode = GlobToRegexErrc::Ok;
        return result;
    }

    inline std::string translateGlobPatternToRegex(const std::string& globPattern) {
        GlobToRegexErrc errorCode;
        return translateGlobPatternToRegex(globPattern, errorCode);
    }
}


// dirWalk
#if (__cplusplus >= 201703L) && __has_include(<filesystem>)
#include <filesystem>
#include <functional>

namespace GlobToRegex {
    inline GlobToRegexErrc dirWalk_(
          std::regex::flag_type regexFlags
        , std::filesystem::directory_options directoryOptions
        , const std::filesystem::path& globPatternPath
        , const std::function<bool(const std::filesystem::path&)> callback
    ) {
        std::regex rgx;

        {
            const auto s = globPatternPath.generic_u8string();
            const auto* p = (const char*) s.c_str();
            GlobToRegexErrc error;
            const std::string regexStr = translateGlobPatternToRegex(p, error);
            if(any(error)) {
                return error;
            }
            rgx = std::regex(regexStr, regexFlags);
        }

        const auto containsSpecialRecursiveGlobChar = [](const std::filesystem::path& path) -> bool {
            const auto s = path.generic_u8string();
            const char* p = (const char*) s.c_str();
            while(*p) {
                const auto c = *p++;
                if(c == '*' && *p == '*') {
                    return true;
                }
            }
            return false;
        };

        // Move to parent while path contains any special glob character.
        const auto findBasePath = [&](const std::filesystem::path& path) -> std::filesystem::path {
            const auto containsSpecialGlobChar = [](const std::filesystem::path& path) -> bool {
                const auto s = path.generic_u8string();
                const char* p = (const char*) s.c_str();
                while(*p) {
                    const auto c = *p++;
                    if(c == '*' || c == '?' || c == '[' || c == ']') {
                        return true;
                    }
                }
                return false;
            };

            auto p = std::filesystem::path(path);
            while(! p.empty()) {
                if(! containsSpecialGlobChar(p)) {
                    return p;
                }
                const auto parent = p.parent_path();
                const auto filename = p.filename();
                p = parent;
            }
            return p;
        };

        const auto basePath = findBasePath(globPatternPath);
        const auto globPath = std::filesystem::relative(globPatternPath, basePath);
        if(! std::filesystem::exists(basePath)) {
            return GlobToRegexErrc::BadBasePath;
        }

        const auto& match = [&](const std::filesystem::path& path) -> bool {
            const std::filesystem::path p2 = std::filesystem::weakly_canonical(path);
            return std::regex_match((const char*) p2.generic_u8string().c_str(), rgx);
        };

        if(containsSpecialRecursiveGlobChar(globPath)) {
            // Recursive
            for(const auto& e : std::filesystem::recursive_directory_iterator(basePath, directoryOptions)) {
                const std::filesystem::path path = e.path();
                if(match(path)) {
                    if(! callback(path)) {
                        break;
                    }
                }
            }
        } else {
            // Non recursive
            for(const auto& e : std::filesystem::directory_iterator(basePath, directoryOptions)) {
                const std::filesystem::path path = e.path();
                if(match(e.path())) {
                    if(! callback(path)) {
                        break;
                    }
                }
            }
        }
        return GlobToRegexErrc::Ok;
    }

    inline GlobToRegexErrc dirWalk(
          bool caseSensitive
        , bool followSimlink
        , const std::filesystem::path& globPatternPath
        , const std::function<bool(const std::filesystem::path&)> callback
    ) {
        std::regex::flag_type regexFlags = std::regex::ECMAScript;
        if(! caseSensitive) {
            regexFlags |= std::regex::icase;
        }

        std::filesystem::directory_options directoryOptions = std::filesystem::directory_options::skip_permission_denied;
        if(followSimlink) {
            directoryOptions |= std::filesystem::directory_options::follow_directory_symlink;
        }

        return dirWalk_(regexFlags, directoryOptions, globPatternPath, callback);
    }
}
#endif

#endif // glob_to_regex_hpp_included
