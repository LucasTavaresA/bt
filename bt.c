#ifndef BT_FREESTANDING
#	include <errno.h>
#	include <stdio.h>
#	include <stdlib.h>
#	include <string.h>
#	include <stdbool.h>
#	include <sys/stat.h>

#	include "span/span.c"
#	include "log.c"
#endif

/// just a immutable string
typedef const char* const bt_string;

/// given an output and the inputs, checks if output needs to recompile.
/// halts if the stat() of any files fails, https://pubs.opengroup.org/onlinepubs/009696799/functions/stat.html.
static bool bt_needs_recompile(bt_string output, string_span inputs);

/// prints them runs a command with system().
/// halts on non-zero exit status.
void bt_run(bt_string cmd);

#ifdef BT_IMPLEMENTATION

#	ifndef BT_TEMP_BUFFER_SIZE
#		define BT_TEMP_BUFFER_SIZE ((size_t)1024)
#	endif

#	ifndef BT_CC
#		if defined(_WIN32)
#			if defined(__GNUC__)
#				define BT_CC "cc"
#			elif defined(__clang__)
#				define BT_CC "clang"
#			elif defined(_MSC_VER)
#				define BT_CC "cl.exe"
#			endif
#		else
#			define BT_CC "cc"
#		endif
#	endif

#	ifndef BT_CFLAGS
#		if defined(_MSC_VER) && !defined(__clang__)
#			define BT_CFLAGS "/std:c11"
#		else
#			define BT_CFLAGS "-std=c11"
#		endif
#	endif

#	ifndef BT_CWARNINGS
#		if defined(_MSC_VER) && !defined(__clang__)
#			define BT_CWARNINGS "/W4 /nologo /D_CRT_SECURE_NO_WARNINGS"
#		else
#			define BT_CWARNINGS "-Wall -Wextra -pedantic"
#		endif
#	endif

#	ifndef BT_STAT
#		define BT_STAT stat
#	endif

#	ifndef BT_ERRNO
#		define BT_ERRNO errno
#	endif

#	ifndef BT_ENOENT
#		define BT_ENOENT ENOENT
#	endif

#	ifndef BT_STRERROR
#		define BT_STRERROR strerror
#	endif

#	ifndef BT_ST_MTIME
#		define BT_ST_MTIME st_mtime
#	endif

#	ifndef BT_SYSTEM
#		define BT_SYSTEM system
#	endif

static bool bt_needs_recompile(bt_string output, string_span inputs)
{
	struct BT_STAT st = {0};

	if (BT_STAT(output, &st) < 0)
	{
		if (BT_ERRNO == BT_ENOENT)
		{
			// theres no output
			return true;
		}

		LOG_HALT(LOG_ERROR_CODE, "could not stat %s: %s", output,
						 BT_STRERROR(BT_ERRNO));
	}

	int output_time = st.BT_ST_MTIME;

	for (size_t i = 0; i < inputs.size; ++i)
	{
		if (BT_STAT(inputs.data[i], &st) < 0)
		{
			LOG_HALT(LOG_ERROR_CODE, "could not stat %s: %s", inputs.data[i],
							 BT_STRERROR(BT_ERRNO));
		}

		int input_time = st.BT_ST_MTIME;

		if (input_time > output_time)
		{
			// a input is newer than the output
			return true;
		}
	}

	return false;
}

void bt_run(bt_string cmd)
{
	LOG_MSG(" %s\n", cmd);

	if (BT_SYSTEM(cmd) != 0)
	{
		LOG_HALT(LOG_ERROR_CODE, "failed to run command '%s': %s", cmd,
						 BT_STRERROR(BT_ERRNO));
	}
}

/// creates a stack buffer, formats using snprintf() then runs with system().
/// halts if BT_TEMP_BUFFER_SIZE is too small.
/// halts on non-zero exit status.
#	define BT_RUNF(fmt, ...)                                                                                \
		{                                                                                                      \
			char cmd[BT_TEMP_BUFFER_SIZE];                                                                       \
			int needed = snprintf(cmd, BT_TEMP_BUFFER_SIZE, fmt, ##__VA_ARGS__);                                 \
			LOG_ASSERT(                                                                                          \
					(needed >= 0 && needed < BT_TEMP_BUFFER_SIZE),                                                   \
					"%s:%d: formatted string is too long, needed %d bytes, have %zu, increase BT_TEMP_BUFFER_SIZE!", \
					__FILE__, __LINE__, needed, BT_TEMP_BUFFER_SIZE);                                                \
			bt_run(cmd);                                                                                         \
		}                                                                                                      \
		while (0)

/// creates a stack buffer, formats using snprintf() then runs with system().
/// halts if bufsize is too small.
/// halts on non-zero exit status.
#	define BT_RUNF_N(bufsize, fmt, ...)                                                                   \
		{                                                                                                    \
			char cmd[bufsize];                                                                                 \
			int needed = snprintf(cmd, bufsize, fmt, ##__VA_ARGS__);                                           \
			LOG_ASSERT(                                                                                        \
					(needed >= 0 && needed < bufsize),                                                             \
					"%s:%d: formatted string is too long, needed %d bytes, have %zu, increase BT_RUNF_N bufsize!", \
					__FILE__, __LINE__, needed, bufsize);                                                          \
			bt_run(cmd);                                                                                       \
		}                                                                                                    \
		while (0)

#	define BT_REBUILD_URSELF(argv, ...)                                     \
		if (bt_needs_recompile(argv[0], STRING_SPAN(__FILE__, ##__VA_ARGS__))) \
		{                                                                      \
			BT_RUNF_N(BT_TEMP_BUFFER_SIZE, "cc -o %s %s", argv[0], __FILE__);    \
			execv(argv[0], (char**)argv);                                        \
			perror("failed to run the rebuilt executable");                      \
			exit(1);                                                             \
		}

#endif	// BT_IMPLEMENTATION
// Licensed under the LGPL3 or later versions of the LGPL license.
// See the LICENSE file in the project root for more information.
