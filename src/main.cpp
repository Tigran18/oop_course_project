#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include "../include/presentation.hpp"
#include "../include/Tokenizer.hpp"

using namespace std;

struct SlideShow {
    vector<string> slides;
    size_t current = 0;
    bool loaded = false;

    void open(const string& path) {
        fstream file(path, ios::in);
        if (!file) {
            cout << "[ERR] Cannot open file: " << path << "\n";
            return;
        }
        slides.clear();
        string line;
        while (getline(file, line)) {
            slides.push_back(line);
        }
        file.close();
        if (slides.empty()) {
            cout << "[WARN] File is empty\n";
            return;
        }
        loaded = true;
        current = 0;
        show();
    }

    void show() {
        if (!loaded) {
            cout << "[ERR] No presentation loaded\n";
            return;
        }
        cout << "[SLIDE] " << slides[current] << "\n";
    }

    void next() {
        if (!loaded) {
            cout << "[ERR] No presentation loaded\n";
            return;
        }
        if (current < slides.size() - 1) {
            current++;
        } 
        else {
            cout << "[WARN] Already at last slide\n";
        }
        show();
    }

    void prev() {
        if (!loaded) {
            cout << "[ERR] No presentation loaded\n";
            return;
        }
        if (current > 0) {
            current--;
        } 
        else {
            cout << "[WARN] Already at first slide\n";
        }
        show();
    }
};

int main(int argc, char* argv[]) {
    SlideShow ss;
    if (argc > 1) {
        ss.open(argv[1]);
    }
    bool exitProgram = false;
    cout << "=== PPTX CLI Slideshow ===\n";
    cout << "Commands: next, prev, show, exit\n";
    while (!exitProgram) {
        cout << "> ";
        string cmd;
        cin >> cmd;
        if (cmd == "next") {
            ss.next();
        } 
        else if (cmd == "prev") {
            ss.prev();
        } 
        else if (cmd == "show") {
            ss.show();
        } 
        else if (cmd == "exit") {
            exitProgram = true;
            cout << "[INFO] Exiting slideshow\n";
        } 
        else {
            cout << "[ERR] Unknown command: " << cmd << "\n";
        }
    }
    return 0;
}
