/*-----------------------------------------------------------------------------
	Run Susie plug-in    Copyright (c) TORO
 ----------------------------------------------------------------------------*/
#include <windows.h>
#include "torowin.h"
#include "susie.h"
#include "run.h"

BOOL CheckHeader(void)
{
	char header[SUSIE_CHECK_SIZE + 16];
	HANDLE hFile;
	int result;

	if ( IsSupported == NULL ) return FALSE;
	hFile = OpenFileHeader(header);
	if ( hFile == INVALID_HANDLE_VALUE ) return FALSE;

	if ( UseUNICODE && (IsSupportedW != NULL) ){
		result = IsSupportedW(sourcename, header);
	}else{
		result = IsSupported(sourcenameA, header);
	}
	if ( result != 0 ) return TRUE;
	printoutf(L" %s is not support.\r\n", sourcename);
	return FALSE;
}

// spibench の同名関数の改変版
// ( http://cetus.sakura.ne.jp/softlab/toolbox2/index.html#spibench )
void SaveDibData(LPCWSTR lpBmpFn, HLOCAL HBInfo, HLOCAL HBm)
{
	BITMAPFILEHEADER bmfh;
	void *lpbmh;
	void *lpdib;
	HANDLE  hFile;
	DWORD bmh_bytes, dib_bytes, tmp;

	lpbmh = LocalLock(HBInfo);
	lpdib = LocalLock(HBm);
	if ( lpbmh == NULL || lpdib == NULL ||
		*(DWORD *)lpbmh < sizeof(BITMAPCOREHEADER) || *(DWORD *)lpbmh > 256 ) {
		printout(L"The plug-in has returned an invalid memory block.\n");
		LocalUnlock(HBInfo);
		LocalUnlock(HBm);
		return;
	}
	hFile = CreateFileW(lpBmpFn, GENERIC_WRITE, FILE_SHARE_READ, NULL,
					   CREATE_ALWAYS, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
	if ( hFile == INVALID_HANDLE_VALUE ){
		printoutf(L"Can't Open - %s\n", lpBmpFn);
		LocalUnlock(HBInfo);
		LocalUnlock(HBm);
		return;
	}

	if ( (bmh_bytes = *(DWORD *)lpbmh) < sizeof(BITMAPINFOHEADER) ){ /* OS/2 format */
		LPBITMAPCOREHEADER lpbmch = (LPBITMAPCOREHEADER)lpbmh;
		if ( lpbmch->bcBitCount <= 8 ){
			bmh_bytes += sizeof(RGBTRIPLE) << lpbmch->bcBitCount;
		}
		dib_bytes = ((DWORD)lpbmch->bcWidth * lpbmch->bcBitCount + 31) / 32 * 4
					 * lpbmch->bcHeight;
	} else {											/* Windows format */
		LPBITMAPINFOHEADER lpbmih = (LPBITMAPINFOHEADER)lpbmh;
		if ( lpbmih->biCompression == BI_BITFIELDS &&
			 (lpbmih->biBitCount == 16 || lpbmih->biBitCount == 32) &&
			bmh_bytes < sizeof(BITMAPINFOHEADER) + 3 * sizeof(DWORD))
			bmh_bytes = sizeof(BITMAPINFOHEADER) + 3 * sizeof(DWORD);
		if (lpbmih->biClrUsed != 0){
			bmh_bytes += lpbmih->biClrUsed * sizeof(RGBQUAD);
		}else if (lpbmih->biBitCount <= 8){
			bmh_bytes += sizeof(RGBQUAD) << lpbmih->biBitCount;
		}
		if (lpbmih->biCompression == BI_RGB ||
			lpbmih->biCompression == BI_BITFIELDS) {
			dib_bytes = ((DWORD)lpbmih->biWidth * lpbmih->biBitCount + 31) / 32
						 * 4 * labs(lpbmih->biHeight);
		} else {
			dib_bytes = lpbmih->biSizeImage;
		}
	}
	bmfh.bfType = 0x4D42;
	bmfh.bfOffBits = sizeof(BITMAPFILEHEADER) + bmh_bytes;
	bmfh.bfSize    = bmfh.bfOffBits + dib_bytes;
	bmfh.bfReserved1 = bmfh.bfReserved2 = 0;

	if ( !WriteFile(hFile, &bmfh, sizeof(BITMAPFILEHEADER), &tmp, NULL) ||
		 !WriteFile(hFile, lpbmh, bmh_bytes, &tmp, NULL) ||
		 !WriteFile(hFile, lpdib, dib_bytes, &tmp, NULL)) {
		 printoutf(L"Write Error - %s\n", lpBmpFn);
	}
	CloseHandle(hFile);
	LocalUnlock(HBInfo);
	LocalUnlock(HBm);
}

int DoGetPicture(MODELIST RunMode, HLOCAL &HBInfo, HLOCAL &HBm)
{
	int result;
	GETPICTURE RunApiA;
	GETPICTUREW RunApiW;

	if ( CheckHeader() == FALSE ) return SUSIEERROR_UNKNOWNFORMAT;

	if ( RunMode == MODE_GETPREVIEW ){
		RunApiA = GetPreview;
		RunApiW = GetPreviewW;
	}else{
		RunApiA = GetPicture;
		RunApiW = GetPictureW;
	}
	if ( RunApiA == NULL ){
		printout(L" API not found.\r\n");
		return SUSIEERROR_INTERNAL;
	}

	HBInfo = HBm = INVALID_HANDLE_VALUE;
	if ( UseUNICODE && (RunApiW != NULL) ){
		result = RunApiW(sourcename, 0, SUSIE_SOURCE_DISK, &HBInfo, &HBm, (FARPROC)SusieProgressCallbackCheck, (LONG_PTR)sourcenameA);
	}else{
		result = RunApiA(sourcenameA, 0, SUSIE_SOURCE_DISK, &HBInfo, &HBm, (FARPROC)SusieProgressCallbackCheck, (LONG_PTR)sourcenameA);
	}
	if ( result != SUSIEERROR_NOERROR ){
		LoadSourceImage();
		if ( testmem != 0 ){
			if ( UseUNICODE && (RunApiW != NULL) ){
				result = RunApiW((WCHAR *)SourceImage, SourceSize, SUSIE_SOURCE_MEM, &HBInfo, &HBm, (FARPROC)SusieProgressCallbackCheck, (LONG_PTR)sourcenameA);
			}else{
				result = RunApiA(SourceImage, SourceSize, SUSIE_SOURCE_MEM, &HBInfo, &HBm, (FARPROC)SusieProgressCallbackCheck, (LONG_PTR)sourcenameA);
			}
		}
		if ( result != SUSIEERROR_NOERROR ){
			printout( (RunMode == MODE_GETPREVIEW) ? L"GetPreview" : L"GetPicture");
			PluginResult(result);
		}
	}
	return result;
}

void RunGetPicture(MODELIST RunMode)
{
	int result;
	HLOCAL HBInfo, HBm;

	result = DoGetPicture(RunMode, HBInfo, HBm);
	if ( result != SUSIEERROR_NOERROR ) return;

	if ( UseCreatePicture ){
		HMODULE hXPIPlugin = NULL;

		if ( xpipath[0] != '\0' ){
			hXPIPlugin = LoadLibrary(xpipath);
			if ( hXPIPlugin != NULL ){
				CreatePicture = (CREATEPICTURE)GetProcAddress(hXPIPlugin, "CreatePicture");
				CreatePictureW = (CREATEPICTUREW)GetProcAddress(hXPIPlugin, "CreatePictureW");
			}
		}

		if ( (CreatePicture == NULL) && (CreatePictureW == NULL) ){
			printout(L" CreatePicture not found.\r\n");
		}else{
			if ( UseUNICODE && (CreatePictureW != NULL) ){
				result = CreatePictureW(targetname, SUSIE_DEST_DISK, &HBInfo, &HBm, NULL, NULL, 0);
			}else{
				result = CreatePicture(targetnameA, SUSIE_DEST_DISK, &HBInfo, &HBm, NULL, NULL, 0);
			}
			if ( result != 0 ){
				PluginResult(result);
			}
		}
		if ( hXPIPlugin != NULL ) FreeLibrary(hXPIPlugin);
	}else{
		SaveDibData(targetname, HBInfo, HBm);
	}
	LocalFree(HBInfo);
	LocalFree(HBm);
}

void RunShowPicture(MODELIST RunMode)
{
	int result = SUSIEERROR_NOTSUPPORT;
	HLOCAL HBInfo, HBm;

	result = DoGetPicture(RunMode, HBInfo, HBm);
	if ( result != SUSIEERROR_NOERROR ) return;

	ShowPictureInfo(HBInfo, HBm);
	ShowImageToConsole(HBInfo, HBm);
	LocalFree(HBInfo);
	LocalFree(HBm);
}

void FixTimestamp(const WCHAR *filename, susie_time_t timestamp)
{
	HANDLE hFile;
	FILETIME ftime;
	unsigned __int64 tmptime;

	hFile = CreateFileW(filename, GENERIC_WRITE,
			FILE_SHARE_WRITE | FILE_SHARE_READ, NULL,
			OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
	if ( hFile == INVALID_HANDLE_VALUE ) return;
	tmptime = ((unsigned __int64)timestamp * 10000000) + 0x19db1ded53e8000;
	ftime.dwLowDateTime = (DWORD)tmptime;
	ftime.dwHighDateTime = (DWORD)(ULONG_PTR)(tmptime >> 32);
	SetFileTime(hFile, &ftime, &ftime, &ftime);

	CloseHandle(hFile);
}


WCHAR *FindLastEntryW(WCHAR *path)
{
	WCHAR *separator;

	for ( ;; ){
		separator = strchrW(path, ':');
		if ( separator == NULL ) break;
		*separator = '$'; // ドライブ指定、ストレージ指定を無効化
	}
	for(;;){
		WCHAR chr;

		chr = *path;
		if ( chr == '\0' ) return separator;
		if ( (chr == '/') || (chr == '\\') ) separator = path;
		path++;
	}
}

char *FindLastEntryA(char *path)
{
	char *separator = NULL;

	for ( ;; ){
		separator = strchr(path, ':');
		if ( separator == NULL ) break;
		*separator = '$'; // ドライブ指定、ストレージ指定を無効化
	}
	for(;;){
		BYTE chr;

		chr = (BYTE)*path;
		if ( chr == '\0' ) return separator;
		if ( chr >= 0x81 ){ // S-JIS 判定
			if ( (chr < 0xa0) && (chr >= 0xe0) ){ // S-JIS 判定
				if ( path[1] != '\0' ){
					path += 2;
					continue;
				}
			}
		}else{
			if ( (chr == '/' ) || (chr == '\\') ) separator = path;
		}
		path++;
	}
}

DWORD MakeDirectories(const WCHAR *dst)
{
	BOOL err;
	DWORD result = NO_ERROR;
	WCHAR shortdst[EXMAX_PATH], *wp;

	err = CreateDirectoryW(dst, NULL);
	if ( err == FALSE ){
		result = GetLastError();

		// FILE_ATTRIBUTE_VIRTUAL のときは、ERROR_INVALID_PARAMETER がでる
		// Samba で ERROR_INVALID_LEVEL がでる
		if ( result == ERROR_PATH_NOT_FOUND ){
			wcscpy(shortdst, dst);
			wp = FindLastEntryW(shortdst);
			if ( *wp ){
				*wp = '\0';
				result = MakeDirectories(shortdst);
				if ( result == NO_ERROR ){
					err = CreateDirectoryW(dst, NULL);
					if ( err == FALSE ) result = GetLastError();
				}
			}
		}
	}
	return result;
}

void RunArchive(MODELIST RunMode)
{
	int result;
	HLOCAL hInfFile;
	WCHAR destpathW[0x1000];

	if ( CheckHeader() == FALSE ) return;

	if ( targetname[0] == '\0' ){
		strcpyW(targetname, L".");
		strcpy(targetnameA, ".");
	}

	if ( UseUNICODE && (GetArchiveInfoW != NULL) ){
		hInfFile = INVALID_HANDLE_VALUE;
		result = GetArchiveInfoW(sourcename, 0, SUSIE_SOURCE_DISK, &hInfFile);
		if ( result != SUSIEERROR_NOERROR ){
			printout(L"GetArchiveInfoW");
			PluginResult(result);
		}else {
			SUSIE_FINFOW *finfo;
			UINT infocount, infosize;
			UINT maxcount;
			UINT count = 0;
			WCHAR timebuf[32];
			WCHAR pathbuf[EXMAX_PATH], *nameptr;
			size_t itemlen;

			finfo = (SUSIE_FINFOW *)LocalLock(hInfFile);
			infosize = LocalSize(hInfFile);

			itemlen = strlenW(archiveitem);
			infocount = infosize / sizeof(SUSIE_FINFOW);
			maxcount = (infosize + sizeof(SUSIE_FINFOW) - 1) / sizeof(SUSIE_FINFOW);
			for ( ; count < maxcount ; finfo++, count++ ){
				if ( finfo->method[0] == '\0' ) break;

				if ( archiveitem[0] != '\0' ){
					size_t namelen;
					namelen = strlenW(finfo->filename);
					if ( itemlen > namelen ) continue;
					if ( stricmpW(finfo->filename  + (namelen - itemlen), archiveitem ) != 0 ){
						continue;
					}
					if ( itemlen < namelen ){
						WCHAR chr;
						chr = finfo->filename[namelen - itemlen];
						if ( (chr != '\\') && (chr != '/') ) continue;
					}
				}

				switch (RunMode){
					case MODE_GETFILE: {
						nameptr = targetname;
						while ( (*nameptr == '\\') || (*nameptr == '/') ) nameptr++; // root 指定を除去
						strcpyW(destpathW, nameptr);
						if ( finfo->path[0] == '\0' ){
							strcpyW(pathbuf, finfo->filename);
						}else{
							wsprintfW(pathbuf, L"%s\\%s", finfo->path, finfo->filename);
						}
						nameptr = FindLastEntryW(pathbuf);
						if ( nameptr != NULL ){
							*nameptr = '\0';
							if ( UseSubdirectory ){
								strcatW(destpathW, L"\\");
								strcatW(destpathW, pathbuf);
							}
						}
						MakeDirectories(destpathW);
						result = GetFileW(sourcename, finfo->position, destpathW, SUSIE_SOURCE_DISK | SUSIE_DEST_DISK, (FARPROC)SusieProgressCallbackCheck, (LONG_PTR)&sourcenameA);
						if ( result == SUSIEERROR_NOERROR ){
							strcatW(destpathW, L"\\");
							strcatW(destpathW, (nameptr != NULL) ? nameptr : pathbuf);
							FixTimestamp(destpathW, finfo->timestamp);
						}else{
							printout(finfo->filename);
							PluginResult(result);
						}
						break;
					}

					default:
						GetTimeStrings(timebuf, finfo->timestamp);
						if ( finfo->filename[0] == '\0' ){
							printoutf(L"%s <dir>      %s%s\r\n", timebuf, finfo->path,  finfo->filename);
						}else{
							printoutf(L"%s %10d %s%s\r\n", timebuf, finfo->filesize, finfo->path,  finfo->filename);
						}
						break;
				}
				if ( itemlen != '\0' ) break;
			}
			LocalUnlock(hInfFile);
			LocalFree(hInfFile);
			return;
		}
	}

	hInfFile = INVALID_HANDLE_VALUE;
	result = GetArchiveInfo(sourcenameA, 0, SUSIE_SOURCE_DISK, &hInfFile);
	if ( result != SUSIEERROR_NOERROR ){
		printout(L"GetArchiveInfo");
		PluginResult(result);
		return;
	}
	{
		SUSIE_FINFO *finfo;
		UINT infocount, infosize;
		UINT maxcount;
		UINT count = 0;
		WCHAR timebuf[32];
		char destpathA[EXMAX_PATH];
		char pathbufA[EXMAX_PATH], *nameptr;
		size_t itemlen;

		finfo = (SUSIE_FINFO *)LocalLock(hInfFile);
		infosize = LocalSize(hInfFile);

		itemlen = strlen(archiveitemA);
		infocount = infosize / sizeof(SUSIE_FINFO);
		maxcount = (infosize + sizeof(SUSIE_FINFO) - 1) / sizeof(SUSIE_FINFO);
		for ( ; count < maxcount ; finfo++, count++ ){
			if ( finfo->method[0] == '\0' ) break;

			if ( itemlen != '\0' ){
				size_t namelen;
				namelen = strlen(finfo->filename);
				if ( itemlen > namelen ) continue;
				if ( stricmp(finfo->filename  + (namelen - itemlen), archiveitemA ) != 0 ){
					continue;
				}
				if ( itemlen < namelen ){
					char chr;
					chr = finfo->filename[namelen - itemlen];
					if ( (chr != '\\') && (chr != '/') ) continue;
				}
			}

			switch (RunMode){
				case MODE_GETFILE: {
					nameptr = targetnameA;
					while ( (*nameptr == '\\') || (*nameptr == '/') ) nameptr++; // root 指定を除去
					strcpy(destpathA, nameptr);
					if ( finfo->path[0] == '\0' ){
						strcpy(pathbufA, finfo->filename);
					}else{
						wsprintfA(pathbufA, "%s\\%s", finfo->path, finfo->filename);
					}
					nameptr = FindLastEntryA(pathbufA);
					if ( nameptr != NULL ){
						*nameptr = '\0';
						if ( UseSubdirectory ){
							strcat(destpathA, "\\");
							strcat(destpathA, pathbufA);
						}
					}
					AnsiToUnicode(destpathA, destpathW, MAX_PATH);
					MakeDirectories(destpathW);
					result = GetFile(sourcenameA, finfo->position, destpathA, SUSIE_SOURCE_DISK | SUSIE_DEST_DISK, (FARPROC)SusieProgressCallbackCheck, (LONG_PTR)&sourcenameA);
					if ( result == SUSIEERROR_NOERROR ){
						strcat(destpathA, "\\");
						strcat(destpathA, (nameptr != NULL) ? nameptr : pathbufA);
						AnsiToUnicode(destpathA, destpathW, MAX_PATH);
						FixTimestamp(destpathW, finfo->timestamp);
					}else{
						printoutf(L"%hs", finfo->filename);
						PluginResult(result);
					}
					break;
				}

				default:
					GetTimeStrings(timebuf, finfo->timestamp);
					if ( finfo->filename[0] == '\0' ){
						printoutf(L"%s <dir>      ", timebuf);
					}else{
						printoutf(L"%s %10d ", timebuf, finfo->filesize);
					}
					printoutf(L"%hs%hs\r\n", finfo->path,  finfo->filename);
					break;
			}
			if ( itemlen != '\0' ) break;
		}
		LocalUnlock(hInfFile);
		LocalFree(hInfFile);
	}
}
