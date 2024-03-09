/*-----------------------------------------------------------------------------
	Run Susie plug-in							Copyright (c) TORO
 ----------------------------------------------------------------------------*/
#include <windows.h>
#include "torowin.h"
#include "susie.h"
#include "run.h"

HMODULE hPlugin;
GETPLUGININFO	GetPluginInfo;
GETPLUGININFOW	GetPluginInfoW;
ISSUPPORTED		IsSupported;
ISSUPPORTEDW	IsSupportedW;
GETPICTURE		GetPicture;
GETPICTUREW		GetPictureW;
GETPREVIEW		GetPreview;
GETPREVIEWW		GetPreviewW;
GETPICTUREINFO	GetPictureInfo;
GETPICTUREINFOW	GetPictureInfoW;
GETARCHIVEINFO	GetArchiveInfo;
GETARCHIVEINFOW	GetArchiveInfoW;
GETFILE			GetFile;
GETFILEW		GetFileW;
GETFILEINFO		GetFileInfo;
GETFILEINFOW	GetFileInfoW;
CONFIGURATIONDLG ConfigurationDlg;
CREATEPICTURE	CreatePicture;
CREATEPICTUREW	CreatePictureW;

WCHAR spipath[MAX_PATH] = L"", xpipath[MAX_PATH] = L"";
WCHAR sourcename[EXMAX_PATH] = L"", targetname[EXMAX_PATH] = L"", archiveitem[EXMAX_PATH] = L"";
char sourcenameA[EXMAX_PATH] = "", targetnameA[EXMAX_PATH] = "", archiveitemA[EXMAX_PATH] = "";

HANDLE hStdout;
DWORD OldStdoutMode = 0;
OSVERSIONINFO OSver;
BOOL EnableESC = FALSE;
BOOL UseCreatePicture = FALSE;
BOOL UseSubdirectory = TRUE;

#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
 #define ENABLE_VIRTUAL_TERMINAL_PROCESSING 4 // Win10 RS2 以降
#endif

// ----------------------------------------------------------------------------
void GetQuotedParameter(LPCTSTR *commandline, TCHAR *param, const TCHAR *parammax)
{
	const TCHAR *ptr, *ptrfirst, *ptrlast;
	TCHAR *dest;

	dest = param;
	ptrfirst = ptr = *commandline + 1;
	for ( ; ; ){
		TCHAR code;

		code = *ptr;
		if ( (code == '\0') || (code == '\r') || (code == '\n') ){
			ptrlast = ptr;
			break;
		}
		if ( code != '\"' ){
			ptr++;
			continue;
		}
		// " を見つけた場合の処理
		if ( *(ptr + 1) != '\"' ){	// "" エスケープ?
			ptrlast = ptr++; // 単独 " … ここで終わり
			break;
		}
		// エスケープ処理
		{
			size_t copysize;

			copysize = ptr - ptrfirst + 1;
			if ( (dest + copysize) >= parammax ){ // buffer overflow?
				// " が 1文字エスケープされるので ">=" でok
				ptrlast = ptr;
				break;
			}
			memcpy(dest, ptrfirst, TSTROFF(copysize));
			dest += copysize;
			ptrfirst = (ptr += 2); // " x 2
			continue;
		}
	}
	*commandline = ptr;
	{
		size_t ptrsize;

		ptrsize = ptrlast - ptrfirst;
		if ( (dest + ptrsize) > parammax ){ // buffer overflow?
			tstrcpy(param,T("<flow!!>"));
		}else{
			memcpy(dest, ptrfirst, TSTROFF(ptrsize));
			*(dest + ptrsize) = '\0';
		}
	}
}

/*-------------------------------------
	行末(\0,\a,\d)か？

RET:	==0 :EOL	!=0 :Code
-------------------------------------*/
UTCHAR USEFASTCALL IsEOL(LPCTSTR *str)
{
	UTCHAR code;

	code = **str;
	if ( code == 0 ) return 0;
	if ( code == 0xd ){
		if ( *(*str + 1) == 0x0a ) (*str)++;
		return 0;
	}
	if ( code == 0xa ){
		if ( *(*str + 1) == 0x0d ) (*str)++;
		return 0;
	}
	return code;
}

/*-----------------------------------------------------------------------------
	一つ分のパラメータを抽出する。先頭と末尾の空白は除去する

RET:	先頭の文字(何もなかったら 0)
-----------------------------------------------------------------------------*/
UTCHAR GetLineParam(LPCTSTR *str, TCHAR *param)
{
	UTCHAR code,bottom;
	const TCHAR *src;
	TCHAR *dst,*max;

	src = *str;
	dst = param;
	max = dst + MAX_PATH - 1;
	for ( ;; ){
		code = *src;
		if ( (code == ' ') || (code == '\t') || (code == '\r') || (code == '\n') ){
			src++;
			continue;
		}
		break;
	}
	bottom = code;
	if ( code == '\0' ){
		*str = src;
		*dst = '\0';
		return '\0';
	}
	if ( code == '\"' ){
		GetQuotedParameter(&src, dst, max);
	}else{
		do {
			src++;
			if ( code <= ' ' )	break;
			if ( code == '\"' ){
				*dst++ = code;
				while( '\0' != (code = IsEOL(&src)) ){
					src++;
					*dst++ = code;
					if ( (code == '\"') && (*src != '\"') ) break;
				}
			}else{
				*dst++ = code;
			}
			if ( dst >= max ) break;
		}while( '\0' != (code = IsEOL(&src)) );
		*dst = '\0';
	}
	*str = src;
	return bottom;
}

void tInitConsole(void)
{
	OSver.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionEx(&OSver);

	hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
	GetConsoleMode(hStdout, &OldStdoutMode);
	if ( OSver.dwBuildNumber >= WINTYPE_10_BUILD_RS2 ){
		SetConsoleMode(hStdout, OldStdoutMode | ENABLE_PROCESSED_OUTPUT | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
		EnableESC = TRUE;
	}
}

void tReleaseConsole(void)
{
	SetConsoleMode(hStdout, OldStdoutMode);
}

int EscSq_FGcolors[16] = {
	30, 34, 32, 36, 31, 35, 33, 37,
	90, 94, 92, 96, 91, 95, 93, 97
};
int EscSq_BGcolors[16] = {
	40, 44, 42, 46, 41, 45, 43, 47,
	100, 104, 102, 106, 101, 105, 103, 107
};

void SetColor(DWORD color)
{
	if ( EnableESC ){
		printoutf(L"\x1b[%d;%dm", EscSq_FGcolors[color & 0xf], EscSq_BGcolors[(color >> 8) & 0xf]);
	}else{
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), (WORD)color);
	}
}

void USEFASTCALL printout(const WCHAR *str)
{
	DWORD size;

	WriteConsoleW(hStdout, str, (DWORD)strlenW(str), &size, NULL);
}

void printoutf(const WCHAR *message, ...)
{
	WCHAR buf[0x800];
	va_list argptr;

	va_start(argptr, message);
	wvsprintfW(buf, message, argptr);
	printout(buf);
}

void printoutfColor(DWORD color, const WCHAR *message, ...)
{
	WCHAR buf[0x800];
	va_list argptr;

	SetColor(color);
	va_start(argptr, message);
	wvsprintfW(buf, message, argptr);
	printout(buf);
	ResetColor();
}

void USEFASTCALL printoutA(const char *str)
{
	DWORD size;

	WriteConsoleA(hStdout, str, (DWORD)strlen(str), &size, NULL);
}

void printoutfA(const char *message, ...)
{
	char buf[0x800];
	va_list argptr;

	va_start(argptr, message);
	wvsprintfA(buf, message, argptr);
	printoutA(buf);
}

void printoutfColorA(DWORD color, const char *message, ...)
{
	char buf[0x800];
	va_list argptr;

	SetColor(color);
	va_start(argptr, message);
	wvsprintfA(buf, message, argptr);
	printoutA(buf);
	ResetColor();
}

BOOL LoadPlugin(WCHAR *filename)
{
	hPlugin = LoadLibrary(filename);
	if ( hPlugin == NULL ) return FALSE;
	GetPluginInfo = (GETPLUGININFO)GetProcAddress(hPlugin, "GetPluginInfo");
	GetPluginInfoW = (GETPLUGININFOW)GetProcAddress(hPlugin, "GetPluginInfoW");
	IsSupported = (ISSUPPORTED)GetProcAddress(hPlugin, "IsSupported");
	IsSupportedW = (ISSUPPORTEDW)GetProcAddress(hPlugin, "IsSupportedW");
	GetPicture = (GETPICTURE)GetProcAddress(hPlugin, "GetPicture");
	GetPreview = (GETPREVIEW)GetProcAddress(hPlugin, "GetPreview");
	GetPictureInfo = (GETPICTUREINFO)GetProcAddress(hPlugin, "GetPictureInfo");
	GetPictureW = (GETPICTUREW)GetProcAddress(hPlugin, "GetPictureW");
	GetPreviewW = (GETPREVIEWW)GetProcAddress(hPlugin, "GetPreviewW");
	GetArchiveInfo = (GETARCHIVEINFO)GetProcAddress(hPlugin, "GetArchiveInfo");
	GetArchiveInfoW = (GETARCHIVEINFOW)GetProcAddress(hPlugin, "GetArchiveInfoW");
	GetPictureInfoW = (GETPICTUREINFOW)GetProcAddress(hPlugin, "GetPictureInfoW");
	GetFile = (GETFILE)GetProcAddress(hPlugin, "GetFile");
	GetFileW = (GETFILEW)GetProcAddress(hPlugin, "GetFileW");
	GetFileInfo = (GETFILEINFO)GetProcAddress(hPlugin, "GetFileInfo");
	GetFileInfoW = (GETFILEINFOW)GetProcAddress(hPlugin, "GetFileInfoW");
	ConfigurationDlg = (CONFIGURATIONDLG)GetProcAddress(hPlugin, "ConfigurationDlg");
	CreatePicture = (CREATEPICTURE)GetProcAddress(hPlugin, "CreatePicture");
	CreatePictureW = (CREATEPICTUREW)GetProcAddress(hPlugin, "CreatePictureW");
	return TRUE;
}

const WCHAR *GetTimeStrings(WCHAR *dest, susie_time_t timestamp)
{
	SYSTEMTIME sTime;
	FILETIME ftime, lTime;
	unsigned __int64 tmptime;

	tmptime = ((unsigned __int64)timestamp * 10000000) + 0x19db1ded53e8000;
	ftime.dwLowDateTime = (DWORD)tmptime;
	ftime.dwHighDateTime = (DWORD)(ULONG_PTR)(tmptime >> 32);

	FileTimeToLocalFileTime(&ftime, &lTime);
	FileTimeToSystemTime(&lTime, &sTime);

	wsprintfW(dest, T("%04d-%02d-%02d%3d:%02d:%02d"),
			sTime.wYear, sTime.wMonth, sTime.wDay,
			sTime.wHour, sTime.wMinute, sTime.wSecond);
	return dest;
}

#define PrintAPIload(name) printout( (name == NULL) ? L" x" : L" A");printout( (name##W == NULL) ? L" x " UNICODESTR(#name) L"\r\n" : L" W " UNICODESTR(#name) L"\r\n");
#define PrintAPIload1(name) printout( (name == NULL) ? L" x   " UNICODESTR(#name) L"\r\n" : L" O   " UNICODESTR(#name) L"\r\n");

void ShowAPIlist(void)
{
	printoutfColor(COLOR_TITLE, L"\'%s\' function list\r\n", spipath);

	PrintAPIload(GetPluginInfo);
	PrintAPIload(IsSupported);
	PrintAPIload(GetPicture);
	PrintAPIload(GetPreview);
	PrintAPIload(GetPictureInfo);
	PrintAPIload(GetArchiveInfo);
	PrintAPIload(GetFileInfo);
	PrintAPIload(GetFile);
	PrintAPIload1(ConfigurationDlg);
	PrintAPIload(CreatePicture);
}

HANDLE OpenFileHeader(char *header)
{
	HANDLE hFile;
	DWORD size;

	memset(header, 0, SUSIE_CHECK_SIZE);
	if ( sourcename[0] == '\0' ) return INVALID_HANDLE_VALUE;

	hFile = CreateFileW(sourcename, GENERIC_READ,
			FILE_SHARE_WRITE | FILE_SHARE_READ, NULL,
			OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
	if ( hFile == INVALID_HANDLE_VALUE ){
		printoutf(L" %s open error.\r\n", sourcename);
		return hFile;
	}
	memset(header + SUSIE_CHECK_SIZE, WRITECHECK_BYTE, 16);
	ReadFile(hFile, header, SUSIE_CHECK_SIZE, &size, NULL);
	CloseHandle(hFile);
	return hFile;
}

const WCHAR PixSeqence[] = L"\x1b[38;2;%d;%d;%dm\x1b[48;2;%d;%d;%dm\u2584";

/* DIBのヘッダからパレットを作成する */
HPALETTE DIBtoPalette(HTBMP *hTbmp, int maxY)
{
	struct {
		WORD palVersion;
		WORD palNumEntries;
		PALETTEENTRY palPalEntry[256];
	} lPal;
	PALETTEENTRY *ppal;
	RGBQUAD *rgb;
	int r, g, b;
	DWORD ClrUsed;
	BYTE *src, *dst;
	BITMAPINFOHEADER *dib;

	dib = hTbmp->DIB;
	ClrUsed = dib->biClrUsed ? dib->biClrUsed : (DWORD)(1 << dib->biBitCount);
	lPal.palNumEntries = (WORD)ClrUsed;
	lPal.palVersion = 0x300;
	ppal = lPal.palPalEntry;
	if ( ClrUsed > 256 ){	//フルカラー画像は全体に散らばったパレットを作成
		HDC hDC;
		int VideoBits;

		hDC = GetDC(NULL);
		VideoBits = GetDeviceCaps(hDC, BITSPIXEL);
		ReleaseDC(NULL, hDC);
		if ( VideoBits > 8 ) return NULL; // 画面が256超なのでそのまま表示可能

		rgb = hTbmp->nb.rgb2;
		lPal.palNumEntries = 6 * 6 * 6;
		for( b = 0 ; b <= 255 ; b += 51 ){
			for(g = 0 ; g <= 255 ; g += 51){
				for(r = 0 ; r <= 255 ; r += 51, ppal++){
					rgb->rgbRed = ppal->peRed     = (BYTE)r;
					rgb->rgbGreen = ppal->peGreen = (BYTE)g;
					rgb->rgbBlue = ppal->peBlue   = (BYTE)b;
					rgb->rgbReserved = ppal->peFlags = 0;
					rgb++;
				}
			}
		}

		if ( dib->biBitCount == 24 ){
			int x, y;

			src = dst = hTbmp->bits;
			dib->biSizeImage = hTbmp->size.cx * maxY;
			for ( y = 0; y < maxY ; y++ ){
				for ( x = 0; x < dib->biWidth ; x++ ){
					r = ((*(src++) + 25) / 51);	// red
					g = ((*(src++) + 25) / 51);	// green
												// blue
					*dst++ = (BYTE)(r * 36 + g * 6 + ((*(src++) +25) / 51));
				}
				src += (4 - ALIGNMENT_BITS(src)) & 3;
				dst += (4 - ALIGNMENT_BITS(dst)) & 3;
			}
			dib->biBitCount = 8;
			dib->biCompression = BI_RGB;
			dib->biClrUsed = 6*6*6;
			dib->biClrImportant = 0;
			hTbmp->nb.dib2 = *dib;
			hTbmp->DIB = &hTbmp->nb.dib2;
		}
	}else{
		DWORD ci;

		rgb = (LPRGBQUAD)((BYTE *)hTbmp->DIB + hTbmp->PaletteOffset);
		if ( IsBadReadPtr(rgb, ClrUsed * sizeof(RGBQUAD)) ) return NULL;

		for ( ci = 0; ci < ClrUsed; ci++, rgb++, ppal++ ){
			ppal->peRed = rgb->rgbRed;
			ppal->peGreen = rgb->rgbGreen;
			ppal->peBlue = rgb->rgbBlue;
			ppal->peFlags = 0;
		}
	}
	return CreatePalette((LOGPALETTE *)&lPal);
}

BOOL InitBMP(HTBMP *hTbmp)
{
	int palette;
	DWORD offset;
	DWORD color;

	if ( (hTbmp->DIB = (BITMAPINFOHEADER *)LocalLock(hTbmp->info)) == NULL ) goto error;
	if ( (hTbmp->bits = (BYTE *)LocalLock(hTbmp->bm)) == NULL ) goto error;

	hTbmp->hPalette = NULL;

	offset = hTbmp->DIB->biSize;
	color = hTbmp->DIB->biBitCount;
	palette = hTbmp->DIB->biClrUsed;
	if ( (offset < (sizeof(BITMAPINFOHEADER) + 12) ) && (hTbmp->DIB->biCompression == BI_BITFIELDS) ){
		offset += 12;	// 16/32bit のときはビット割り当てがある
	}
	hTbmp->PaletteOffset = offset;
#pragma warning(suppress: 6297) // color は 8以下で、DWORD を越えることがない
	offset += palette ? palette * (DWORD)sizeof(RGBQUAD) :
			((color <= 8) ? (DWORD)(1 << color) * (DWORD)sizeof(RGBQUAD) : 0);

	if ( hTbmp->bits == NULL ){
		hTbmp->bits = (BYTE *)hTbmp->DIB + offset;
	}
	hTbmp->size.cx = hTbmp->DIB->biWidth;
	hTbmp->size.cy = hTbmp->DIB->biHeight;
	if ( (hTbmp->size.cy == 0) || (hTbmp->size.cx <= 0) || (color == 0) || (color > 32) ){ // 大きさ異常?
		return FALSE;
	}
	if ( hTbmp->size.cy < 0 ) hTbmp->size.cy = -hTbmp->size.cy; // トップダウン
	if ( palette && (color <= 8) ) hTbmp->DIB->biClrUsed = 1 << color;

	hTbmp->hPalette = DIBtoPalette(hTbmp, hTbmp->size.cy);
	return TRUE;
error:
	hTbmp->DIB = NULL;
	hTbmp->hPalette = NULL;
	return FALSE;
}

#define ASPACTX 1000

BOOL ShowImageToConsole(HLOCAL HBInfo, HLOCAL HBm)
{
	HTBMP hTbmp;
	HBITMAP hDestBMP;
	HGDIOBJ hOldDstBMP;
	HDC hDC, hDstDC;
	LPVOID lpBits;
	WCHAR buf[0x400];
	BITMAPINFO bmpinfo;
	CONSOLE_SCREEN_BUFFER_INFO screen;
	int show_width, show_height;
	int bmp_width, bmp_height;
	int AspectRate;

	if ( EnableESC == FALSE ) return FALSE;
	GetConsoleScreenBufferInfo(hStdout, &screen);

	hTbmp.info = HBInfo;
	hTbmp.bm = HBm;
	if ( InitBMP(&hTbmp) == FALSE ) return FALSE;
	bmp_width = hTbmp.size.cx;
	bmp_height = hTbmp.size.cy;

	AspectRate = (hTbmp.DIB->biYPelsPerMeter == 0) ? 0 : ((hTbmp.DIB->biXPelsPerMeter * ASPACTX) / hTbmp.DIB->biYPelsPerMeter);
	if ( (AspectRate > (ASPACTX * 3)) ||
		 (AspectRate < (ASPACTX / 3)) ||
		 ((AspectRate > (ASPACTX - (ASPACTX / 10))) &&
		  (AspectRate < (ASPACTX + (ASPACTX / 10)))) ){
		AspectRate = 0;
	}
	if ( AspectRate != 0 ){
		if ( AspectRate >= ASPACTX ){
			bmp_height = (bmp_height * AspectRate) / ASPACTX;
		}else{
			bmp_width = (bmp_width * ASPACTX) / AspectRate;
		}
	}

	show_width = screen.srWindow.Right - screen.srWindow.Left - 1;
	if ( show_width > bmp_width ) show_width = bmp_width;
	show_height = (screen.srWindow.Bottom - screen.srWindow.Top - 1) * 2;
	if ( show_height > bmp_height ) show_height = bmp_height;
	if ( ((show_width << 15)  / bmp_width) >
		 ((show_height << 15) / bmp_height) ){
		show_width = (bmp_width * show_height) / bmp_height;
	}else{
		show_height = (bmp_height * show_width) / bmp_width;
	}
	if ( show_width == 0 ) show_width = 1;
	if ( show_height == 0 ) show_height = 1;

	hDC = GetDC(NULL);
	hDstDC = CreateCompatibleDC(NULL);
	bmpinfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmpinfo.bmiHeader.biWidth = show_width;
	bmpinfo.bmiHeader.biHeight = show_height;
	bmpinfo.bmiHeader.biPlanes = 1;
	bmpinfo.bmiHeader.biBitCount = 32;
	bmpinfo.bmiHeader.biCompression = BI_RGB;
	bmpinfo.bmiHeader.biSizeImage = 0;
	bmpinfo.bmiHeader.biClrUsed = 0;
	bmpinfo.bmiHeader.biClrImportant = 0;

	hDestBMP = CreateDIBSection(NULL, &bmpinfo, DIB_RGB_COLORS, &lpBits, NULL, 0);

	// 元画像を複写
	hOldDstBMP = SelectObject(hDstDC, hDestBMP);
	SetStretchBltMode(hDstDC, HALFTONE);

	// 追加画像を複写
	StretchDIBits(hDstDC,
			0, 0, bmpinfo.bmiHeader.biWidth, bmpinfo.bmiHeader.biHeight,
			0, 0, hTbmp.size.cx, hTbmp.size.cy,
			hTbmp.bits, (BITMAPINFO *)hTbmp.DIB, DIB_RGB_COLORS, SRCCOPY);

	SelectObject(hDstDC, hOldDstBMP);
	DeleteDC(hDstDC);
	ReleaseDC(NULL, hDC);

	{
		int x, y;
		COLORREF pix1, pix2;

		y = bmpinfo.bmiHeader.biHeight - 2;
		if ( y <= 0 ){
			for ( x = 0 ; x < bmpinfo.bmiHeader.biWidth ; x++ ){
				pix1 = *(COLORREF *)(BYTE *)((BYTE *)lpBits + x * sizeof(COLORREF));
				wsprintfW(buf, PixSeqence,
					GetBValue(pix1), GetGValue(pix1), GetRValue(pix1),
					0, 0, 0);
				printout(buf);
			}
			printout(T("\x1b[m\r\n"));
		}else for ( ; y >= 0 ; y -= 2)
		{
			for ( x = 0 ; x < bmpinfo.bmiHeader.biWidth ; x++ ){
				if ( y > 0 ){
					pix1 = *(COLORREF *)(BYTE *)((BYTE *)lpBits + (x + y * bmpinfo.bmiHeader.biWidth) * sizeof(COLORREF));
				}else{
					pix1 = 0;
				}
				pix2 = *(COLORREF *)(BYTE *)((BYTE *)lpBits + (x + (y + 1) * bmpinfo.bmiHeader.biWidth) * sizeof(COLORREF));
				wsprintfW(buf, PixSeqence,
					GetBValue(pix1), GetGValue(pix1), GetRValue(pix1),
					GetBValue(pix2), GetGValue(pix2), GetRValue(pix2) );
				printout(buf);
			}
			printout(T("\x1b[m\r\n"));
		}
	}
	// 保存
	DeleteObject(hDestBMP);

	LocalUnlock(hTbmp.info);
	LocalUnlock(hTbmp.bm);
	return TRUE;
}
