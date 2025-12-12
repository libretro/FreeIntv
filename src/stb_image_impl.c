/* STB Image Implementation
 * This file provides the implementation for stb_image functions.
 * Only one C file should define STB_IMAGE_IMPLEMENTATION.
 */

/* Disable thread-local storage for iOS armv7 compatibility */
#ifdef __APPLE__
#define STBI_NO_THREAD_LOCALS
#endif

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
