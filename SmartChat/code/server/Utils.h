#ifndef UTILS_H
#define UTILS_H

#include <vector>
#include <string>

std::vector<std::string> Split(std::string s, char c) {
    std::vector<std::string> ret;
    int last = 0;
    for (int i = 0; i < s.length(); i++) {
        if (s[i] == c) {
            if (i != last) ret.push_back(s.substr(last, i - last));
            last = i + 1;
        } 
    }
    if (last != s.length()) ret.push_back(s.substr(last, s.length() - last));
    return ret;
}

#endif