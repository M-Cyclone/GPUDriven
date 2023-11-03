#include <cstdlib>
#include <iostream>
#include <stdexcept>

#include "core/app.h"

#include "utils/log.h"

int main()
{
    try
    {
        Log::init();
        App{}.run();
    }
    catch (std::exception& e)
    {
        LogError("[Statard Exception] {}.", e.what());
        std::system("pause");
        return -1;
    }

    return 0;
}
