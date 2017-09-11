#pragma once

#ifdef ANDROID_NDK
#include <android/log.h>

#define log_err(fmt, args...) \
	__android_log_print(ANDROID_LOG_ERROR, "kasui", "%s(%d): " fmt "\n", __FILE__, __LINE__, ##args);

#define log_debug(fmt, args...) \
	__android_log_print(ANDROID_LOG_INFO, "kasui", "%s(%d): " fmt "\n", __FILE__, __LINE__, ##args);

#else

#define log_err(fmt, args...) \
	fprintf(stderr, "[ERROR] %s(%d): " fmt "\n", __FILE__, __LINE__, ##args);

#define log_debug(fmt, args...) \
	fprintf(stderr, "[DEBUG] %s(%d): " fmt "\n", __FILE__, __LINE__, ##args);

#endif
