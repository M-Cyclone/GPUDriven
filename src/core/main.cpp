#include <iostream>
#include <stdexcept>

#include "core/app.h"

int main()
{
    try
    {
        App{}.run();
    }
    catch (std::exception& e)
    {
        std::cerr << "[Statard Exception]\n" << e.what() << std::endl;
        std::system("pause");
        return -1;
    }

    return 0;
}
