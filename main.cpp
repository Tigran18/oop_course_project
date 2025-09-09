#include <iostream>
#include <string>
#include "presentation.hpp"

using namespace std;

int main() {
    Presentation* current=nullptr;

    while(true){
        string cmd;
        cin>>cmd;
        if (cmd=="exit") {
            break;
        }
        cout<<"Unknown command: "<<cmd<<endl;
    }

    delete current;
    return 0;
}