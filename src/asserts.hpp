#pragma once

#include <iostream>
#include <sstream>

#ifndef SERVER_BUILD
#include "SDL.h"
#endif // SERVER_BUILD

#if defined(_MSC_VER)
#include <intrin.h>
#define DebuggerBreak()		do{ __debugbreak(); } while(0)
#define __SHORT_FORM_OF_FILE__		\
	(strrchr(__FILE__, '\\')		\
	? strrchr(__FILE__, '\\') + 1	\
	: __FILE__						\
	)
#else
#include <signal.h>
#define DebuggerBreak()		do{ raise(SIGINT); }while(0)
#define __SHORT_FORM_OF_FILE__		\
	(strrchr(__FILE__, '/')			\
	? strrchr(__FILE__, '/') + 1	\
	: __FILE__						\
	)
#endif

#ifdef SERVER_BUILD
#define ASSERT_LOG(_a,_b)															\
	do {																			\
		if(!(_a)) {																	\
			std::cerr << "CRITICAL: " << __SHORT_FORM_OF_FILE__ << ":" << __LINE__	\
				<< " : " << _b << "\n";												\
			DebuggerBreak();														\
			exit(1);																\
		}																			\
	} while(0)

#define LOG_INFO(_a)																\
	do {																			\
		std::ostringstream _s;														\
		std::cerr << "INFO: " << __SHORT_FORM_OF_FILE__ << ":" << __LINE__ << " : " \
			<< _a << "\n";															\
	} while(0)

#define LOG_DEBUG(_a)																\
	do {																			\
		std::ostringstream _s;														\
		std::cerr << "DEBUG: " << __SHORT_FORM_OF_FILE__ << ":" << __LINE__ << " : "\
			<< _a << "\n";															\
	} while(0)

#define LOG_WARN(_a)																\
	do {																			\
		std::ostringstream _s;														\
		std::cerr << "WARN: " << __SHORT_FORM_OF_FILE__ << ":" << __LINE__ << " : " \
			<< _a << "\n";															\
	} while(0)

#define LOG_ERROR(_a)																\
	do {																			\
		std::ostringstream _s;														\
		std::cerr << "ERROR: " << __SHORT_FORM_OF_FILE__ << ":" << __LINE__ << " : "\
			 << _a << "\n";															\
	} while(0)

#else

#define ASSERT_LOG(_a,_b)															\
	do {																			\
		if(!(_a)) {																	\
			std::ostringstream _s;													\
			_s << __SHORT_FORM_OF_FILE__ << ":" << __LINE__ << " : " << _b << "\n";	\
			SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION, _s.str().c_str(), "");	\
			DebuggerBreak();														\
			exit(1);																\
		}																			\
	} while(0)

#define LOG_INFO(_a)																\
	do {																			\
		std::ostringstream _s;														\
		_s << __SHORT_FORM_OF_FILE__ << ":" << __LINE__ << " : " << _a << "\n";		\
		SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, _s.str().c_str(), "");			\
	} while(0)

#define LOG_DEBUG(_a)																\
	do {																			\
		std::ostringstream _s;														\
		_s << __SHORT_FORM_OF_FILE__ << ":" << __LINE__ << " : " << _a << "\n";		\
		SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, _s.str().c_str(), "");			\
	} while(0)

#define LOG_WARN(_a)																\
	do {																			\
		std::ostringstream _s;														\
		_s << __SHORT_FORM_OF_FILE__ << ":" << __LINE__ << " : " << _a << "\n";		\
		SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, _s.str().c_str(), "");			\
	} while(0)

#define LOG_ERROR(_a)																\
	do {																			\
		std::ostringstream _s;														\
		_s << __SHORT_FORM_OF_FILE__ << ":" << __LINE__ << " : " << _a << "\n";		\
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, _s.str().c_str(), "");			\
	} while(0)

#endif

#ifndef DECLARE_CALLABLE
#define DECLARE_CALLABLE(aaa)
#define BEGIN_DEFINE_CALLABLE(aaa, bbb) variant aaa ## xxx () { variant value; bbb* obj_ptr = aaa::factory(nullptr, variant()); bbb& obj = *obj_ptr;
#define END_DEFINE_CALLABLE(aaa)	delete obj_ptr; }
#define DEFINE_FIELD(aaa, bbb)
#define DEFINE_SET_FIELD
#endif
