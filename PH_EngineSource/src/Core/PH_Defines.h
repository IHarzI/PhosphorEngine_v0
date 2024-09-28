#pragma once

// Undef Windows stuff that conflicts
#ifdef _WIN32
#undef min 
#undef max 
#undef ERROR 
#undef DELETE 
#undef MessageBox 
#undef Error
#undef OK
#undef CONNECT_DEFERRED 
#endif

#undef ABS
#undef SIGN
#undef MIN
#undef MAX
#undef CLAMP

#define PH_STATIC_LOCAL static
#define PH_STATIC_GLOBAL static

#define PH_INLINE inline

