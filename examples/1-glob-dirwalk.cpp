// Directory traversal with glob pattern matching (C++17)
//
// gcc  : g++     -std=c++17 -I .. 1-glob-dirwalk.cpp && ./a.out
// clang: clang++ -std=c++17 -I .. 1-glob-dirwalk.cpp && ./a.out
// MSVC : cl.exe /std:c++17 /Zc:__cplusplus /EHsc /I .. 1-glob-dirwalk.cpp && .\1-glob-dirwalk.exe

#include <stdio.h>
#include "glob_to_regex.hpp"

#if _WIN32
#include <windows.h>  // SetConsoleOutputCP(), GetConsoleOutputCP()
struct Win32CodePageUtf8 {
    Win32CodePageUtf8() : ocp(GetConsoleOutputCP()) {
        SetConsoleOutputCP(65001);
    }

    ~Win32CodePageUtf8() {
        SetConsoleOutputCP(ocp);
    }

    UINT ocp;
} win32CodePageUtf8;
#endif


int main(int argc, const char** argv) {
#if _WIN32
    const bool caseSensitivity = false;
    char* str = nullptr;
    size_t len = 0;
    _dupenv_s(&str, &len, "USERPROFILE");
    const auto home = std::string(str);
    free(str);
#else
    const bool caseSensitivity = true;
    const auto home = std::string(getenv("HOME"));
#endif
    const bool followSimlink = true;

    // Find all .txt file under $HOME (%USERPROFILE% on Windows) recursively.
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
