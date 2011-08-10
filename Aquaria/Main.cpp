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
//#define SANITY_TEST

#ifdef SANITY_TEST
	#include "Core.h"
	#include "Quad.h"
	
	class SanityTest : public Core
	{
		std::string dud;
	public:

		SanityTest() : Core(dud)
		{
		}
		void init()
		{
			Core::init();

			if (!createWindow(800,600,32,0, "Aquaria"))	return;
			debugLog("Init Input Library...");
				initInputLibrary();
			debugLog("OK");

			debugLog("Init Sound Library...");
				initSoundLibrary();
			debugLog("OK");
			
			debugLog("Init Graphics Library...");
				initGraphicsLibrary(0, 1);	
				core->enable2D(800);
				//core->initFrameBuffer();
			debugLog("OK");

			renderObjectLayers.resize(2);

			Quad *q = new Quad;
			q->setTexture("gfx/Logo");
			q->position = Vector(400,300);
			addRenderObject(q, 1);
			
		}
		void onUpdate(float dt)
		{
			Core::onUpdate(dt);
			if (core->getKeyState(KEY_ESCAPE))
				quit();
		}
	};

	int WINAPI WinMain(	HINSTANCE	hInstance,			// Instance
						HINSTANCE	hPrevInstance,		// Previous Instance
						LPSTR		lpCmdLine,			// Command Line Parameters
						int			nCmdShow)			// Window Show State
	{
		#ifdef _DEBUG
			_CrtSetDbgFlag ( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF); 
			_CrtSetReportMode ( _CRT_ERROR, _CRTDBG_MODE_DEBUG);
		#endif
	 
		{
			SanityTest core;
			core.init();
			core.main();
			core.shutdown();

		}
		return (0);
	}

#else

	#include "DSQ.h"

#ifdef BBGE_BUILD_WINDOWS
	#include <shellapi.h>
#endif



	void enumerateTest()
	{
#ifdef BBGE_BUILD_SDL
		SDL_Rect **modes;
		/* Get available fullscreen/hardware modes */
		modes=SDL_ListModes(NULL, SDL_FULLSCREEN|SDL_HWSURFACE);

#ifdef BBGE_BUILD_WINDOWS
		/* Check is there are any modes available */
		if(modes == (SDL_Rect **)0){
			MessageBox(0, "No modes available!\n", "", MB_OK);
			return;
		}

		/* Check if or resolution is restricted */
		if(modes == (SDL_Rect **)-1){
			MessageBox(0, "All resolutions available.\n", "", MB_OK);
		}
		else{
			/* Print valid modes */
			printf("Available Modes\n");
			for(int i=0;modes[i];++i){
				std::ostringstream os;
				os << "[" << modes[i]->w << "x" << modes[i]->h << "]";
				MessageBox(0, os.str().c_str(), "", MB_OK);
				//printf("  %d x %d\n", modes[i]->w, modes[i]->h);
			}
		}
#endif
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

		DSQ core(GetCommandLine());

#elif defined(BBGE_BUILD_SDL)

	static inline void check_beta(void)
	{
		#if defined(BBGE_BUILD_UNIX) && defined(BETAEXPIRE)
		bool bail = false;

		fprintf(stderr, "\n\n\n");
		fprintf(stderr, "*********************************************************\n");
		fprintf(stderr, "*********************************************************\n");
		fprintf(stderr, "*********************************************************\n");
		fprintf(stderr, "*********************************************************\n");
		fprintf(stderr, "*********************************************************\n");

		if ( time(NULL) > (BETAEXPIRE + 14 * 24 * 60 * 60) ) {
			fprintf(stderr,
				"Sorry, but this beta of the game has expired, and will no\n"
				" longer run. This is to prevent tech support on out-of-date\n"
				" and prerelease versions of the game. Please go to\n"
				" http://www.bit-blot.com/ for information on getting a release\n"
				" version that does not expire.\n");
			bail = true;
		} else {
			fprintf(stderr, "     Warning: This is a beta version of AQUARIA.\n");
		}

		fprintf(stderr, "*********************************************************\n");
		fprintf(stderr, "*********************************************************\n");
		fprintf(stderr, "*********************************************************\n");
		fprintf(stderr, "*********************************************************\n");
		fprintf(stderr, "*********************************************************\n");
		fprintf(stderr, "\n\n\n");

		fflush(stderr);

		if (bail) {
			while (true) {
				_exit(0);
			}
		}
		#endif
	}

	extern "C" int main(int argc,char *argv[])
	{
		check_beta();
        
#ifdef BBGE_BUILD_WINDOWS
	#if defined(AQUARIA_DEMO) || defined(AQUARIA_FULL)
		if (!exists("ran", false))
		{
			std::ofstream out("ran");
			for (int i = 0; i < 32; i++)
				out << rand()%1000;
			out.close();

			ShellExecute(NULL, "open", "aqconfig.exe", NULL, NULL, SW_SHOWNORMAL);

			exit(0);
		}
	#endif
	
		remove("ran");
#endif

		std::string fileSystem = "";

#ifdef BBGE_BUILD_UNIX
		const char *envPath = getenv("AQUARIA_DATA_PATH");
		if (envPath != NULL)
			fileSystem = envPath;
#endif

		DSQ core(fileSystem);
#endif	 

		{			
			core.init();
			//enumerateTest();
			core.main();
			core.shutdown();
		}

#ifdef BBGE_BUILD_WINDOWS
		std::ofstream out("ran");
		for (int i = 0; i < 1; i++)
			out << rand()%1000;
		out.close();
#endif

		return (0);
	}

#endif
