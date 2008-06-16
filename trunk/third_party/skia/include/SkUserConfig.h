#ifndef SkUserConfig_DEFINED
#define SkUserConfig_DEFINED

/*	This file is included before all other headers, except for SkPreConfig.h.
	That file uses various heuristics to make a "best guess" at settings for
	the following build defines.

	However, in this file you can override any of those decisions by either
	defining new symbols, or #undef symbols that were already set.
*/

#include <new>

namespace common_base {
void* InformativeNew(
    unsigned long size, const char* type, const char* file, unsigned int line);
}  // namespace common_base

using common_base::InformativeNew;

// We do not want this defined but the code does not compile without
// it.
#define SK_SUPPORT_MIPMAP

#undef SK_DEBUG
#define SK_RELEASE
#define SK_DEBUGBREAK(expression) \
  ((expression) ? ((void)0) : \
  EnterDebugger(__FILE__, __LINE__, "Assert: " #expression))

#define SkNEW(type) \
  new type
//  new (InformativeNew(sizeof(type), #type, __FILE__, __LINE__)) type
#define SkNEW_ARGS(type, args) \
  new type args
//  new (InformativeNew(sizeof(type), #type, __FILE__, __LINE__)) type args
#define SkDELETE(obj) delete obj

#undef SK_SCALAR_IS_FLOAT
#define SK_SUPPORT_IMAGE_ENCODE
#endif
