#pragma once

#include <string>

namespace originalengine {

struct ApplicationConfig {
    std::string title{"OriginalEngine"};
    int width{1280};
    int height{720};
    bool resizable{true};
};

class Application {
public:
    explicit Application(ApplicationConfig config = {});
    int run();

private:
    ApplicationConfig config_;
};

} // namespace originalengine
