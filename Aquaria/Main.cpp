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


#ifdef BBGE_BUILD_WINDOWS
	#include <shellapi.h>
#endif


static void MakeRan(void)
{
#ifdef BBGE_BUILD_WINDOWS
    std::ofstream out("ran");
    for (int i = 0; i < 32; i++)
        out << rand()%1000;
    out.close();
#endif
}

static void StartAQConfig()
{
#if defined(BBGE_BUILD_WINDOWS)
#if defined(AQUARIA_DEMO) || defined(AQUARIA_FULL)
    if (!exists("ran", false))
    {
        MakeRan();
        if(exists("aqconfig.exe", false))
        {
            ShellExecute(NULL, "open", "aqconfig.exe", NULL, NULL, SW_SHOWNORMAL);
            exit(0);
        }
    }
#endif
    remove("ran");
#endif
}

static void CheckConfig(void)
{
#ifdef BBGE_BUILD_WINDOWS
    bool hasCfg = exists("usersettings.xml", false, true);
    if(!hasCfg)
        StartAQConfig();
#endif
}


#if defined(BBGE_BUILD_WINDOWS) && !defined(BBGE_BUILD_SDL)
	int WINAPI WinMain(	HINSTANCE	hInstance,			// Instance
						HINSTANCE	hPrevInstance,		// Previous Instance
						LPSTR		lpCmdLine,			// Command Line Parameters
						int			nCmdShow)			// Window Show State
	{
		#ifdef _DEBUG
			_CrtSetDbgFlag ( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF); 
			_CrtSetReportMode ( _CRT_ERROR, _CRTDBG_MODE_DEBUG);
		#endif

        std::string dsqParam = GetCommandLine();

#else

	extern "C" int main(int argc,char *argv[])
	{
		std::string dsqParam = ""; // fileSystem

#ifdef BBGE_BUILD_UNIX
		const char *envPath = getenv("AQUARIA_DATA_PATH");
		if (envPath != NULL)
			dsqParam = envPath;
#endif
#endif

        CheckConfig();

        {
            DSQ dsql(dsqParam);
            dsql.init();
            dsql.main();
            dsql.shutdown();
        }

        MakeRan();

		return (0);
	}

