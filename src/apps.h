#ifndef AOS_APPS_H
#define AOS_APPS_H

#include "launcher.h"

namespace Aos {

// The sentinel is excluded from the count. Add native apps before it.
constexpr App NativeApps[] = {
  {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr}
};
constexpr int NativeAppCount = sizeof(NativeApps) / sizeof(NativeApps[0]) - 1;

}
#endif
