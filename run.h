/*-----------------------------------------------------------------------------
	Run Susie plug-in							Copyright (c) TORO
 ----------------------------------------------------------------------------*/
#ifdef __cplusplus
extern "C" {
#endif

#define COPYRIGHT UNICODESTR(APPNAME) L" Version 0.3 (c)TORO"

enum MODELIST { MODE_DEFAULT = -1, MODE_ABOUT = 0, MODE_CONFIG, MODE_RUN, MODE_GETPICTURE, MODE_GETPREVIEW, MODE_GETFILE, MODE_TEST, MODE_TEST_ZEROSIZE};

#define CONSOLE_DEFAULT_COLOR (CONSOLE_F_DWHITE | CONSOLE_B_BLACK)
#define CONSOLE_F_BLACK 0
#define CONSOLE_F_DBLUE (FOREGROUND_BLUE)
#define CONSOLE_F_DGREEN (FOREGROUND_GREEN)
#define CONSOLE_F_DCYAN (FOREGROUND_BLUE | FOREGROUND_GREEN)
#define CONSOLE_F_DRED (FOREGROUND_RED)
#define CONSOLE_F_DMAGENTA (FOREGROUND_RED | FOREGROUND_BLUE)
#define CONSOLE_F_DYELLOW (FOREGROUND_RED | FOREGROUND_GREEN)
#define CONSOLE_F_DWHITE (FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE)
#define CONSOLE_F_LBLACK 0
#define CONSOLE_F_LBLUE (FOREGROUND_INTENSITY | FOREGROUND_BLUE)
#define CONSOLE_F_LGREEN (FOREGROUND_INTENSITY | FOREGROUND_GREEN)
#define CONSOLE_F_LCYAN (FOREGROUND_INTENSITY | FOREGROUND_BLUE | FOREGROUND_GREEN)
#define CONSOLE_F_LRED (FOREGROUND_INTENSITY | FOREGROUND_RED)
#define CONSOLE_F_LMAGENTA (FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_BLUE)
#define CONSOLE_F_LYELLOW (FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_GREEN)
#define CONSOLE_F_LWHITE (FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE)

#define CONSOLE_B_BLACK 0
#define CONSOLE_B_DBLUE (BACKGROUND_BLUE)
#define CONSOLE_B_DGREEN (BACKGROUND_GREEN)
#define CONSOLE_B_DCYAN (BACKGROUND_BLUE | BACKGROUND_GREEN)
#define CONSOLE_B_DRED (BACKGROUND_RED)
#define CONSOLE_B_DMAGENTA (BACKGROUND_RED | BACKGROUND_BLUE)
#define CONSOLE_B_DYELLOW (BACKGROUND_RED | BACKGROUND_GREEN)
#define CONSOLE_B_DWHITE (BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE)
#define CONSOLE_B_LBLACK 0
#define CONSOLE_B_LBLUE (BACKGROUND_INTENSITY | BACKGROUND_BLUE)
#define CONSOLE_B_LGREEN (BACKGROUND_INTENSITY | BACKGROUND_GREEN)
#define CONSOLE_B_LCYAN (BACKGROUND_INTENSITY | BACKGROUND_BLUE | BACKGROUND_GREEN)
#define CONSOLE_B_LRED (BACKGROUND_INTENSITY | BACKGROUND_RED)
#define CONSOLE_B_LMAGENTA (BACKGROUND_INTENSITY | BACKGROUND_RED | BACKGROUND_BLUE)
#define CONSOLE_B_LYELLOW (BACKGROUND_INTENSITY | BACKGROUND_RED | BACKGROUND_GREEN)
#define CONSOLE_B_LWHITE (BACKGROUND_INTENSITY | BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE)

#define COLOR_TITLE (CONSOLE_F_LWHITE | CONSOLE_B_BLACK)
#define COLOR_APINAME (CONSOLE_F_LWHITE | CONSOLE_B_BLACK)
#define COLOR_COMMENT (CONSOLE_F_LYELLOW | CONSOLE_B_BLACK)

extern HMODULE hPlugin;
extern GETPLUGININFO	GetPluginInfo;
extern GETPLUGININFOW	GetPluginInfoW;
extern ISSUPPORTED		IsSupported;
extern ISSUPPORTEDW		IsSupportedW;
extern GETPICTURE		GetPicture;
extern GETPICTUREW		GetPictureW;
extern GETPREVIEW		GetPreview;
extern GETPREVIEWW		GetPreviewW;
extern GETPICTUREINFO	GetPictureInfo;
extern GETPICTUREINFOW	GetPictureInfoW;
extern GETARCHIVEINFO	GetArchiveInfo;
extern GETARCHIVEINFOW	GetArchiveInfoW;
extern GETFILE			GetFile;
extern GETFILEW			GetFileW;
extern GETFILEINFO		GetFileInfo;
extern GETFILEINFOW	GetFileInfoW;
extern CONFIGURATIONDLG ConfigurationDlg;
extern CREATEPICTURE CreatePicture;
extern CREATEPICTUREW CreatePictureW;

#define WRITECHECK_BYTE 0x7f
#define WRITECHECK_WCHAR 0x7f7f
#define WRITECHECK_DWORD 0x7f7f7f7f
#define WRITECHECK_QWORD 0x7f7f7f7f7f7f7f7f
#define WRITECHECK_QMASK 0xff00000000000000
#define WRITECHECK_QTOP  0x7f00000000000000

#define EXMAX_PATH 0x400
extern WCHAR spipath[MAX_PATH], xpipath[MAX_PATH];
extern WCHAR sourcename[EXMAX_PATH], targetname[EXMAX_PATH], archiveitem[EXMAX_PATH];
extern char sourcenameA[EXMAX_PATH], targetnameA[EXMAX_PATH], archiveitemA[EXMAX_PATH];
extern int testmem, testdisk;
extern char *SourceImage;
extern DWORD SourceSize;
extern BOOL UseCreatePicture;
extern BOOL UseSubdirectory;

extern BOOL LoadPlugin(WCHAR *filename);
#define FreePlugin() FreeLibrary(hPlugin);

extern BOOL CheckHeader(void);
extern void RunGetPicture(MODELIST RunMode);
extern void RunShowPicture(MODELIST RunMode);
extern void RunArchive(MODELIST RunMode);
extern void TestPlugin(void);

extern void PluginResult(int result);
extern void ShowAPIlist(void);
extern void ShowPictureInfo(HLOCAL HBInfo, HLOCAL HBm);
extern void LoadSourceImage(void);
extern int __stdcall SusieProgressCallbackCheck(int, int, LONG_PTR lData);
extern HANDLE OpenFileHeader(char *header);

extern void tInitConsole(void);
extern void tReleaseConsole(void);
extern void SetColor(DWORD color);
#define ResetColor() SetColor(CONSOLE_DEFAULT_COLOR)

extern void USEFASTCALL printout(const WCHAR *str);
extern void USEFASTCALL printoutA(const char *str);
extern void printoutf(const WCHAR *message, ...);
extern void printoutfA(const char *message, ...);
extern void printoutfColor(DWORD color, const WCHAR *message, ...);
extern void printoutfColorA(DWORD color, const char *message, ...);

extern BOOL ShowImageToConsole(HLOCAL HBInfo, HLOCAL HBm);

extern void GetQuotedParameter(LPCTSTR *commandline,TCHAR *param,const TCHAR *parammax);
extern UTCHAR USEFASTCALL IsEOL(LPCTSTR *str);
extern UTCHAR GetLineParam(LPCTSTR *str, TCHAR *param);

extern const WCHAR *GetTimeStrings(WCHAR *dest, susie_time_t timestamp);

typedef struct {
	BYTE *bits; // ビットマップのイメージ
	HLOCAL info, bm;
	HPALETTE hPalette;
	SIZE size; // 画像の大きさ(DIBの値はトップダウンもあるので使わないこと)
	BITMAPINFOHEADER *DIB;
	DWORD PaletteOffset;

	struct {		// 減色パレット
		BITMAPINFOHEADER dib2;
		RGBQUAD rgb2[256];
	} nb;
} HTBMP;

#ifdef __cplusplus
}
#endif
