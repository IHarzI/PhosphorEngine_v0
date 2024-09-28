#pragma once

#define PH_ASSERTIONS_ENABLED 1

#ifdef PH_ASSERTIONS_ENABLED

#if _MSC_VER
#include <intrin.h>
#define PH_DEBUG_BREAK() __debugbreak()
#else
#define PH_DEBUG_BREAK() __builtin_trap()
#endif

#include "PH_Logger.h"

#define PH_ASSERT(expr)													\
	if(!(expr)){														\
		PhosphorEngine::_ph_report_assertion_failure(#expr, "", __FILE__, __LINE__);			\
		PH_DEBUG_BREAK();												\
	};													

#define PH_ASSERT_MSG(expr,msg)										\
	if (!(expr)) {													\
																	\
			PhosphorEngine::_ph_report_assertion_failure(#expr, msg, __FILE__, __LINE__);		\
			PH_DEBUG_BREAK();										\
	};

#ifdef _PH_DEBUG
#define PH_DEBUG_ASSERT(expr)										\
	if (!(expr)) {													\
																	\
			PhosphorEngine::_ph_report_assertion_failure(#expr, "", __FILE__, __LINE__);		\
			PH_DEBUG_BREAK();										\
	};
	
#define PH_DEBUG_ASSERT_MSG(expr,msg)								\
	if (!(expr)) {													\
			PhosphorEngine::_ph_report_assertion_failure(#expr, msg, __FILE__, __LINE__);		\
			PH_DEBUG_BREAK();										\
	};

#else
#define PH_DEBUG_ASSERT(expr)
#define PH_DEBUG_ASSERT_MSG(expr,msg)
#endif // _PH_DEBUG

#else

#define PH_ASSERT(expr)		
#define PH_ASSERT_MSG(expr, msg)
#define PH_DEBUG_ASSERT(expr)
#define PH_DEBUG_ASSERT_MSG(expr,msg)

#endif // PH_ASSERTIONS_ENABLED

#define PH_VULKAN_ERROR(AssertMsg) \
{ char Msg[500] = "Vulkan Error | Vulkan Check Failed: "; strcat(Msg, AssertMsg); PH_ERROR(&Msg[0]); }

#define PH_VULKAN_CHECK_MSG(VulkanCallResult, AssertMsg) \
if(VulkanCallResult != VK_SUCCESS)	\
	PH_VULKAN_ERROR(AssertMsg)

#define PH_VULKAN_CHECK(VulkanCallResult)				 \
if(VulkanCallResult != VK_SUCCESS)	PH_VULKAN_ERROR("Failed Check");
