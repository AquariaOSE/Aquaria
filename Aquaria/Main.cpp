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

#include <filesystem>
#include "DSQ.h"


#ifdef BBGE_BUILD_WINDOWS
	#include <shellapi.h>
#endif


static void MakeRan(void)
{
#ifdef BBGE_BUILD_WINDOWS
    std::ofstream out("ran");
    if(out)
    {
        for (int i = 0; i < 32; i++)
            out << rand()%1000;
        out.close();
    }
#endif
}

static void StartAQConfig()
{
#if defined(BBGE_BUILD_WINDOWS)
    if (!exists("ran", false, true))
    {
        MakeRan();
        if(exists("AQConfig.exe", false, true))
        {
            ShellExecute(NULL, "open", "AQConfig.exe", NULL, NULL, SW_SHOWNORMAL);
            exit(0);
        }
    }
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
		std::string extraDataDir = "";
        std::string appImageExtraDir = "";
        const char *appImageDir = 0;

		const char *envPath = 0;
#ifdef BBGE_BUILD_UNIX
		envPath = getenv("AQUARIA_DATA_PATH");
		if (envPath)
		{
			dsqParam = envPath;
		}
        appImageDir = getenv("APPIMAGE");
        if (appImageDir)
        {
            std::filesystem::path appImagePath = appImageDir;
            appImageExtraDir = appImagePath.parent_path();
        }
#endif
#ifdef AQUARIA_DEFAULT_DATA_DIR
		if(!envPath)
			dsqParam = AQUARIA_DEFAULT_DATA_DIR;
#endif
#ifdef AQUARIA_EXTRA_DATA_DIR
		extraDataDir = AQUARIA_EXTRA_DATA_DIR;
#endif

#endif

        CheckConfig();

        {
            DSQ dsql(dsqParam, extraDataDir, appImageExtraDir);
            dsql.init();
            dsql.main();
            dsql.shutdown();
        }

        MakeRan();

		return (0);
	}

