/*
Copyright (C) 2007, 2010 - Bit-Blot

This file is part of Aquaria.

Aquaria is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#include "DSQ.h"
#include <SDL.h>
#include <SDL_main.h>
#include "Randomness.h"

#define B_STACKTRACE_IMPL
#include "b_stacktrace.h"



static void onCrash()
{
	char *bt = b_stacktrace_get_string();
	fputs(bt, stderr);

	time_t rawtime;
	time(&rawtime);
	struct tm *timeinfo = localtime(&rawtime);

	char buf[128];
	strftime(buf, sizeof(buf), "aquaria-crash-%Y-%m-%d_%H-%M-%S.txt", timeinfo);
	FILE *f = fopen(buf, "w");
	if(f)
	{
		fwrite(bt, 1, strlen(bt), f);
		fflush(f);
		fclose(f);
		openURL(buf);
	}
	else
		messageBox("Aquaria crash!", bt);
	free(bt);
}

#ifdef _WIN32
#include <DbgHelp.h>
#pragma comment(lib, "DbgHelp.lib")
static LONG WINAPI UnhandledExceptionHandler(EXCEPTION_POINTERS* ep)
{
	DWORD code = ep->ExceptionRecord->ExceptionCode;
	if (code == DBG_PRINTEXCEPTION_C || code == DBG_CONTROL_C)
		return EXCEPTION_CONTINUE_SEARCH;

	HANDLE hFile = CreateFileA("aquaria-minidump.dmp", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile != INVALID_HANDLE_VALUE)
	{
		MINIDUMP_EXCEPTION_INFORMATION dumpInfo = { GetCurrentThreadId(), ep, FALSE };
		MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hFile, MiniDumpWithDataSegs, &dumpInfo, NULL, NULL);
		CloseHandle(hFile);
	}

	onCrash();
	return EXCEPTION_CONTINUE_SEARCH;
}
#else
#include <signal.h>
static void signalHandler(int signal, siginfo_t* info, void* context)
{
	onCrash();
	_exit(-1);
}
#endif


void setupCrashHandler()
{
#ifdef _WIN32
	SetUnhandledExceptionFilter(UnhandledExceptionHandler);
#else
	struct sigaction sa;
	sa.sa_sigaction = my_handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART | SA_SIGINFO;
	sigaction(SIGSEGV, &sa, NULL);
	sigaction(SIGABRT, &sa, NULL);
	sigaction(SIGFPE, &sa, NULL);
	sigaction(SIGILL, &sa, NULL);
	sigaction(SIGBUS, &sa, NULL);
#endif
}


extern "C" int main(int argc,char *argv[])
{
	setupCrashHandler();

	std::string dsqParam = ""; // fileSystem
	std::string extraDataDir = "";

	const char *envPath = 0;
#ifdef BBGE_BUILD_UNIX
	envPath = getenv("AQUARIA_DATA_PATH");
	if (envPath)
	{
		dsqParam = envPath;
	}
#endif
#ifdef AQUARIA_DEFAULT_DATA_DIR
	if(!envPath)
		dsqParam = AQUARIA_DEFAULT_DATA_DIR;
#endif
#ifdef AQUARIA_EXTRA_DATA_DIR
	extraDataDir = AQUARIA_EXTRA_DATA_DIR;
#endif

	// Couple pointers to help enhance entropy, suppported by the system's ASLR if available
	{
		void *p = malloc(1);
		Randomness::init((uintptr_t)argv, (uintptr_t)&dsqParam, (uintptr_t)&(malloc), (uintptr_t)p);
		free(p);
	}

	{
		DSQ dsql(dsqParam, extraDataDir);
		dsql.init();
		dsql.run();
		dsql.shutdown();
	}

	return (0);
}
