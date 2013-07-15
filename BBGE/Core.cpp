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
#include "Core.h"
#include "Texture.h"
#include "AfterEffect.h"
#include "Particles.h"

#include <time.h>
#include <iostream>

#ifdef BBGE_BUILD_UNIX
#include <limits.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#endif

#include <assert.h>

#if __APPLE__
#include <Carbon/Carbon.h>
#endif

#if BBGE_BUILD_WINDOWS
#include <shlobj.h>
#include <direct.h>
#endif

#ifdef BBGE_BUILD_SDL
	#include "SDL_syswm.h"
	static SDL_Surface *gScreen=0;
	bool ignoreNextMouse=false;
	Vector unchange;
#endif

Core *core = 0;

#ifdef BBGE_BUILD_WINDOWS
	HICON icon_windows = 0;
#endif

void Core::initIcon()
{
#ifdef BBGE_BUILD_WINDOWS
	HINSTANCE handle = ::GetModuleHandle(NULL);
	//if (icon_windows)
	//	::DestroyIcon(icon_windows);

	icon_windows = ::LoadIcon(handle, "icon");

	SDL_SysWMinfo wminfo;
	SDL_VERSION(&wminfo.version)
	if (SDL_GetWMInfo(&wminfo) != 1)
	{
		//errorLog("wrong SDL version");
		// error: wrong SDL version
	}

	HWND hwnd = wminfo.window;

	::SetClassLong(hwnd, GCL_HICON, (LONG) icon_windows);
#endif
}

void Core::resetCamera()
{
	cameraPos = Vector(0,0);
}

ParticleEffect* Core::createParticleEffect(const std::string &name, const Vector &position, int layer, float rotz)
{
	ParticleEffect *e = new ParticleEffect();
	e->load(name);
	e->position = position;
	e->start();
	e->setDie(true);
	e->rotation.z = rotz;
	core->getTopStateData()->addRenderObject(e, layer);
	return e;
}

void Core::unloadDevice()
{
	for (int i = 0; i < renderObjectLayers.size(); i++)
	{
		RenderObjectLayer *r = &renderObjectLayers[i];
		RenderObject *robj = r->getFirst();
		while (robj)
		{
			robj->unloadDevice();
			robj = r->getNext();
		}
	}
	frameBuffer.unloadDevice();

	if (afterEffectManager)
		afterEffectManager->unloadDevice();
}

void Core::reloadDevice()
{
	for (int i = 0; i < renderObjectLayers.size(); i++)
	{
		RenderObjectLayer *r = &renderObjectLayers[i];
		r->reloadDevice();
		RenderObject *robj = r->getFirst();
		while (robj)
		{
			robj->reloadDevice();
			robj = r->getNext();
		}
	}
	frameBuffer.reloadDevice();

	if (afterEffectManager)
		afterEffectManager->reloadDevice();
}

void Core::resetGraphics(int w, int h, int fullscreen, int vsync, int bpp)
{
	if (fullscreen == -1)
		fullscreen = _fullscreen;

	if (vsync == -1)
		vsync = _vsync;

	if (w == -1)
		w = width;

	if (h == -1)
		h = height;

	if (bpp == -1)
		bpp = _bpp;

	unloadDevice();
	unloadResources();

	shutdownGraphicsLibrary();

	initGraphicsLibrary(w, h, fullscreen, vsync, bpp);
	
	enable2DWide(w, h);

	reloadResources();
	reloadDevice();


	resetTimer();
}

void Core::toggleScreenMode(int t)
{
#ifdef BBGE_BUILD_GLFW
/*
		glfwCloseWindow();

		createWindow(800,600,32,false,"");
		initGraphicsLibrary(false, true);
		enable2D(800);
		//reloadResources();
		*/
#endif
#ifdef BBGE_BUILD_SDL
	sound->pause();
	resetGraphics(-1, -1, t);
	cacheRender();
	resetTimer();
	sound->resume();
#endif
}

void Core::updateCursorFromJoystick(float dt, int spd)
{
	//debugLog("updating mouse from joystick");

	core->mouse.position += joystick.position*dt*spd;

/*
	if (!joystick.position.isZero())
		setMousePosition(core->mouse.position);
	*/

	doMouseConstraint();
}

void Core::setWindowCaption(const std::string &caption, const std::string &icon)
{
#ifdef BBGE_BUILD_SDL
	SDL_WM_SetCaption(caption.c_str(), icon.c_str());
#endif
}

RenderObjectLayer *Core::getRenderObjectLayer(int i)
{
	if (i == LR_NONE)
		return 0;
	return &renderObjectLayers[i];
}

#if defined(BBGE_BUILD_WINDOWS) && !defined(BBGE_BUILD_SDL)
	LPDIRECTINPUT8			g_pDI       = NULL; // The DirectInput object
	LPDIRECTINPUTDEVICE8	g_pKeyboard = NULL; // The keyboard device
	LPDIRECTINPUTDEVICE8	g_pMouse	= NULL;

	D3DCOLOR				d3dColor	=0xFFFFFFFF;
#endif

#ifdef BBGE_BUILD_DIRECTX

	__int64 timerStart=0, timerEnd=0, timerFreq=0;
	//Direct3D 9 interface
	IDirect3D9* d3d						= NULL;
	//Capabilities of graphics adapter
	D3DCAPS9 d3dCaps;

	//Direct3D present parameters
	D3DPRESENT_PARAMETERS d3dPresent;
	LPDIRECT3DDEVICE9       g_pd3dDevice = NULL; // Our rendering device
	LPD3DXSPRITE			d3dSprite	= NULL;
	LPD3DXMATRIXSTACK		d3dMatrixStack = NULL;
	IDirect3DVertexBuffer9* vertexBuffer	= NULL;
	IDirect3DVertexBuffer9* preTransVertexBuffer = NULL;

	//Custom vertex
	struct TLVERTEX
	{
	    float x;
	    float y;
	    float z;
	    //float rhw;
	    D3DCOLOR colour;
	    float u;
	    float v;
	};
	const DWORD D3DFVF_TLVERTEX = D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX1;
	struct PTLVERTEX
	{
	    float x;
	    float y;
	    float z;
	    float rhw;
	    D3DCOLOR colour;
	    float u;
	    float v;
	};

	const DWORD D3DFVF_PTLVERTEX = D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1;
#endif



	#ifdef BBGE_BUILD_DIRECTX
	/*
	LPDIRECT3DVERTEXBUFFER9 g_pVB        = NULL; // Buffer to hold vertices
	LPDIRECT3DTEXTURE9      g_pTexture   = NULL; // Our texture
	*/
	// A structure for our custom vertex type
	struct CUSTOMVERTEX
	{
	    FLOAT x, y, z, rhw; // The transformed position for the vertex
	    DWORD color;        // The vertex color
	};

	// Our custom FVF, which describes our custom vertex structure
	#define D3DFVF_CUSTOMVERTEX (D3DFVF_XYZRHW|D3DFVF_DIFFUSE)

	LPD3DXMATRIXSTACK Core::getD3DMatrixStack()
	{
		return d3dMatrixStack;
	}

	LPDIRECT3DDEVICE9 Core::getD3DDevice()
	{
		return g_pd3dDevice;
	}

	LPD3DXSPRITE Core::getD3DSprite()
	{
		return d3dSprite;
	}

	LRESULT WINAPI MsgProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
	{
		switch( msg )
		{
	        case WM_DESTROY:
				//Cleanup();
				PostQuitMessage( 0 );
				return 0;
		}

	    return DefWindowProc( hWnd, msg, wParam, lParam );
}
	void Core::blitD3DVerts(IDirect3DTexture9 *texture, float v1x, float v1y, float v2x, float v2y, float v3x, float v3y, float v4x, float v4y)
	{
		TLVERTEX* vertices;

		//Lock the vertex buffer
		vertexBuffer->Lock(0, 0, (void**)&vertices, NULL);

		vertices[0].colour = d3dColor;
		vertices[0].x = v1x;
		vertices[0].y = v1y;
		vertices[0].z = 1.0f;
		vertices[0].u = 0.0f;
		vertices[0].v = 1.0f-1.0f;

		vertices[1].colour = d3dColor;
		vertices[1].x = v2x;
		vertices[1].y = v2y;
		vertices[1].z = 1.0f;
		vertices[1].u = 1.0f;
		vertices[1].v = 1.0f-1.0f;

		vertices[2].colour = d3dColor;
		vertices[2].x = v3x;
		vertices[2].y = v3y;
		vertices[2].z = 1.0f;
		vertices[2].u = 1.0f;
		vertices[2].v = 1.0f-0.0f;

		vertices[3].colour = d3dColor;
		vertices[3].x = v4x;
		vertices[3].y = v4y;
		vertices[3].z = 1.0f;
		vertices[3].u = 0.0f;
		vertices[3].v = 1.0f-0.0f;
		//Unlock the vertex buffer
		vertexBuffer->Unlock();

		//Set texture
		g_pd3dDevice->SetTexture (0, texture);

		//Draw image
		g_pd3dDevice->DrawPrimitive (D3DPT_TRIANGLEFAN, 0, 2);
	}

	void Core::blitD3DEx (IDirect3DTexture9 *texture, int w2, int h2, float u1, float v1, float u2, float v2)
	{
		TLVERTEX* vertices;

		/*
		int w2=width/2;
		int h2=height/2;
		*/
		//Lock the vertex buffer
		vertexBuffer->Lock(0, 0, (void**)&vertices, NULL);

		//Setup vertices
		//A -0.5f modifier is applied to vertex coordinates to match texture
		//and screen coords. Some drivers may compensate for this
		//automatically, but on others texture alignment errors are introduced
		//More information on this can be found in the Direct3D 9 documentation
		vertices[0].colour = d3dColor;
		vertices[0].x = -0.5f*w2;
		vertices[0].y = -0.5f*h2;
		vertices[0].z = 1.0f;
		//vertices[0].rhw = 1.0f;
		vertices[0].u = u1;
		vertices[0].v = 1.0f-v2;

		vertices[1].colour = d3dColor;
		vertices[1].x = 0.5f*w2;
		vertices[1].y = -0.5f*h2;
		vertices[1].z = 1.0f;
		//vertices[1].rhw = 1.0f;
		vertices[1].u = u2;
		vertices[1].v = 1.0f-v2;

		vertices[2].colour = d3dColor;
		vertices[2].x = 0.5f*w2;
		vertices[2].y = 0.5f*h2;
		vertices[2].z = 1.0f;
		//vertices[2].rhw = 1.0f;
		vertices[2].u = u2;
		vertices[2].v = 1.0f-v1;

		vertices[3].colour = d3dColor;
		vertices[3].x = -0.5f*w2;
		vertices[3].y = 0.5f*h2;
		vertices[3].z = 1.0f;
		//vertices[3].rhw = 1.0f;
		vertices[3].u = u1;
		vertices[3].v = 1.0f-v1;
		//Unlock the vertex buffer
		vertexBuffer->Unlock();

		//Set texture
		g_pd3dDevice->SetTexture (0, texture);

		//Draw image
		g_pd3dDevice->DrawPrimitive (D3DPT_TRIANGLEFAN, 0, 2);
	}

	void Core::blitD3DGradient(D3DCOLOR ulc0, D3DCOLOR ulc1, D3DCOLOR ulc2, D3DCOLOR ulc3)
	{
		TLVERTEX* vertices;

		//Lock the vertex buffer
		vertexBuffer->Lock(0, 0, (void**)&vertices, NULL);
		vertices[0].colour = ulc0;
		vertices[0].x = -0.5f;
		vertices[0].y = -0.5f;
		vertices[0].z = 1.0f;
		//vertices[0].rhw = 1.0f;
		vertices[0].u = 0.0f;
		vertices[0].v = 1.0f-1.0f;

		vertices[1].colour = ulc1;
		vertices[1].x = 0.5f;
		vertices[1].y = -0.5f;
		vertices[1].z = 1.0f;
		//vertices[1].rhw = 1.0f;
		vertices[1].u = 1.0f;
		vertices[1].v = 1.0f-1.0f;

		vertices[2].colour = ulc2;
		vertices[2].x = 0.5f;
		vertices[2].y = 0.5f;
		vertices[2].z = 1.0f;
		//vertices[2].rhw = 1.0f;
		vertices[2].u = 1.0f;
		vertices[2].v = 1.0f-0.0f;

		vertices[3].colour = ulc3;
		vertices[3].x = -0.5f;
		vertices[3].y = 0.5f;
		vertices[3].z = 1.0f;
		//vertices[3].rhw = 1.0f;
		vertices[3].u = 0.0f;
		vertices[3].v = 1.0f-0.0f;
		//Unlock the vertex buffer
		vertexBuffer->Unlock();

		//Set texture
		//g_pd3dDevice->SetTexture (0, texture);
		g_pd3dDevice->SetTexture (0, 0);

		//Draw image
		g_pd3dDevice->DrawPrimitive (D3DPT_TRIANGLEFAN, 0, 2);
	}

	void Core::blitD3DPreTrans(IDirect3DTexture9 *texture, float x, float y, int w2, int h2)
	{
		/*
		PTLVERTEX* vertices;
		//Lock the vertex buffer
		preTransVertexBuffer->Lock(0, 0, (void**)&vertices, NULL);
		*/
		TLVERTEX* vertices;
		//Lock the vertex buffer
		vertexBuffer->Lock(0, 0, (void**)&vertices, NULL);


		//Setup vertices
		//A -0.5f modifier is applied to vertex coordinates to match texture
		//and screen coords. Some drivers may compensate for this
		//automatically, but on others texture alignment errors are introduced
		//More information on this can be found in the Direct3D 9 documentation
		vertices[0].colour = d3dColor;
		vertices[0].x = x-0.5f*w2;
		vertices[0].y = y-0.5f*h2;
		vertices[0].z = 1.0f;
		//vertices[0].rhw = 1.0f;
		vertices[0].u = 0.0f;
		vertices[0].v = 1.0f-1.0f;

		vertices[1].colour = d3dColor;
		vertices[1].x = x+0.5f*w2;
		vertices[1].y = y-0.5f*h2;
		vertices[1].z = 1.0f;
		//vertices[1].rhw = 1.0f;
		vertices[1].u = 1.0f;
		vertices[1].v = 1.0f-1.0f;

		vertices[2].colour = d3dColor;
		vertices[2].x = x+0.5f*w2;
		vertices[2].y = y+0.5f*h2;
		vertices[2].z = 1.0f;
		//vertices[2].rhw = 1.0f;
		vertices[2].u = 1.0f;
		vertices[2].v = 1.0f-0.0f;

		vertices[3].colour = d3dColor;
		vertices[3].x = x-0.5f*w2;
		vertices[3].y = y+0.5f*h2;
		vertices[3].z = 1.0f;
		//vertices[3].rhw = 1.0f;
		vertices[3].u = 0.0f;
		vertices[3].v = 1.0f-0.0f;
		/*
		//Unlock the vertex buffer
		preTransVertexBuffer->Unlock();
		*/
		vertexBuffer->Unlock();


		//Set texture
		g_pd3dDevice->SetTexture (0, texture);

		//Draw image
		g_pd3dDevice->DrawPrimitive (D3DPT_TRIANGLEFAN, 0, 2);
	}
	void Core::blitD3D (IDirect3DTexture9 *texture, int w2, int h2)
	{
		TLVERTEX* vertices;
		//D3DCOLOR d3dColor = 0xFFFFFFFF;

		/*
		int w2=width/2;
		int h2=height/2;
		*/
		//Lock the vertex buffer
		vertexBuffer->Lock(0, 0, (void**)&vertices, NULL);

		//Setup verticeserr
		//A -0.5f modifier is applied to vertex coordinates to match texture
		//and screen coords. Some drivers may compensate for this
		//automatically, but on others texture alignment ors are introduced
		//More information on this can be found in the Direct3D 9 documentation
		vertices[0].colour = d3dColor;
		vertices[0].x = -0.5f*w2;
		vertices[0].y = -0.5f*h2;
		vertices[0].z = 1.0f;
		//vertices[0].rhw = 1.0f;
		vertices[0].u = 0.0f;
		vertices[0].v = 1.0f-1.0f;

		vertices[1].colour = d3dColor;
		vertices[1].x = 0.5f*w2;
		vertices[1].y = -0.5f*h2;
		vertices[1].z = 1.0f;
		//vertices[1].rhw = 1.0f;
		vertices[1].u = 1.0f;
		vertices[1].v = 1.0f-1.0f;

		vertices[2].colour = d3dColor;
		vertices[2].x = 0.5f*w2;
		vertices[2].y = 0.5f*h2;
		vertices[2].z = 1.0f;
		//vertices[2].rhw = 1.0f;
		vertices[2].u = 1.0f;
		vertices[2].v = 1.0f-0.0f;

		vertices[3].colour = d3dColor;
		vertices[3].x = -0.5f*w2;
		vertices[3].y = 0.5f*h2;
		vertices[3].z = 1.0f;
		//vertices[3].rhw = 1.0f;
		vertices[3].u = 0.0f;
		vertices[3].v = 1.0f-0.0f;
		//Unlock the vertex buffer
		vertexBuffer->Unlock();

		//Set texture
		g_pd3dDevice->SetTexture (0, texture);

		//Draw image
		g_pd3dDevice->DrawPrimitive (D3DPT_TRIANGLEFAN, 0, 2);
	}

	HRESULT InitD3D( HWND hWnd, bool fullscreen, int vsync)
	{
		// Create the D3D object.
		HRESULT hr;

		//Make Direct3D object
		d3d = Direct3DCreate9(D3D_SDK_VERSION);

		//Make sure NULL pointer was not returned
		if (!d3d)
			return FALSE;

		//Get device capabilities
		ZeroMemory (&d3dCaps, sizeof(d3dCaps));
		if (FAILED(d3d->GetDeviceCaps (D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, &d3dCaps)))
			return FALSE;

		//Setup present parameters
		ZeroMemory(&d3dPresent,sizeof(d3dPresent));
		d3dPresent.hDeviceWindow = hWnd;

		//Check if windowed
		if (!fullscreen)
		{
			D3DDISPLAYMODE d3ddm;
			RECT rWindow;

			//Get display mode
			d3d->GetAdapterDisplayMode (D3DADAPTER_DEFAULT, &d3ddm);

			//Get window bounds
			GetClientRect (hWnd, &rWindow);

			//Setup screen dimensions
			core->width = rWindow.right - rWindow.left;
			core->height = rWindow.bottom - rWindow.top;

			//Setup backbuffer
			d3dPresent.Windowed = true;
			d3dPresent.BackBufferWidth = rWindow.right - rWindow.left;
			d3dPresent.BackBufferHeight = rWindow.bottom - rWindow.top;
		}
		else
		{
			d3dPresent.Windowed = false;
			d3dPresent.BackBufferWidth = core->width;
			d3dPresent.BackBufferHeight = core->height;
		}
		d3dPresent.BackBufferFormat = D3DFMT_A8R8G8B8;
		d3dPresent.BackBufferCount = 1;
		d3dPresent.SwapEffect = D3DSWAPEFFECT_DISCARD;

		if (vsync>0)
			d3dPresent.PresentationInterval = D3DPRESENT_INTERVAL_ONE;
		else
			d3dPresent.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;


		//Check if hardware vertex processing is available
		if (d3dCaps.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT)
		{
			debugLog("hardware T&L!");
			//Create device with hardware vertex processing
			hr = d3d->CreateDevice(D3DADAPTER_DEFAULT,D3DDEVTYPE_HAL, hWnd,
				D3DCREATE_HARDWARE_VERTEXPROCESSING, &d3dPresent, &g_pd3dDevice);
		}
		else
		{
			debugLog("no hardware T&L.");
			//Create device with software vertex processing
			hr = d3d->CreateDevice(D3DADAPTER_DEFAULT,D3DDEVTYPE_HAL, hWnd,
				D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dPresent, &g_pd3dDevice);
		}

		//Make sure device was created
		if (FAILED(hr))
		{
			errorLog ("directx init failed");
			return false;
		}

		g_pd3dDevice->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
		g_pd3dDevice->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
		// Turn off culling
		g_pd3dDevice->SetRenderState( D3DRS_CULLMODE, D3DCULL_CCW );
		/*
			D3DCULL_NONE = 1,
			D3DCULL_CW = 2,
			D3DCULL_CCW = 3,
		*/
		// Turn off D3D lighting
		g_pd3dDevice->SetRenderState( D3DRS_LIGHTING, FALSE );
		// Turn on the zbuffer
		g_pd3dDevice->SetRenderState( D3DRS_ZENABLE, FALSE);

		D3DXCreateSprite(core->getD3DDevice(), &d3dSprite);
		D3DXCreateMatrixStack(0, &d3dMatrixStack);

		//Set vertex shader
		g_pd3dDevice->SetVertexShader(NULL);
		g_pd3dDevice->SetFVF(D3DFVF_TLVERTEX);

		//Create vertex buffer
		g_pd3dDevice->CreateVertexBuffer(sizeof(TLVERTEX) * 4, NULL, D3DFVF_TLVERTEX, D3DPOOL_MANAGED, &vertexBuffer, NULL);
		g_pd3dDevice->SetStreamSource(0, vertexBuffer, 0, sizeof(TLVERTEX));

		/*
		g_pd3dDevice->CreateVertexBuffer(sizeof(PTLVERTEX) * 4, NULL, D3DFVF_TLVERTEX, D3DPOOL_MANAGED, &preTransVertexBuffer, NULL);
		g_pd3dDevice->SetStreamSource(0, preTransVertexBuffer, 0, sizeof(PTLVERTEX));
		*/

		g_pd3dDevice->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0,0,0), 1.0f, 0);

		g_pd3dDevice->SetRenderState( D3DRS_DIFFUSEMATERIALSOURCE, D3DMCS_COLOR1);
		g_pd3dDevice->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
		g_pd3dDevice->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
		g_pd3dDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);

		return S_OK;
	}
#endif


void Core::setColor(float r, float g, float b, float a)
{
#ifdef BBGE_BUILD_OPENGL
	glColor4f(r, g, b, a);
#endif
#ifdef BBGE_BUILD_DIRECTX
	d3dColor = D3DCOLOR_RGBA(int(r*255), int(g*255), int(b*255), int(a*255));
#endif
}

void Core::bindTexture(int stage, unsigned int handle)
{
#ifdef BBGE_BUILD_DIRECTX
	getD3DDevice()->SetTexture(stage, (IDirect3DBaseTexture9*)handle);
#endif
#ifdef BBGE_BUILD_OPENGL
	//glBindTexture(GL_TEXTURE_2D, handle);
#endif
}

void Core::translateMatrixStack(float x, float y, float z)
{
#ifdef BBGE_BUILD_OPENGL
	glTranslatef(x, y, z);
#endif
#ifdef BBGE_BUILD_DIRECTX
	/*
	D3DXMATRIX matTranslation;
	D3DXMatrixTranslation (&matTranslation, x, y, 0);
	*/
	/*
	float usex, usey;
    usex = x - (float)core->getWindowWidth() / 2;
    usey = -y + (float)core->getWindowHeight() / 2;
	*/
	//core->getD3DMatrixStack()->MultMatrixLocal(&matTranslation);
	core->getD3DMatrixStack()->TranslateLocal(x, y, z);
#endif
}

void Core::scaleMatrixStack(float x, float y, float z)
{
#ifdef BBGE_BUILD_OPENGL
	glScalef(x, y, z);
#endif
#ifdef BBGE_BUILD_DIRECTX
	if (x != 1 || y != 1)
		core->getD3DMatrixStack()->ScaleLocal(x, y, 1);
#endif
}

void Core::rotateMatrixStack(float x, float y, float z)
{
#ifdef BBGE_BUILD_OPENGL
	glRotatef(0, 0, 1, z);
#endif
#ifdef BBGE_BUILD_DIRECTX
	if (z != 0)
	{
		D3DXVECTOR3 axis(0,0,1);
		core->getD3DMatrixStack()->RotateAxisLocal(&axis,D3DXToRadian(z));
	}
#endif
}

void Core::applyMatrixStackToWorld()
{
#ifdef BBGE_BUILD_DIRECTX
	g_pd3dDevice->SetTransform(D3DTS_WORLD, core->getD3DMatrixStack()->GetTop());
#endif
}

void Core::rotateMatrixStack(float z)
{
#ifdef BBGE_BUILD_OPENGL
	glRotatef(0, 0, 1, z);
#endif
#ifdef BBGE_BUILD_DIRECTX
	//core->getD3DMatrixStack()->RotateAxis(0, 0, z);
	/*
	D3DXVECTOR3 axis(0,0,1);
	float angle = D3DXToRadian(z);
	if (angle == D3DX_PI)
		angle += 0.001f;
	core->getD3DMatrixStack()->RotateAxisLocal(&axis,angle);
	*/
	if (z != 0)
	{
		D3DXMATRIX mat;
		D3DXMatrixRotationZ(&mat,D3DXToRadian(z));
		core->getD3DMatrixStack()->MultMatrixLocal(&mat);
	}
#endif
}

bool Core::getShiftState()
{
	return getKeyState(KEY_LSHIFT) || getKeyState(KEY_RSHIFT);
}

bool Core::getAltState()
{
	return getKeyState(KEY_LALT) || getKeyState(KEY_RALT);
}

bool Core::getCtrlState()
{
	return getKeyState(KEY_LCONTROL) || getKeyState(KEY_RCONTROL);
}

bool Core::getMetaState()
{
	return getKeyState(KEY_LMETA) || getKeyState(KEY_RMETA);
}

void Core::errorLog(const std::string &s)
{
	messageBox("Error!", s);
	debugLog(s);
}

void Core::messageBox(const std::string &title, const std::string &msg)
{
	::messageBox(title, msg);
}

void Core::debugLog(const std::string &s)
{
	if (debugLogActive)
	{
		_logOut << s << std::endl;
	}
#ifdef _DEBUG
	std::cout << s << std::endl;
#endif
}

#ifdef BBGE_BUILD_WINDOWS
static bool checkWritable(const std::string& path, bool warn, bool critical)
{
	bool writeable = false;
	std::string f = path + "/~chk_wrt.tmp";
	FILE *fh = fopen(f.c_str(), "w");
	if(fh)
	{
		writeable = fwrite("abcdef", 5, 1, fh) == 1;
		fclose(fh);
		unlink(f.c_str());
	}
	if(!writeable)
	{
		if(warn)
		{
			std::ostringstream os;
			os << "Trying to use \"" << path << "\" as user data path, but it is not writeable.\n"
				<< "Please make sure the game is allowed to write to that directory.\n"
				<< "You can move the game to another location and run it there,\n"
				<< "or try running it as administrator, that may help as well.";
			if(critical)
				os << "\n\nWill now exit.";
			MessageBoxA(NULL, os.str().c_str(), "Need to write but can't!", MB_OK | MB_ICONERROR);
		}
		if(critical)
			exit(1);
	}
	return writeable;
}
#endif


const float SORT_DELAY = 10;
Core::Core(const std::string &filesystem, const std::string& extraDataDir, int numRenderLayers, const std::string &appName, int particleSize, std::string userDataSubFolder)
: ActionMapper(), StateManager(), appName(appName)
{
	sound = NULL;
	screenCapScale = Vector(1,1,1);
	timeUpdateType = TIMEUPDATE_DYNAMIC;
	_extraDataDir = extraDataDir;

	fixedFPS = 60;

	if (userDataSubFolder.empty())
		userDataSubFolder = appName;
		
#if defined(BBGE_BUILD_UNIX)
	const char *envr = getenv("HOME");
	if (envr == NULL)
        envr = ".";  // oh well.
	const std::string home(envr);

	mkdir(home.c_str(), 0700);  // just in case.

	// "/home/icculus/.Aquaria" or something. Spaces are okay.
	#ifdef BBGE_BUILD_MACOSX
	const std::string prefix("Library/Application Support/");
	#else
	const std::string prefix(".");
	#endif

	userDataFolder = home + "/" + prefix + userDataSubFolder;
	mkdir(userDataFolder.c_str(), 0700);
	debugLogPath = userDataFolder + "/";
	mkdir((userDataFolder + "/screenshots").c_str(), 0700);
	std::string prefpath(getPreferencesFolder());
	mkdir(prefpath.c_str(), 0700);
#else
	debugLogPath = "";
	userDataFolder = ".";

	#ifdef BBGE_BUILD_WINDOWS
	{
		if(checkWritable(userDataFolder, true, true)) // working dir?
		{
			puts("Using working directory as user directory.");
		}
		// TODO: we may want to use a user-specific path under windows as well
		// if the code below gets actually used, pass 2x false to checkWritable() above.
		// not sure about this right now -- FG
		/*else
		{
			puts("Working directory is not writeable...");
			char pathbuf[MAX_PATH];
			if(SHGetSpecialFolderPathA(NULL, &pathbuf[0], CSIDL_APPDATA, 0))
			{
				userDataFolder = pathbuf;
				userDataFolder += '/';
				userDataFolder += userDataSubFolder;
				for(uint32 i = 0; i < userDataFolder.length(); ++i)
					if(userDataFolder[i] == '\\')
						userDataFolder[i] = '/';
				debugLogPath = userDataFolder + "/";
				puts(("Using \"" + userDataFolder + "\" as user directory.").c_str());
				CreateDirectoryA(userDataFolder.c_str(), NULL);
				checkWritable(userDataFolder, true, true);
			}
			else
				puts("Failed to retrieve appdata path, using working dir."); // too bad, but can't do anything about it
		}
		*/
	}
	#endif
#endif

	_logOut.open((debugLogPath + "debug.log").c_str());
	debugLogActive = true;

	debugLogTextures = true;
	
	grabInputOnReentry = -1;

	srand(time(NULL));
	old_dt = 0;
	current_dt = 0;

	aspectX = 4;
	aspectY = 3;

	virtualOffX = virtualOffY = 0;
	vw2 = 0;
	vh2 = 0;

	viewOffX = viewOffY = 0;

	/*
	aspectX = 1440;  //4.0f;
	aspectY = 900;   //3.0f;
	*/

	particleManager = new ParticleManager(particleSize);
#ifdef BBGE_BUILD_SDL
	nowTicks = thenTicks = 0;
#endif
	_hasFocus = false;
	lib_graphics = lib_sound = lib_input = false;
	clearColor = Vector(0,0,0);
	updateCursorFromMouse = true;
	mouseConstraint = false;
	mouseCircle = 0;
	overrideStartLayer = 0;
	overrideEndLayer = 0;
	coreVerboseDebug = false;
	frameOutputMode = false;
	updateMouse = true;
	particlesPaused = false;
	joystickAsMouse = false;
	currentLayerPass = 0;
	flipMouseButtons = 0;
	joystickOverrideMouse = false;
	joystickEnabled = false;
	doScreenshot = false;
	baseCullRadius = 1;
	width = height = 0;
	afterEffectManagerLayer = 0;
	renderObjectLayers.resize(1);
	invGlobalScale = 1.0;
	invGlobalScaleSqr = 1.0;
	renderObjectCount = 0;
	avgFPS.resize(1);
	minimized = false;
	sortFlag = true;
	sortTimer = SORT_DELAY;
	numSavedScreenshots = 0;
	shuttingDown = false;
	clearedGarbageFlag = false;
	nestedMains = 0;
	afterEffectManager = 0;
	loopDone = false;
	core = this;

	#ifdef BBGE_BUILD_WINDOWS
		hRC = 0;
		hDC = 0;
		hWnd = 0;
	#endif

	for (int i = 0; i < KEY_MAXARRAY; i++)
	{
		keys[i] = 0;
	}

	aspect = (aspectX/aspectY);//320.0f/240.0f;
	//1.3333334f;

	globalResolutionScale = globalScale = Vector(1,1,1);

	initRenderObjectLayers(numRenderLayers);

	initPlatform(filesystem);
}

void Core::initPlatform(const std::string &filesystem)
{
#if defined(BBGE_BUILD_MACOSX) && !defined(BBGE_BUILD_MACOSX_NOBUNDLEPATH)
	// FIXME: filesystem not handled
	CFBundleRef mainBundle = CFBundleGetMainBundle();
	//CFURLRef resourcesURL = CFBundleCopyResourcesDirectoryURL(mainBundle);
	CFURLRef resourcesURL = CFBundleCopyBundleURL(mainBundle);
	char path[PATH_MAX];
	if (!CFURLGetFileSystemRepresentation(resourcesURL, TRUE, (UInt8 *)path, PATH_MAX))
	{
		// error!
		debugLog("CFURLGetFileSystemRepresentation");
	}
	CFRelease(resourcesURL);
	debugLog(path);
	chdir(path);
#elif defined(BBGE_BUILD_UNIX)
	if (!filesystem.empty())
	{
		if (chdir(filesystem.c_str()) == 0)
			return;
		else
			debugLog("Failed to chdir to filesystem path " + filesystem);
	}
#ifdef BBGE_DATA_PREFIX
	if (chdir(BBGE_DATA_PREFIX) == 0 && chdir(appName.c_str()) == 0)
		return;
	else
		debugLog("Failed to chdir to filesystem path " BBGE_DATA_PREFIX + appName);
#endif
	char path[PATH_MAX];
	// always a symlink to this process's binary, on modern Linux systems.
	const ssize_t rc = readlink("/proc/self/exe", path, sizeof (path));
	if ( (rc == -1) || (rc >= sizeof (path)) )
	{
		// error!
		debugLog("readlink");
	}
	else
	{
		path[rc] = '\0';
		char *ptr = strrchr(path, '/');
		if (ptr != NULL)
		{
			*ptr = '\0';
			debugLog(path);
			if (chdir(path) != 0)
				debugLog("Failed to chdir to executable path" + std::string(path));
		}
	}
#endif
#ifdef BBGE_BUILD_WINDOWS
	if(filesystem.length())
	{
		if(_chdir(filesystem.c_str()) != 0)
		{
			debugLog("chdir failed: " + filesystem);
		}
	}
	// FIXME: filesystem not handled
#endif
}

std::string Core::getPreferencesFolder()
{
#ifdef BBGE_BUILD_UNIX
	return userDataFolder + "/preferences";
#endif
#ifdef BBGE_BUILD_WINDOWS
	return "";
#endif
}

std::string Core::getUserDataFolder()
{
	return userDataFolder;
}

#if BBGE_BUILD_UNIX
#include <sys/types.h>
#include <pwd.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>

// based on code I wrote for PhysicsFS: http://icculus.org/physfs/
//  the zlib license on physfs allows this cut-and-pasting.
static int locateOneElement(char *buf)
{
	char *ptr;
	DIR *dirp;

	if (access(buf, F_OK) == 0)
		return(1);  // quick rejection: exists in current case.

	ptr = strrchr(buf, '/');  // find entry at end of path.
	if (ptr == NULL)
	{
		dirp = opendir(".");
		ptr = buf;
	}
	else
	{
		*ptr = '\0';
		dirp = opendir(buf);
		*ptr = '/';
		ptr++;  // point past dirsep to entry itself.
	}

	struct dirent *dent;
	while ((dent = readdir(dirp)) != NULL)
	{
		if (strcasecmp(dent->d_name, ptr) == 0)
		{
			strcpy(ptr, dent->d_name); // found a match. Overwrite with this case.
			closedir(dirp);
			return(1);
		}
	}

	// no match at all...
	closedir(dirp);
	return(0);
}
#endif


std::string Core::adjustFilenameCase(const char *_buf)
{
#ifdef BBGE_BUILD_UNIX  // any case is fine if not Linux.
	int rc = 1;
	char *buf = (char *) alloca(strlen(_buf) + 1);
	strcpy(buf, _buf);

	char *ptr = buf;
	while ((ptr = strchr(ptr + 1, '/')) != 0)
	{
		*ptr = '\0';  // block this path section off
		rc = locateOneElement(buf);
		*ptr = '/'; // restore path separator
		if (!rc)
			break;  // missing element in path.
	}

	// check final element...
	if (rc)
		rc = locateOneElement(buf);

	#if 0
	if (strcmp(_buf, buf) != 0)
	{
		fprintf(stderr, "Corrected filename case: '%s' => '%s (%s)'\n",
		        _buf, buf, rc ? "found" : "not found");
	}
	#endif

	return std::string(buf);
#else
	return std::string(_buf);
#endif
}


Core::~Core()
{
	if (particleManager)
	{
		delete particleManager;
	}
	if (sound)
	{
		delete sound;
		sound = 0;
	}
	debugLog("~Core()");
	_logOut.close();
	core = 0;
}

bool Core::hasFocus()
{
	return _hasFocus;
}

void Core::setInputGrab(bool on)
{
	if (isWindowFocus())
	{
#ifdef BBGE_BUILD_SDL
		SDL_WM_GrabInput(on?SDL_GRAB_ON:SDL_GRAB_OFF);
#endif
	}
}

void Core::setReentryInputGrab(int on)
{
	if (grabInputOnReentry == -1)
	{
		setInputGrab(on);
	}
	else
	{
		setInputGrab(grabInputOnReentry);
	}
}

bool Core::isFullscreen()
{
	return _fullscreen;
}

bool Core::isShuttingDown()
{
	return shuttingDown;
}

void Core::init()
{
	setupFileAccess();

	flags.set(CF_CLEARBUFFERS);
	quitNestedMainFlag = false;
#ifdef BBGE_BUILD_GLFW
	if (!glfwInit())
		exit(0);
#endif
#ifdef BBGE_BUILD_SDL
	// Disable relative mouse motion at the edges of the screen, which breaks
	// mouse control for absolute input devices like Wacom tablets and touchscreens.
	SDL_putenv((char *) "SDL_MOUSE_RELATIVE=0");

	if((SDL_Init(0))==-1)
	{
		exit_error("Failed to init SDL");
	}
	
#endif
	/*
#ifdef BBGE_BUILD_DIRECTX
	if (!glfwInit())
		exit(0);
#endif
		*/
	loopDone = false;
	clearedGarbageFlag = false;

	initInputCodeMap();

	//glfwSetWindowSizeCallback(lockWindowSize);
}

void Core::initRenderObjectLayers(int num)
{
	renderObjectLayers.resize(num);
	renderObjectLayerOrder.resize(num);
	for (int i = 0; i < num; i++)
	{
		renderObjectLayerOrder[i] = i;
	}
}

bool Core::initSoundLibrary(const std::string &defaultDevice)
{
	debugLog("Creating SoundManager");
	sound = new SoundManager(defaultDevice);
	debugLog("Done");
	return sound != 0;
}

Vector Core::getGameCursorPosition()
{
	return core->cameraPos + mouse.position * Vector(1/core->globalScale.x, 1/core->globalScale.y, 1);
}

Vector Core::getGamePosition(const Vector &v)
{
	return core->cameraPos + (v * Vector(1/core->globalScale.x, 1/core->globalScale.y, 1));
}

bool Core::getMouseButtonState(int m)
{
#ifdef BBGE_BUILD_SDL
	int mcode=m;

	switch(m)
	{
	case 0: mcode=1; break;
	case 1: mcode=3; break;
	case 2: mcode=2; break;
	}

	Uint8 mousestate = SDL_GetMouseState(0,0);

	return mousestate & SDL_BUTTON(mcode);
#endif
	return false;
}

bool Core::getKeyState(int k)
{
#ifdef BBGE_BUILD_GLFW
	return glfwGetKey(k)==GLFW_PRESS;
#endif

#ifdef BBGE_BUILD_SDL
	if (k >= KEY_MAXARRAY || k < 0)
	{
		return 0;
	}
	return keys[k];
#endif

#ifdef BBGE_BUILD_WINDOWS
	if (k >= KEY_MAXARRAY || k < 0)
	{
		return 0;
	}
	return keys[k];
#endif

	return 0;
}

//#ifdef BBGE_BUILD_DIRECTX

//float sensitivity = 1.0;

Vector joychange;
Vector lastjoy;
void readJoystickData()
{
	/*
	if (core->joystickEnabled && core->numJoysticks > 0)
	{
		//DIJOYSTATE2 js;
		core->joysticks[0]->poll(&core->joystate);

		Vector joy = Vector(core->joystate.lX, core->joystate.lY);
		joychange = joy-lastjoy;
		lastjoy = joy;
		core->joystickPosition = Vector(core->joystate.lX - (65536/2), core->joystate.lY - (65536/2));
		core->joystickPosition /= (65536/2);

		// HACK: super hacky!!
		core->mouse.buttons.left = (core->joystate.rgbButtons[0] & 0x80) ? Buttons::DOWN : Buttons::UP;
		core->mouse.buttons.right = (core->joystate.rgbButtons[1] & 0x80) ? Buttons::DOWN : Buttons::UP;
	}
	*/
}

void readMouseData()
{
#if defined(BBGE_BUILD_WINDOWS) && !defined(BBGE_BUILD_SDL)
	if (!core->updateMouse) return;
    HRESULT       hr;
    DIMOUSESTATE2 dims2;      // DirectInput Mouse state structure

    if( NULL == g_pMouse )
        return;

    // Get the input's device state, and put the state in dims
    ZeroMemory( &dims2, sizeof(dims2) );
    hr = g_pMouse->GetDeviceState( sizeof(DIMOUSESTATE2), &dims2 );
    if( FAILED(hr) )
    {
        // DirectInput may be telling us that the input stream has been
        // interrupted.  We aren't tracking any state between polls, so
        // we don't have any special reset that needs to be done.
        // We just re-acquire and try again.

        // If input is lost then acquire and keep trying
        hr = g_pMouse->Acquire();
        while( hr == DIERR_INPUTLOST )
            hr = g_pMouse->Acquire();

        // hr may be DIERR_OTHERAPPHASPRIO or other errors.  This
        // may occur when the app is minimized or in the process of
        // switching, so just try again later
        return;
    }

	//float sensitivity = float(core->width) / float(core->getVirtualWidth());
	float sensitivity = 1;
	core->mouse.position.x += dims2.lX*sensitivity;
	core->mouse.position.y += dims2.lY*sensitivity;
	core->mouse.position.z += dims2.lZ;
	core->mouse.change.x = dims2.lX*sensitivity;
	core->mouse.change.y = dims2.lY*sensitivity;
	core->mouse.change.z = dims2.lZ;
	core->mouse.scrollWheelChange = dims2.lZ;
	if (!core->flipMouseButtons)
	{
		core->mouse.buttons.left = (dims2.rgbButtons[0] & 0x80) ? DOWN : UP;
		core->mouse.buttons.right = (dims2.rgbButtons[1] & 0x80) ? DOWN : UP;
	}
	else
	{
		core->mouse.buttons.left = (dims2.rgbButtons[1] & 0x80) ? DOWN : UP;
		core->mouse.buttons.right = (dims2.rgbButtons[0] & 0x80) ? DOWN : UP;
	}
	core->mouse.buttons.middle = (dims2.rgbButtons[2] & 0x80) ? DOWN : UP;

#elif defined(BBGE_BUILD_SDL)
	//core->mouse.position += dMouse;
#elif defined(BBGE_BUILD_GLFW)
	//HACK: may not always want 800x600 virtual
	/*
	static int lastx=400, lasty=300;
	int x, y;
	glfwGetMousePos(&x,&y);
	int mickeyx,mickeyy;
	mickeyx = x - lastx;
	mickeyy = y - lasty;
	lastx = x;
	lasty = y;
	core->mouse.position.x += mickeyx;
	core->mouse.position.y += mickeyy;
	*/

	int x,y;
	glfwGetMousePos(&x,&y);
	core->mouse.position = Vector(x, y);


/*
	int mid_x = core->width / 2;
	int mid_y = core->height / 2;
	int dx=0,dy=0;
	int x,y;
	glfwGetMousePos(&x, &y);
	// Don't do anything if mouse hasn't moved
	if (x == mid_x && y == mid_y)
	{
	}
	else
	{
		dx = x - mid_x;
		dy = y - mid_y;
	}

	std::ostringstream os;
	os << "d(" << dx << ", " << dy <<")";
	debugLog(os.str());

	core->mouse.position += Vector(dx, dy);


	// Now move the mouse back to the middle, because
	// we don't care where it really is, just how much
	// it moves.
	glfwSetMousePos(mid_x, mid_y);
	*/



	core->mouse.buttons.left = glfwGetMouseButton(GLFW_MOUSE_BUTTON_LEFT) ? DOWN : UP;
	core->mouse.buttons.right = glfwGetMouseButton(GLFW_MOUSE_BUTTON_RIGHT) ? DOWN : UP;
	core->mouse.buttons.middle = glfwGetMouseButton(GLFW_MOUSE_BUTTON_MIDDLE) ? DOWN : UP;
	core->mouse.scrollWheel = glfwGetMouseWheel();
#endif
}

void readKeyData()
{

#if defined(BBGE_BUILD_WINDOWS) && !defined(BBGE_BUILD_SDL)
	if( NULL == g_pKeyboard )
		return;
	HRESULT hr;
	BYTE    diks[256];
    // Get the input's device state, and put the state in dims
    ZeroMemory( diks, sizeof(diks) );
    hr = g_pKeyboard->GetDeviceState( sizeof(diks), diks );
    if( FAILED(hr) )
    {
        // DirectInput may be telling us that the input stream has been
        // interrupted.  We aren't tracking any state between polls, so
        // we don't have any special reset that needs to be done.
        // We just re-acquire and try again.

        // If input is lost then acquire and keep trying
        hr = g_pKeyboard->Acquire();
        while( hr == DIERR_INPUTLOST )
            hr = g_pKeyboard->Acquire();

        // hr may be DIERR_OTHERAPPHASPRIO or other errors.  This
        // may occur when the app is minimized or in the process of
        // switching, so just try again later
        return;
    }

    // Make a string of the index values of the keys that are down
    for(int i = 0; i < 256; i++ )
    {
        core->keys[i] = ( diks[i] & 0x80 );
    }
#endif
}
//#endif


bool Core::initJoystickLibrary(int numSticks)
{
	//joystickEnabled = false;
#ifdef BBGE_BUILD_SDL
	SDL_InitSubSystem(SDL_INIT_JOYSTICK);
#endif

	if (numSticks > 0)
		joystick.init(0);

	joystickEnabled = true;
	/*
	numJoysticks = Joystick::deviceCount();
	std::ostringstream os;
	os << "Found " << numJoysticks << " joysticks";
	debugLog(os.str());
	if (numJoysticks > 0)
	{
		if (numJoysticks > 4)
			numJoysticks = 4;

		// HACK: memory leak... add code to clean this up!
		for (int i = 0; i < numJoysticks; i++) {
			joysticks[i] = new Joystick(i);
			joysticks[i]->open();

			// Print the name of the joystick.
			char name[MAX_PATH];
			joysticks[i]->deviceName(name);
			std::ostringstream os;
			os << "   Joystick " << i << ": " << name;
			debugLog(os.str());
		}
		joystickEnabled = true;
		return true;
	}
	*/

	return true;
}

bool Core::initInputLibrary()
{
	core->mouse.position = Vector(getWindowWidth()/2, getWindowHeight()/2);

#ifdef BBGE_BUILD_GFLW
	glfwDisable(GLFW_MOUSE_CURSOR);
	//glfwEnable( GLFW_SYSTEM_KEYS );
#endif
	for (int i = 0; i < KEY_MAXARRAY; i++)
	{
		keys[i] = 0;
	}
#if defined(BBGE_BUILD_WINDOWS) && !defined(BBGE_BUILD_SDL)

	HRESULT hr;
    BOOL    bExclusive = true;
    BOOL    bForeground = true;
    //BOOL    bImmediate = true;
    BOOL    bDisableWindowsKey = false;
    DWORD   dwCoopFlags;

    if( bExclusive )
        dwCoopFlags = DISCL_EXCLUSIVE;
    else
        dwCoopFlags = DISCL_NONEXCLUSIVE;

    if( bForeground )
        dwCoopFlags |= DISCL_FOREGROUND;
    else
        dwCoopFlags |= DISCL_BACKGROUND;

    // Disabling the windows key is only allowed only if we are in foreground nonexclusive
    if( bDisableWindowsKey && !bExclusive && bForeground )
        dwCoopFlags |= DISCL_NOWINKEY;

    // Create a DInput object
    if( FAILED( hr = DirectInput8Create( GetModuleHandle(NULL), DIRECTINPUT_VERSION,
                                         IID_IDirectInput8, (VOID**)&g_pDI, NULL ) ) )
        return false;

    // Obtain an interface to the system keyboard device.
    if( FAILED( hr = g_pDI->CreateDevice( GUID_SysKeyboard, &g_pKeyboard, NULL ) ) )
        return false;

    // Set the data format to "Keyboard format" - a predefined data format
    //
    // A data format specifies which controls on a device we
    // are interested in, and how they should be reported.
    //
    // This tells DirectInput that we will be passing an array
    // of 256 bytes to IDirectInputDevice::GetDeviceState.
    if( FAILED( hr = g_pKeyboard->SetDataFormat( &c_dfDIKeyboard ) ) )
        return false;

    // Set the cooperativity level to let DirectInput know how
    // this device should interact with the system and with other
    // DirectInput applications.
    hr = g_pKeyboard->SetCooperativeLevel( this->hWnd, dwCoopFlags );
    if( hr == DIERR_UNSUPPORTED && !bForeground && bExclusive )
    {
		debugLog("could not set cooperative level");
        //FreeDirectInput();
		//errorLog ("failed to init input");
		/*
        MessageBox( hDlg, _T("SetCooperativeLevel() returned DIERR_UNSUPPORTED.\n")
                          _T("For security reasons, background exclusive Keyboard\n")
                          _T("access is not allowed."), _T("Keyboard"), MB_OK );
		*/
        //return false;;
    }

	/*
    if( FAILED(hr) )
	{
		errorLog("failed to init input");
		return false;
	}
	*/


    // Acquire the newly created device
    g_pKeyboard->Acquire();


//#ifdef BBGE_BUILD_DIRECTX

	if( FAILED( hr = g_pDI->CreateDevice( GUID_SysMouse, &g_pMouse, NULL ) ) )
        return false;

    // Set the data format to "Mouse format" - a predefined data format
    //
    // A data format specifies which controls on a device we
    // are interested in, and how they should be reported.
    //
    // This tells DirectInput that we will be passing a
    // DIMOUSESTATE2 structure to IDirectInputDevice::GetDeviceState.
    if( FAILED( hr = g_pMouse->SetDataFormat( &c_dfDIMouse2 ) ) )
        return false;

    // Set the cooperativity level to let DirectInput know how
    // this device should interact with the system and with other
    // DirectInput applications.
    hr = g_pMouse->SetCooperativeLevel( this->hWnd, dwCoopFlags );
    if( hr == DIERR_UNSUPPORTED && !bForeground && bExclusive )
    {
        //FreeDirectInput();
		//errorLog ("mouse failed");
		debugLog("could not set cooperative level");
        //return false;
    }

	/*
    if( FAILED(hr) )
		return false;
	*/

    // Acquire the newly created device
    g_pMouse->Acquire();

#endif



	// joystick init
//#endif
	return true;
}

void Core::onUpdate(float dt)
{
	if (minimized) return;
	ActionMapper::onUpdate(dt);
	StateManager::onUpdate(dt);


	core->mouse.lastPosition = core->mouse.position;
	core->mouse.lastScrollWheel = core->mouse.scrollWheel;

	readKeyData();
	readMouseData();
	readJoystickData();
	pollEvents();
	joystick.update(dt);






	/*
	std::ostringstream os;
	os << "x: " << joystate.lX << " y: " << joystate.lY;
	os << " frx: " << joystate.lFRx << " fry: " << joystate.lFRy;
	debugLog(os.str());
	*/

	/*
	if (joystickOverrideMouse && !joychange.isZero())
	{
		Vector joy(joystate.lX, joystate.lY);
		//core->mouse.position += joychange * 0.001f;
		core->mouse.position = Vector(400,300) + ((joy * 600) / (65536/2))-300;
	}
	*/


	/*

	*/


	/*
	if (mouse.position.x < 0)
		mouse.position.x = 0;
	if (mouse.position.x > core->getVirtualWidth())
		mouse.position.x = core->getVirtualWidth();
	if (mouse.position.y < 0)
		mouse.position.y = 0;
	if (mouse.position.y > core->getVirtualHeight())
		mouse.position.y = core->getVirtualHeight();
	*/

	onMouseInput();

	/*
#ifdef BBGE_BUILD_GLFW
	glfwSetMousePos(mouse.position.x, mouse.position.y);
#endif
	*/
#ifdef BBGE_BUILD_DIRECTX
#endif
	//core->mouse.change = core->mouse.position - core->mouse.lastPosition;

	//core->mouse.scrollWheelChange = core->mouse.scrollWheel - core->mouse.lastScrollWheel;





	//script.update(dt);

	cameraPos.update(dt);
	globalScale.update(dt);

	if (afterEffectManager)
	{
		afterEffectManager->update(dt);
	}

	if (!sortFlag)
	{
		if (sortTimer>0)
		{
			sortTimer -= dt;
			if (sortTimer <= 0)
			{
				sortTimer = SORT_DELAY;
				sort();
			}
		}
	}
}

Vector Core::getClearColor()
{
	return clearColor;
}

void Core::setClearColor(const Vector &c)
{
	clearColor = c;

#ifdef BBGE_BUILD_OPENGL
	glClearColor(c.x, c.y, c.z, 0.0);
#endif

#ifdef BBGE_BUILD_DIRECTX

#endif
}

void Core::setSDLGLAttributes()
{
	std::ostringstream os;
	os << "setting vsync: " << _vsync;
	debugLog(os.str());

#ifdef BBGE_BUILD_SDL
	SDL_GL_SetAttribute(SDL_GL_SWAP_CONTROL, _vsync);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
#endif
}


#ifdef GLAPIENTRY
#undef GLAPIENTRY
#endif

#ifdef BBGE_BUILD_WINDOWS
#define GLAPIENTRY APIENTRY
#else
#define GLAPIENTRY
#endif

unsigned int Core::dbg_numRenderCalls = 0;

#ifdef BBGE_BUILD_OPENGL_DYNAMIC
#define GL_FUNC(ret,fn,params,call,rt) \
    extern "C" { \
        static ret (GLAPIENTRY *p##fn) params = NULL; \
        ret GLAPIENTRY fn params { ++Core::dbg_numRenderCalls; rt p##fn call; } \
    }
#include "OpenGLStubs.h"
#undef GL_FUNC

static bool lookup_glsym(const char *funcname, void **func)
{
	*func = SDL_GL_GetProcAddress(funcname);
	if (*func == NULL)
	{
		std::ostringstream os;
		os << "Failed to find OpenGL symbol \"" << funcname << "\"\n";
		errorLog(os.str());
		return false;
	}
	return true;
}

static bool lookup_all_glsyms(void)
{
	bool retval = true;
	#define GL_FUNC(ret,fn,params,call,rt) \
		if (!lookup_glsym(#fn, (void **) &p##fn)) retval = false;
	#include "OpenGLStubs.h"
	#undef GL_FUNC
	return retval;
}
#endif


bool Core::initGraphicsLibrary(int width, int height, bool fullscreen, int vsync, int bpp, bool recreate)
{	
	static bool didOnce = false;
	
	aspectX = width;
	aspectY = height;

	aspect = (aspectX/aspectY);

	

	this->width = width;
	this->height = height;
	_vsync = vsync;
	_fullscreen = fullscreen;
	_bpp = bpp;

	_hasFocus = false;

#if defined(BBGE_BUILD_SDL)

	//setenv("SDL_VIDEO_CENTERED", "1", 1);
	//SDL_putenv("SDL_VIDEO_WINDOW_POS=400,300");

	// have to cast away constness, since SDL_putenv() might be #defined to
	//  putenv(), which takes a (char *), and freaks out newer GCC releases
	//  when you try to pass a (const!) string literal here...  --ryan.
	SDL_putenv((char *) "SDL_VIDEO_CENTERED=1");
	SDL_putenv((char *) "LIBGL_DEBUG=verbose"); // temp, to track errors on linux with nouveau drivers.

	if (recreate)
	{
		if (SDL_InitSubSystem(SDL_INIT_VIDEO) < 0)
		{
			exit_error(std::string("SDL Error: ") + std::string(SDL_GetError()));
		}

#if BBGE_BUILD_OPENGL_DYNAMIC
		if (SDL_GL_LoadLibrary(NULL) == -1)
		{
			std::string err = std::string("SDL_GL_LoadLibrary Error: ") + std::string(SDL_GetError());
			SDL_Quit();
			exit_error(err);
		}
#endif
	}

	setWindowCaption(appName, appName);

	initIcon();
    // Create window

	setSDLGLAttributes();

	//if (!didOnce)
	{
		Uint32 flags = 0;
		flags = SDL_OPENGL;
		if (fullscreen)
			flags |= SDL_FULLSCREEN;

		gScreen = SDL_SetVideoMode(width, height, bpp, flags);
		if (gScreen == NULL)
		{
			std::ostringstream os;
			os << "Couldn't set resolution [" << width << "x" << height << "]\n" << SDL_GetError();
			SDL_Quit();
			exit_error(os.str());
		}

#if BBGE_BUILD_OPENGL_DYNAMIC
		if (!lookup_all_glsyms())
		{
			std::ostringstream os;
			os << "Couldn't load OpenGL symbols we need\n";
			SDL_Quit();
			exit_error(os.str());
		}
#endif
	}

	setWindowCaption(appName, appName);

	SDL_WM_GrabInput(grabInputOnReentry==0 ? SDL_GRAB_OFF : SDL_GRAB_ON);
	char name[256];
	SDL_VideoDriverName((char*)name, 256);

	glViewport(0, 0, width, height);
	glScissor(0, 0, width, height);

	std::ostringstream os2;
	os2 << "Video Driver Name [" << name << "]";
	debugLog(os2.str());

	SDL_ShowCursor(SDL_DISABLE);
	SDL_PumpEvents();

	for (int i = 0; i < KEY_MAXARRAY; i++)
	{
		keys[i] = 0;
	}

#ifdef BBGE_BUILD_WINDOWS
	SDL_SysWMinfo wmInfo;
	SDL_GetWMInfo(&wmInfo);
	hWnd = wmInfo.window;
#endif

#endif

#if defined(BBGE_BUILD_OPENGL)
	glEnable(GL_TEXTURE_2D);							// Enable Texture Mapping
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);				// Black Background
	glClearDepth(1.0);								// Depth Buffer Setup
	glDisable(GL_CULL_FACE);
	
	//glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
	//glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);

	glLoadIdentity();
	
	glFinish();

#ifdef BBGE_BUILD_GLFW
	glfwSwapInterval(vsync);
#endif

#endif



#if defined(BBGE_BUILD_DIRECTX)

	// Initialize Direct3D
	if( SUCCEEDED( InitD3D( this->hWnd, fullscreen, vsync ) ) )
	{
		// Show the window
		ShowWindow( this->hWnd, SW_SHOWDEFAULT );
		UpdateWindow( this->hWnd );
		//initPipeline(PT_NORMAL);
	}
	else
	{
		errorLog("Could not init D3D");
		exit(-1);
	}

#endif

	setClearColor(clearColor);
	
	clearBuffers();
	showBuffer();

	lib_graphics = true;

	_hasFocus = true;

	enumerateScreenModes();

	if (!didOnce)
		didOnce = true;

	// init success
	return true;
}

void Core::enumerateScreenModes()
{
	screenModes.clear();

#ifdef BBGE_BUILD_SDL
	SDL_Rect **modes;
	int i;

	modes=SDL_ListModes(NULL, SDL_FULLSCREEN|SDL_HWSURFACE);

	if(modes == (SDL_Rect **)0){
		debugLog("No modes available!");
		return;
	}

	if(modes == (SDL_Rect **)-1){
		debugLog("All resolutions available.");
	}
	else{
		int c=0;
		for(i=0;modes[i];++i){
			c++;
		}
		for (i=c-1;i>=0;i--)
		{
			if (modes[i]->w > modes[i]->h)
			{
				screenModes.push_back(ScreenMode(i, modes[i]->w, modes[i]->h));
			}
		}
	}
#endif
}

void Core::shutdownSoundLibrary()
{
}

void Core::shutdownGraphicsLibrary(bool killVideo)
{
#ifdef BBGE_BUILD_SDL
	glFinish();
	if (killVideo) {
		SDL_QuitSubSystem(SDL_INIT_VIDEO);
		SDL_WM_GrabInput(SDL_GRAB_OFF);

		FrameBuffer::resetOpenGL();

		gScreen = 0;

#if BBGE_BUILD_OPENGL_DYNAMIC
		// reset all the entry points to NULL, so we know exactly what happened
		//  if we call a GL function after shutdown.
		#define GL_FUNC(ret,fn,params,call,rt) \
			p##fn = NULL;
		#include "OpenGLStubs.h"
		#undef GL_FUNC
#endif
	}
#endif

	_hasFocus = false;

	lib_graphics = false;

#ifdef BBGE_BUILD_WINDOWS
	if (icon_windows)
	{
		::DestroyIcon(icon_windows);
		icon_windows = 0;
	}
#endif

}

void Core::quit()
{
	enqueueJumpState("STATE_QUIT");
	//loopDone = true;
	//popAllStates();
}

void Core::applyState(const std::string &state)
{
	if (nocasecmp(state, "state_quit")==0)
	{
		loopDone = true;
	}
	StateManager::applyState(state);
}

#ifdef BBGE_BUILD_GLFW
void GLFWCALL windowResize(int w, int h)
{
	// this gets called on minimize + restore?
	if (w == 0 && h == 0)
	{
		core->minimized = true;
		return;
	}
	else
		core->minimized = false;
	if (w != core->width || h != core->height)
		glfwSetWindowSize(core->width,core->height);
}
#endif


#ifdef BBGE_BUILD_WINDOWS
void centerWindow(HWND hwnd)
{
    int x, y;
    HWND hwndDeskTop;
    RECT rcWnd, rcDeskTop;
    // Get a handle to the desktop window
    hwndDeskTop = ::GetDesktopWindow();
    // Get dimension of desktop in a rect
    ::GetWindowRect(hwndDeskTop, &rcDeskTop);
    // Get dimension of main window in a rect
    ::GetWindowRect(hwnd, &rcWnd);
    // Find center of desktop
	x = (rcDeskTop.right - rcDeskTop.left)/2;
	y = (rcDeskTop.bottom - rcDeskTop.top)/2;
    x -= (rcWnd.right - rcWnd.left)/2;
	y -= (rcWnd.bottom - rcWnd.top)/2;
    // Set top and left to center main window on desktop
    ::SetWindowPos(hwnd, HWND_TOP, x, y, 0, 0, SWP_NOSIZE);
//	::ShowWindow(hwnd, 1);
}
#endif

void Core::adjustWindowPosition(int x, int y)
{
#ifdef BBGE_BUILD_WINDOWS
	RECT rcWnd;
	::GetWindowRect(hWnd, &rcWnd);
	rcWnd.left += x;
	rcWnd.top += y;
	::SetWindowPos(hWnd, HWND_TOP, rcWnd.left, rcWnd.top, 0, 0, SWP_NOSIZE);
#endif
}

bool Core::createWindow(int width, int height, int bits, bool fullscreen, std::string windowTitle)
{
	this->width = width;
	this->height = height;

	redBits = greenBits = blueBits = alphaBits = 0;
#ifdef BBGE_BUILD_SDL
	return true;
#endif

#ifdef BBGE_BUILD_GLFW
	int redbits, greenbits, bluebits, alphabits;
	redbits = greenbits = bluebits = 8;
	alphabits = 0;
	switch(bits)
	{
	case 16:
		redbits = 5;
		greenbits = 6;
		bluebits = 5;
	break;
	case 24:
		redbits = 8;
		bluebits = 8;
		greenbits = 8;
		alphabits = 0;
	break;
	case 32:
		redbits = 8;
		greenbits = 8;
		bluebits = 8;
		alphabits = 8;
	break;
	case 8:
		redbits = 2;
		greenbits = 2;
		bluebits = 2;
	break;
	}
	if (glfwOpenWindow(width, height, redbits, greenbits, bluebits, 0, 0, 0, fullscreen ? GLFW_FULLSCREEN : GLFW_WINDOW) == GL_TRUE)
	{
		glfwSetWindowTitle(windowTitle.c_str());
		resize(width,height);


#ifdef BBGE_BUILD_WINDOWS
		this->hWnd = (HWND)glfwGetWindowHandle();

		if (!fullscreen)	centerWindow(hWnd);
#endif

		glfwSetWindowSizeCallback(windowResize);

		redBits = glfwGetWindowParam(GLFW_RED_BITS);
		blueBits = glfwGetWindowParam(GLFW_BLUE_BITS);
		greenBits = glfwGetWindowParam(GLFW_GREEN_BITS);
		alphaBits = glfwGetWindowParam(GLFW_ALPHA_BITS);

		if (redBits < 8 && (bits == 32 || bits == 24))
		{
			int sayBits = 32;
			std::ostringstream os;
			os << "(" << width << ", " << height << ") " << sayBits << "-bit mode could not be enabled. Please try setting your desktop to " << sayBits << "-bit color depth";
			if (!fullscreen)
				os << ", or try running in fullscreen.";
			else
				os << ".";
			os << " This resolution may not be supported on your machine.";
			errorLog(os.str());
			exit(0); return false;
		}
		return true;
	}
	else
		return false;
#endif

#ifdef BBGE_BUILD_DIRECTX
	// Register the window class
    WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, MsgProc, 0L, 0L,
                      GetModuleHandle(NULL), NULL, NULL, NULL, NULL,
                      windowTitle.c_str(), NULL };
    RegisterClassEx( &wc );

	this->hWnd = CreateWindow( windowTitle.c_str(), windowTitle.c_str(),
							WS_OVERLAPPEDWINDOW, 100, 100, width, height+10,
							GetDesktopWindow(), NULL, wc.hInstance, NULL );
	return true;
#endif
}

// No longer part of C/C++ standard
#ifndef M_PI
#define M_PI           3.14159265358979323846
#endif

static void
bbgePerspective(float fovy, float aspect, float zNear, float zFar)
{
    float sine, cotangent, deltaZ;
    float radians = fovy / 2.0f * M_PI / 180.0f;

    deltaZ = zFar - zNear;
    sine = sinf(radians);
    if ((deltaZ == 0.0f) || (sine == 0.0f) || (aspect == 0.0f)) {
        return;
    }
    cotangent = cosf(radians) / sine;

    GLfloat m[4][4] = {
        { 1.0f, 0.0f, 0.0f, 0.0f },
        { 0.0f, 1.0f, 0.0f, 0.0f },
        { 0.0f, 0.0f, 1.0f, 0.0f },
        { 0.0f, 0.0f, 0.0f, 1.0f }
    };
    m[0][0] = (GLfloat) (cotangent / aspect);
    m[1][1] = (GLfloat) cotangent;
    m[2][2] = (GLfloat) (-(zFar + zNear) / deltaZ);
    m[2][3] = -1.0f;
    m[3][2] = (GLfloat) (-2.0f * zNear * zFar / deltaZ);
    m[3][3] = 0.0f;

    glMultMatrixf(&m[0][0]);
}

void Core::setPixelScale(int pixelScaleX, int pixelScaleY)
{
	/*
	piScaleX = pixelScaleX;
	piScaleY = pixelScaleY;
	*/
	virtualWidth = pixelScaleX;
	//MAX(virtualWidth, 800);
	virtualHeight = pixelScaleY;//int((pixelScale*aspectY)/aspectX);					//assumes 4:3 aspect ratio
	this->baseCullRadius = sqrtf(sqr(getVirtualWidth()/2) + sqr(getVirtualHeight()/2));

	std::ostringstream os;
	os << "virtual(" << virtualWidth << ", " << virtualHeight << ")";
	debugLog(os.str());
	
	vw2 = virtualWidth/2;
	vh2 = virtualHeight/2;

	center = Vector(baseVirtualWidth/2, baseVirtualHeight/2);


	virtualOffX = 0;
	virtualOffY = 0;

	int diff = 0;

	diff = virtualWidth-baseVirtualWidth;
	if (diff > 0)
		virtualOffX = ((virtualWidth-baseVirtualWidth)/2);
	else
		virtualOffX = 0;


	diff = virtualHeight-baseVirtualHeight;
	if (diff > 0)
		virtualOffY = ((virtualHeight-baseVirtualHeight)/2);
	else
		virtualOffY = 0;
}

// forcePixelScale used by Celu

void Core::enable2DWide(int rx, int ry)
{
	float aspect = float(rx) / float(ry);
	if (aspect >= 1.3f)
	{
		int vw = int(float(baseVirtualHeight) * (float(rx)/float(ry)));
		//vw = MAX(vw, baseVirtualWidth);
		core->enable2D(vw, baseVirtualHeight, 1);
	}
	else
	{
		int vh = int(float(baseVirtualWidth) * (float(ry)/float(rx)));
		//vh = MAX(vh, baseVirtualHeight);
		core->enable2D(baseVirtualWidth, vh, 1);
	}
}

static void bbgeOrtho2D(float left, float right, float bottom, float top)
{
    glOrtho(left, right, bottom, top, -1.0, 1.0);
}

void Core::enable2D(int pixelScaleX, int pixelScaleY, bool forcePixelScale)
{
	// why do this again? don't really get it
	/*
	if (mode == MODE_2D)
	{
		if (forcePixelScale || (pixelScaleX!=0 && core->width!=pixelScaleX) || (pixelScaleY!=0 && core->height!=pixelScaleY))
		{
			float widthFactor = core->width/float(pixelScaleX);
			float heightFactor = core->height/float(pixelScaleY);
			core->globalResolutionScale = Vector(widthFactor,heightFactor,1.0f);
			setPixelScale(pixelScaleX, pixelScaleY);

			std::ostringstream os;
			os << "top of call: ";
			os << "widthFactor: " << widthFactor;
			os << "heightFactor: " << heightFactor;
			debugLog(os.str());
		}
		return;
	}
	*/
	
#ifdef BBGE_BUILD_OPENGL

    GLint viewPort[4];
    glGetIntegerv(GL_VIEWPORT, viewPort);

    glMatrixMode(GL_PROJECTION);
    //glPushMatrix();
    glLoadIdentity();

	float vw=0,vh=0;

	viewOffX = viewOffY = 0;

	float aspect = float(width)/float(height);

	if (aspect >= 1.4f)
	{
		vw = float(baseVirtualWidth * viewPort[3]) / float(baseVirtualHeight);

		viewOffX = (viewPort[2] - vw) * 0.5f;
	}
	else if (aspect < 1.3f)
	{
		vh = float(baseVirtualHeight * viewPort[2]) / float(baseVirtualWidth);

		viewOffY = (viewPort[3] - vh) * 0.5f;
	}



	/*
	vh = float(baseVirtualHeight * viewPort[2]) / float(baseVirtualWidth);

	viewOffY = (viewPort[3] - vh) * 0.5f;
	*/
	

	/*
	std::ostringstream os;
	os << "vw: " << vw << " OFFX: " << viewOffX << " ";
	os << "vh: " << vh << " OFFY: " << viewOffY;
	debugLog(os.str());
	*/


	/*
	float aspect = float(width) / float (height);

	if (aspect < 1.3f)
	{
		viewOffX *= 0.5f;
	}
	*/

	
//#else
//	int offx=0,offy=0;
//#endif

	//+offx
	//-offx
	//glOrtho(0.0f,viewPort[2],viewPort[3],0.0f,-1000.0f,1000.0f);
	//glOrtho(0.0f+offx,viewPort[2]+offx,viewPort[3]+offy,0.0f+offy,-1.0f,1.0f);
	bbgeOrtho2D(0.0f-viewOffX,viewPort[2]-viewOffX,viewPort[3]-viewOffY,0.0f-viewOffY);
	/*
	static bool doOnce = false;
	if (!doOnce)
	{
		glOrtho(0.0f,viewPort[2],viewPort[3],0.0f,-10.0f,10.0f);
		doOnce = true;
	}
	*/
	//glOrtho(-viewPort[2]/2,viewPort[2]/2,viewPort[3]/2,-viewPort[3]/2,-10.0f,10.0f);
    //glOrtho(0, viewPort[2], 0, viewPort[3], -100, 100);

    glMatrixMode(GL_MODELVIEW);
    //glPushMatrix();
    glLoadIdentity();

	setupRenderPositionAndScale();
#endif

#ifdef BBGE_BUILD_DIRECTX
	D3DXMATRIX matOrtho;
	D3DXMATRIX matIdentity;

	//Setup orthographic projection matrix

	D3DXMatrixOrthoOffCenterLH(&matOrtho, 0, getWindowWidth(), getWindowHeight(), 0, 1, 10);
	//D3DXMatrixOrthoLH (&matOrtho, getWindowWidth(), getWindowHeight(), 1.0f, 10.0f);
	D3DXMatrixIdentity (&matIdentity);
	g_pd3dDevice->SetTransform (D3DTS_PROJECTION, &matOrtho);
	g_pd3dDevice->SetTransform (D3DTS_WORLD, &matIdentity);
	g_pd3dDevice->SetTransform (D3DTS_VIEW, &matIdentity);
	// For our world matrix, we will just leave it as the identity
	/*
    D3DXMATRIXA16 matWorld;
    D3DXMatrixIdentity( &matWorld );
    //D3DXMatrixRotationX( &matWorld, 0/1000.0f );
    g_pd3dDevice->SetTransform( D3DTS_WORLD, &matWorld );

    // Set up our view matrix. A view matrix can be defined given an eye point,
    // a point to lookat, and a direction for which way is up. Here, we set the
    // eye five units back along the z-axis and up three units, look at the
    // origin, and define "up" to be in the y-direction.
    D3DXVECTOR3 vEyePt( 0.0f, 0.0f,0.0f );
    D3DXVECTOR3 vLookatPt( 0.0f, 0.0f, 0.0f );
    D3DXVECTOR3 vUpVec( 0.0f, 1.0f, 0.0f );
    D3DXMATRIXA16 matView;
    D3DXMatrixLookAtLH( &matView, &vEyePt, &vLookatPt, &vUpVec );
    g_pd3dDevice->SetTransform( D3DTS_VIEW, &matView );

    // For the projection matrix, we set up a perspective transform (which
    // transforms geometry from 3D view space to 2D viewport space, with
    // a perspective divide making objects smaller in the distance). To build
    // a perpsective transform, we need the field of view (1/4 pi is common),
    // the aspect ratio, and the near and far clipping planes (which define at
    // what distances geometry should be no longer be rendered).
	///LPDIRECT3DVIEWPORT3 Viewport;

	D3DVIEWPORT9 viewport;
	viewport.Width = core->getWindowWidth();
	viewport.Height = core->getWindowHeight();
	viewport.MaxZ = 5;
	viewport.MinZ = -5;
	viewport.X = 0;
	viewport.Y = 0;

	g_pd3dDevice->SetViewport( &viewport );

	D3DXMATRIXA16 matProj;
	D3DXMatrixOrthoLH(&matProj, 800, 600, -5, 5);
	g_pd3dDevice->SetTransform( D3DTS_PROJECTION, &matProj );
	*/

	// Create the viewport
	/*
	if (FAILED(g_pd3dDevice->CreateViewport(&Viewport,NULL)))
	{ errorLog("Failed to create a viewport"); };
	if (FAILED(g_pd3dDevice->AddViewport(Viewport)))
	{ errorLog("Failed to add a viewport"); };
	if (FAILED(g_pd3dDevice->SetViewport2(&Viewdata)))
	{ errorLog("Failed to set Viewport data"); };
	g_pd3dDevice->SetCurrentViewport(Viewport);
	*/

	/*
	D3DXMATRIXA16 matProj;
	D3DXMatrixOrthoLH(&matProj, 4, 3, -5, 5);
    g_pd3dDevice->SetTransform( D3DTS_PROJECTION, &matProj );
	*/

	/*
   D3DVIEWPORT9 viewport;
   g_pd3dDevice->GetViewport(&viewport);
   D3DXMATRIX matProj;
   D3DXMatrixOrthoLH(&matProj, viewport.Width, viewport.Height, -10, 10);
   g_pd3dDevice->SetTransform( D3DTS_PROJECTION, &matProj );
   */


#endif

	if (forcePixelScale || (pixelScaleX!=0 && core->width!=pixelScaleX) || (pixelScaleY!=0 && core->height!=pixelScaleY))
	{
		/*
		float f = core->width/float(pixelScale);
		core->globalResolutionScale = Vector(f,f,1.0f);
		*/
		//debugLog("HEEEREEE");
		float widthFactor = core->width/float(pixelScaleX);
		float heightFactor = core->height/float(pixelScaleY);
		//float heightFactor = 
		core->globalResolutionScale = Vector(widthFactor,heightFactor,1.0f);
		setPixelScale(pixelScaleX, pixelScaleY);

		//core->globalResolutionScale = Vector(1.5,1.5,1);
		/*
		std::ostringstream os;
		os << "bottom of call: ";
		os << "widthFactor: " << widthFactor;
		os << " heightFactor: " << heightFactor;
		debugLog(os.str());
		*/
	}
	setPixelScale(pixelScaleX, pixelScaleY);

	//core->globalResolutionScale.x = 1.6;

	//setupRenderPositionAndScale();

	
}

void Core::quitNestedMain()
{
	if (getNestedMains() > 1)
	{
		quitNestedMainFlag = true;
	}
}

void Core::resetTimer()
{
#ifdef BBGE_BUILD_GLFW
	glfwSetTime(0);
#endif
#ifdef BBGE_BUILD_SDL
	nowTicks = thenTicks = SDL_GetTicks();
#endif
#ifdef BBGE_BUILD_DIRECTX
	QueryPerformanceCounter((LARGE_INTEGER*)&timerEnd);
	timerStart = timerEnd;
#endif

	for (int i = 0; i < avgFPS.size(); i++)
	{
		avgFPS[i] = 0;
	}
}

void Core::setDockIcon(const std::string &ident)
{
}

void Core::setMousePosition(const Vector &p)
{
	Vector lp = core->mouse.position;

	core->mouse.position = p;
#if !defined(BBGE_BUILD_WINDOWS) && defined(BBGE_BUILD_GLFW)
	glfwSetMousePos(p.x,p.y);
#endif
#ifdef BBGE_BUILD_SDL
	float px = p.x + virtualOffX;
	float py = p.y;// + virtualOffY;

	SDL_WarpMouse( px * (float(width)/float(virtualWidth)), py * (float(height)/float(virtualHeight)));

	/*
	ignoreNextMouse = true;
	unchange = core->mouse.position - lp;
	*/
#endif

	/*
	std::ostringstream os;
	os << "setting position (" << p.x << ", " << p.y << ")";
	debugLog(os.str());
	*/
}

// used to update all render objects either uniformly or as part of a time sliced update process
void Core::updateRenderObjects(float dt)
{
	//HACK: we may not always be assuming virtual 800x600
	Vector cameraC = core->cameraPos + Vector(400,300);
	for (int c = 0; c < renderObjectLayers.size(); c++)
	{

		RenderObjectLayer *rl = &renderObjectLayers[c];

		if (!rl->update)
			continue;

		for (RenderObject *r = rl->getFirst(); r; r = rl->getNext())
		{
			r->update(dt);
		}
	}

	if (loopDone)
		return;

	if (clearedGarbageFlag)
	{
		clearedGarbageFlag = false;
	}
}

std::string Core::getEnqueuedJumpState()
{
	return this->enqueuedJumpState;
}

int screenshotNum = 0;
std::string getScreenshotFilename()
{
	while (true)
	{
		std::ostringstream os;
		os << core->getUserDataFolder() << "/screenshots/screen" << screenshotNum << ".tga";
		screenshotNum ++;
        std::string str(os.str());
		if (!core->exists(str))  // keep going until we hit an unused filename.
			return str;
	}
}

uint32 Core::getTicks()
{
#ifdef BBGE_BUILD_SDL
	return SDL_GetTicks();
#endif
	return 0;
}

float Core::stopWatch(int d)
{
	if (d)
	{
		stopWatchStartTime = getTicks()/1000.0f;
		return stopWatchStartTime;
	}
	else
	{
		return (getTicks()/1000.0f) - stopWatchStartTime;
	}

	return 0;
}

bool Core::isWindowFocus()
{
#ifdef BBGE_BUILD_SDL
	return ((SDL_GetAppState() & SDL_APPINPUTFOCUS) != 0);
#endif
	return true;
}

void Core::onBackgroundUpdate()
{
#if BBGE_BUILD_SDL
	SDL_Delay(200);
#endif
}

void Core::main(float runTime)
{
	bool verbose = coreVerboseDebug;
	if (verbose) debugLog("entered Core::main");
	// cannot nest loops when the game is over
	if (loopDone) return;

	//QueryPerformanceCounter((LARGE_INTEGER*)&lastTime);
	//QueryPerformanceFrequency((LARGE_INTEGER*)&freq);
	float dt;
	float counter = 0;
	int frames = 0;
	float real_dt = 0;
	//std::ofstream out("debug.log");

#if (!defined(_DEBUG) || defined(BBGE_BUILD_UNIX)) && defined(BBGE_BUILD_SDL)
	bool wasInactive = false;
#endif

#ifdef BBGE_BUILD_GLFW
	if (runTime == -1)
		glfwSetTime(0);
#endif
#ifdef BBGE_BUILD_DIRECTX
	// HACK: find out how to use performance counter again Query


	if (verbose) debugLog("Performance Counter");

	if (!QueryPerformanceFrequency((LARGE_INTEGER*)&freq))
	{
		errorLog ("could not get clock freq");
		return;
	}
	QueryPerformanceCounter((LARGE_INTEGER*)&timerStart);
	/*
	DWORD ticks = GetTickCount();
	DWORD newTicks;
	*/
#endif

#ifdef BBGE_BUILD_SDL
	nowTicks = thenTicks = SDL_GetTicks();
#endif

	//int i;

	nestedMains++;
	// HACK: Why block this?
	/*
	if (nestedMains > 1 && runTime <= 0)
		return;
	*/

#ifdef BBGE_BUILD_DIRECTX
	MSG msg;
	ZeroMemory( &msg, sizeof(msg) );
#endif

	while((runTime == -1 && !loopDone) || (runTime >0))									// Loop That Runs While done=FALSE
	{
		BBGE_PROF(Core_main);
#ifdef BBGE_BUILD_DIRECTX
		if( PeekMessage( &msg, NULL, 0U, 0U, PM_REMOVE ) )
		{
			TranslateMessage( &msg );
			DispatchMessage( &msg );
		}
#endif


#ifdef BBGE_BUILD_GLFW
		if (verbose) debugLog("glfwSetTime");
		dt = glfwGetTime();
		glfwSetTime(0);
#endif

#ifdef BBGE_BUILD_DIRECTX
		/*
		newTicks = GetTickCount();
		*/
		QueryPerformanceCounter((LARGE_INTEGER*)&timerEnd);
		dt = (float(timerEnd-timerStart)/float(freq));
		timerStart = timerEnd;
//		dt = float(newTicks)/1000.0f;
		//dt = float(newTicks - ticks)/1000.0f;
		//ticks = newTicks;
#endif

#ifdef BBGE_BUILD_SDL
		if (timeUpdateType == TIMEUPDATE_DYNAMIC)
		{
			nowTicks = SDL_GetTicks();
		}
		/*
		else
		{
			if (nowTicks == 0)
			{
				nowTicks = SDL_GetTicks();
			}
		}
		*/
		dt = (nowTicks-thenTicks)/1000.0;
		thenTicks = nowTicks;
		//thenTicks = SDL_GetTicks();
#endif

		if (verbose) debugLog("avgFPS");
		if (!avgFPS.empty())
		{
			/*
			if (avgFPS[0] <= 0)
			{
				for (int i = 0; i < avgFPS.size(); i++)
					avgFPS[i] = dt;
			}
			*/
			int i = 0;
			for (i = avgFPS.size()-1; i > 0; i--)
			{
				avgFPS[i] = avgFPS[i-1];
			}
			avgFPS[0] = dt;

			float c=0;
			int n = 0;
			for (i = 0; i < avgFPS.size(); i++)
			{
				if (avgFPS[i] > 0)
				{
					c += avgFPS[i];
					n ++;
				}
			}
			if (n > 0) // && n == avgFPS.size() ??
			{
				c /= n;
				dt = c;
			}
			/*
			std::ostringstream os;
			os << dt;
			debugLog(os.str());
			*/
		}

#if !defined(_DEBUG) && defined(BBGE_BUILD_SDL)
		if (verbose) debugLog("checking window active");

		if (lib_graphics && (wasInactive || !settings.runInBackground))
		{
			if (isWindowFocus())
			{
				_hasFocus = true;
				if (wasInactive)
				{
					debugLog("WINDOW ACTIVE");
					
					setReentryInputGrab(1);

					wasInactive = false;
				}
			}
			else
			{
				if (_hasFocus)
				{
					if (!wasInactive)
						debugLog("WINDOW INACTIVE");

					wasInactive = true;
					_hasFocus = false;

					setReentryInputGrab(0);

					sound->pause();

					core->joystick.rumble(0,0,0);

					while (!isWindowFocus())
					{
						pollEvents();
						//debugLog("app not in input focus");
						onBackgroundUpdate();

						resetTimer();
					}

					debugLog("app back in focus, reset");

					// Don't do this on Linux, it's not necessary and causes big stalls.
					//  We don't actually _lose_ the device like Direct3D anyhow.
					#ifndef BBGE_BUILD_UNIX
					if (_fullscreen)
					{
						// calls reload device - reloadDevice()
						resetGraphics(width, height);
					}
					#endif

					resetTimer();

					sound->resume();

					resetTimer();
					
					SDL_ShowCursor(SDL_DISABLE);

					continue;
				}
			}
		}
#endif

		if (timeUpdateType == TIMEUPDATE_FIXED)
		{
			real_dt = dt;
			dt = 1.0f/float(fixedFPS);
		}

		old_dt = dt;

		if (verbose) debugLog("modify dt");
		modifyDt(dt);

		current_dt = dt;

		if (verbose) debugLog("check runtime/quit");

		if (quitNestedMainFlag)
		{
			quitNestedMainFlag = false;
			break;
		}
		if (runTime>0)
		{
			runTime -= dt;
			if (runTime < 0)
				runTime = 0;
		}

		// UPDATE
		if (verbose) debugLog("post processing fx update");
		postProcessingFx.update(dt);

		if (verbose) debugLog("update eventQueue");
		eventQueue.update(dt);

		if (verbose) debugLog("Update render objects");

		updateRenderObjects(dt);

		if (verbose) debugLog("Update particle manager");

		if (particleManager)
			particleManager->update(dt);

		if (verbose) debugLog("sound update");
		sound->update(dt);

		if (verbose) debugLog("onUpdate");
		onUpdate(dt);

		if (nestedMains == 1)
			clearGarbage();

		if (loopDone)
			break;

		updateCullData();

		dbg_numRenderCalls = 0;

		if (settings.renderOn)
		{
			if (verbose) debugLog("dark layer prerender");
			if (darkLayer.isUsed())
			{
				darkLayer.preRender();
			}

			if (verbose) debugLog("render");
			render();

			if (verbose) debugLog("showBuffer");
			showBuffer();

			BBGE_PROF(STOP);

			if (verbose) debugLog("clearGarbage");
			if (nestedMains == 1)
				clearGarbage();


			if (verbose) debugLog("frame counter");
			frames++;

			counter += dt;
			if (counter > 1)
			{
				fps = frames;
				frames = counter = 0;
			}
		}

		if (doScreenshot)
		{
			if (verbose) debugLog("screenshot");

			doScreenshot = false;

			saveScreenshotTGA(getScreenshotFilename());
			prepScreen(0);
		}
		
		// wait
		if (timeUpdateType == TIMEUPDATE_FIXED)
		{
			static float avg_diff=0;
			static int avg_diff_count=0;

			float diff = (1.0f/float(fixedFPS)) - real_dt;

			avg_diff_count++;
			avg_diff += diff;
			
			char buf[256];
			sprintf(buf, "real_dt: %5.4f \n realFPS: %5.4f \n fixedFPS: %5.4f \n diff: %5.4f \n delay: %5.4f \n avgdiff: %5.8f", float(real_dt), float(real_dt>0?(1.0f/real_dt):0.0f), float(fixedFPS), float(diff), float(diff*1000), float(avg_diff/(float)avg_diff_count));
			fpsDebugString = buf;

			/*
			std::ostringstream os;
			os << "real_dt: " << real_dt << "\n realFPS: " << (1.0/real_dt) << "\n fixedFPS: " << fixedFPS << "\n diff: " << diff << "\n delay: " << diff*1000;
			fpsDebugString = os.str();
			*/

#ifdef BBGE_BUILD_SDL
			nowTicks = SDL_GetTicks();
			
			if (diff > 0)
			{
				//Sleep(diff*1000);
				//SDL_Delay(diff*1000);
				while ((SDL_GetTicks() - nowTicks) < (diff*1000))
				{
					//wend, bitch
				}
			}

			//nowTicks = SDL_GetTicks();
#endif

		}	
	}
	if (verbose) debugLog("bottom of function");
	quitNestedMainFlag = false;
	if (nestedMains==1)
		clearGarbage();
	nestedMains--;
	if (verbose) debugLog("exit Core::main");
}

// less than through pointer
bool RenderObject_lt(RenderObject* x, RenderObject* y)
{
	return x->getSortDepth() < y->getSortDepth();
}

// greater than through pointer
bool RenderObject_gt(RenderObject* x, RenderObject* y)
{
	return x->getSortDepth() > y->getSortDepth();
}

void Core::sortLayer(int layer)
{
	if (layer >= 0 && layer < renderObjectLayers.size())
		renderObjectLayers[layer].sort();
}

void Core::sort()
{
	/*
	if (sortEnabled)
		renderObjects.sort(RenderObject_lt);
	*/
	// sort layeres independantly

	/*
	for (int i = renderObjects.size()-1; i >= 0; i--)
	{
		bool flipped = false;
		for (int j = 0; j < i; j++)
		{
			//position.z
			//position.z
			//!renderObjects[j]->parent && !renderObjects[j+1]->parent &&
			if (renderObjects[j]->getSortDepth() > renderObjects[j+1]->getSortDepth())
			{
				RenderObject *temp;
				temp = renderObjects[j];
				renderObjects[j] = renderObjects[j+1];
				renderObjects[j+1] = temp;
				flipped = true;
			}
		}
		if (!flipped) break;
	}
	*/

}

void Core::clearBuffers()
{
	if (flags.get(CF_CLEARBUFFERS))
	{
#ifdef BBGE_BUILD_OPENGL
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);	// Clear The Screen And The Depth Buffer
#endif
#ifdef BBGE_BUILD_DIRECTX
		g_pd3dDevice->Clear( 0, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(int(clearColor.x*255),int(clearColor.y*255),int(clearColor.z*255)), 1.0f, 0 );
#endif
	}
}

void Core::setupRenderPositionAndScale()
{
#ifdef BBGE_BUILD_OPENGL
	glScalef(globalScale.x*globalResolutionScale.x*screenCapScale.x, globalScale.y*globalResolutionScale.y*screenCapScale.y, globalScale.z*globalResolutionScale.z);
	glTranslatef(-(cameraPos.x+cameraOffset.x), -(cameraPos.y+cameraOffset.y), -(cameraPos.z+cameraOffset.z));
#endif
}

void Core::setupGlobalResolutionScale()
{
	glScalef(globalResolutionScale.x, globalResolutionScale.y, globalResolutionScale.z);
}

void Core::initFrameBuffer()
{
	frameBuffer.init(-1, -1, true);
}

void Core::setMouseConstraint(bool on)
{
/*
	if (mouseConstraint && !on)
	{
		setMousePosition(mouse.position);
	}
	*/
	mouseConstraint = on;
}

void Core::setMouseConstraintCircle(int circle)
{
	mouseConstraint = true;
	mouseCircle = circle;
}

/*
void Core::clearKeys()
{
	for (int i = 0; i < KEY_MAXARRAY; i++)
	{
		keys[i] = 0;
	}
}
*/

int Core::getVirtualOffX()
{
	return virtualOffX;
}

int Core::getVirtualOffY()
{
	return virtualOffY;
}

void Core::centerMouse()
{
	setMousePosition(Vector((virtualWidth/2) - core->getVirtualOffX(), virtualHeight/2));
}

bool Core::doMouseConstraint()
{
	if (mouseConstraint)
	{
		//- core->getVirtualOffX()
		//- virtualOffX
		Vector h = Vector(core->center.x , core->center.y);
		Vector d = mouse.position - h;
		if (!d.isLength2DIn(mouseCircle))
		{
			d.setLength2D(mouseCircle);
			mouse.position = h+d;
			//warpMouse = true;
			return true;
		}
	}
	return false;
}

void Core::pollEvents()
{
#if defined(BBGE_BUILD_SDL)
	bool warpMouse=false;

	/*
	Uint8 *keystate = SDL_GetKeyState(NULL);
	for (int i = 0; i < KEY_MAXARRAY; i++)
	{
		keys[i] = keystate[i];
	}
	*/

	if (updateMouse)
	{
		int x, y;
		Uint8 mousestate = SDL_GetMouseState(&x,&y);

		if (mouse.buttonsEnabled)
		{
			mouse.buttons.left		= mousestate & SDL_BUTTON(1)?DOWN:UP;
			mouse.buttons.right		= mousestate & SDL_BUTTON(3)?DOWN:UP;
			mouse.buttons.middle	= mousestate & SDL_BUTTON(2)?DOWN:UP;

			mouse.pure_buttons = mouse.buttons;

			if (flipMouseButtons)
			{
				std::swap(mouse.buttons.left, mouse.buttons.right);
			}
		}
		else
		{
			mouse.buttons.left = mouse.buttons.right = mouse.buttons.middle = UP;
		}

		mouse.scrollWheelChange = 0;
		mouse.change = Vector(0,0);
	}





	SDL_Event event;

	

	while ( SDL_PollEvent (&event) ) {
		switch (event.type) {
			case SDL_KEYDOWN:
			{
				#if __APPLE__
				if ((event.key.keysym.sym == SDLK_q) && (event.key.keysym.mod & KMOD_META))
				#else
				if ((event.key.keysym.sym == SDLK_F4) && (event.key.keysym.mod & KMOD_ALT))
				#endif
				{
					quitNestedMain();
					quit();
				}

				if ((event.key.keysym.sym == SDLK_g) && (event.key.keysym.mod & KMOD_CTRL))
				{
					// toggle mouse grab with the magic hotkey.
					grabInputOnReentry = (grabInputOnReentry)?0:-1;
					setReentryInputGrab(1);
				}
				else if (_hasFocus)
				{
					int k = (int)event.key.keysym.sym;
					keys[k] = 1;
				}
			}
			break;

			case SDL_KEYUP:
			{
				if (_hasFocus)
				{
					int k = (int)event.key.keysym.sym;
					keys[k] = 0;
				}
			}
			break;

			case SDL_MOUSEMOTION:
			{
				if (_hasFocus && updateMouse)
				{
					mouse.lastPosition = mouse.position;

					mouse.position.x = ((event.motion.x) * (float(virtualWidth)/float(getWindowWidth()))) - getVirtualOffX();
					mouse.position.y = event.motion.y * (float(virtualHeight)/float(getWindowHeight()));

					mouse.change = mouse.position - mouse.lastPosition;

					if (doMouseConstraint()) warpMouse = true;
				}
			}
			break;

			case SDL_MOUSEBUTTONDOWN:
			{
				if (_hasFocus && updateMouse)
				{
					switch(event.button.button)
					{
					case 4:
						mouse.scrollWheelChange = 1;
					break;
					case 5:
						mouse.scrollWheelChange = -1;
					break;
					}
				}
			}
			break;

			case SDL_MOUSEBUTTONUP:
			{
				if (_hasFocus && updateMouse)
				{
					switch(event.button.button)
					{
					case 4:
						mouse.scrollWheelChange = 1;
					break;
					case 5:
						mouse.scrollWheelChange = -1;
					break;
					}
				}
			}
			break;

			case SDL_QUIT:
				SDL_Quit();
				_exit(0);
				//loopDone = true;
				//quit();
			break;

			case SDL_SYSWMEVENT:
			{
				/*
				debugLog("SYSWM!");
				if (event.syswm.type == WM_ACTIVATE)
				{
					debugLog("ACTIVE");
					this->unloadDevice();
					this->reloadDevice();
				}
				else
				{
					debugLog("NOT ACTIVE");
					this->unloadDevice();
				}
				*/
			}
			break;

			default:
			break;
		}
	}

	if (updateMouse)
	{
		mouse.scrollWheel += mouse.scrollWheelChange;

		if (warpMouse)
		{
			setMousePosition(mouse.position);
		}
	}

#endif
}

#define _VLN(x, y, x2, y2) glVertex2f(x, y); glVertex2f(x2, y2);

void Core::print(int x, int y, const char *str, float sz)
{
	//Prof(Core_print);
	/*
	glLoadIdentity();
	core->setupRenderPositionAndScale();
	*/
	///glPushAttrib(GL_ALL_ATTRIB_BITS);

#ifdef BBGE_BUILD_OPENGL
	glBindTexture(GL_TEXTURE_2D, 0);

	glPushMatrix();
	//sz *= 8;
	//float osz = sz;
	float xx = x;
	float yy = y;
	glTranslatef(x, y-0.5f*sz, 0);
	x = y = 0;
	xx = 0; yy = 0;
	bool isLower = false, wasLower = false;
	int c=0;

	/*
	if (a == 1)
		glDisable(GL_BLEND);
	else
		glEnable(GL_BLEND);
	glColor4f(r,g,b,a);
	*/
	glLineWidth(1);
	glScalef(sz*0.75f, sz, 1);

	glBegin(GL_LINES);

	while (str[c] != '\0')
	{
		if (str[c] <= 'z' && str[c] >= 'a')
			isLower = true;
		else
			isLower = false;

		/*
		if (isLower)
			glScalef(sz*0.5f, sz*0.5f, 1);
		else if (wasLower)
		{
			glScalef(sz, sz, 1);
			wasLower = false;
		}
		*/

		switch(toupper(str[c]))
		{
		case '_':
			_VLN(xx, y+1, xx+1, y+1)
		break;
		case '-':
			_VLN(xx, y+0.5f, xx+1, y+0.5f)
		break;
		case '~':
			_VLN(xx, y+0.5f, xx+0.25f, y+0.4f)
			_VLN(xx+0.25f, y+0.4f, xx+0.75f, y+0.6f)
			_VLN(xx+0.75f, y+0.6f, xx+1, y+0.5f)
		break;
		case 'A':
			_VLN(xx, y, xx+1, y)
			_VLN(xx+1, y, xx+1, y+1)
			_VLN(xx, y, xx, y+1)
			_VLN(xx, y+0.5f, xx+1, y+0.5f)
		break;
		case 'B':
			_VLN(xx, y, xx+1, y)
			_VLN(xx+1, y, xx+1, y+1)
			_VLN(xx, y, xx, y+1)
			_VLN(xx, y+0.5f, xx+1, y+0.5f)
			_VLN(xx, y+1, xx+1, y+1)
		break;
		case 'C':
			_VLN(xx, y, xx+1, y)
			_VLN(xx, y, xx, y+1)
			_VLN(xx, y+1, xx+1, y+1)
		break;
		case 'D':
			_VLN(xx, y, xx+1, y+0.2f)
			_VLN(xx, y, xx, y+1)
			_VLN(xx, y+1, xx+1, y+1)
			_VLN(xx+1, y+0.2f, xx+1, y+1)
		break;
		case 'E':
			_VLN(xx, y, xx+1, y)
			_VLN(xx, y, xx, y+1)
			_VLN(xx, y+0.5f, xx+1, y+0.5f)
			_VLN(xx, y+1, xx+1, y+1)
		break;
		case 'F':
			_VLN(xx, y, xx+1, y)
			_VLN(xx, y, xx, y+1)
			_VLN(xx, y+0.5f, xx+1, y+0.5f)
		break;
		case 'G':
			_VLN(xx, y, xx+1, y)
			_VLN(xx, y, xx, y+1)
			_VLN(xx, y+1, xx+1, y+1)
			_VLN(xx+1, y+0.5f, xx+1, y+1)
		break;
		case 'H':
			_VLN(xx, y, xx, y+1)
			_VLN(xx, y+0.5f, xx+1, y+0.5f)
			_VLN(xx+1, y, xx+1, y+1)
		break;
		case 'I':
			_VLN(xx+0.5f, y, xx+0.5f, y+1)
			_VLN(xx, y, xx+1, y)
			_VLN(xx, y+1, xx+1, y+1)
		break;
		case 'J':
			_VLN(xx+1, y, xx+1, y+1)
			_VLN(xx, y, xx+1, y)
			_VLN(xx, y+1, xx+1, y+1)
			_VLN(xx, y+1, xx, y+0.75f)
		break;
		case 'K':
			_VLN(xx, y, xx, y+1)
			_VLN(xx, y+0.25f, xx+1, y)
			_VLN(xx, y+0.25f, xx+1, y+1)
		break;
		case 'L':
			_VLN(xx, y, xx, y+1)
			_VLN(xx, y+1, xx+1, y+1)
		break;
		case 'M':
			_VLN(xx, y, xx, y+1)
			_VLN(xx+1, y, xx+1, y+1)
			_VLN(xx, y, xx+0.5f, y+0.5f)
			_VLN(xx+1, y, xx+0.5f, y+0.5f)
		break;
		case 'N':
			_VLN(xx, y, xx, y+1)
			_VLN(xx+1, y, xx+1, y+1)
			_VLN(xx, y, xx+1, y+1)
		break;
		case 'O':
			_VLN(xx, y, xx, y+1)
			_VLN(xx+1, y, xx+1, y+1)
			_VLN(xx, y+1, xx+1, y+1)
			_VLN(xx, y, xx+1, y)
		break;
		case 'P':
			_VLN(xx, y, xx+1, y)
			_VLN(xx, y, xx, y+1)
			_VLN(xx, y+0.5f, xx+1, y+0.5f)
			_VLN(xx+1, y+0.5f, xx+1, y)
		break;
		case 'Q':
			_VLN(xx, y, xx, y+1)
			_VLN(xx+1, y, xx+1, y+1)
			_VLN(xx, y+1, xx+1, y+1)
			_VLN(xx, y, xx+1, y)
			_VLN(xx, y+0.5f, xx+1.25f, y+1.25f)
		break;
		case 'R':
			_VLN(xx, y, xx+1, y)
			_VLN(xx, y, xx, y+1)
			_VLN(xx, y+0.5f, xx+1, y+0.5f)
			_VLN(xx+1, y+0.5f, xx+1, y)
			_VLN(xx, y+0.5f, xx+1, y+1)
		break;
		case 'S':
			_VLN(xx, y, xx+1, y)
			_VLN(xx, y, xx, y+0.5f)
			_VLN(xx, y+0.5f, xx+1, y+0.5f)
			_VLN(xx+1, y+0.5f, xx+1, y+1)
			_VLN(xx, y+1, xx+1, y+1)
		break;
		case 'T':
			_VLN(xx, y, xx+1, y)
			_VLN(xx+0.5f, y, xx+0.5f, y+1)
		break;
		case 'U':
			_VLN(xx, y+1, xx+1, y+1)
			_VLN(xx, y, xx, y+1)
			_VLN(xx+1, y, xx+1, y+1)
		break;
		case 'V':
			_VLN(xx, y, xx+0.5f, y+1)
			_VLN(xx+1, y, xx+0.5f, y+1)
		break;
		case 'W':
			_VLN(xx, y, xx+0.25f, y+1)
			_VLN(xx+0.25f, y+1, xx+0.5f, y+0.5f)
			_VLN(xx+0.5f, y+0.5f, xx+0.75f, y+1)
			_VLN(xx+1, y, xx+0.75f, y+1)
		break;
		case 'X':
			_VLN(xx, y, xx+1, y+1)
			_VLN(xx+1, y, xx, y+1)
		break;
		case 'Y':
			_VLN(xx, y, xx+0.5f, y+0.5f)
			_VLN(xx+1, y, xx+0.5f, y+0.5f)
			_VLN(xx+0.5f, y+0.5f, xx+0.5f, y+1)
		break;
		case 'Z':
			_VLN(xx, y, xx+1, y)
			_VLN(xx, y+1, xx+1, y)
			_VLN(xx, y+1, xx+1, y+1)
		break;

		case '1':
			_VLN(xx+0.5f, y, xx+0.5f, y+1)
			_VLN(xx, y+1, xx+1, y+1)
			_VLN(xx+0.5f, y, xx+0.25f, y+0.25f)
		break;
		case '2':
			_VLN(xx, y, xx+1, y)
			_VLN(xx+1, y, xx+1, y+0.5f)
			_VLN(xx+1, y+0.5f, xx, y+0.5f)
			_VLN(xx, y+0.5f, xx, y+1)
			_VLN(xx, y+1, xx+1, y+1)
		break;
		case '3':
			_VLN(xx, y, xx+1, y)
			_VLN(xx, y+1, xx+1, y+1)
			_VLN(xx, y+0.5f, xx+1, y+0.5f)
			_VLN(xx+1, y, xx+1, y+1)
		break;
		case '4':
			_VLN(xx+1, y, xx+1, y+1)
			_VLN(xx+1, y, xx, y+0.5f)
			_VLN(xx, y+0.5f, xx+1, y+0.5f)
		break;
		case '5':
			_VLN(xx, y, xx+1, y)
			_VLN(xx, y, xx, y+0.5f)
			_VLN(xx+1, y+0.5f, xx, y+0.5f)
			_VLN(xx+1, y+0.5f, xx+1, y+1)
			_VLN(xx, y+1, xx+1, y+1)
		break;
		case '6':
			_VLN(xx, y, xx+1, y)
			_VLN(xx, y, xx, y+1)
			_VLN(xx+1, y+0.5f, xx, y+0.5f)
			_VLN(xx+1, y+0.5f, xx+1, y+1)
			_VLN(xx, y+1, xx+1, y+1)
		break;
		case '7':
			_VLN(xx+1, y, xx+0.5f, y+1)
			_VLN(xx, y, xx+1, y)
		break;
		case '8':
			_VLN(xx, y, xx+1, y)
			_VLN(xx+1, y, xx+1, y+1)
			_VLN(xx, y, xx, y+1)
			_VLN(xx, y+0.5f, xx+1, y+0.5f)
			_VLN(xx, y+1, xx+1, y+1)
		break;
		case '9':
			_VLN(xx, y, xx+1, y)
			_VLN(xx+1, y, xx+1, y+1)
			_VLN(xx, y+0.5f, xx+1, y+0.5f)
			_VLN(xx, y+0.5f, xx, y)
		break;
		case '0':
			_VLN(xx, y, xx, y+1)
			_VLN(xx+1, y, xx+1, y+1)
			_VLN(xx, y+1, xx+1, y+1)
			_VLN(xx, y, xx+1, y)
			_VLN(xx, y, xx+1, y+1)
		break;
		case '.':
			_VLN(xx+0.4f, y+1, xx+0.6f, y+1)
		break;
		case ',':
			_VLN(xx+0.5f, y+0.75f, xx+0.5f, y+1.0f);
			_VLN(xx+0.5f, y+1.0f, xx+0.2f, y+1.25f);
		break;
		case ' ':
		break;
		case '(':
		case '[':
			_VLN(xx, y, xx, y+1);
			_VLN(xx, y, xx+0.25f, y);
			_VLN(xx, y+1, xx+0.25f, y+1);
		break;
		case ')':
		case ']':
			_VLN(xx+1, y, xx+1, y+1);
			_VLN(xx+1, y, xx+0.75f, y);
			_VLN(xx+1, y+1, xx+0.75f, y+1);
		break;
		case ':':
			_VLN(xx+0.5f, y, xx+0.5f, y+0.25f);
			_VLN(xx+0.5f, y+0.75f, xx+0.5f, y+1);
		break;
		case '/':
			_VLN(xx, y+1, xx+1, y);
		break;
		default:
			/*
			std::ostringstream os;
			os << "Core::print doesn't know char: " << str[c];
			debugLog(os.str());
			*/
		break;
		}
		if (isLower)
		{
			wasLower = true;

		}
		c++;
		xx += 1.4f;
	}
	glEnd();

	glPopMatrix();
	//glPopAttrib();

#endif
}

void Core::cacheRender()
{
	render();
	// what if the screen was full white? then you wouldn't want to clear buffers
	//clearBuffers();
	showBuffer();
	resetTimer();
}

void Core::updateCullData()
{
	// update cull data
	//this->cullRadius = int((getVirtualWidth())*invGlobalScale);
	this->cullRadius = baseCullRadius * invGlobalScale;
	this->cullRadiusSqr = (float)this->cullRadius * (float)this->cullRadius;
	this->cullCenter = cameraPos + Vector(400.0f*invGlobalScale,300.0f*invGlobalScale);
	screenCullX1 = cameraPos.x;
	screenCullX2 = cameraPos.x + 800*invGlobalScale;
	screenCullY1 = cameraPos.y;
	screenCullY2 = cameraPos.y + 600*invGlobalScale;

	
	int cx = core->cameraPos.x + 400*invGlobalScale;
	int cy = core->cameraPos.y + 300*invGlobalScale;
	screenCenter = Vector(cx, cy);
}

void Core::render(int startLayer, int endLayer, bool useFrameBufferIfAvail)
{

	BBGE_PROF(Core_render);
	//HWND hwnd = _glfwWin.Wnd;

	if (startLayer == -1 && endLayer == -1 && overrideStartLayer != 0)
	{
		startLayer = overrideStartLayer;
		endLayer = overrideEndLayer;
	}

	if (core->minimized) return;
	onRender();

	invGlobalScale = 1.0f/globalScale.x;
	invGlobalScaleSqr = invGlobalScale * invGlobalScale;

	RenderObject::lastTextureApplied = 0;

	updateCullData();



	renderObjectCount = 0;
	processedRenderObjectCount = 0;
	totalRenderObjectCount = 0;


#ifdef BBGE_BUILD_OPENGL
	glBindTexture(GL_TEXTURE_2D, 0);
	glLoadIdentity();									// Reset The View
	clearBuffers();

	if (afterEffectManager && frameBuffer.isInited() && useFrameBufferIfAvail)
	{
		frameBuffer.startCapture();
	}

	setupRenderPositionAndScale();
#endif

#ifdef BBGE_BUILD_DIRECTX
	bool doRender = false;

	core->getD3DMatrixStack()->LoadIdentity();


	core->scaleMatrixStack(globalScale.x*globalResolutionScale.x, globalScale.y*globalResolutionScale.y);
	core->translateMatrixStack(-(cameraPos.x+cameraOffset.x), -(cameraPos.y+cameraOffset.y));

	clearBuffers();
	if( SUCCEEDED( g_pd3dDevice->BeginScene() ) )
    {
		doRender = true;
		//d3dSprite->Begin(D3DXSPRITE_BILLBOARD | D3DXSPRITE_ALPHABLEND);
    }

#endif


	/*
	//default
	if (renderObjectLayerOrder.empty())
	{
		renderObjectLayerOrder.resize(renderObjectLayers.size());
		for (int i = 0; i < renderObjectLayerOrder.size(); i++)
		{
			renderObjectLayerOrder[i] = i;
		}
	}
	*/
	RenderObject::rlayer = 0;

	for (int c = 0; c < renderObjectLayerOrder.size(); c++)
	//for (int i = 0; i < renderObjectLayers.size(); i++)
	{
		int i = renderObjectLayerOrder[c];
		if (i == -1) continue;
		if ((startLayer != -1 && endLayer != -1) && (i < startLayer || i > endLayer)) continue;

		if (i == postProcessingFx.layer)
		{
			postProcessingFx.preRender();
		}
		if (i == postProcessingFx.renderLayer)
		{
			postProcessingFx.render();
		}

		if (darkLayer.isUsed() )
		{
			/*
			if (i == darkLayer.getLayer())
			{
				darkLayer.preRender();
			}
			*/
			if (i == darkLayer.getRenderLayer())
			{
				darkLayer.render();
			}

			if (i == darkLayer.getLayer() && startLayer != i)
			{
				continue;
			}
		}

		if (afterEffectManager && afterEffectManager->active && i == afterEffectManagerLayer)
		{
			afterEffectManager->render();
		}

		RenderObjectLayer *r = &renderObjectLayers[i];
		RenderObject::rlayer = r;
		if (r->visible)
		{
			if (r->startPass == r->endPass)
			{
				r->renderPass(RenderObject::RENDER_ALL);
			}
			else
			{
				for (int pass = r->startPass; pass <= r->endPass; pass++)
				{
					r->renderPass(pass);
				}
			}
		}
	}

#ifdef BBGE_BUILD_DIRECTX
	if (doRender)
	{
		// End the scene
		//d3dSprite->End();
		//core->getD3DMatrixStack()->Pop();
		g_pd3dDevice->EndScene();
	}
#endif

}

void Core::showBuffer()
{
	BBGE_PROF(Core_showBuffer);
#ifdef BBGE_BUILD_SDL
	SDL_GL_SwapBuffers();
	//glFlush();
#endif
#ifdef BBGE_BUILD_GLFW
	glfwSwapBuffers();
	//_glfwPlatSwapBuffers();
#endif
#ifdef BBGE_BUILD_DIRECTX
	// Present the backbuffer contents to the display
    g_pd3dDevice->Present( NULL, NULL, NULL, NULL );
#endif
}

// WARNING: only for use during shutdown
// otherwise, textures will try to remove themselves
// when destroy is called on them
void Core::clearResources()
{
	std::vector<Resource*> deletedResources;
	int i;
	for (i = 0; i < resources.size(); i++)
	{
		int j = 0;
		for (j = 0; j < deletedResources.size(); j++)
		{
			if (deletedResources[j] == resources[i])
				break;
		}
		if (j == deletedResources.size())
		{
			deletedResources.push_back (resources[i]);
			Resource *r = resources[i];
			r->destroy();
			delete r;
		}
	}
	resourceNames.clear();
	resources.clear();
}

void Core::shutdownInputLibrary()
{
#if defined(BBGE_BUILD_WINDOWS) && !defined(BBGE_BUILD_SDL)
	g_pKeyboard->Unacquire();
	g_pKeyboard->Release();
	g_pKeyboard = 0;
	g_pMouse->Unacquire();
	g_pMouse->Release();
	g_pMouse = 0;
#endif
}

void Core::shutdownJoystickLibrary()
{
	if (joystickEnabled) {
		joystick.shutdown();
#ifdef BBGE_BUIDL_SDL
		SDL_QuitSubSystem(SDL_INIT_JOYSTICK);
#endif
		joystickEnabled = false;
	}
}

void Core::clearRenderObjects()
{
	for (int i = 0; i < renderObjectLayers.size(); i++)
	{
		/*
		for (int j = 0; j < renderObjectLayers[i].renderObjects.size(); j++)
		{
			RenderObject *r = renderObjectLayers[i].renderObjects[j];
		*/
		RenderObject *r = renderObjectLayers[i].getFirst();
		while (r)
		{
			if (r)
			{
				removeRenderObject(r, DESTROY_RENDER_OBJECT);
			}
			r = renderObjectLayers[i].getNext();
		}
	}
}

void Core::shutdown()
{
	// pop all the states


	debugLog("Core::shutdown");
	shuttingDown = true;

	debugLog("Shutdown Joystick Library...");
		shutdownJoystickLibrary();
	debugLog("OK");

	debugLog("Shutdown Input Library...");
		shutdownInputLibrary();
	debugLog("OK");

	debugLog("Shutdown All States...");
		popAllStates();
	debugLog("OK");

	debugLog("Clear State Instances...");
		clearStateInstances();
	debugLog("OK");

	debugLog("Clear All Remaining RenderObjects...");
		clearRenderObjects();
	debugLog("OK");

	debugLog("Clear All Resources...");
		clearResources();
	debugLog("OK");


	debugLog("Clear State Objects...");
		clearStateObjects();
	debugLog("OK");

	if (afterEffectManager)
	{
		debugLog("Delete AEManager...");
			delete afterEffectManager;
			afterEffectManager = 0;
		debugLog("OK");
	}


	if (sound)
	{
		debugLog("Shutdown Sound Library...");
			sound->stopAll();
			delete sound;
			sound = 0;
		debugLog("OK");
	}

	debugLog("Core's framebuffer...");
		frameBuffer.unloadDevice();
	debugLog("OK");

	debugLog("Shutdown Graphics Library...");
		shutdownGraphicsLibrary();
	debugLog("OK");



#ifdef BBGE_BUILD_GLFW
	debugLog("Terminate GLFW...");
		//killGlWindow();
		glfwTerminate();
	debugLog("OK");
#endif

#ifdef BBGE_BUILD_VFS
	debugLog("Unload VFS...");
		vfs.Clear();
	debugLog("OK");
#endif


#ifdef BBGE_BUILD_SDL
	debugLog("SDL Quit...");
		SDL_Quit();
	debugLog("OK");
#endif
}

//util funcs

void Core::instantQuit()
{
#ifdef BBGE_BUILD_SDL
    SDL_Event event;
    event.type = SDL_QUIT;
    SDL_PushEvent(&event);
#endif
}

bool Core::exists(const std::string &filename)
{
	return ::exists(filename, false); // defined in Base.cpp
}

Resource* Core::findResource(const std::string &name)
{
	for (int i = 0; i < resources.size(); i++)
	{
		if (resources[i]->name == name)
		{
			return resources[i];
		}
	}
	return 0;
}


Texture* Core::findTexture(const std::string &name)
{
	//stringToUpper(name);
	//std::ofstream out("texturefind.log");
	int sz = resources.size();
	for (int i = 0; i < sz; i++)
	{
		//out << resources[i]->name << " is " << name << " ?" << std::endl;
		//NOTE: ensure all names are lowercase before this point
		if (resources[i]->name == name)
		{
			return (Texture*)resources[i];
		}
	}
	return 0;
}

std::string Core::getInternalTextureName(const std::string &name)
{
	std::string n = name;
	stringToUpper(n);
	return n;
}

// This handles unix/win32 relative paths: ./rel/path
// Unix abs paths: /home/user/...
// Win32 abs paths: C:/Stuff/.. and also C:\Stuff\...
#define ISPATHROOT(x) (x[0] == '.' || x[0] == '/' || ((x).length() > 1 && x[1] == ':'))

std::string Core::getTextureLoadName(const std::string &texture)
{
	std::string loadName = texture;

	if (texture.empty() || !ISPATHROOT(texture))
	{
		if (texture.find(baseTextureDirectory) == std::string::npos)
			loadName = baseTextureDirectory + texture;
	}
	return loadName;
}

Texture *Core::doTextureAdd(const std::string &texture, const std::string &loadName, std::string internalTextureName)
{
	if (texture.empty() || !ISPATHROOT(texture))
	{
		if (texture.find(baseTextureDirectory) != std::string::npos)
			internalTextureName = internalTextureName.substr(baseTextureDirectory.size(), internalTextureName.size());
	}

	if (internalTextureName.size() > 4)
	{
		if (internalTextureName[internalTextureName.size()-4] == '.')
		{
			internalTextureName = internalTextureName.substr(0, internalTextureName.size()-4);
		}
	}

	stringToLowerUserData(internalTextureName);
	Texture *t = core->findTexture(internalTextureName);
	if (t)
	{
		t->addRef();

		Texture::textureError = TEXERR_OK;

		/*
		std::ostringstream os;
		os << "reference texture: " << internalTextureName << " ref: " << t->getRef();
		debugLog(os.str());
		*/

		//msg ("found texture " + internalTextureName);
		return t;
	}

	t = new Texture;
	t->name = internalTextureName;
	t->load(loadName);
	t->addRef();
	//resources.push_back (t);
	addResource(t);

	if (debugLogTextures)
	{
		std::ostringstream os;
		os << "LOADED TEXTURE FROM DISK: [" << internalTextureName << "] ref: " << t->getRef() << " idx: " << resources.size()-1;
		debugLog(os.str());
	}

	return t;
}

Texture* Core::addTexture(const std::string &textureName)
{
	if (textureName.empty()) return 0;

	BBGE_PROF(Core_addTexture);

	Texture *texPointer = 0;

	std::string texture = textureName;
	stringToLowerUserData(texture);
	std::string internalTextureName = texture;
	std::string loadName = getTextureLoadName(texture);

	if (!texture.empty() && texture[0] == '@')
	{
		texture = secondaryTexturePath + texture.substr(1, texture.size());
		loadName = texture;
	}
	else if (!secondaryTexturePath.empty() && texture[0] != '.' && texture[0] != '/')
	{
		std::string t = texture;
		std::string ln = loadName;
		texture = secondaryTexturePath + texture;
		loadName = texture;
		texPointer = doTextureAdd(texture, loadName, internalTextureName);
		if (Texture::textureError != TEXERR_OK)
		{
			if (texPointer)
			{
				texPointer->destroy();
				texPointer = 0;
			}
			texPointer = doTextureAdd(t, ln, internalTextureName);
		}
	}
	else
		texPointer = doTextureAdd(texture, loadName, internalTextureName);

	return texPointer;
}

void Core::removeTexture(std::string texture)
{
	//std::string internalName = baseTextureDirectory + texture;
	removeResource(texture, DESTROY);
}


void Core::addRenderObject(RenderObject *o, int layer)
{
	if (!o) return;
	o->layer = layer;
	if (layer < 0 || layer >= renderObjectLayers.size())
	{
		std::ostringstream os;
		os << "attempted to add render object to invalid layer [" << layer << "]";
		errorLog(os.str());
	}
	renderObjectLayers[layer].add(o);
}

void Core::switchRenderObjectLayer(RenderObject *o, int toLayer)
{
	if (!o) return;
	renderObjectLayers[o->layer].remove(o);
	renderObjectLayers[toLayer].add(o);
	o->layer = toLayer;
}

void Core::unloadResources()
{
	for (int i = 0; i < resources.size(); i++)
	{
		resources[i]->unload();
	}
}

void Core::onReloadResources()
{
}

void Core::reloadResources()
{
	for (int i = 0; i < resources.size(); i++)
	{
		resources[i]->reload();
	}
	onReloadResources();
}

void Core::addResource(Resource *r)
{
	resources.push_back(r);
	resourceNames.push_back(r->name);
	if (r->name.empty())
	{
		debugLog("Empty name resource added");
	}
}

void Core::removeResource(std::string name, RemoveResource removeFlag)
{
	//Resource *r = findResource(name);
	//int idx = 0;
	int i = 0;
	std::vector<Resource*>copy;
	copy = resources;
	resources.clear();


	std::vector <std::string> copyNames;
	copyNames = resourceNames;
	resourceNames.clear();

	bool isDestroyed = false;


	for (i = 0; i < copy.size(); i++)
	{
#ifdef _DEBUG
			std::string s = copy[i]->name;
#endif
		if (!isDestroyed && copy[i]->name == name)
		{
			if (removeFlag == DESTROY)
			{
				copy[i]->destroy();
				delete copy[i];
				isDestroyed = true;
			}
			continue;
		}
		// also remove other entries of the same resource
		else if (isDestroyed && copyNames[i] == name)
		{
			continue;
		}
		else
		{
			resources.push_back(copy[i]);
			resourceNames.push_back(copy[i]->name);
		}
	}
}

void Core::deleteRenderObjectMemory(RenderObject *r)
{
	//if (!r->allocStatic)
	delete r;
}

void Core::removeRenderObject(RenderObject *r, RemoveRenderObjectFlag flag)
{
	if (r)
	{
		if (r->layer != LR_NONE && !renderObjectLayers[r->layer].empty())
		{
			renderObjectLayers[r->layer].remove(r);
		}
		if (flag != DO_NOT_DESTROY_RENDER_OBJECT )
		{
			r->destroy();

			deleteRenderObjectMemory(r);
		}
	}
}


void Core::enqueueRenderObjectDeletion(RenderObject *object)
{
	if (!object->_dead) // && !object->staticallyAllocated)
	{
		garbage.push_back (object);
		object->_dead = true;
	}
}

void Core::clearGarbage()
{
	BBGE_PROF(Core_clearGarbage);
	// HACK: optimize this (use a list instead of a queue)

	for (RenderObjectList::iterator i = garbage.begin(); i != garbage.end(); i++)
	{
		removeRenderObject(*i, DO_NOT_DESTROY_RENDER_OBJECT);

		(*i)->destroy();
	}

	for (RenderObjectList::iterator i = garbage.begin(); i != garbage.end(); i++)
	{
		deleteRenderObjectMemory(*i);
	}

	garbage.clear();

	// to clear resources
	for (std::vector<Resource*>::iterator i = resources.begin(); i != resources.end(); )
	{
		if ((*i)->getRef() == 0)
		{
			clearedGarbageFlag = true;
			delete (*i);
			i = resources.erase(i);
			continue;
		}

		if ((*i)->getRef() < 0)
		{
			errorLog("Texture ref < 0");
		}

		i++;
	}
}

bool Core::canChangeState()
{
	return (nestedMains<=1);
}

/*
int Core::getVirtualWidth()
{
	return virtualWidth;
}

int Core::getVirtualHeight()
{
	return virtualHeight;
}
*/

// Take a screenshot of the specified region of the screen and store it
// in a 32bpp pixel buffer.  delete[] the returned buffer when it's no
// longer needed.
unsigned char *Core::grabScreenshot(int x, int y, int w, int h)
{
#ifdef BBGE_BUILD_OPENGL

	unsigned char *imageData;

	unsigned int size = sizeof(unsigned char) * w * h * 4;
	imageData = new unsigned char[size];

	glPushAttrib(GL_ALL_ATTRIB_BITS);
	glDisable(GL_BLEND);
	glDisable(GL_ALPHA_TEST); glDisable(GL_BLEND);
	glDisable(GL_DEPTH_TEST); glDisable(GL_DITHER); glDisable(GL_FOG);
	glDisable(GL_LIGHTING); glDisable(GL_LOGIC_OP);
	glDisable(GL_STENCIL_TEST); glDisable(GL_TEXTURE_1D);
	glDisable(GL_TEXTURE_2D); glPixelTransferi(GL_MAP_COLOR, GL_FALSE);
	glPixelTransferi(GL_RED_SCALE, 1); glPixelTransferi(GL_RED_BIAS, 0);
	glPixelTransferi(GL_GREEN_SCALE, 1); glPixelTransferi(GL_GREEN_BIAS, 0);
	glPixelTransferi(GL_BLUE_SCALE, 1); glPixelTransferi(GL_BLUE_BIAS, 0);
	glPixelTransferi(GL_ALPHA_SCALE, 1); glPixelTransferi(GL_ALPHA_BIAS, 0);
	glRasterPos2i(0, 0);
	glReadPixels(x, y, w, h, GL_RGBA, GL_UNSIGNED_BYTE, (GLvoid*)imageData);
	glPopAttrib();

	// Force all alpha values to 255.
	unsigned char *c = imageData;
	for (int x = 0; x < w; x++)
	{
		for (int y = 0; y < h; y++, c += 4)
		{
			c[3] = 255;
		}
	}

	return imageData;

#else

	#warning FIXME: Need to implement non-GL grabScreenshot().
	// Avoid crashing, at least.
	return new unsigned char[sizeof(unsigned char) * w * h * 4];

#endif
}

// Like grabScreenshot(), but grab from the center of the screen.
unsigned char *Core::grabCenteredScreenshot(int w, int h)
{
	return grabScreenshot(core->width/2 - w/2, core->height/2 - h/2, w, h);
}

// takes a screen shot and saves it to a TGA image
int Core::saveScreenshotTGA(const std::string &filename)
{
	int w = getWindowWidth(), h = getWindowHeight();
	unsigned char *imageData = grabCenteredScreenshot(w, h);
	return tgaSave(filename.c_str(),w,h,32,imageData);
}

void Core::saveCenteredScreenshotTGA(const std::string &filename, int sz)
{
	int w=sz, h=sz;
	int hsm = (w * 3.0f) / 4.0f;
	unsigned char *imageData = grabCenteredScreenshot(w, hsm);

	int imageDataSize = sizeof(unsigned char) * w * hsm * 4;
	int tgaImageSize = sizeof(unsigned char) * w * h * 4;
	unsigned char *tgaImage = new unsigned char[tgaImageSize];
	memcpy(tgaImage, imageData, imageDataSize);
	memset(tgaImage + imageDataSize, 0, tgaImageSize - imageDataSize);
	delete[] imageData;

	int savebits = 32;
	tgaSave(filename.c_str(),w,h,savebits,tgaImage);
}

void Core::saveSizedScreenshotTGA(const std::string &filename, int sz, int crop34)
{
	debugLog("saveSizedScreenshot");

	int w, h;
	unsigned char *imageData;
	w = sz;
	h = sz;
	float fsz = (float)sz;

	unsigned int size = sizeof(unsigned char) * w * h * 3;
	imageData = (unsigned char *)malloc(size);

	float wbit = fsz;//+1;
	float hbit = ((fsz)*(3.0f/4.0f));

	int width = core->width-1;
	int height = core->height-1;
	int diff = 0;

	if (crop34)
	{
		width = int((core->height*4.0f)/3.0f);
		diff = (core->width - width)/2;
		width--;
	}

	float zx = wbit/(float)width;
	float zy = hbit/(float)height;

	float copyw = w*(1/zx);
	float copyh = h*(1/zy);



	std::ostringstream os;
	os << "wbit: " << wbit << " hbit: " << hbit << std::endl;
	os << "zx: " << zx << " zy: " << zy << std::endl;
	os << "w: " << w << " h: " << h << std::endl;
	os << "width: " << width << " height: " << height << std::endl;
	os << "copyw: " << copyw << " copyh: " << copyh << std::endl;
	debugLog(os.str());

	glRasterPos2i(0, 0);
	
	/*
	glPushAttrib(GL_ALL_ATTRIB_BITS);

	glDisable(GL_BLEND);

	glDisable(GL_ALPHA_TEST); glDisable(GL_BLEND);
	glDisable(GL_DEPTH_TEST); glDisable(GL_DITHER); glDisable(GL_FOG);
	glDisable(GL_LIGHTING); glDisable(GL_LOGIC_OP);
	glDisable(GL_STENCIL_TEST); glDisable(GL_TEXTURE_1D);
	glDisable(GL_TEXTURE_2D); glPixelTransferi(GL_MAP_COLOR,
		GL_FALSE); glPixelTransferi(GL_RED_SCALE, 1);
	glPixelTransferi(GL_RED_BIAS, 0); glPixelTransferi(GL_GREEN_SCALE, 1);
	glPixelTransferi(GL_GREEN_BIAS, 0); glPixelTransferi(GL_BLUE_SCALE, 1);
	glPixelTransferi(GL_BLUE_BIAS, 0); glPixelTransferi(GL_ALPHA_SCALE, 1);
	glPixelTransferi(GL_ALPHA_BIAS, 0);
	*/

	//glPixelStorei(GL_PACK_ALIGNMENT, 1);

	debugLog("pixel zoom");
	glPixelZoom(zx,zy);
	glFlush();

	glPixelZoom(1,1);
	debugLog("copy pixels");
	glCopyPixels(diff, 0, width, height, GL_COLOR);
	glFlush();

	debugLog("read pixels");
	glReadPixels(0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, (GLvoid*)imageData);
	glFlush();

	int savebits = 24;
	debugLog("saving bpp");
	tgaSave(filename.c_str(),w,h,savebits,imageData);

	debugLog("pop");
	//glPopAttrib();

	debugLog("done");
}

void Core::save64x64ScreenshotTGA(const std::string &filename)
{
#ifdef BBGE_BUILD_OPENGL
	int w, h;
	unsigned char *imageData;

// compute width and heidth of the image
	//w = xmax - xmin;
	//h = ymax - ymin;
	w = 64;
	h = 64;

// allocate memory for the pixels
	imageData = (unsigned char *)malloc(sizeof(unsigned char) * w * h * 4);

// read the pixels from the frame buffer

	//glReadPixels(xmin,ymin,xmax,ymax,GL_RGBA,GL_UNSIGNED_BYTE, (GLvoid *)imageData);
	glPixelZoom(64.0f/(float)getVirtualWidth(), 48.0f/(float)getVirtualHeight());
	glCopyPixels(0, 0, getVirtualWidth(), getVirtualHeight(), GL_COLOR);

	glReadPixels(0,0,64,64,GL_RGBA,GL_UNSIGNED_BYTE, (GLvoid *)imageData);


	unsigned char *c = imageData;
	for (int x=0; x < w; x++)
	{
		for (int y=0; y< h; y++)
		{
			c += 3;
			(*c) = 255;
			c ++;
		}
	}


// save the image
	tgaSave(filename.c_str(),64,64,32,imageData);
	glPixelZoom(1,1);
#endif

	// do NOT free imageData here
	// it IS freed in tgaSave
	//free(imageData);
}




// saves an array of pixels as a TGA image (frees the image data passed in)
int Core::tgaSave(	const char	*filename,
		short int	width,
		short int	height,
		unsigned char	pixelDepth,
		unsigned char	*imageData) {

	unsigned char cGarbage = 0, type,mode,aux;
	short int iGarbage = 0;
	int i;
	FILE *file;

// open file and check for errors
	file = fopen(adjustFilenameCase(filename).c_str(), "wb");
	if (file == NULL) {
		delete [] imageData;
		return (int)false;
	}

// compute image type: 2 for RGB(A), 3 for greyscale
	mode = pixelDepth / 8;
	if ((pixelDepth == 24) || (pixelDepth == 32))
		type = 2;
	else
		type = 3;

// write the header
	if (fwrite(&cGarbage, sizeof(unsigned char), 1, file) != 1
		|| fwrite(&cGarbage, sizeof(unsigned char), 1, file) != 1
		|| fwrite(&type, sizeof(unsigned char), 1, file) != 1
		|| fwrite(&iGarbage, sizeof(short int), 1, file) != 1
		|| fwrite(&iGarbage, sizeof(short int), 1, file) != 1
		|| fwrite(&cGarbage, sizeof(unsigned char), 1, file) != 1
		|| fwrite(&iGarbage, sizeof(short int), 1, file) != 1
		|| fwrite(&iGarbage, sizeof(short int), 1, file) != 1
		|| fwrite(&width, sizeof(short int), 1, file) != 1
		|| fwrite(&height, sizeof(short int), 1, file) != 1
		|| fwrite(&pixelDepth, sizeof(unsigned char), 1, file) != 1
		|| fwrite(&cGarbage, sizeof(unsigned char), 1, file) != 1)
	{
		fclose(file);
		delete [] imageData;
		return (int)false;
	}

// convert the image data from RGB(A) to BGR(A)
	if (mode >= 3)
	for (i=0; i < width * height * mode ; i+= mode) {
		aux = imageData[i];
		imageData[i] = imageData[i+2];
		imageData[i+2] = aux;
	}

// save the image data
	if (fwrite(imageData, sizeof(unsigned char),
			width * height * mode, file) != width * height * mode)
	{
		fclose(file);
		delete [] imageData;
		return (int)false;
	}

	fclose(file);
	delete [] imageData;

	return (int)true;
}

// saves a series of files with names "filenameX"
int Core::tgaSaveSeries(char		*filename,
			 short int		width,
			 short int		height,
			 unsigned char	pixelDepth,
			 unsigned char	*imageData) {

	char *newFilename;
	int status;

// compute the new filename by adding the
// series number and the extension
	newFilename = (char *)malloc(sizeof(char) * strlen(filename)+8);

	sprintf(newFilename,"%s%d",filename,numSavedScreenshots);

// save the image
	status = tgaSave(newFilename,width,height,pixelDepth,imageData);

//increase the counter
	if (status == (int)true)
		numSavedScreenshots++;
	free(newFilename);
	return(status);
}

 void Core::screenshot()
 {
	 doScreenshot = true;
//	ilutGLScreenie();
 }


 #include "DeflateCompressor.h"

 // saves an array of pixels as a TGA image (frees the image data passed in)
int Core::zgaSave(	const char	*filename,
		short int	w,
		short int	h,
		unsigned char	depth,
		unsigned char	*imageData) {

	ByteBuffer::uint8 type,mode,aux, pixelDepth = depth;
	ByteBuffer::uint8 cGarbage = 0;
	ByteBuffer::uint16 iGarbage = 0;
	ByteBuffer::uint16 width = w, height = h;

// open file and check for errors
	FILE *file = fopen(adjustFilenameCase(filename).c_str(), "wb");
	if (file == NULL) {
		delete [] imageData;
		return (int)false;
	}

// compute image type: 2 for RGB(A), 3 for greyscale
	mode = pixelDepth / 8;
	if ((pixelDepth == 24) || (pixelDepth == 32))
		type = 2;
	else
		type = 3;

// convert the image data from RGB(A) to BGR(A)
	if (mode >= 3)
	for (int i=0; i < width * height * mode ; i+= mode) {
		aux = imageData[i];
		imageData[i] = imageData[i+2];
		imageData[i+2] = aux;
	}

	ZlibCompressor z;
	z.SetForceCompression(true);
	z.reserve(width * height * mode + 30);
	z	<< cGarbage
		<< cGarbage
		<< type
		<< iGarbage
		<< iGarbage
		<< cGarbage
		<< iGarbage
		<< iGarbage
		<< width
		<< height
		<< pixelDepth
		<< cGarbage;

	z.append(imageData, width * height * mode);
	z.Compress(3);

// save the image data
	if (fwrite(z.contents(), 1, z.size(), file) != z.size())
	{
		fclose(file);
		delete [] imageData;
		return (int)false;
	}

	fclose(file);
	delete [] imageData;

	return (int)true;
}



#include "ttvfs_zip/VFSZipArchiveLoader.h"

void Core::setupFileAccess()
{
#ifdef BBGE_BUILD_VFS
	debugLog("Init VFS...");

	if(!ttvfs::checkCompat())
		exit_error("ttvfs not compatible");

	vfs.AddArchiveLoader(new ttvfs::VFSZipArchiveLoader);

	if(!vfs.LoadFileSysRoot(false))
	{
		exit_error("Failed to setup file access");
	}
	
	vfs.Prepare();


	ttvfs::VFSDir *override = vfs.GetDir("override");
	if(override)
	{
		debugLog("Mounting override dir...");
		override->load(true);
		vfs.Mount("override", "", true);
	}

	// If we ever want to read from a container...
	//vfs.AddArchive("aqfiles.zip", false, "");

	if(_extraDataDir.length())
	{
		debugLog("Mounting extra data dir: " + _extraDataDir);
		vfs.MountExternalPath(_extraDataDir.c_str(), "", true, true);
	}


	debugLog("Done");
#endif
}
