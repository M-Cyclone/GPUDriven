#pragma once

#ifndef NDEBUG
#    define VK_EXCEPT(call)                                                                                                                \
        do                                                                                                                                 \
        {                                                                                                                                  \
            if (VkResult res = (call); res != VK_SUCCESS)                                                                                  \
            {                                                                                                                              \
                throw ::Graphics::VkException(__LINE__, __FILE__, res);                                                                    \
            }                                                                                                                              \
        } while (false)
#else
#    define VK_EXCEPT(call)                                                                                                                \
        do                                                                                                                                 \
        {                                                                                                                                  \
            VkResult res = (call);                                                                                                         \
            (void)res;                                                                                                                     \
        } while (false)
#endif  // NDEBUG
