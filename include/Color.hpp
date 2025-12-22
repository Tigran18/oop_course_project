#pragma once
#include <iostream>
#include <cstdlib>

#if defined(_WIN32)
    #include <io.h>
    #define ISATTY _isatty
    #define FILENO _fileno
#else
    #include <unistd.h>
    #define ISATTY isatty
    #define FILENO fileno
#endif

inline bool colorEnabled()
{
    if (std::getenv("SLIDESHOW_NO_COLOR")) return false;
    return ISATTY(FILENO(stdout)) != 0;
}

struct ColorCode
{
    const char* code;
    operator const char*() const { return colorEnabled() ? code : ""; }
};

inline const ColorCode RESET   { "\033[0m"  };
inline const ColorCode RED     { "\033[31m" };
inline const ColorCode GREEN   { "\033[32m" };
inline const ColorCode YELLOW  { "\033[33m" };
inline const ColorCode BLUE    { "\033[34m" };
inline const ColorCode MAGENTA { "\033[35m" };
inline const ColorCode CYAN    { "\033[36m" };

inline std::ostream& info()
{
    if (colorEnabled()) return std::cout << GREEN << "[INFO] " << RESET;
    return std::cout << "[INFO] ";
}

inline std::ostream& error()
{
    if (colorEnabled()) return std::cout << RED << "[ERR] " << RESET;
    return std::cout << "[ERR] ";
}

inline std::ostream& success()
{
    if (colorEnabled()) return std::cout << CYAN << "[SUCCESS] " << RESET;
    return std::cout << "[SUCCESS] ";
}
