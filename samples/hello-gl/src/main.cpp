#include "HelloGlApp.hpp"

#include "pixie/app/Run.hpp"
#include "pixie/app/RunOptions.hpp"

int main(int argc, char** argv) {
    pixie::RunConfig config;
    config.title = "hello GL";
    if (!pixie::ParseRunOptions(argc, argv, "hello-gl", config)) return 2;
    HelloGlApp app;
    return pixie::Run(app, config);
}
