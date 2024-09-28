#ifndef DEBUG_H
#define DEBUG_H

#include "config.h"

#if defined(CONFIG_DEBUG_MODE) && CONFIG_DEBUG_MODE != 0
  #define DEBUG(code) code
#else
  #define DEBUG(code)
#endif

#endif