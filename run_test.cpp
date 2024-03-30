/*-----------------------------------------------------------------------------
	Run Susie plug-in							Copyright (c) TORO
 ----------------------------------------------------------------------------*/
#include <windows.h>
#include "torowin.h"
#include "susie.h"
#include "run.h"

int CommentCount = 0;
BOOL CallbackError = FALSE;

BOOL type_extract = FALSE;
char *SourceImage = NULL, *CompareSourceImage = NULL;
DWORD SourceSize, SourceAllocSize;
int testmem = -1;
int testdisk = -1;

// �W�J�v���O�C���p
BOOL type_extract_mem = FALSE;
HLOCAL ArchiveInfo = NULL;
HLOCAL ArchiveInfoW = NULL;
SUSIE_FINFO *finfo_file = NULL, *finfo_mem = NULL;
SUSIE_FINFOW *finfo_fileW = NULL, *finfo_memW = NULL;
UINT infosize_file, infosize_mem;

#define MAX_TEST_ON_MEMORY (50 * 1024 * 1024) // �\�[�X���������̎��̍ő�\�l

#define PrintAPIname(name) printoutfColor(COLOR_APINAME, L"\r\n" UNICODESTR(name) L"\r\n")

void ShowComment(const WCHAR *message, ...)
{
	WCHAR buf[0x800];
	va_list argptr;

	SetColor(COLOR_COMMENT);
	va_start(argptr, message);
	wvsprintfW(buf, message, argptr);
	printout(buf);
	ResetColor();
	CommentCount++;
}

const WCHAR *errortext[] = {
	L"Not Support",
	L"no error",
	L"User cancel",
	L"Unknown format",
	L"Broken DATA",
	L"Empty memory",
	L"Fault memory",
	L"Fault read",
	L"Window error",
	L"Internal",
	L"File write",
	L"End of file"
};

void PluginResult(int result)
{
	const WCHAR *text;

	if ( (result >= SUSIEERROR_NOTSUPPORT) && (result <= SUSIEERROR_EOF) ){
		text = errortext[result - SUSIEERROR_NOTSUPPORT];
	}else if ( result == -101 ){
		text = errortext[0];
	}else if ( (result >= 101) && (result <= 110) ){
		text = errortext[result - 99];
	}else{
		text = L" �� ����`�R�[�h";
		CommentCount++;
	}
	printoutf(L" return code: %d (%s)\r\n", result, text);
}

void LoadSourceImage(void)
{
	HANDLE hFile;

	if ( testmem == testmem_zerosize ){
		SourceSize = 0;
		SourceAllocSize = 0;

		SourceImage = (char *)malloc(0);
		CompareSourceImage = (char *)malloc(0);
		strcpy(sourcenameA, "NUL");
		strcpyW(sourcename, L"NUL");
		return;
	}

	if ( sourcename[0] == '\0' ){
		testmem = 0;
		return;
	}

	hFile = CreateFileW(sourcename, GENERIC_READ,
			FILE_SHARE_WRITE | FILE_SHARE_READ, NULL,
			OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
	if ( hFile != INVALID_HANDLE_VALUE ){
		LARGE_INTEGER filesize;
		DWORD size;

		GetFileSizeEx(hFile, &filesize);
		SourceSize = filesize.u.LowPart;
		SourceAllocSize = filesize.u.LowPart + 256;
		if ( testmem < 0 ){
			testmem = filesize.QuadPart <= MAX_TEST_ON_MEMORY;
		}
		if ( testmem ){
			SourceImage = (char *)malloc(SourceAllocSize);
			CompareSourceImage = (char *)malloc(SourceAllocSize);
			memset(SourceImage, WRITECHECK_BYTE, SourceAllocSize);
			ReadFile(hFile, SourceImage, filesize.u.LowPart, &size, NULL);
			memcpy(CompareSourceImage, SourceImage, SourceAllocSize);
		}else{
			printout(L"���\�[�X�t�@�C�����傫�����߃`�F�b�N�ł��Ȃ�\r\n");
		}
		CloseHandle(hFile);
	}else{
		printoutf(L"%s open error.\r\n", sourcename);
		testmem = 0;
	}
}

int __stdcall SusieProgressCallbackCheck(int nNum, int nDenom, LONG_PTR lData)
{
	if ( CallbackError == FALSE ){
		if ( lData != (LONG_PTR)sourcenameA ){
#ifdef _WIN64
			if ( lData == (DWORD)(DWORD_PTR)sourcenameA ){
				ShowComment(L" �� SUSIE_PROGRESS �� lData �� LONG_PTR �łȂ��Along �̂��߁A��� 32bit ���n����Ă��Ȃ�((lData(%p) != %p)�A����̃`�F�b�N���X�L�b�v\r\n", lData, sourcename);
			}else
#endif
			ShowComment(L" �� SUSIE_PROGRESS(nNum = %d, nDenom = %d, lData = %p) �ُ�(!= %p)�A����̃`�F�b�N���X�L�b�v\r\n", nNum, nDenom, lData, sourcenameA);
			CallbackError = TRUE;
		}
	}
	return 0;
}

const WCHAR *Compressions[] = {
	L"BI_RGB", L"BI_RLE8", L"BI_RLE4", L"BI_BITFIELDS",
	L"BI_JPEG", L"BI_PNG", L"BI_ALPHABITFIELDS"
};
#ifndef BI_ALPHABITFIELDS
  #define BI_ALPHABITFIELDS 6
#endif

typedef struct {
  DWORD bV5Size;
  LONG bV5Width;
  LONG bV5Height;
  WORD bV5Planes;
  WORD bV5BitCount;
  DWORD bV5Compression;
  DWORD bV5SizeImage;
  LONG bV5XPelsPerMeter;
  LONG bV5YPelsPerMeter;
  DWORD bV5ClrUsed;
  DWORD bV5ClrImportant;
  DWORD bV5RedMask;
  DWORD bV5GreenMask;
  DWORD bV5BlueMask;
  DWORD bV5AlphaMask;
  DWORD bV5CSType;
  CIEXYZTRIPLE bV5Endpoints;
  DWORD bV5GammaRed;
  DWORD bV5GammaGreen;
  DWORD bV5GammaBlue;
  DWORD bV5Intent;
  DWORD bV5ProfileData;
  DWORD bV5ProfileSize;
  DWORD bV5Reserved;
} BITMAPV5HEADER;

void ShowPictureInfo(HLOCAL HBInfo, HLOCAL HBm)
{
	BITMAPV5HEADER *bmpinfo;
	DWORD size, bmpsize, linesize, calc_bmpsize;

	if ( HBInfo == NULL ){
		ShowComment(L" �� pHBInfo == NULL\r\n");
	}
	if ( HBm == NULL ){
		ShowComment(L" �� pHBm == NULL\r\n");
	}
	bmpinfo = (BITMAPV5HEADER *)LocalLock(HBInfo);
	size = LocalSize(HBInfo);
	bmpsize = LocalSize(HBm);
	if ( size < 40 ){
		ShowComment(L" �� HBInfo �̑傫���������� %d < 40\r\n", size );
	}else{
		const WCHAR *wptr;
		if ( bmpinfo->bV5Size == 40 ){
			printout(L" BITMAPINFO\r\n");
		}else if ( bmpinfo->bV5Size == sizeof(BITMAPV5HEADER) ){
			printout(L" BITMAPV5HEADER\r\n");
		}else if ( bmpinfo->bV5Size < sizeof(BITMAPINFO) ){
			ShowComment(L" �� BITMAPINFO biSize �������� %d < 40\r\n", bmpinfo->bV5Size );
		}else if ( bmpinfo->bV5Size < sizeof(BITMAPINFO) ){
			ShowComment(L" �� BITMAPINFO biSize �s���T�C�Y %d != %d\r\n", bmpinfo->bV5Size, sizeof(BITMAPINFO) );
		}
		printoutf(L" biWidth: %d pix   biHeight: %d pix\r\n", bmpinfo->bV5Width, bmpinfo->bV5Height );

		if ( (bmpinfo->bV5Width <= 0) || (bmpinfo->bV5Width >= 0x20000) ){
			ShowComment(L" �� biWidth ���ُ�l\r\n");
		}
		if ( (bmpinfo->bV5Height < 0) ){
			ShowComment(L" �� �g�b�v�_�E��DIB\r\n");
		}
		if ( (bmpinfo->bV5Height <= -0x20000) || (bmpinfo->bV5Height >= 0x20000) ){
			ShowComment(L" �� biHeight ���ُ�l\r\n");
		}
		if ( bmpinfo->bV5Planes != 1 ){
			ShowComment(L" �� biPlanes �� 1 �ȊO(%d)\r\n", bmpinfo->bV5Planes);
		}
		printoutf(L" biBitCount: %d bit (%d colors)\r\n", bmpinfo->bV5BitCount, 1 << bmpinfo->bV5BitCount );
		if ( !( (bmpinfo->bV5BitCount <= 1) || (bmpinfo->bV5BitCount == 4) ||
				(bmpinfo->bV5BitCount == 8) || (bmpinfo->bV5BitCount == 16) ||
				(bmpinfo->bV5BitCount == 24) || (bmpinfo->bV5BitCount == 32))){
			ShowComment(L" �� biBitCount ���ُ�l\r\n");
		}
		if ( (bmpinfo->bV5BitCount == 16) || (bmpinfo->bV5BitCount == 32) ){
			ShowComment(L" �� %d bit �F�͑Ή����Ă��Ȃ��A�v���P�[�V��������\r\n", bmpinfo->bV5BitCount);
		}
		if ( bmpinfo->bV5Compression > BI_ALPHABITFIELDS ){
			wptr = L"?";
		}else{
			wptr = Compressions[bmpinfo->bV5Compression];
		}
		printoutf(L" biCompression: %d (%s)\r\n", bmpinfo->bV5Compression, wptr);
		if ( bmpinfo->bV5Compression != BI_RGB ){
			ShowComment(L" �� BI_RGB �ȊO�͑Ή����Ă��Ȃ��A�v���P�[�V��������\r\n");
		}
		linesize = DwordBitSize(bmpinfo->bV5Width * bmpinfo->bV5BitCount);
		calc_bmpsize = linesize * ((bmpinfo->bV5Height >= 0) ? bmpinfo->bV5Height : -bmpinfo->bV5Height);

		printoutf(L" biSizeImage: %d bytes (HBm: %d bytes. calculated: %d bytes)\r\n", bmpinfo->bV5SizeImage, bmpsize, calc_bmpsize);
		if ( ((bmpinfo->bV5Compression != BI_RGB) && (bmpinfo->bV5SizeImage == 0)) ||
			 (bmpinfo->bV5SizeImage > bmpsize) ){
			ShowComment(L" �� biSizeImage ���ُ�l\r\n");
		}
		if ( ((bmpinfo->bV5Compression == BI_RGB) || (bmpinfo->bV5Compression == BI_BITFIELDS)) ){
			if ( (bmpinfo->bV5SizeImage != 0) && (bmpinfo->bV5SizeImage != calc_bmpsize) ){
				ShowComment(L" �� biSizeImage �� calculated �ƈ�v���Ȃ�\r\n");
			}
			if ( bmpsize < calc_bmpsize ){
				ShowComment(L" �� �r�b�g�}�b�v�� calculated ��菭�Ȃ�\r\n");
			}
		}
		printoutf(L" biXPelsPerMeter: %d (%d ppi) biYPelsPerMeter %d (%d ppi)\r\n",
				bmpinfo->bV5XPelsPerMeter, bmpinfo->bV5XPelsPerMeter / 39,
				bmpinfo->bV5YPelsPerMeter, bmpinfo->bV5YPelsPerMeter / 39);
		if ( bmpinfo->bV5XPelsPerMeter != bmpinfo->bV5YPelsPerMeter ){
			ShowComment(L" �� �A�X�y�N�g�䂪 1 �ȊO\r\n");
		}
		printoutf(L" biClrUsed: %d   biClrImportant %d\r\n\r\n",
				bmpinfo->bV5ClrUsed , bmpinfo->bV5ClrImportant);
		if ( bmpinfo->bV5Compression == BI_BITFIELDS ){
			printoutf(L" Bit mask R:%x G:%x B:%x  Alpha:%x\r\n",
					bmpinfo->bV5RedMask , bmpinfo->bV5GreenMask,
					bmpinfo->bV5BlueMask, bmpinfo->bV5AlphaMask);
		}
	}

	LocalUnlock(HBInfo);
	LocalUnlock(HBm);
}

void TestGetPicture_check(HLOCAL HBInfo, HLOCAL HBm)
{
	ShowPictureInfo(HBInfo, HBm);
	LocalFree(HBInfo);
	LocalFree(HBm);
}

void CheckInvaildFilename(int result)
{
	PluginResult(result);
	if ( result == SUSIEERROR_NOERROR ){
		ShowComment(L" �� �\�[�X�t�@�C���������̂ɐ���I��\r\n");
	}
}

void TestGetPictureMain(GETPICTURE TestApi)
{
	int result;
	char buf[MAX_PATH];
	HLOCAL HBInfo, HBm;

	if ( testdisk ){
		printout(L"SOURCE_DISK(non-existence)");
		strcpy(buf, "dummy_file_name");
		HBInfo = HBm = INVALID_HANDLE_VALUE;
		result = TestApi(buf, 0, SUSIE_SOURCE_DISK, &HBInfo, &HBm, (FARPROC)SusieProgressCallbackCheck, (LONG_PTR)sourcenameA);
		CheckInvaildFilename(result);

		printout(L"SOURCE_DISK");
		strcpy(buf, sourcenameA);
		HBInfo = HBm = INVALID_HANDLE_VALUE;
		result = TestApi(buf, 0, SUSIE_SOURCE_DISK, &HBInfo, &HBm, (FARPROC)SusieProgressCallbackCheck, (LONG_PTR)sourcenameA);
		PluginResult(result);
		if ( strcmp(buf, sourcenameA) != 0 ){
			ShowComment(L" �� filename �j��\r\n");
		}
		if ( result == SUSIEERROR_NOERROR ){
			TestGetPicture_check(HBInfo, HBm);
		}
	}

	if ( testmem ){
		printout(L"SOURCE_MEM");
		HBInfo = HBm = INVALID_HANDLE_VALUE;
		result = TestApi(SourceImage, SourceSize, SUSIE_SOURCE_MEM, &HBInfo, &HBm, (FARPROC)SusieProgressCallbackCheck, (LONG_PTR)sourcenameA);
		PluginResult(result);
		if ( memcmp(SourceImage, CompareSourceImage, SourceAllocSize) != 0 ){
			ShowComment(L" �� buf �j��\r\n");
		}
		if ( result == SUSIEERROR_NOERROR ){
			TestGetPicture_check(HBInfo, HBm);
		}
	}
}

void TestGetPictureWMain(GETPICTUREW TestApiW)
{
	int result;
	WCHAR buf[MAX_PATH];
	HLOCAL HBInfo, HBm;

	printout(L"SOURCE_DISK(non-existence)");
	strcpyW(buf, L"dummy_file_name");
	HBInfo = HBm = INVALID_HANDLE_VALUE;
	result = TestApiW(buf, 0, SUSIE_SOURCE_DISK, &HBInfo, &HBm, (FARPROC)SusieProgressCallbackCheck, (LONG_PTR)sourcenameA);
	CheckInvaildFilename(result);

	if ( testdisk ){
		printout(L"SOURCE_DISK");
		strcpyW(buf, sourcename);
		HBInfo = HBm = INVALID_HANDLE_VALUE;
		result = TestApiW(buf, 0, SUSIE_SOURCE_DISK, &HBInfo, &HBm, (FARPROC)SusieProgressCallbackCheck, (LONG_PTR)sourcenameA);
		PluginResult(result);
		if ( strcmpW(buf, sourcename) != 0 ){
			ShowComment(L" �� filename ���j��\r\n");
		}
		if ( result == SUSIEERROR_NOERROR ){
			TestGetPicture_check(HBInfo, HBm);
		}
	}

	if ( testmem ){
		printout(L"SOURCE_MEM");
		HBInfo = HBm = INVALID_HANDLE_VALUE;
		result = TestApiW((WCHAR *)SourceImage, SourceSize, SUSIE_SOURCE_MEM, &HBInfo, &HBm, (FARPROC)SusieProgressCallbackCheck, (LONG_PTR)sourcenameA);
		PluginResult(result);
		if ( memcmp(SourceImage, CompareSourceImage, SourceAllocSize) != 0 ){
			ShowComment(L" �� buf ���j��\r\n");
		}
		if ( result == SUSIEERROR_NOERROR ){
			TestGetPicture_check(HBInfo, HBm);
		}
	}
}

void TestGetPicture(void)
{
	if ( GetPicture != NULL ){
		PrintAPIname("GetPicture(LPCSTR buf, LONG_PTR len, unsigned int flag, HLOCAL *pHBInfo, HLOCAL *pHBm, SUSIE_PROGRESS lpPrgressCallback, LONG_PTR lData)");
		TestGetPictureMain(GetPicture);
	}else if ( type_extract == FALSE ){
		ShowComment(L" �� GetPicture ���Ȃ�\r\n");
	}

	if ( UseUNICODE && (GetPictureW != NULL) ){
		PrintAPIname("GetPictureW(LPCWSTR buf, LONG_PTR len, unsigned int flag, HLOCAL *pHBInfo, HLOCAL *pHBm, SUSIE_PROGRESS lpPrgressCallback, LONG_PTR lData)");
		TestGetPictureWMain(GetPictureW);
	}
}

void TestGetPreview(void)
{
	if ( GetPreview != NULL ){
		PrintAPIname("GetPreview(LPCSTR buf, LONG_PTR len, unsigned int flag, HLOCAL *pHBInfo, HLOCAL *pHBm, SUSIE_PROGRESS lpPrgressCallback, LONG_PTR lData)");
		TestGetPictureMain(GetPreview);
	}else if ( type_extract == FALSE ){
		ShowComment(L" �� GetPreview ���Ȃ�\r\n");
	}

	if ( UseUNICODE && (GetPreviewW != NULL) ){
		PrintAPIname("GetPreviewW(LPCWSTR buf, LONG_PTR len, unsigned int flag, HLOCAL *pHBInfo, HLOCAL *pHBm, SUSIE_PROGRESS lpPrgressCallback, LONG_PTR lData)");
		TestGetPictureWMain(GetPreviewW);
	}
}

void TestGetPictureInfo_check(struct PictureInfo *pinfo)
{
	printoutf(L" (left, top) = (%d, %d)\r\n", pinfo->left, pinfo->top);
	printoutf(L" (width, height) = (%d, %d)\r\n", pinfo->width, pinfo->height);
	printoutf(L" x_density: %d ppi y_density %d ppi\r\n",
			pinfo->x_density, pinfo->y_density);
	printoutf(L" colorDepth: %d bit (%d colors)\r\n", pinfo->colorDepth, 1 << pinfo->colorDepth );

	if ( pinfo->left == WRITECHECK_DWORD ){
		ShowComment(L" �� left ���ύX����Ă��Ȃ�\r\n");
	}
	if ( pinfo->top == WRITECHECK_DWORD ){
		ShowComment(L" �� top ���ύX����Ă��Ȃ�\r\n");
	}
	if ( pinfo->width == WRITECHECK_DWORD ){
		ShowComment(L" �� width ���ύX����Ă��Ȃ�\r\n");
	}
	if ( pinfo->height == WRITECHECK_DWORD ){
		ShowComment(L" �� height ���ύX����Ă��Ȃ�\r\n");
	}
	if ( pinfo->x_density == WRITECHECK_DWORD ){
		ShowComment(L" �� x_density ���ύX����Ă��Ȃ�\r\n");
	}
	if ( pinfo->y_density == WRITECHECK_DWORD ){
		ShowComment(L" �� y_density ���ύX����Ă��Ȃ�\r\n");
	}
	if ( pinfo->colorDepth == WRITECHECK_DWORD ){
		ShowComment(L" �� colorDepth ���ύX����Ă��Ȃ�\r\n");
	}

#ifdef _WIN64
	if ( ((LONG_PTR)pinfo->hInfo & WRITECHECK_QMASK) == ((LONG_PTR)WRITECHECK_QTOP) ){
		ShowComment(L" �� hInfo �̃A���C�������g������Ă���(hInfo = %p)\r\n", pinfo->hInfo);
	}else if ( pinfo->hInfo == (HANDLE)WRITECHECK_QWORD )
#else
	if ( pinfo->hInfo == (HANDLE)WRITECHECK_DWORD )
#endif
	{
		ShowComment(L" �� hInfo ���ύX����Ă��Ȃ�(hInfo = %p)\r\n", pinfo->hInfo);
	}else if ( pinfo->hInfo == NULL ){
		printout(L" hInfo: NULL\r\n");
	}else{
		char *info, *infolast;
		DWORD size;

		info = (char *)LocalLock(pinfo->hInfo);
		size = LocalSize(pinfo->hInfo);
		printoutf(L" hInfo: %d bytes\r\n", size);
		if ( size == 0 ){
			ShowComment(L" �� hInfo �̑傫���� 0 \r\n");
		}else{
			WCHAR *infoW;
			DWORD sizeW;
			UINT codepage;

			infolast = (char *)memchr(info, 0, size);
			if ( infolast == NULL ){
				ShowComment(L" �� hInfo �� \0 �I�[�łȂ� \r\n");
				info[size - 1] = '\0';
			}

			if ( (size >= 4) && (memcmp(info, UTF8HEADER, UTF8HEADERSIZE) == 0) ){
				info += UTF8HEADERSIZE;
				size -= UTF8HEADERSIZE;
				printout(L"   (utf-8): ");
				codepage = CP_UTF8;
			}else{
				printout(L"   (ShiftJIS): ");
				codepage = CP_ACP;
			}

			sizeW = MultiByteToWideChar(codepage, 0, info, size, NULL, 0);
			infoW = (WCHAR *)malloc(sizeW);
			if ( infoW != NULL ){
				MultiByteToWideChar(codepage, 0, info, size, infoW, sizeW);
				printout(infoW);
				free(infoW);
			}else{
				printout(L"memory error");
			}
		}
		LocalUnlock(pinfo->hInfo);
		LocalFree(pinfo->hInfo);
	}
}

void TestGetPictureInfo(void)
{
	struct PictureInfo pinfo;

	if ( GetPictureInfo != NULL ){
		int result;
		char buf[MAX_PATH];

		PrintAPIname("GetPictureInfo(LPCSTR buf, LONG_PTR len, unsigned int flag, struct PictureInfo *lpInfo)");

		if ( testdisk ){
			printout(L"SOURCE_DISK(non-existence)");
			strcpy(buf, "dummy_file_name");
			result = GetPictureInfo(buf, 0, SUSIE_SOURCE_DISK, &pinfo);
			CheckInvaildFilename(result);

			printout(L"SOURCE_DISK");
			strcpy(buf, sourcenameA);
			memset(&pinfo, WRITECHECK_BYTE, sizeof(pinfo));
			result = GetPictureInfo(buf, 0, SUSIE_SOURCE_DISK, &pinfo);
			if ( strcmp(buf, sourcenameA) != 0 ){
				ShowComment(L" �� filename �j��\r\n");
			}
			if ( result == SUSIEERROR_NOERROR ){
				TestGetPictureInfo_check(&pinfo);
			}
		}

		if ( testmem ){
			printout(L"SOURCE_MEM");
			result = GetPictureInfo(SourceImage, SourceSize, SUSIE_SOURCE_MEM, &pinfo);
			PluginResult(result);
			if ( memcmp(SourceImage, CompareSourceImage, SourceAllocSize) != 0 ){
				ShowComment(L" �� buf �j��\r\n");
			}
			if ( result == SUSIEERROR_NOERROR ){
				TestGetPictureInfo_check(&pinfo);
			}
		}
	}

	if ( UseUNICODE && (GetPictureInfoW != NULL) ){
		int result;
		WCHAR buf[MAX_PATH];

		PrintAPIname("GetPictureInfoW(LPCWSTR buf, LONG_PTR len, unsigned int flag, struct PictureInfo *lpInfo)");

		if ( testdisk ){
			printout(L"SOURCE_DISK(non-existence)");
			strcpyW(buf, L"dummy_file_name");
			result = GetPictureInfoW(buf, 0, SUSIE_SOURCE_DISK, &pinfo);
			CheckInvaildFilename(result);

			printout(L"SOURCE_DISK");
			strcpyW(buf, sourcename);
			memset(&pinfo, WRITECHECK_BYTE, sizeof(pinfo));
			result = GetPictureInfoW(buf, 0, SUSIE_SOURCE_DISK, &pinfo);
			PluginResult(result);
			if ( strcmpW(buf, sourcename) != 0 ){
				ShowComment(L" �� filename �j��\r\n");
			}
			if ( result == SUSIEERROR_NOERROR ){
				TestGetPictureInfo_check(&pinfo);
			}
		}

		if ( testmem ){
			printout(L"SOURCE_MEM");
			result = GetPictureInfoW((WCHAR *)SourceImage, SourceSize, SUSIE_SOURCE_MEM, &pinfo);
			PluginResult(result);
			if ( memcmp(SourceImage, CompareSourceImage, SourceAllocSize) != 0 ){
				ShowComment(L" �� buf �j��\r\n");
			}
			if ( result == SUSIEERROR_NOERROR ){
				TestGetPictureInfo_check(&pinfo);
			}
		}
	}
}

void TestGetPluginInfo_noinfo(int infono)
{
	if ( infono < 4 ){
		ShowComment(L" �� infono=%d: infono �� 0-3 �܂ŕK�{\r\n", infono);
	}
	if ( (infono > 2) && (infono & 1) ){
		ShowComment(L" �� infono=%d: 2n+1 ������̂� 2n+2 ���Ȃ�\r\n", infono);
	}
}

void TestGetPluginInfo(void)
{
	BOOL unicode = FALSE;
	int infono;

	for (;; unicode = TRUE){
		if ( unicode == FALSE ){ // ansi
			if ( GetPluginInfo == NULL ){
				ShowComment(L" �� GetPluginInfo ���Ȃ�\r\n");
				continue;
			}
			PrintAPIname("GetPluginInfo(int infono, LPSTR buf, int buflen)");
		}else{ // unicode
			if ( !UseUNICODE || (GetPluginInfoW == NULL) ) break;

			PrintAPIname("GetPluginInfoW(int infono, LPWSTR buf, int buflen)");
		}

		for ( infono = 0; ; infono++ ){
			#define INFOBUFLEN 512
			#define INFOBUFCHECKLEN 500
			char bufA[INFOBUFLEN];
			WCHAR bufW[INFOBUFLEN], cbuf[32], *memo;
			int result, result_checklen, result_susielen;

			if ( infono >= 2 ){
				result_susielen = 255;
			}else if ( infono == 1 ){
				result_susielen = 128;
			}else{
				result_susielen = 8;
			}

			if ( unicode == FALSE ){
				if ( infono >= 1 ){
					memset(bufA, WRITECHECK_BYTE, sizeof(bufA));
					result = GetPluginInfo(infono, bufA, 4);
					bufA[5] = '\0';
					if ( result != 0 ){
						if ( bufA[4] != WRITECHECK_BYTE ){
							ShowComment(L" �� infono=%d: buflen = 4 �̂Ƃ��Abuf[4] ���z���ď�������\r\n", infono);
						}else if ( strlen(bufA) > 4 ){
							ShowComment(L" �� infono=%d: �؂�̂Ď��A������ \\0 �łȂ�\r\n", infono);
						}
					}
				}

				memset(bufA, WRITECHECK_BYTE, sizeof(bufA));
				result = GetPluginInfo(infono, bufA, INFOBUFCHECKLEN);
				if ( result == 0 ){
					TestGetPluginInfo_noinfo(infono);
					break;
				}
				bufA[INFOBUFLEN - 1] = '\0';
				result_checklen = strlen(bufA);
				AnsiToUnicode(bufA, bufW, INFOBUFCHECKLEN);
			}else{
				if ( infono >= 1 ){
					memset(bufW, WRITECHECK_BYTE, sizeof(bufW));
					result = GetPluginInfoW(infono, bufW, 4);
					bufW[5] = '\0';
					if ( result != 0 ){
						if ( bufW[4] != WRITECHECK_WCHAR ){
							ShowComment(L" �� infono=%d: buflen = 4 �̂Ƃ��Abuf[4] ���z���ď�������\r\n", infono);
						}else if ( strlenW(bufW) > 4 ){
							ShowComment(L" �� infono=%d: �؂�̂Ď��A������ \\0 �łȂ�\r\n", infono);
						}
					}
				}

				memset(bufW, WRITECHECK_BYTE, sizeof(bufW));
				bufW[0] = '\0';
				result = GetPluginInfoW(infono, bufW, INFOBUFCHECKLEN);
				if ( result == 0 ){
					TestGetPluginInfo_noinfo(infono);
					break;
				}
				bufW[INFOBUFLEN - 1] = '\0';
				result_checklen = strlenW(bufW);
			}

			if ( result > INFOBUFCHECKLEN ){
				ShowComment(L" �� infono=%d: �߂�l(%d)���w��ő�l(%d)��\r\n", infono, result, INFOBUFCHECKLEN);
				result = INFOBUFCHECKLEN - 1;
			}
			if ( result_checklen > result ){
				ShowComment(L" �� infono=%d: buf �̖����� \\0 �łȂ�\r\n", infono);
			}
			if ( result_checklen >= result_susielen ){
				ShowComment(L" �� infono=%d: Susie(%d)�ł͈����Ȃ�����\r\n", infono, result_susielen);
			}

			memo = L"";
			if ( infono == 0 ){
				if ( strcmpW(bufW, L"00AM") == 0 ){
					type_extract = TRUE;
					memo = L" (Export filter)";
				}else if ( strcmpW(bufW, L"00IN") == 0 ){
					memo = L" (Import filter)";
				}else if ( strcmpW(bufW, L"T0XN") == 0 ){
					memo = L" (AtoB Converter export Plug-in)";
				}else{
					ShowComment(L" �� �s���v���O�C���o�[�W����\r\n");
				}
				strcpyW(cbuf, L"type");
			}else if ( infono == 1 ){
				strcpyW(cbuf, L"name");
			}else if ( infono & 1 ){
				wsprintfW(cbuf, L"name #%d", infono / 2);
			}else{
				wsprintfW(cbuf, L"ext  #%d", infono / 2);
			}
			printoutf(L" %d(%s): %s%s\r\n", infono, cbuf, bufW, memo);

			if ( result != result_checklen ){
				ShowComment(L" �� infono=%d: �߂�l:%d != ������:%d\r\n", infono, result, result_checklen);
			}
			if ( (infono >= 2) && ((infono & 1) == 0) ){
				if ( (bufW[0] == '.') || (strstrW(bufW, L";.") != NULL) ){
					ShowComment(L" �� infono=%d: �Ή��g���q���u*.ext�v�`���Ŗ���\r\n", infono);
				}
			}
		}
		if ( unicode ) break;
	}
}

#define IsSupportedT(unicode, nameA, nameW, dw) (!unicode ? IsSupported(nameA, dw) : IsSupportedW(nameW, dw))
int PrintSupport(int result)
{
	printoutf(L" %d (%s)\r\n", result, result ? L"support" : L"not support");
	return result;
}

void TestIsSupported(void)
{
	char header[SUSIE_CHECK_SIZE + 16], backup_header[SUSIE_CHECK_SIZE + 16];
	char dummy_header[SUSIE_CHECK_SIZE + 16];
	HANDLE hFile;
	BOOL unicode = FALSE;

	int result;
	char bufA[MAX_PATH];
	WCHAR bufW[MAX_PATH];

	if ( sourcename[0] == '\0' ) return;
	hFile = OpenFileHeader(header);
	if ( hFile == INVALID_HANDLE_VALUE ) return;
	memcpy(backup_header, header, sizeof(backup_header));
	memset(dummy_header, WRITECHECK_BYTE, sizeof(dummy_header));

	for (;; unicode = TRUE){
		if ( unicode == FALSE ){ // ansi
			if ( IsSupported == NULL ){
				ShowComment(L" �� IsSupported �����݂��Ȃ�\r\n");
				continue;
			}

			PrintAPIname("IsSupported(LPCSTR filename, void *dw)");
			strcpy(bufA, "non-existence");
		}else{ // unicode
			if ( !UseUNICODE || (IsSupportedW == NULL) ) break;

			PrintAPIname("IsSupportedW(LPCSTR filename, void *dw)");
			strcpyW(bufW, L"non-existence");
		}
		printout(L" filename is memory(dummy data):");
		result = PrintSupport(IsSupportedT(unicode, bufA, bufW, dummy_header));
		if ( result ) ShowComment(L" �� ���݂��Ȃ��t�@�C���ł��Ή�����\r\n");

		printoutf(L" filename is file handle(bad handle):");
		result = PrintSupport(IsSupportedT(unicode, bufA, bufW, (void *)WRITECHECK_BYTE));
		if ( result ) ShowComment(L" �� ���݂��Ȃ��n���h���ł��Ή�����\r\n");

		if ( unicode == FALSE ){ // ansi
			strcpy(bufA, sourcenameA);
		}else{ // unicode
			strcpyW(bufW, sourcename);
		}

		printout(L" filename is memory:     ");
		PrintSupport(IsSupportedT(unicode, bufA, bufW, header));

		if ( (!unicode ? strcmp(bufA, sourcenameA) : strcmpW(bufW, sourcename)) != 0 ){
			ShowComment(L" �� filename �j��\r\n");
		}
		if ( memcmp(header, backup_header, sizeof(header)) != 0 ){
			ShowComment(L" �� dw �j��\r\n");
		}
		{
			hFile = CreateFileW(sourcename, GENERIC_READ,
					FILE_SHARE_WRITE | FILE_SHARE_READ, NULL,
					OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
			if ( hFile != INVALID_HANDLE_VALUE ){
				if ( (LONG_PTR)hFile < 0x10000 ){
					printout(L" filename is file handle:");
					PrintSupport(IsSupportedT(unicode, bufA, bufW, hFile));
				}else{
					printout(L"* �t�@�C���n���h���̃e�X�g���ł��Ȃ�(over 16bit).\r\n");
				}
				CloseHandle(hFile);
			}
		}
		if ( unicode ) break;
	}
}

void TestGetArchiveInfo_SUSIE_FINFO(SUSIE_FINFO *finfo, UINT infosize)
{
	UINT maxcount;
	UINT count = 0;
	WCHAR timebuf[32];

	maxcount = (infosize + sizeof(SUSIE_FINFO) - 1) / sizeof(SUSIE_FINFO);
	if ( infosize < 1 ){
		ShowComment(L" �� lphInf �̑傫�������������߁A�t�@�C�����̏I�������o�ł��Ȃ�\r\n");
	}
	for ( ; count < maxcount ;){
		if ( finfo->method[0] == '\0' ){
			if ( (count + 1) != maxcount ){
				printoutf(L"�� �m�ۂ���������(%d)���t�@�C�����̐�(%dx%d=%d)�����Ȃ�\r\n", infosize, count, sizeof(SUSIE_FINFO), count * sizeof(SUSIE_FINFO));
			}
			break;
		}
		printoutf(L"-#%d-method: %hs\r\n", count, finfo->method);
		printoutf(L" #%d position: %lu\r\n", count, finfo->position);
		printoutf(L" #%d compsize: %lu\r\n", count, finfo->compsize);
		printoutf(L" #%d filesize: %lu\r\n", count, finfo->filesize);
		printoutf(L" #%d timestamp: %s\r\n", count, GetTimeStrings(timebuf, finfo->timestamp));
		printoutf(L" #%d path: '%hs'\r\n", count, finfo->path);
		printoutf(L" #%d filename: '%hs'\r\n", count, finfo->filename);
		printoutf(L" #%d crc: %u\r\n", count, finfo->crc);

		finfo++;
		count++;
		if ( count == maxcount ){
			ShowComment(L" �� �Ō�̃t�@�C����� method[0] == '\\0' �łȂ�\r\n");
		}
	}
	printoutf(L" %d Entries\r\n", count);
}

void Compare_SUSIE_FINFO_one(SUSIE_FINFO *finfo1, SUSIE_FINFO *finfo2, int count)
{
	if ( strcmp((const char *)finfo1->method, (const char *)finfo2->method) != 0 ){
		ShowComment(L" �� #%d method �s��v(%hs != %hs)\r\n", count, finfo1->method, finfo2->method);
	}
	if ( finfo1->position != finfo2->position ){
		ShowComment(L" �� #%d position �s��v(%d != %d)\r\n", count, finfo1->position, finfo2->position);
	}
	if ( finfo1->compsize != finfo2->compsize ){
		ShowComment(L" �� #%d compsize �s��v\r\n", count);
	}
	if ( finfo1->filesize != finfo2->filesize ){
		ShowComment(L" �� #%d filesize �s��v(%d != %d)\r\n", count, finfo1->filesize, finfo2->filesize);
	}
	if ( finfo1->timestamp != finfo2->timestamp ){
		ShowComment(L" �� #%d timestamp �s��v\r\n", count);
	}
	if ( strcmp(finfo1->path, finfo2->path) != 0 ){
		ShowComment(L" �� #%d path �s��v\r\n", count);
	}
	if ( strcmp(finfo1->filename, finfo2->filename) != 0 ){
		ShowComment(L" �� #%d filename �s��v(%hs != %hs)\r\n", count, finfo1->filename, finfo2->filename);
	}
	if ( finfo1->crc != finfo2->crc ){
		ShowComment(L" �� #%d crc �s��v\r\n", count);
	}
}

void Compare_SUSIE_FINFO(SUSIE_FINFO *finfo1, SUSIE_FINFO *finfo2, UINT infosize)
{
	UINT maxcount;
	UINT count = 0;
	int OldCommentCount = CommentCount;

	printout(L" Compare SUSIE_FINFO file <-> mem\r\n");
	maxcount = (infosize + sizeof(SUSIE_FINFO) - 1) / sizeof(SUSIE_FINFO);
	for ( ; count < maxcount ; finfo1++, finfo2++, count++ ){
		if ( (finfo1->method[0] == '\0') || (finfo2->method[0] == '\0') ){
			if ( (finfo1->method[0] != '\0') || (finfo2->method[0] != '\0') ){
				printout(L"X\r\n");
				ShowComment(L" �� �\�[�X�� memory �� file �ŏ�񐔂��قȂ�\r\n");
			}else{
				printout(L".");
			}
			break;
		}
		Compare_SUSIE_FINFO_one(finfo1, finfo2, count);
	}
	printout(L"\r\n");
	if ( CommentCount == OldCommentCount )  printout(L" -> All match\r\n");
}

void TestGetArchiveInfo_SUSIE_FINFOW(SUSIE_FINFOW *finfo, UINT infosize)
{
	UINT maxcount;
	UINT count = 0;
	WCHAR timebuf[32];

	maxcount = (infosize + sizeof(SUSIE_FINFOW) - 1) / sizeof(SUSIE_FINFOW);
	if ( infosize < 1 ){
		ShowComment(L" �� lphInf �̑傫�������������߁A�t�@�C�����̏I�������o�ł��Ȃ�\r\n");
	}
	for ( ; count < maxcount ;){
		if ( finfo->method[0] == '\0' ){
			if ( (count + 1) != maxcount ){
				ShowComment(L"�� �m�ۂ���������(%d)���t�@�C�����̐�(%dx%d=%d)�����Ȃ�\r\n", infosize, count, sizeof(SUSIE_FINFO), count * sizeof(SUSIE_FINFO));
			}
			break;
		}
		printoutf(L"-#%d-method: %hs\r\n", count, finfo->method);
		printoutf(L" #%d position: %lu\r\n", count, finfo->position);
		printoutf(L" #%d compsize: %lu\r\n", count, finfo->compsize);
		printoutf(L" #%d filesize: %lu\r\n", count, finfo->filesize);
		printoutf(L" #%d timestamp: %s\r\n", count, GetTimeStrings(timebuf, finfo->timestamp));
		printoutf(L" #%d path: '%s'\r\n", count, finfo->path);
		printoutf(L" #%d filename: '%s'\r\n", count, finfo->filename);
		printoutf(L" #%d crc: %u\r\n", count, finfo->crc);

		finfo++;
		count++;
		if ( count == maxcount ){
			ShowComment(L" �� �Ō�̃t�@�C����� method[0] == '\\0' �łȂ�\r\n");
		}
	}
	printoutf(L" %d Entries\r\n", count);
}

void Compare_SUSIE_FINFOW_one(SUSIE_FINFOW *finfo1, SUSIE_FINFOW *finfo2, int count)
{
	if ( strcmp((const char *)finfo1->method, (const char *)finfo2->method) != 0 ){
		ShowComment(L" �� #%d method �s��v(%S != %S)\r\n", count, finfo1->method, finfo2->method);
	}
	if ( finfo1->position != finfo2->position ){
		ShowComment(L" �� #%d position �s��v(%d != %d)\r\n", count, finfo1->position, finfo2->position);
	}
	if ( finfo1->compsize != finfo2->compsize ){
		ShowComment(L" �� #%d compsize �s��v(%d != %d)\r\n", count, finfo1->compsize, finfo2->compsize);
	}
	if ( finfo1->filesize != finfo2->filesize ){
		ShowComment(L" �� #%d filesize �s��v(%d != %d)\r\n", count, finfo1->filesize, finfo2->filesize);
	}
	if ( finfo1->timestamp != finfo2->timestamp ){
		ShowComment(L" �� #%d timestamp �s��v\r\n", count);
	}
	if ( strcmpW(finfo1->path, finfo2->path) != 0 ){
		ShowComment(L" �� #%d path �s��v(%s != %s)\r\n", count, finfo1->path, finfo2->path);
	}
	if ( strcmpW(finfo1->filename, finfo2->filename) != 0 ){
		ShowComment(L" �� #%d filename �s��v(%s != %s)\r\n", count, finfo1->filename, finfo2->filename);
	}
	if ( finfo1->crc != finfo2->crc ){
		ShowComment(L" �� #%d crc �s��v\r\n", count);
	}
}

void Compare_SUSIE_FINFOW(SUSIE_FINFOW *finfo1, SUSIE_FINFOW *finfo2, UINT infosize)
{
	UINT maxcount;
	UINT count = 0;
	int OldCommentCount = CommentCount;

	printout(L" Compare SUSIE_FINFOW file <-> mem\r\n");
	maxcount = (infosize + sizeof(SUSIE_FINFOW) - 1) / sizeof(SUSIE_FINFOW);
	for ( ; count < maxcount ; finfo1++, finfo2++, count++ ){
		if ( finfo1->method[0] == '\0' ){
			if ( finfo2->method[0] != '\0' ){
				ShowComment(L" �� �\�[�X�� memory �� file �ŏ�񐔂��قȂ�\r\n");
			}
			break;
		}
		Compare_SUSIE_FINFOW_one(finfo1, finfo2, count);
	}
	if ( CommentCount == OldCommentCount )  printout(L" -> All match\r\n");
}

void Compare_SUSIE_FINFO_AW(SUSIE_FINFO *finfo1, SUSIE_FINFOW *finfo2, UINT info1size)
{
	UINT maxcount;
	UINT count = 0;
	int OldCommentCount = CommentCount;
	WCHAR text[SUSIE_PATH_MAX];

	printout(L" Compare SUSIE_FINFO with SUSIE_FINFOW\r\n");
	maxcount = (info1size + sizeof(SUSIE_FINFO) - 1) / sizeof(SUSIE_FINFO);
	for ( ; count < maxcount ; finfo1++, finfo2++, count++ ){
		printout(L".");
		if ( finfo1->method[0] == '\0' ){
			if ( finfo2->method[0] != '\0' ){
				ShowComment(L" �� �\�[�X�� memory �� file �ŏ�񐔂��قȂ�\r\n");
			}
			break;
		}
		if ( finfo1->position != finfo2->position ){
			ShowComment(L" �� #%d position �s��v(%d != %d)\r\n", count, finfo1->position, finfo2->position);
		}
		if ( finfo1->compsize != finfo2->compsize ){
			ShowComment(L" �� #%d compsize �s��v(%d != %d)\r\n", count, finfo1->compsize, finfo2->compsize);
		}
		if ( finfo1->filesize != finfo2->filesize ){
			ShowComment(L" �� #%d filesize �s��v(%d != %d)\r\n", count, finfo1->filesize, finfo2->filesize);
		}
		if ( finfo1->timestamp != finfo2->timestamp ){
			ShowComment(L" �� #%d timestamp �s��v(%d != %d)\r\n", count, finfo1->timestamp, finfo2->timestamp);
		}
		AnsiToUnicode(finfo1->path, text, SUSIE_PATH_MAX);
		if ( strcmpW(text, finfo2->path) != 0 ){
			ShowComment(L" �� #%d path �s��v(%s != %s)\r\n", count, text, finfo2->path);
		}
		AnsiToUnicode(finfo1->filename, text, SUSIE_PATH_MAX);
		if ( strcmpW(text, finfo2->filename) != 0 ){
			ShowComment(L" �� #%d filename �s��v(%s != %s)\r\n", count, text, finfo2->filename);
		}
		if ( finfo1->crc != finfo2->crc ){
			ShowComment(L" �� #%d crc �s��v(%d, %d)\r\n", count, finfo1->crc, finfo2->crc);
		}
	}
	printout(L"X\r\n");
	if ( CommentCount == OldCommentCount )  printout(L" -> All match\r\n");
}

void TestGetArchiveInfo(void)
{
	if ( sourcename[0] == '\0' ) return;

	if ( GetArchiveInfo != NULL ){
		int result;
		char buf[MAX_PATH];
		HLOCAL hInfFile, hInfMem;

		PrintAPIname("GetArchiveInfo(LPCSTR buf, LONG_PTR len, unsigned int flag, HLOCAL *lphInf)");

		if ( testdisk ){
			printout(L"SOURCE_DISK(non-existence)");
			strcpy(buf, "dummy_file_name");
			hInfFile = INVALID_HANDLE_VALUE;
			result = GetArchiveInfo(buf, 0, SUSIE_SOURCE_DISK, &hInfFile);
			CheckInvaildFilename(result);
			if ( hInfFile != NULL ){
				ShowComment(L" �� �\�[�X�t�@�C���������Ƃ��� *lphInf = NULL �ɂ���K�v������\r\n");
			}

			printout(L"SOURCE_DISK(non-existence)");
			strcpy(buf, sourcenameA);
			hInfFile = INVALID_HANDLE_VALUE;
			result = GetArchiveInfo(buf, 0, SUSIE_SOURCE_DISK, &hInfFile);
			PluginResult(result);
			if ( strcmp(buf, sourcenameA) != 0 ){
				ShowComment(L" �� filename �j��\r\n");
			}
			if ( result != SUSIEERROR_NOERROR ){
				if ( hInfFile != NULL ){
					ShowComment(L" �� �G���[�̂Ƃ��� *lphInf = NULL �ɂ���K�v������\r\n");
				}
			}else{
				if ( hInfFile == NULL ){
					ShowComment(L" �� ����I���Ȃ̂� *lphInf == NULL\r\n");
				}else{
					ArchiveInfo = hInfFile;
					finfo_file = (SUSIE_FINFO *)LocalLock(hInfFile);
					infosize_file = LocalSize(hInfFile);
					TestGetArchiveInfo_SUSIE_FINFO(finfo_file, infosize_file);
				}
			}
		}

		if ( testmem ){
			if ( finfo_file != NULL ) printout(L"\r\n");
			printout(L"SOURCE_MEM");
			result = GetArchiveInfo(SourceImage, SourceSize, SUSIE_SOURCE_MEM, &hInfMem);
			PluginResult(result);
			if ( memcmp(SourceImage, CompareSourceImage, SourceAllocSize) != 0 ){
				ShowComment(L" �� buf �j��\r\n");
			}
			if ( result != SUSIEERROR_NOERROR ){
				if ( hInfMem != NULL ){
					ShowComment(L" �� �G���[�̂Ƃ��� *lphInf = NULL �ɂ���K�v������\r\n");
				}
			}else{
				if ( hInfMem == NULL ){
					ShowComment(L" �� ����I���Ȃ̂� *lphInf == NULL\r\n");
				}else{
					if ( ArchiveInfo == NULL ) ArchiveInfo = hInfMem;
					finfo_mem = (SUSIE_FINFO *)LocalLock(hInfMem);
					infosize_mem = LocalSize(hInfFile);
					if ( finfo_file == NULL ){
						TestGetArchiveInfo_SUSIE_FINFO(finfo_mem, infosize_mem);
						finfo_file = finfo_mem;
						infosize_file = infosize_mem;
					}else{
						Compare_SUSIE_FINFO(finfo_file, finfo_mem, infosize_mem);
						LocalUnlock(hInfMem);
						LocalFree(hInfMem);
					}
				}
			}
		}
	}
	if ( UseUNICODE && (GetArchiveInfoW != NULL) ){
		int result;
		WCHAR buf[MAX_PATH];
		HLOCAL hInfFile, hInfMem;
		UINT infosize_fileW, infosize_memW;

		PrintAPIname("GetArchiveInfoW(LPCWSTR buf, LONG_PTR len, unsigned int flag, HLOCAL *lphInf)");

		if ( testdisk ){
			printout(L"SOURCE_DISK(non-existence)");
			strcpyW(buf, L"dummy_file_name");
			hInfFile = INVALID_HANDLE_VALUE;
			result = GetArchiveInfoW(buf, 0, SUSIE_SOURCE_DISK, &hInfFile);
			CheckInvaildFilename(result);
			if ( hInfFile != NULL ){
				ShowComment(L" �� �\�[�X�t�@�C���������Ƃ��� *lphInf = NULL �ɂ���K�v������\r\n");
			}

			printout(L"SOURCE_DISK");
			strcpyW(buf, sourcename);
			hInfFile = INVALID_HANDLE_VALUE;
			result = GetArchiveInfoW(buf, 0, SUSIE_SOURCE_DISK, &hInfFile);
			PluginResult(result);
			if ( strcmpW(buf, sourcename) != 0 ){
				ShowComment(L" �� filename �j��\r\n");
			}
			if ( result != SUSIEERROR_NOERROR ){
				if ( hInfFile != NULL ){
					ShowComment(L" �� �G���[�̂Ƃ��� *lphInf = NULL �ɂ���K�v������\r\n");
				}
			}else{
				if ( hInfFile == NULL ){
					ShowComment(L" �� ����I���Ȃ̂� *lphInf == NULL\r\n");
				}else{
					ArchiveInfoW = hInfFile;
					finfo_fileW = (SUSIE_FINFOW *)LocalLock(hInfFile);
					infosize_fileW = LocalSize(hInfFile);
					TestGetArchiveInfo_SUSIE_FINFOW(finfo_fileW, infosize_fileW);
				}
			}
		}

		if ( testmem ){
			if ( finfo_file != NULL ) printout(L"\r\n");
			printout(L"SOURCE_MEM");
			result = GetArchiveInfoW((LPCWSTR)SourceImage, SourceSize, SUSIE_SOURCE_MEM, &hInfMem);
			PluginResult(result);
			if ( memcmp(SourceImage, CompareSourceImage, SourceAllocSize) != 0 ){
				ShowComment(L" �� buf �j��\r\n");
			}
			if ( result != SUSIEERROR_NOERROR ){
				if ( hInfMem != NULL ){
					ShowComment(L" �� �G���[�̂Ƃ��� *lphInf = NULL �ɂ���K�v������\r\n");
				}
			}else{
				if ( hInfMem == NULL ){
					ShowComment(L" �� ����I���Ȃ̂� *lphInf == NULL\r\n");
				}else{
					if ( ArchiveInfoW == NULL ) ArchiveInfoW = hInfMem;
					finfo_memW = (SUSIE_FINFOW *)LocalLock(hInfMem);
					infosize_memW = LocalSize(hInfFile);
					if ( finfo_file == NULL ){
						TestGetArchiveInfo_SUSIE_FINFOW(finfo_memW, infosize_memW);
						finfo_fileW = finfo_memW;
						infosize_file = infosize_memW;
					}else{
						Compare_SUSIE_FINFOW(finfo_fileW, finfo_memW, infosize_memW);
						LocalUnlock(hInfMem);
						LocalFree(hInfMem);
					}
				}
			}
		}
		if ( (finfo_file != NULL) && (finfo_fileW != NULL) ){
			Compare_SUSIE_FINFO_AW(finfo_file, finfo_fileW, infosize_mem);
		}
	}
}

void TestGetFileInfo(void)
{
	UINT maxcount;
	UINT count;
	int OldCommentCount;

	if ( type_extract == FALSE ) return;
	if ( infosize_file == NULL ) return;
	maxcount = (infosize_file + sizeof(SUSIE_FINFO) - 1) / sizeof(SUSIE_FINFO);

	if ( GetFileInfo != NULL ){
		int result_file, result_mem;
		int check_file = 0, check_mem = 0;
		char textA[SUSIE_PATH_MAX * 2];
		SUSIE_FINFO *finfoAptr, finfoA;

		PrintAPIname("GetFileInfo(LPCSTR buf, LONG_PTR len, LPCSTR filename, unsigned int flag, SUSIE_FINFO *lpInfo)");

		if ( finfo_file == NULL ){
			ShowComment(L" �� GetArchiveInfo���������Ă��Ȃ��̂Ńe�X�g�ł��Ȃ�\r\n");
		}else{
			printout(L" Compare GetFileInfo with GetArchiveInfo\r\n");
			OldCommentCount = CommentCount;
			finfoAptr = finfo_file;
			for ( count = 0; count < maxcount ; finfoAptr++, count++ ){
				if ( finfoAptr->method[0] == '\0' ) break;
				strcpy(textA, finfoAptr->path);
				// �������u�\�v�̍l�������Ă��Ȃ�
				if ( (textA[0] != '\0') && (textA[strlen(textA) - 1] != '\\') ){
					strcat(textA, "\\");
				}
				strcat(textA, finfoAptr->filename);
				if ( testdisk ){
					memset(&finfoA, WRITECHECK_BYTE, sizeof(finfoA));
					result_file = GetFileInfo(sourcenameA, 0, textA, SUSIE_SOURCE_DISK, &finfoA);
					if ( result_file == SUSIEERROR_NOERROR ){
						printout(L"d");
						Compare_SUSIE_FINFO_one(&finfoA, finfoAptr, count);
						check_file++;
					}
				}
				if ( testmem ){
					memset(&finfoA, WRITECHECK_BYTE, sizeof(finfoA));
					result_mem = GetFileInfo(SourceImage, SourceSize, textA, SUSIE_SOURCE_MEM, &finfoA);
					if ( result_mem == SUSIEERROR_NOERROR ){
						printout(L"m");
						Compare_SUSIE_FINFO_one(&finfoA, finfoAptr, count);
						check_mem++;
					}else if ( result_file != SUSIEERROR_NOERROR ){
						ShowComment(L"\r\n �� #%d(%hs) �擾�s��(%d)\r\n", count, textA, result_file);
					}
				}
			}
			if ( CommentCount == OldCommentCount ){
				printoutf(L" -> All match( file:%d mem:%d / all:%d)\r\n",
						check_file, check_mem, count);
			}
			if ( testmem ){
				if ( memcmp(SourceImage, CompareSourceImage, SourceAllocSize) != 0 ){
					ShowComment(L" �� buf �j��\r\n");
				}
			}
		}
	}else{
		printout(L" �� GetFileInfo ���Ȃ�\r\n");
	}

	if ( UseUNICODE && (GetFileInfoW != NULL) ){
		int result_file, result_mem;
		int check_file = 0, check_mem = 0;
		WCHAR textW[SUSIE_PATH_MAX * 2];
		SUSIE_FINFOW *finfoWptr, finfoW;

		PrintAPIname("GetFileInfoW(LPCWSTR buf, LONG_PTR len, LPCWSTR filename, unsigned int flag, SUSIE_FINFOW *lpInfo)");

		if ( finfo_fileW == NULL ){
			ShowComment(L" �� GetArchiveInfoW���������Ă��Ȃ��̂Ńe�X�g�ł��Ȃ�\r\n");

		}else{
			printout(L" Compare GetFileInfoW with GetArchiveInfoW\r\n");
			OldCommentCount = CommentCount;
			finfoWptr = finfo_fileW;
			for ( count = 0; count < maxcount ; finfoWptr++, count++ ){
				if ( finfoWptr->method[0] == '\0' ) break;
				strcpyW(textW, finfoWptr->path);
				if ( (textW[0] != '\0') && (textW[strlenW(textW) - 1] != '\\') ){
					strcatW(textW, L"\\");
				}
				strcatW(textW, finfoWptr->filename);
				if ( testdisk ){
					memset(&finfoW, WRITECHECK_BYTE, sizeof(finfoW));
					result_file = GetFileInfoW(sourcename, 0, textW, SUSIE_SOURCE_DISK, &finfoW);
					if ( result_file == SUSIEERROR_NOERROR ){
						printout(L"d");
						Compare_SUSIE_FINFOW_one(&finfoW, finfoWptr, count);
						check_file++;
					}
				}
				if ( testmem ){
					memset(&finfoW, WRITECHECK_BYTE, sizeof(finfoW));
					result_mem = GetFileInfoW((const WCHAR *)SourceImage, SourceSize, textW, SUSIE_SOURCE_MEM, &finfoW);
					if ( result_mem == SUSIEERROR_NOERROR ){
						printout(L"m");
						Compare_SUSIE_FINFOW_one(&finfoW, finfoWptr, count);
						check_mem++;
					}else if ( result_file != SUSIEERROR_NOERROR ){
						ShowComment(L" �� #%d(%s) �擾�s��(%d)\r\n", count, textW, result_mem);
					}
				}
			}
			if ( CommentCount == OldCommentCount ){
				printoutf(L" -> All match( file:%d mem:%d / all:%d)\r\n",
						check_file, check_mem, count);
			}
			if ( testmem ){
				if ( memcmp(SourceImage, CompareSourceImage, SourceAllocSize) != 0 ){
					ShowComment(L" �� buf �j��\r\n");
				}
			}
		}
	}
}

void TestGetFile(void)
{
	UINT maxcount;
	UINT count;
	int OldCommentCount;
	WCHAR temppathW[MAX_PATH];
	char temppathA[MAX_PATH];

	if ( type_extract == FALSE ) return;
	if ( infosize_file == NULL ) return;
	maxcount = (infosize_file + sizeof(SUSIE_FINFO) - 1) / sizeof(SUSIE_FINFO);

	GetTempPath(MAX_PATH, temppathW);
	strcatW(temppathW, L"\\runspx");
	CreateDirectoryW(temppathW, NULL);
	UnicodeToAnsi(temppathW, temppathA, MAX_PATH);

	printoutf(L"\r\n*** GetFile temporary directroy: %s\r\n", temppathW);

	if ( GetFile != NULL ){
		int result;
		int check_file_mem = 0, check_file_file = 0;
		int check_mem_file = 0, check_mem_mem = 0;
		int check;
		HLOCAL hImage;
		char textA[SUSIE_PATH_MAX * 2], dest[MAX_PATH], destfile[MAX_PATH + SUSIE_PATH_MAX];
		SUSIE_FINFO *finfoAptr;

		PrintAPIname("GetFile(LPCSTR src, LONG_PTR len, LPSTR dest, unsigned int flag, SUSIE_PROGRESS prgressCallback, LONG_PTR lData)");
		if ( finfo_file == NULL ){
			ShowComment(L" �� GetArchiveInfo���������Ă��Ȃ��̂Ńe�X�g�ł��Ȃ�\r\n");
		}else{
			OldCommentCount = CommentCount;
			finfoAptr = finfo_file;
			for ( count = 0; count < maxcount ; finfoAptr++, count++ ){
				if ( finfoAptr->method[0] == '\0' ) break;
				check = 0;
				strcpy(textA, finfoAptr->path);
				// �������u�\�v�̍l�������Ă��Ȃ�
				if ( (textA[0] != '\0') && (textA[strlen(textA) - 1] != '\\') ){
					strcat(textA, "\\");
				}
				strcat(textA, finfoAptr->filename);
				if ( testdisk ){
					result = GetFile(sourcenameA, finfoAptr->position, (LPSTR)&hImage, SUSIE_SOURCE_DISK | SUSIE_DEST_MEM, (FARPROC)SusieProgressCallbackCheck, (LONG_PTR)&sourcenameA);
					if ( result == SUSIEERROR_NOERROR ){
						printout(L"Dm");
						check++;
						if ( hImage == NULL ){
							ShowComment(L" �� #%d(%hs, disk->mem) �擾�s��\r\n", count, textA);
						}else{
							if ( LocalSize(hImage) < finfoAptr->filesize ){
								ShowComment(L" �� #%d(%hs, disk->mem) �擾�������� filesize ��菬����(%d < %d)\r\n", count, textA, LocalSize(hImage), finfoAptr->filesize);
							}else{
								check_file_mem++;
							}
							LocalFree(hImage);
						}
					}
				}
				if ( testmem ){
					result = GetFile(SourceImage + finfoAptr->position, SourceSize, (LPSTR)&hImage, SUSIE_SOURCE_MEM | SUSIE_DEST_MEM, (FARPROC)SusieProgressCallbackCheck, (LONG_PTR)&sourcenameA);
					if ( result == SUSIEERROR_NOERROR ){
						printout(L"Mm");
						check++;
						if ( hImage == NULL ){
							ShowComment(L" �� #%d(%hs, mem->mem) �擾�s��\r\n", count, textA);
						}else{
							if ( LocalSize(hImage) < finfoAptr->filesize ){
								ShowComment(L" �� #%d(%hs, mem->mem) �擾�������� filesize ��菬����(%d < %d)\r\n", count, textA, LocalSize(hImage), finfoAptr->filesize);
							}else{
								check_mem_mem++;
							}
							LocalFree(hImage);
						}
					}
				}

				strcpy(dest, temppathA);
				strcpy(destfile, temppathA);
				strcat(destfile, "\\");
				strcat(destfile, finfoAptr->filename);
				DeleteFileA(destfile);

				if ( testdisk ){
					result = GetFile(sourcenameA, finfoAptr->position, dest, SUSIE_SOURCE_DISK | SUSIE_DEST_DISK, (FARPROC)SusieProgressCallbackCheck, (LONG_PTR)&sourcenameA);
					if ( result == SUSIEERROR_NOERROR ){
						HANDLE hFile;
						LARGE_INTEGER filesize;

						printout(L"Dd");
						check++;
						hFile = CreateFileA(destfile, GENERIC_READ,
								FILE_SHARE_WRITE | FILE_SHARE_READ, NULL,
								OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
						if ( hFile == INVALID_HANDLE_VALUE ){
							ShowComment(L" �� #%d(%hs, file->file) �擾�s��\r\n", count, destfile);
						}else{
							GetFileSizeEx(hFile, &filesize);
							CloseHandle(hFile);
							if ( filesize.QuadPart != finfoAptr->filesize ){
								ShowComment(L" �� #%d(%hs, file->file) �t�@�C���T�C�Y�� filesize �ƕs��v(%d != %d)\r\n", count, textA, filesize.u.LowPart, finfoAptr->filesize);
							}else{
								check_file_file++;
							}
						}
					}
				}

				if ( testmem ){
					result = GetFile(SourceImage, SourceSize, dest, SUSIE_SOURCE_MEM | SUSIE_DEST_DISK, (FARPROC)SusieProgressCallbackCheck, (LONG_PTR)&sourcenameA);
					if ( result == SUSIEERROR_NOERROR ){
						HANDLE hFile;
						LARGE_INTEGER filesize;

						printout(L"Md");
						check++;
						hFile = CreateFileA(destfile, GENERIC_READ,
								FILE_SHARE_WRITE | FILE_SHARE_READ, NULL,
								OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
						if ( hFile == INVALID_HANDLE_VALUE ){
							ShowComment(L" �� #%d(%hs, mem->file) �擾�s��\r\n", count, destfile);
						}else{
							GetFileSizeEx(hFile, &filesize);
							CloseHandle(hFile);
							if ( filesize.QuadPart != finfoAptr->filesize ){
								ShowComment(L" �� #%d(%hs, mem->file) �t�@�C���T�C�Y�� filesize �ƕs��v(%d != %d )\r\n", count, textA, filesize.u.LowPart, finfoAptr->filesize);
							}else{
								check_file_file++;
							}
						}
					}
				}

				if ( check == 0 ){
					printoutf(L" �� #%d(%hs) �擾�s��\r\n", count, textA);
				}
			}
			if ( CommentCount == OldCommentCount ){
				printoutf(L" -> All match( file->mem:%d, mem->mem:%d, file->file:%d, mem->file:%d  / all:%d)\r\n",
						check_file_mem, check_mem_mem, check_file_file, check_mem_file, count);
			}
			if ( testmem ){
				if ( memcmp(SourceImage, CompareSourceImage, SourceAllocSize) != 0 ){
					ShowComment(L" �� buf �j��\r\n");
				}
			}
		}
	}else{
		printout(L" �� GetFile ���Ȃ�\r\n");
	}

	if ( UseUNICODE && (GetFileW != NULL) ){
		int result;
		int check_file_mem = 0, check_file_file = 0;
		int check_mem_file = 0, check_mem_mem = 0;
		int check;
		HLOCAL hImage;
		WCHAR textW[SUSIE_PATH_MAX * 2], dest[MAX_PATH], destfile[MAX_PATH + SUSIE_PATH_MAX];
		SUSIE_FINFOW *finfoWptr;

		PrintAPIname("GetFileW(LPCWSTR src, LONG_PTR len, LPWSTR dest, unsigned int flag, SUSIE_PROGRESS prgressCallback, LONG_PTR lData)");
		if ( finfo_fileW == NULL ){
			ShowComment(L" �� GetArchiveInfoW���������Ă��Ȃ��̂Ńe�X�g�ł��Ȃ�\r\n");
		}else{
			OldCommentCount = CommentCount;
			finfoWptr = finfo_fileW;
			for ( count = 0; count < maxcount ; finfoWptr++, count++ ){
				if ( finfoWptr->method[0] == '\0' ) break;
				check = 0;
				strcpyW(textW, finfoWptr->path);
				if ( (textW[0] != '\0') && (textW[strlenW(textW) - 1] != '\\') ){
					strcatW(textW, L"\\");
				}
				strcatW(textW, finfoWptr->filename);
				if ( testdisk ){
					result = GetFileW(sourcename, finfoWptr->position, (LPWSTR)&hImage, SUSIE_SOURCE_DISK | SUSIE_DEST_MEM, (FARPROC)SusieProgressCallbackCheck, (LONG_PTR)&sourcenameA);
					if ( result == SUSIEERROR_NOERROR ){
						printout(L"Dm");
						check++;
						if ( hImage == NULL ){
							ShowComment(L" �� #%d(%s, disk->mem) �擾�s��\r\n", count, textW);
						}else{
							if ( LocalSize(hImage) < finfoWptr->filesize ){
								ShowComment(L" �� #%d(%s, disk->mem) �擾�������� filesize ��菬����(%d < %d)\r\n", count, textW, LocalSize(hImage), finfoWptr->filesize);
							}else{
								check_file_mem++;
							}
							LocalFree(hImage);
						}
					}
				}
				if ( testmem ){
					result = GetFileW((WCHAR *)(char *)(SourceImage + finfoWptr->position), SourceSize, (LPWSTR)&hImage, SUSIE_SOURCE_MEM | SUSIE_DEST_MEM, (FARPROC)SusieProgressCallbackCheck, (LONG_PTR)&sourcenameA);
					if ( result == SUSIEERROR_NOERROR ){
						printout(L"Mm");
						check++;
						if ( hImage == NULL ){
							ShowComment(L" �� #%d(%s, mem->mem) �擾�s��\r\n", count, textW);
						}else{
							if ( LocalSize(hImage) < finfoWptr->filesize ){
								ShowComment(L" �� #%d(%s, mem->mem) �擾�������� filesize ��菬����(%d < %d)\r\n", count, textW, LocalSize(hImage), finfoWptr->filesize);
							}else{
								check_mem_mem++;
							}
							LocalFree(hImage);
						}
					}
				}

				strcpyW(dest, temppathW);
				strcpyW(destfile, temppathW);
				strcatW(destfile, L"\\");
				strcatW(destfile, finfoWptr->filename);
				DeleteFileW(destfile);

				if ( testdisk ){
					result = GetFileW(sourcename, finfoWptr->position, dest, SUSIE_SOURCE_DISK | SUSIE_DEST_DISK, (FARPROC)SusieProgressCallbackCheck, (LONG_PTR)&sourcenameA);
					if ( result == SUSIEERROR_NOERROR ){
						HANDLE hFile;
						LARGE_INTEGER filesize;

						printout(L"Dd");
						check++;
						hFile = CreateFileW(destfile, GENERIC_READ,
								FILE_SHARE_WRITE | FILE_SHARE_READ, NULL,
								OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
						if ( hFile == INVALID_HANDLE_VALUE ){
							ShowComment(L" �� #%d(%s, file->file) �擾�s��\r\n", count, destfile);
						}else{
							GetFileSizeEx(hFile, &filesize);
							CloseHandle(hFile);
							if ( filesize.QuadPart != finfoWptr->filesize ){
								ShowComment(L" �� #%d(%s, file->file) �t�@�C���T�C�Y�� filesize �ƕs��v(%d != %d)\r\n", count, textW, filesize.u.LowPart, finfoWptr->filesize);
							}else{
								check_file_file++;
							}
						}
					}
				}
				if ( testmem ){
					result = GetFileW((WCHAR *)SourceImage, SourceSize, dest, SUSIE_SOURCE_MEM | SUSIE_DEST_DISK, (FARPROC)SusieProgressCallbackCheck, (LONG_PTR)&sourcenameA);
					if ( result == SUSIEERROR_NOERROR ){
						HANDLE hFile;
						LARGE_INTEGER filesize;

						printout(L"Md");
						check++;
						hFile = CreateFileW(destfile, GENERIC_READ,
								FILE_SHARE_WRITE | FILE_SHARE_READ, NULL,
								OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
						if ( hFile == INVALID_HANDLE_VALUE ){
							ShowComment(L" �� #%d(%s, mem->file) �擾�s��\r\n", count, destfile);
						}else{
							GetFileSizeEx(hFile, &filesize);
							CloseHandle(hFile);
							if ( filesize.QuadPart != finfoWptr->filesize ){
								ShowComment(L" �� #%d(%s, mem->file) �t�@�C���T�C�Y�� filesize �ƕs��v(%d != %d)\r\n", count, textW, filesize.u.LowPart, finfoWptr->filesize);
							}else{
								check_file_file++;
							}
						}
					}
				}

				if ( check == 0 ){
					printoutf(L" �� #%d(%s) �擾�s��\r\n", count, textW);
				}
			}
			if ( CommentCount == OldCommentCount ){
				printoutf(L" -> All match( file->mem:%d, mem->mem:%d, file->file:%d, mem->file:%d  / all:%d)\r\n",
						check_file_mem, check_mem_mem, check_file_file, check_mem_file, count);
			}
			if ( testmem ){
				if ( memcmp(SourceImage, CompareSourceImage, SourceAllocSize) != 0 ){
					ShowComment(L" �� buf �j��\r\n");
				}
			}
		}
	}
}

void TestCreatePicture(void)
{
	WCHAR temppathW[MAX_PATH], tempnameW[MAX_PATH];
	char temppathA[MAX_PATH], tempnameA[MAX_PATH];
	HLOCAL HBInfo, HBm;
	int width = 1023, height = 511;
	DWORD linesize;
	BITMAPINFOHEADER *info;
	BYTE *bits;

	if ( (CreatePicture == NULL) && (CreatePictureW == NULL) ) return;

	GetTempPath(MAX_PATH, temppathW);
	strcatW(temppathW, L"\\runspx");
	CreateDirectoryW(temppathW, NULL);
	UnicodeToAnsi(temppathW, temppathA, MAX_PATH);

	PrintAPIname("int __stdcall CreatePicture(LPCSTR filepath, unsigned int flag, HANDLE *pHBInfo, HANDLE *pHBm, struct PictureInfo *lpInfo, SUSIE_PROGRESS progressCallback, LONG_PTR lData);");

	printoutf(L"\r\n*** CreatePicture temporary directroy: %s\r\n", temppathW);

	linesize = DwordBitSize(width * 24);

	HBInfo = LocalAlloc(LMEM_MOVEABLE, sizeof(BITMAPINFOHEADER));
	HBm = LocalAlloc(LMEM_MOVEABLE, linesize * height);
	info = (BITMAPINFOHEADER *)LocalLock(HBInfo);
	info->biSize = sizeof(BITMAPINFOHEADER);
	info->biWidth = width;
	info->biHeight = height;
	info->biPlanes = 1;
	info->biBitCount = 24;
	info->biCompression = BI_RGB;
	info->biSizeImage = 0;//linesize * height;
	info->biXPelsPerMeter = info->biYPelsPerMeter = 2835;
	info->biClrUsed = 0;
	info->biClrImportant = 0;

	LocalUnlock(HBInfo);

	bits = (BYTE *)LocalLock(HBm);
	int x, y;
	BYTE *dest;
	DWORD color = 0;
	for ( x = 0; x < width; x++){
		dest = bits + x * 3;
		for ( y = 0; y < height; y++, dest += linesize){
			dest[0] = (BYTE)color;
			dest[1] = (BYTE)(color >> 8);
			dest[2] = (BYTE)(color >> 16);
		}
		color += 0xffffff / width;
	}
	LocalUnlock(HBm);

	for ( int infono = 2; ; infono += 2 ){
		char bufA[INFOBUFLEN], *extA, *p;
		WCHAR extW[INFOBUFLEN];
		int result;

		result = GetPluginInfo(infono, bufA, INFOBUFCHECKLEN);
		if ( result == 0 ) break;
		bufA[INFOBUFCHECKLEN - 1] = '\0';
		extA = strchr(bufA, '.');
		if ( extA == NULL ) continue;
		p = extA;
		while ( (*p != '\0') && (*p != ';') ) p++;
		*p = '\0';
		AnsiToUnicode(extA, extW, INFOBUFLEN);

		printoutf(L"Type: %s\r\n", extW);

		if ( CreatePicture != NULL ){
			printout(L" CreatePicture");
			wsprintfA(tempnameA, "%s\\CreatePicture 1023 x 511 24bit%s", temppathA, extA);
			result = CreatePicture(tempnameA, SUSIE_DEST_DISK | SUSIE_DEST_REJECT_UNKNOWN_TYPE, &HBInfo, &HBm, NULL, NULL, 0);
			PluginResult(result);
		}
		if ( UseUNICODE && (CreatePictureW != NULL) ){
			printout(L" CreatePictureW");
			wsprintfW(tempnameW, L"%s\\CreatePictureW 1023 x 511 24bit%s", temppathW, extW);
			result = CreatePictureW(tempnameW, SUSIE_DEST_DISK | SUSIE_DEST_REJECT_UNKNOWN_TYPE, &HBInfo, &HBm, NULL, NULL, 0);
			PluginResult(result);
		}
	}

	LocalFree(HBInfo);
	LocalFree(HBm);
}

void TestPlugin(void)
{
	DWORD oldtls, nowtls;

	oldtls = TlsAlloc();
	TlsFree(oldtls);

	ShowAPIlist();
	LoadSourceImage();

	TestGetPluginInfo();
	TestIsSupported();

	TestGetPicture();
	TestGetPreview();
	TestGetPictureInfo();

	TestGetArchiveInfo();
	TestGetFileInfo();
	TestGetFile();
	TestCreatePicture();

	nowtls = TlsAlloc(); // Win10 �ł� 1087(0x43f) �܂Ŋ��蓖�Ăł���
	TlsFree(nowtls);
	if ( oldtls != nowtls ){
		printoutf(L"\r\n Plug-in used %d Thread Local Storate(TLS) (%d -> %d)\r\n", nowtls - oldtls, oldtls, nowtls);
	}

	if ( CommentCount == 0 ){
		printout(L"\r\n*** no comment ***\r\n");
	}else{
		printoutfColor(COLOR_COMMENT, L"\r\n*** Check point: %d ***\r\n", CommentCount);
	}
	printout(L"\r\nEnd test.\r\n");
}
