// VisualBoyAdvance - Nintendo Gameboy/GameboyAdvance (TM) emulator.
// Copyright (C) 1999-2003 Forgotten
// Copyright (C) 2004 Forgotten and the VBA development team

// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2, or(at your option)
// any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software Foundation,
// Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

#include "stdafx.h"
#include <stdio.h>

#include "resource.h"
#include "MainWnd.h"
#include "Reg.h"
#include "VBA.h"

#include "../gba/Globals.h"
#include "../gb/gbGlobals.h"
#include "../common/Text.h"
#include "../version.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern void winlog(const char *, ...);
extern int RGB_LOW_BITS_MASK;
extern int Init_2xSaI(u32);
extern int systemSpeed;

class GDIDisplay : public IDisplay
{
private:
	u8 *filterData;
	u8  info[sizeof(BITMAPINFOHEADER)+256*sizeof(RGBQUAD)];
public:
	GDIDisplay();
	virtual ~GDIDisplay();

	virtual bool initialize();
	virtual void cleanup();
	virtual void render();
	virtual void checkFullScreen();
	virtual void renderMenu();
	virtual void clear();
	virtual DISPLAY_TYPE getType() { return GDI; };
	virtual void setOption(const char *, int) {}
	virtual int selectFullScreenMode(GUID * *);
};

static int calculateShift(u32 mask)
{
	int m = 0;

	while (mask)
	{
		m++;
		mask >>= 1;
	}

	return m-5;
}

GDIDisplay::GDIDisplay()
{
	filterData = (u8 *)malloc(4*4*256*240);
}

GDIDisplay::~GDIDisplay()
{
	cleanup();
}

void GDIDisplay::cleanup()
{
	if (filterData)
	{
		free(filterData);
		filterData = NULL;
	}
}

bool GDIDisplay::initialize()
{
	theApp.sizeX = 240;
	theApp.sizeY = 160;
	switch (theApp.videoOption)
	{
	case VIDEO_1X:
		theApp.surfaceSizeX = theApp.sizeX;
		theApp.surfaceSizeY = theApp.sizeY;
		break;
	case VIDEO_2X:
		theApp.surfaceSizeX = theApp.sizeX * 2;
		theApp.surfaceSizeY = theApp.sizeY * 2;
		break;
	case VIDEO_3X:
		theApp.surfaceSizeX = theApp.sizeX * 3;
		theApp.surfaceSizeY = theApp.sizeY * 3;
		break;
	case VIDEO_4X:
		theApp.surfaceSizeX = theApp.sizeX * 4;
		theApp.surfaceSizeY = theApp.sizeY * 4;
		break;
	case VIDEO_320x240:
	case VIDEO_640x480:
	case VIDEO_800x600:
	case VIDEO_OTHER:
	{
		int scaleX = (theApp.fsWidth / theApp.sizeX);
		int scaleY = (theApp.fsHeight / theApp.sizeY);
		int min    = scaleX < scaleY ? scaleX : scaleY;
		if (theApp.fsMaxScale)
			min = min > theApp.fsMaxScale ? theApp.fsMaxScale : min;
		theApp.surfaceSizeX = theApp.sizeX * min;
		theApp.surfaceSizeY = theApp.sizeY * min;
		if (theApp.fullScreenStretch)
		{
			theApp.surfaceSizeX = theApp.fsWidth;
			theApp.surfaceSizeY = theApp.fsHeight;
		}
		break;
	}
	}

	theApp.rect.left   = 0;
	theApp.rect.top    = 0;
	theApp.rect.right  = theApp.sizeX;
	theApp.rect.bottom = theApp.sizeY;

	theApp.dest.left   = 0;
	theApp.dest.top    = 0;
	theApp.dest.right  = theApp.surfaceSizeX;
	theApp.dest.bottom = theApp.surfaceSizeY;

	DWORD style   = WS_POPUP | WS_VISIBLE;
	DWORD styleEx = 0;

	if (theApp.videoOption <= VIDEO_4X)
		style |= WS_OVERLAPPEDWINDOW;
	else
		styleEx = 0;

	if (theApp.videoOption <= VIDEO_4X)
		AdjustWindowRectEx(&theApp.dest, style, TRUE, styleEx);
	else
		AdjustWindowRectEx(&theApp.dest, style, FALSE, styleEx);

	int winSizeX = theApp.dest.right-theApp.dest.left;
	int winSizeY = theApp.dest.bottom-theApp.dest.top;

	if (theApp.videoOption > VIDEO_4X)
	{
		winSizeX = theApp.fsWidth;
		winSizeY = theApp.fsHeight;
	}

	int x = 0;
	int y = 0;

	if (theApp.videoOption <= VIDEO_4X)
	{
		x = theApp.windowPositionX;
		y = theApp.windowPositionY;
	}

	// Create a window
	MainWnd *pWnd = new MainWnd;
	theApp.m_pMainWnd = pWnd;

	pWnd->CreateEx(styleEx,
	               theApp.wndClass,
	               VBA_NAME_AND_VERSION,
	               style,
	               x, y, winSizeX, winSizeY,
	               NULL,
	               0);

	if (!(HWND)*pWnd)
	{
		winlog("Error creating Window %08x\n", GetLastError());
		return FALSE;
	}

	theApp.updateMenuBar();

	theApp.adjustDestRect();

	theApp.mode320Available = false;
	theApp.mode640Available = false;
	theApp.mode800Available = false;

	HDC         dc  = GetDC(NULL);
	HBITMAP     hbm = CreateCompatibleBitmap(dc, 1, 1);
	BITMAPINFO *bi  = (BITMAPINFO *)info;
	ZeroMemory(bi, sizeof(info));
	bi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	GetDIBits(dc, hbm, 0, 1, NULL, (LPBITMAPINFO)info, DIB_RGB_COLORS);
	GetDIBits(dc, hbm, 0, 1, NULL, (LPBITMAPINFO)info, DIB_RGB_COLORS);
	DeleteObject(hbm);
	ReleaseDC(NULL, dc);

	if (bi->bmiHeader.biCompression == BI_BITFIELDS)
	{
		systemColorDepth = bi->bmiHeader.biBitCount;
		if (systemColorDepth == 15)
			systemColorDepth = 16;
		systemRedShift   = calculateShift(*((DWORD *)&bi->bmiColors[0]));
		systemGreenShift = calculateShift(*((DWORD *)&bi->bmiColors[1]));
		systemBlueShift  = calculateShift(*((DWORD *)&bi->bmiColors[2]));
		if (systemColorDepth == 16)
		{
			if (systemGreenShift == 6)
			{
				Init_2xSaI(565);
				RGB_LOW_BITS_MASK = 0x821;
			}
			else
			{
				Init_2xSaI(555);
				RGB_LOW_BITS_MASK = 0x421;
			}
		}
		else if (systemColorDepth == 32)
			Init_2xSaI(32);
	}
	else
	{
		systemColorDepth = 32;
		systemRedShift   = 19;
		systemGreenShift = 11;
		systemBlueShift  = 3;

		Init_2xSaI(32);
	}
	theApp.fsColorDepth = systemColorDepth;
	if (systemColorDepth == 24)
		theApp.filterFunction = NULL;
#ifdef MMX
	if (!theApp.disableMMX)
		cpu_mmx = theApp.detectMMX();
	else
		cpu_mmx = 0;
#endif

	switch (systemColorDepth)
	{
	case 16:
	{
		for (int i = 0; i < 0x10000; i++)
		{
			systemColorMap16[i] = ((i & 0x1f) << systemRedShift) |
			                      (((i & 0x3e0) >> 5) << systemGreenShift) |
			                      (((i & 0x7c00) >> 10) << systemBlueShift);
		}
		break;
	}
	case 24:
	case 32:
	{
		for (int i = 0; i < 0x10000; i++)
		{
			systemColorMap32[i] = ((i & 0x1f) << systemRedShift) |
			                      (((i & 0x3e0) >> 5) << systemGreenShift) |
			                      (((i & 0x7c00) >> 10) << systemBlueShift);
		}
		break;
	}
	}
	theApp.updateFilter();
	theApp.updateIFB();

	pWnd->DragAcceptFiles(TRUE);

	return TRUE;
}

void GDIDisplay::clear()
{}

void GDIDisplay::renderMenu()
{
	checkFullScreen();
	theApp.m_pMainWnd->DrawMenuBar();
}

void GDIDisplay::checkFullScreen()
{}

void GDIDisplay::render()
{
	void (*filterFunction)(u8 *, u32, u8 *, u8 *, u32, int, int) = theApp.filterFunction;
	int  filterWidth = theApp.filterWidth, filterHeight = theApp.filterHeight;
/*
    if(textMethod == 1)
    {
        int copyX = 240, copyY = 160;
        if(theApp.cartridgeType == 1)
            if(gbBorderOn) copyX = 256, copyY = 224;
            else           copyX = 160, copyY = 144;

        extern void Simple1x(u8*,u32,u8*,u8*,u32,int,int);
        filterFunction = Simple1x;
        filterWidth = copyX*2;
        filterHeight = copyY*2;
    }
 */
	BITMAPINFO *bi = (BITMAPINFO *)info;
	bi->bmiHeader.biWidth  = filterWidth+1;
	bi->bmiHeader.biHeight = -filterHeight;

	int pitch = filterWidth * 2 + 4;
	if (systemColorDepth == 24)
		pitch = filterWidth * 3;
	else if (systemColorDepth == 32)
		pitch = filterWidth * 4 + 4;

	if (textMethod == 1 && !filterFunction)
	{
		textMethod = 0; // must not be after systemMessage!
		systemMessage(
		    0,
		    "The \"On Game\" text display mode does not work with this combination of renderers and filters.\nThe display mode is automatically being changed to \"In Game\" instead,\nbut this may cause message text to go into AVI recordings and screenshots.\nThis can be reconfigured by choosing \"Options->Video->Text Display Options...\"");
	}

	// moved to VBA.cpp
	/*
	   if(textMethod == 0)
	   {
	        int copyX = 240, copyY = 160;
	        if(theApp.cartridgeType == 1)
	            if(gbBorderOn) copyX = 256, copyY = 224;
	            else           copyX = 160, copyY = 144;

	        DrawTextMessages((u8*)pix, copyX*(systemColorDepth/8)+(systemColorDepth==24?0:4), 0, copyY);
	   }
	 */

	if (filterFunction)
	{
		bi->bmiHeader.biWidth  = filterWidth * 2;
		bi->bmiHeader.biHeight = -filterHeight * 2;

		if (systemColorDepth == 16)
			(*filterFunction)(pix+pitch,
			                  pitch,
			                  (u8 *)theApp.delta,
			                  (u8 *)filterData,
			                  filterWidth*2*2,
			                  filterWidth,
			                  filterHeight);
		else
			(*filterFunction)(pix+pitch,
			                  pitch,
			                  (u8 *)theApp.delta,
			                  (u8 *)filterData,
			                  filterWidth*4*2,
			                  filterWidth,
			                  filterHeight);
	}

	if (theApp.showSpeed && theApp.videoOption > VIDEO_4X)
	{
		char buffer[30];
		if (theApp.showSpeed == 1)
			sprintf(buffer, "%3d%%", systemSpeed);
		else
			sprintf(buffer, "%3d%%(%d, %d fps)", systemSpeed,
			        systemFrameSkip,
			        theApp.showRenderedFrames);

		if (filterFunction)
		{
			int p = filterWidth * 4;
			if (systemColorDepth == 24)
				p = filterWidth * 6;
			else if (systemColorDepth == 32)
				p = filterWidth * 8;
			if (theApp.showSpeedTransparent)
				drawTextTransp((u8 *)filterData,
				               p,
				               10,
				               filterHeight*2-10,
				               buffer);
			else
				drawText((u8 *)filterData,
				         p,
				         10,
				         filterHeight*2-10,
				         buffer);
		}
		else
		{
			if (theApp.showSpeedTransparent)
				drawTextTransp((u8 *)pix,
				               pitch,
				               10,
				               filterHeight-10,
				               buffer);
			else
				drawText((u8 *)pix,
				         pitch,
				         10,
				         filterHeight-10,
				         buffer);
		}
	}
	if (textMethod == 1 && filterFunction)
		DrawTextMessages((u8 *)filterData, filterWidth*systemColorDepth/4, 0, filterHeight*2);

	POINT p;
	p.x = theApp.dest.left;
	p.y = theApp.dest.top;
	CWnd *pWnd = theApp.m_pMainWnd;
	pWnd->ScreenToClient(&p);
	POINT p2;
	p2.x = theApp.dest.right;
	p2.y = theApp.dest.bottom;
	pWnd->ScreenToClient(&p2);

	CDC *dc = pWnd->GetDC();

	StretchDIBits((HDC)*dc,
	              p.x,
	              p.y,
	              p2.x - p.x,
	              p2.y - p.y,
	              0,
	              0,
	              theApp.rect.right,
	              theApp.rect.bottom,
	              filterFunction ? filterData : pix+pitch,
	              bi,
	              DIB_RGB_COLORS,
	              SRCCOPY);

	if (textMethod == 2)
		for (int slot = 0; slot < SCREEN_MESSAGE_SLOTS; slot++)
		{
			if (theApp.screenMessage[slot])
			{
				if (((int)(GetTickCount() - theApp.screenMessageTime[slot]) < theApp.screenMessageDuration[slot]) &&
				    (!theApp.disableStatusMessage || slot == 1 || slot == 2))
				{
					dc->SetBkMode(TRANSPARENT);

					if (outlinedText)
					{
						dc->SetTextColor(textColor != 7 ? RGB(0, 0, 0) : RGB(255, 255, 255));

						// draw black outline
						const static int xd [8] = {-1, 0, 1, 1, 1, 0, -1, -1};
						const static int yd [8] = {-1, -1, -1, 0, 1, 1, 1, 0};
						for (int i = 0; i < 8; i++)
						{
							dc->TextOut(p.x+10+xd[i], p2.y - 20*(slot+1)+yd[i], theApp.screenMessageBuffer[slot]);
						}
					}

					COLORREF color;
					switch (textColor)
					{
					case 0:
						color = RGB(255, 255, 255); break;
					case 1:
						color = RGB(255, 0, 0); break;
					case 2:
						color = RGB(255, 255, 0); break;
					case 3:
						color = RGB(0, 255, 0); break;
					case 4:
						color = RGB(0, 255, 255); break;
					case 5:
						color = RGB(0, 0, 255); break;
					case 6:
						color = RGB(255, 0, 255); break;
					case 7:
						color = RGB(0, 0, 0); break;
					}
					dc->SetTextColor(color);

					// draw center text
					dc->TextOut(p.x+10, p2.y - 20*(slot+1), theApp.screenMessageBuffer[slot]);
				}
				else
				{
					theApp.screenMessage[slot] = false;
				}
			}
		}

	pWnd->ReleaseDC(dc);
}

int GDIDisplay::selectFullScreenMode(GUID * *)
{
	HWND wnd = GetDesktopWindow();
	RECT r;
	GetWindowRect(wnd, &r);
	int w  = (r.right - r.left) & 4095;
	int h  = (r.bottom - r.top) & 4095;
	HDC dc = GetDC(wnd);
	int c  = GetDeviceCaps(dc, BITSPIXEL);
	ReleaseDC(wnd, dc);

	return (c << 24) | (w << 12) | h;
}

IDisplay *newGDIDisplay()
{
	return new GDIDisplay();
}

