/*-----------------------------------------------------------------------------
	Run Susie plug-in    Copyright (c) TORO
 ----------------------------------------------------------------------------*/
#include <windows.h>
#include "torowin.h"
#include "susie.h"
#include "faultcon.h"
#include "run.h"
#include "app.h"

MODELIST RunMode = MODE_DEFAULT;
PVOID UEFvec;

int init(void)
{
	const WCHAR *commandline;
	WCHAR param[MAX_PATH];

	UEFvec = AddVectoredExceptionHandler(0, (PVECTORED_EXCEPTION_HANDLER)UnhandledExceptionFilterProc);
	tInitConsole();

	commandline = GetCommandLine();
	GetLineParam(&commandline, param);
	for (;;){
		GetLineParam(&commandline, param);
		if ( *param == '\0' ) break;
		if ( strcmpW(param, L"-c") == 0 ){
			RunMode = MODE_CONFIG;
			continue;
		}
		if ( strcmpW(param, L"-v") == 0 ){
			RunMode = MODE_ABOUT;
			continue;
		}
		if ( strcmpW(param, L"-b") == 0 ){
			if ( RunMode != MODE_GETPREVIEW ) RunMode = MODE_GETPICTURE;
			GetLineParam(&commandline, param);
			strcpyW(targetname, param);
			UnicodeToAnsi(targetname, targetnameA, EXMAX_PATH);
			continue;
		}

		if ( strcmpW(param, L"-bc") == 0 ){
			UseCreatePicture = TRUE;
			if ( RunMode != MODE_GETPREVIEW ) RunMode = MODE_GETPICTURE;
			GetLineParam(&commandline, param);
			strcpyW(targetname, param);
			UnicodeToAnsi(targetname, targetnameA, EXMAX_PATH);
		}

		if ( strcmpW(param, L"-bp") == 0 ){
			GetLineParam(&commandline, xpipath);
			continue;
		}
		if ( strcmpW(param, L"-r") == 0 ){
			RunMode = MODE_GETPREVIEW;
			continue;
		}
		if ( strcmpW(param, L"-t") == 0 ){
			RunMode = MODE_TEST;
			continue;
		}
		if ( strcmpW(param, L"-tz") == 0 ){
			RunMode = MODE_TEST_ZEROSIZE;
			testmem = 10;
			continue;
		}
		if ( strcmpW(param, L"-p") == 0 ){
			if ( UEFvec != NULL ) RemoveVectoredExceptionHandler(UEFvec);
			UEFvec = NULL;
			continue;
		}
		if ( strcmpW(param, L"-e") == 0 ){
			RunMode = MODE_GETFILE;
			GetLineParam(&commandline, param);
			strcpyW(targetname, param);
			UnicodeToAnsi(targetname, targetnameA, EXMAX_PATH);
			continue;
		}
		if ( strcmpW(param, L"-j") == 0 ){
			UseSubdirectory = FALSE;
			continue;
		}
		if ( strcmpW(param, L"-srcmem") == 0 ){
			testmem = 1;
			if ( testdisk < 0 ) testdisk = 0;
			continue;
		}
		if ( strcmpW(param, L"-srcdisk") == 0 ){
			testdisk = 1;
			if ( testmem < 0 ) testmem = 0;
			continue;
		}

		if ( spipath[0] == '\0' ){
			strcpyW(spipath, param);
			continue;
		}
		if ( sourcename[0] == '\0' ){
			strcpyW(sourcename, param);
			UnicodeToAnsi(sourcename, sourcenameA, EXMAX_PATH);
			continue;
		}
		if ( targetname[0] == '\0' ){
			strcpyW(targetname, param);
			UnicodeToAnsi(targetname, targetnameA, EXMAX_PATH);
			continue;
		}
		if ( archiveitem[0] == '\0' ){
			UseSubdirectory = FALSE;
			strcpyW(archiveitem, param);
			UnicodeToAnsi(archiveitem, archiveitemA, EXMAX_PATH);
			continue;
		}
	}
	return EXIT_SUCCESS;
}

int __cdecl main(int, char *)
{
	int result = EXIT_SUCCESS;

	result = init();

	if ( spipath[0] == '\0' ){
		printout( COPYRIGHT L"\r\n"
				  L" " UNICODESTR(APPNAME) L" plugin-name ...\r\n"
				  L"    -t   API test; plugin-name source -t\r\n"
				  L"    -tz  API test 0 size file; plugin-name source -t\r\n"
				  L"    -b   get picture;   plugin-name source-file -b save-bitmap-name\r\n"
				  L"    -e   extract files; plugin-name source-file -e save-path [file-name]\r\n" );
	}else{
		CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
		if ( LoadPlugin(spipath) == FALSE ){
			printoutf(L"%s load error\r\n", spipath);
			result = EXIT_FAILURE;
		}else{
			switch (RunMode){
				case MODE_ABOUT:
				case MODE_CONFIG:
					if ( ConfigurationDlg != NULL ){
						printout(L"ConfigurationDlg");
						PluginResult(ConfigurationDlg(NULL, RunMode));
					}else{
						printout(L"ConfigurationDlg not found\r\n");
						result = EXIT_FAILURE;
					}
					break;

				case MODE_GETPICTURE:
				case MODE_GETPREVIEW:
					RunGetPicture(RunMode);
					break;

				case MODE_TEST:
				case MODE_TEST_ZEROSIZE:
					TestPlugin();
					break;

				default:
					if ( sourcename[0] == '\0' ){
						ShowAPIlist();
					}else{
						if ( CheckHeader() == FALSE ) break;
						if ( GetPicture != NULL ){
							RunShowPicture(RunMode);
						}else if ( GetArchiveInfo != NULL ){
							RunArchive(RunMode);
						}
					}
			}
			FreePlugin();
		}
		CoUninitialize();
	}
	tReleaseConsole();
	return result;
}
