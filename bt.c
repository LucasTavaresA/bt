#ifndef BT_FREESTANDING
#	include <errno.h>
#	include <stdio.h>
#	include <stdlib.h>
#	include <string.h>
#	include <stdbool.h>
#	include <sys/stat.h>
#	include <unistd.h>
#	ifndef _WIN32
#		include <sys/wait.h>
#	endif
#endif

#ifndef BT_DEF
#	define BT_DEF static inline
#endif

/// just a immutable string
typedef const char* const bt_string;

/// given an output and the inputs, checks if output needs to recompile.
/// halts if the stat() of any files fails, https://pubs.opengroup.org/onlinepubs/009696799/functions/stat.html.
BT_DEF bool bt_needs_recompile(bt_string output,
															 const size_t inputs_count,
															 const char* const inputs[]);

/// prints them runs a command with system().
/// halts on non-zero exit status.
BT_DEF void bt_run(bt_string cmd);

#ifdef BT_IMPLEMENTATION

#	ifndef BT_BUFSIZE
#		define BT_BUFSIZE ((size_t)1024)
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

#	ifndef BT_STDOUT
#		define BT_STDOUT stdout
#	endif

#	ifndef BT_STDERR
#		define BT_STDERR stderr
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

#	ifndef BT_SYSTEM_EXIT_CODE
#		if defined(_WIN32)
#			define BT_SYSTEM_EXIT_CODE(status) (status)
#		else
#			define BT_SYSTEM_EXIT_CODE(status) \
				(WIFEXITED(status) ? WEXITSTATUS(status) : BT_EXIT_OSERR)
#		endif
#	endif

// clang-format off
#define BT_EXIT_OK           0
#define BT_EXIT_USAGE        64   /* command line usage error */
#define BT_EXIT_NOINPUT      66   /* cannot open input */
#define BT_EXIT_SOFTWARE     70   /* internal software error */
#define BT_EXIT_OSERR        71   /* system error (e.g., can't fork) */
#define BT_EXIT_IOERR        74   /* input/output error */
#define BT_EXIT_CONFIG       78   /* configuration error */
// clang-format on

#	ifndef BT_FPRINTF
#		define BT_FPRINTF fprintf
#	endif

#	ifndef BT_EXIT
#		define BT_EXIT exit
#	endif

#	define BT_MSG(format, ...) BT_FPRINTF(BT_STDOUT, format, ##__VA_ARGS__)

#	define BT_HALT(status, format, ...)                   \
		do                                                   \
		{                                                    \
			BT_FPRINTF(BT_STDERR, format "\n", ##__VA_ARGS__); \
			BT_EXIT(status);                                   \
		} while (0)

#	define BT_ASSERT(status, cond, format, ...)  \
		do                                          \
		{                                           \
			if (!(cond))                              \
			{                                         \
				BT_HALT(status, format, ##__VA_ARGS__); \
			}                                         \
		} while (0)

#	define BT_ARGS_ARRAY(type, ...) \
		(type const[])                 \
		{                              \
			__VA_ARGS__                  \
		}

#	define BT_ARGS_ARRAY_COUNTOF(type, ...) \
		BT_COUNTOF(((type const[]){__VA_ARGS__}))

#	define BT_COUNTOF(x) (sizeof(x) / sizeof(*(x)))

#	define BT_PARAM_STR_ARRAY(...)                    \
		BT_ARGS_ARRAY_COUNTOF(const char*, __VA_ARGS__), \
				BT_ARGS_ARRAY(const char*, __VA_ARGS__)

/// creates a stack buffer, formats using snprintf() then runs with system().
/// halts if BT_BUFSIZE is too small.
/// halts on non-zero exit status.
#	define BT_ERUNF(fmt, ...)                                                \
		do                                                                      \
		{                                                                       \
			char cmd[BT_BUFSIZE];                                                 \
			int needed = snprintf(cmd, BT_BUFSIZE, fmt, ##__VA_ARGS__);           \
			BT_ASSERT(                                                            \
					BT_EXIT_CONFIG, (needed >= 0 && needed < BT_BUFSIZE),             \
					"%s: '%s...' formatted string is too long, increase BT_BUFSIZE!", \
					__FUNCTION__, cmd);                                               \
			bt_run(cmd);                                                          \
		} while (0)

/// creates a stack buffer, formats using snprintf() then runs with system().
/// halts if BT_BUFSIZE is too small.
/// does not halt if system() errors
#	define BT_RUNF(fmt, ...)                                                 \
		do                                                                      \
		{                                                                       \
			char cmd[BT_BUFSIZE];                                                 \
			int needed = snprintf(cmd, BT_BUFSIZE, fmt, ##__VA_ARGS__);           \
			BT_ASSERT(                                                            \
					BT_EXIT_CONFIG, (needed >= 0 && needed < BT_BUFSIZE),             \
					"%s: '%s...' formatted string is too long, increase BT_BUFSIZE!", \
					__FUNCTION__, cmd);                                               \
			BT_MSG(" %s\n", cmd);                                                 \
			BT_SYSTEM(cmd);                                                       \
		} while (0)

#	define BT_REBUILD_URSELF(argv, ...)                                       \
		if (bt_needs_recompile(                                                  \
						argv[0], BT_PARAM_STR_ARRAY(_bt_file, __FILE__, ##__VA_ARGS__))) \
		{                                                                        \
			BT_ERUNF(BT_CC " -ggdb -o %s %s", argv[0], __FILE__);                  \
			execv(argv[0], (char**)argv);                                          \
			perror("failed to run the rebuilt executable");                        \
			exit(BT_EXIT_OSERR);                                                   \
		}

bt_string _bt_file = __FILE__;

BT_DEF bool bt_needs_recompile(bt_string output,
															 const size_t inputs_count,
															 const char* const inputs[])
{
	struct BT_STAT st = {0};

	if (BT_STAT(output, &st) < 0)
	{
		if (BT_ERRNO == BT_ENOENT)
		{
			// theres no output
			return true;
		}

		BT_HALT(BT_EXIT_IOERR, "could not stat(%s): %s!", output,
						BT_STRERROR(BT_ERRNO));
	}

	int output_time = st.BT_ST_MTIME;

	for (size_t i = 0; i < inputs_count; ++i)
	{
		if (BT_STAT(inputs[i], &st) < 0)
		{
			BT_HALT(BT_EXIT_NOINPUT, "could not stat(%s): %s!", inputs[i],
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

BT_DEF void bt_run(bt_string cmd)
{
	BT_MSG(" %s\n", cmd);
	int status = BT_SYSTEM(cmd);

	// https://pubs.opengroup.org/onlinepubs/9799919799/
	if (status == -1)
	{
		BT_HALT(BT_EXIT_OSERR, "failed to run command '%s': %s", cmd,
						BT_STRERROR(BT_ERRNO));
	}
	else if (status != 0)
	{
		BT_HALT(BT_SYSTEM_EXIT_CODE(status), "command '%s' failed", cmd);
	}
}
#endif	// BT_IMPLEMENTATION
// Licensed under the LGPL3 or later versions of the LGPL license.
// See the LICENSE file in the project root for more information.
