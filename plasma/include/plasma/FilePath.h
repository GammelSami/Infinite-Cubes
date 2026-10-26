#pragma once
#include <filesystem>
#define PLASMA_FILE_PATH(filename)                                                                                     \
    std::filesystem::path                                                                                              \
    {                                                                                                                  \
        filename                                                                                                       \
    }
