#pragma once
#include <iostream>

#define RESET   "\033[0m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define BLUE    "\033[34m"
#define MAGENTA "\033[35m"
#define CYAN    "\033[36m"

inline std::ostream& info() { 
    return std::cout << GREEN "[INFO] " RESET; 
}

inline std::ostream& error() { 
    return std::cout << RED "[ERR] " RESET; 
}

inline std::ostream& success() { 
    return std::cout << CYAN "[SUCCESS] " RESET; 
}