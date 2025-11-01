#include "../include/Controller.hpp"

int main(int argc, char* argv[]) {
    auto& instance = Controller::instance(argc, argv);
    instance.run();
    return 0;
}