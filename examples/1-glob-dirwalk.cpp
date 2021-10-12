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
