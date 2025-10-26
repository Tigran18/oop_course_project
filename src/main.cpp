#include "../include/Controller.hpp"

std::vector<SlideShow> slideshows;
std::map<std::string, size_t> presentationIndex;
std::vector<std::string> presentationOrder;
size_t currentIndex = 0;

int main(int argc, char* argv[]) {
    Controller controller(argc, argv);
    controller.run();
    return 0;
}