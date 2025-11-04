#ifndef LOG_C
#define LOG_C

#ifndef LOG_STDOUT
#	define LOG_STDOUT stdout
#endif

#ifndef LOG_STDERR
#	define LOG_STDERR stderr
#endif

#ifndef LOG_ERROR_CODE
#	define LOG_ERROR_CODE 1
#endif

#ifndef LOG_FPRINTF
#	define LOG_FPRINTF fprintf
#endif

#ifndef LOG_EXIT
#	define LOG_EXIT exit
#endif

#ifndef LOG_MSG
#	define LOG_MSG(format, ...) LOG_FPRINTF(LOG_STDOUT, format, ##__VA_ARGS__);
#endif

#ifndef LOG_HALT
#	define LOG_HALT(status, format, ...)                    \
		do                                                     \
		{                                                      \
			LOG_FPRINTF(LOG_STDERR, format "\n", ##__VA_ARGS__); \
			LOG_EXIT(status);                                    \
		} while (0)
#endif

#ifndef LOG_ASSERT
#	define LOG_ASSERT(cond, format, ...)                  \
		do                                                   \
		{                                                    \
			if (!(cond))                                       \
			{                                                  \
				LOG_HALT(LOG_ERROR_CODE, format, ##__VA_ARGS__); \
			}                                                  \
		} while (0)
#endif

#endif	// LOG_C
