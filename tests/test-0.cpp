// gcc  : g++     -std=c++11 -I .. test-0.cpp && ./a.out
// clang: clang++ -std=c++11 -I .. test-0.cpp && ./a.out
// MSVC : cl.exe /EHsc /I .. test-0.cpp && .\test-0.exe

#include <stdio.h>
#include "glob_to_regex.hpp"

static int test0() {
    struct TestCase {
        bool        expected;
        std::string pattern;
        std::string str;
    };

    std::vector<TestCase> testCases = {
        { true,     "",         ""          },
        { false,    "",         "a"         },

        { false,    "a",        ""          },
        { true,     "a",        "a"         },
        { false,    "a",        "aa"        },
        { false,    "a",        "b"         },

        { false,    "aa",       ""          },
        { false,    "aa",       "a"         },
        { true,     "aa",       "aa"        },
        { false,    "aa",       "aaa"       },
        { false,    "aa",       "ba"        },

        { false,    "a*",       ""          },
        { true,     "a*",       "a"         },      // "a*" matches "a"
        { true,     "a*",       "aa"        },
        { true,     "a*",       "ab"        },
        { false,    "a*",       "a/"        },
        { false,    "a*",       "ab/"       },

        { false,    "a.*",      ""          },
        { false,    "a.*",      "a"         },      // "a.*" doesn't match "a"
        { true,     "a.*",      "a."        },
        { true,     "a.*",      "a.b"       },
        { false,    "a.*",      "a./"       },

        { false,    "*a",       ""          },
        { true,     "*a",       "a"         },      // "*a" matches "a"
        { true,     "*a",       "aa"        },
        { true,     "*a",       "ba"        },
        { false,    "*a",       "ab"        },
        { false,    "*a",       "a.txt"     },
        { true,     "*a",       "b.txta"    },
        { false,    "*a",       "ba/"       },
        { false,    "*a",       "ba/a"      },

        { true,     "*.txt",    "a.txt"     },
        { true,     "*.txt",    "ab.txt"    },
        { false,    "*.txt",    "atxt"      },
        { false,    "*.txt",    "/a.txt"    },
        { false,    "*.txt",    "a/b.txt"   },

        { true,     "a.*",      "a.txt"     },
        { true,     "a.*",      "a.b"       },
        { false,    "a.*",      "ab.txt"    },
        { false,    "a.*",      "a/a.txt"   },

        { true,     "/a.*",     "/a.txt"    },
        { true,     "/a.*",     "/a.b"      },
        { false,    "/a.*",     "/a"        },
        { false,    "/a.*",     "/ab.txt"   },

        { true,     "./a.*",    "./a.txt"   },
        { true,     "./a.*",    "./a.b"     },
        { false,    "./a.*",    "./a"       },
        { false,    "./a.*",    "./ab.txt"  },

        { true,     "/x/a.*",   "/x/a.txt"  },
        { true,     "/x/a.*",   "/x/a.b"    },
        { false,    "/x/a.*",   "/x/a"      },
        { false,    "/x/a.*",   "/x/ab.txt" },
        { false,    "/x/a.*",   "/y/a.txt"  },
        { false,    "/x/a.*",   "/y/a.b"    },

        { true,     "**/a.txt", "a.txt"         },
        { true,     "**/a.txt", "x/a.txt"       },
        { true,     "**/a.txt", "x/y/z/a/a.txt" },

        { true,     "[!c]at",   "bat"           },
        { false,    "[!c]at",   "cat"           },

        { true,     "a[!3-5]",  "a1"            },
        { true,     "a[!3-5]",  "a2"            },
        { false,    "a[!3-5]",  "a3"            },
        { false,    "a[!3-5]",  "a4"            },
        { false,    "a[!3-5]",  "a5"            },
        { true,     "a[!3-5]",  "a6"            },
        { true,     "a[!3-5]",  "ax"            },
    };

    int errorCount = 0;
    for(const auto& testCase : testCases) {
        static const std::regex::flag_type regexFlags = std::regex::ECMAScript | std::regex::icase;

        const std::string& pat      = testCase.pattern;
        const std::string& str      = testCase.str;
        GlobToRegex::GlobToRegexErrc rxsError;
        const auto rxsStr = GlobToRegex::translateGlobPatternToRegex(pat, rxsError);
        const std::regex  rgx       = std::regex(rxsStr, regexFlags);
        const bool        expected  = testCase.expected;
        const bool        actual    = std::regex_match(str, rgx);
        const bool        ok        = expected == actual;
        if(!ok) { errorCount += 1; }

        printf("%-2s, ", ok ? "OK" : "NG");
        printf("%-5s",  actual ? "true" : "false");
        printf(" <- ");
        printf("ismatch(");
        printf("%-16s", pat.c_str());
        printf("%-16s", str.c_str());
        printf("), regex=");
        printf("%-32s", rxsStr.c_str());
        printf("\n");
    }
    const bool ok = (errorCount == 0);
    printf("%-2s, ", ok ? "OK" : "NG");
    printf("errorCount=%5d", errorCount);
    printf("\n");

    return errorCount;
}


int main(int argc, const char** argv) {
	const auto result = test0();
	exit((result == 0) ? EXIT_SUCCESS : EXIT_FAILURE);
}
