/*-----------------------------------------------------------------------------
	異常終了内容の報告(for console)	(c)TORO
-----------------------------------------------------------------------------*/
#define STRICT
#include <windows.h>
#include <imagehlp.h>
#include "torowin.h"
#include "faultcon.h"
#pragma hdrstop

#define FileProp_Version File_Version
#define AppExeName T("run")

#ifdef _M_ARM
	#define MSGBIT T("ARM")
	#define REGPREFIX(reg) reg
	#define CHECK_LoadDLL "LoadDLL"

	#define CPUTYPE IMAGE_FILE_MACHINE_ARM
	#define LOADWINAPI164(name) {(void (WINAPI **)())&(D ## name), #name}
	#define TAIL64(name) name
	#ifdef UNICODE
		#define LOADWINAPI164TW(name) {(void (WINAPI **)())&(D ## name), #name "W"}
		#define TAIL64T(name) name ## W
	#else
		#define LOADWINAPI164TW(name) {(void (WINAPI **)())&(D ## name), #name}
		#define TAIL64T(name) name
	#endif
#endif

#ifdef _M_ARM64
	#define MSGBIT T("ARM64")
	#define REGPREFIX(reg) reg
	#define CHECK_LoadDLL "LoadDLL64"

	#define CPUTYPE IMAGE_FILE_MACHINE_ARM64
	#define LOADWINAPI164(name) {(void (WINAPI **)())&(D ## name), #name}
	#define TAIL64(name) name
	#ifdef UNICODE
		#define LOADWINAPI164TW(name) {(void (WINAPI **)())&(D ## name), #name "W"}
		#define TAIL64T(name) name ## W
	#else
		#define LOADWINAPI164TW(name) {(void (WINAPI **)())&(D ## name), #name}
		#define TAIL64T(name) name
	#endif
#endif

#ifndef CPUTYPE
	#ifdef _WIN64
		#define MSGBIT T("x64")
		#define REGPREFIX(reg) R ## reg
		#define CHECK_LoadDLL "LoadDLL64"

		#define CPUTYPE IMAGE_FILE_MACHINE_AMD64
		#define LOADWINAPI164(name) {(void (WINAPI **)())&(D ## name), #name "64"}
		#define TAIL64(name) name ## 64
		#ifdef UNICODE
			#define LOADWINAPI164TW(name) {(void (WINAPI **)())&(D ## name), #name "W64"}
			#define TAIL64T(name) name ## W64
		#else
			#define LOADWINAPI164TW(name) {(void (WINAPI **)())&(D ## name), #name "64"}
			#define TAIL64T(name) name ## 64
		#endif
	#else
		#define MSGBIT
		#define REGPREFIX(reg) E ## reg
		#define CHECK_LoadDLL "LoadDLL"

		#define CPUTYPE IMAGE_FILE_MACHINE_I386
		#define LOADWINAPI164(name) {(void (WINAPI **)())&(D ## name), #name}
		#define TAIL64(name) name
		#ifdef UNICODE
			#define LOADWINAPI164TW(name) {(void (WINAPI **)())&(D ## name), #name "W"}
			#define TAIL64T(name) name ## W
		#else
			#define LOADWINAPI164TW(name) {(void (WINAPI **)())&(D ## name), #name}
			#define TAIL64T(name) name
		#endif
	#endif
#endif

extern void printoutf(const WCHAR *message, ...);
extern void USEFASTCALL printout(const WCHAR *str);

const WCHAR NilStr[] = L"";
//------------------------------------------------------------ 例外表示テーブル
#define msgstr_fault 0
#define msgstr_except 1
#define msgstr_unknownthread 2
#define msgstr_memaddress 3
#define msgstr_faultread 4
#define msgstr_faultwrite 5
#define msgstr_div0 6
#define msgstr_faultDEP 7

const WCHAR *msgstringsJ[] = {
	T("\r\n\r\n*** 実行時例外が発生しました。 ***\r\n\r\n"), // 0 msgstr_fault

	T("例外 %XH"), // 1 msgstr_except
	T("不明"), // 2 msgstr_unknownthread
	T("メモリ(") T(PTRPRINTFORMAT) T("H)の%s失敗"), // 3 msgstr_memaddress
	T("読込"), // 4 msgstr_faultread
	T("書込"), // 5 msgstr_faultwrite
	T("0除算"), // 6 msgstr_div0
	T("DEP"), // 7 msgstr_faultDEP
};

const WCHAR *msgstringsE[] = {
	T("*** Detected error. ***\r\n\r\n"), // 0

	T("Exception %XH"),
	T("Unknown"),
	T(PTRPRINTFORMAT) T("H %s memory access fault"),
	T("Read"),
	T("Write"),
	T("Divide by zero"),
	T("DEP"),
};

#ifndef EXCEPTION_IN_PAGE_ERROR
	#define EXCEPTION_IN_PAGE_ERROR 0xC0000006
#endif

#define STACKWALKSIZE 0x3000

#if defined(UNICODE) && !defined(CBA_READ_MEMORY) && !defined(WINEGCC)
typedef struct {
	DWORD SizeOfStruct, BaseOfImage, ImageSize, TimeDateStamp, CheckSum, NumSyms;
	SYM_TYPE SymType;
	WCHAR ModuleName[32], ImageName[256], LoadedImageName[256];
} IMAGEHLP_MODULEW, *PIMAGEHLP_MODULEW;
#endif
DefineWinAPI(BOOL, StackWalk, (DWORD, HANDLE, HANDLE, TAIL64(LPSTACKFRAME), PVOID,
	TAIL64(PREAD_PROCESS_MEMORY_ROUTINE),
	TAIL64(PFUNCTION_TABLE_ACCESS_ROUTINE),
	TAIL64(PGET_MODULE_BASE_ROUTINE),
	TAIL64(PTRANSLATE_ADDRESS_ROUTINE))
) = NULL;

DefineWinAPI(BOOL, SymInitialize, (HANDLE, PCSTR, BOOL)) = NULL;
DefineWinAPI(PVOID, SymFunctionTableAccess, (HANDLE, DWORD_PTR));
DefineWinAPI(BOOL, SymGetSymFromAddr,
	(HANDLE, DWORD_PTR, DWORD_PTR *, TAIL64(IMAGEHLP_SYMBOL) *));
DefineWinAPI(BOOL, SymCleanup, (HANDLE));

DefineWinAPI(DWORD_PTR, SymGetModuleBase, (HANDLE, DWORD_PTR));
DefineWinAPI(BOOL, SymGetModuleInfo, (HANDLE, DWORD_PTR, TAIL64T(IMAGEHLP_MODULE) *)) = NULL;

LOADWINAPISTRUCT IMAGEHLPDLL[] = {
	LOADWINAPI164(StackWalk),
	LOADWINAPI1(SymInitialize),
	LOADWINAPI164(SymFunctionTableAccess),
	LOADWINAPI164(SymGetModuleBase),
	LOADWINAPI164(SymGetSymFromAddr),
	LOADWINAPI1(SymCleanup),
	LOADWINAPI164TW(SymGetModuleInfo),
	{NULL, NULL}
};

#define defThreadQuerySetWin32StartAddress 9

HANDLE LoadWinAPI(const char *DLLname, HMODULE hDLL, LOADWINAPISTRUCT *apis)
{
	if ( hDLL == NULL ){
		hDLL = LoadLibraryA(DLLname);
		if ( hDLL == NULL ) return NULL;
	}

	while (apis->APIname != NULL){
		*apis->APIptr = (void (WINAPI *)())GetProcAddress(hDLL, apis->APIname);
		if ( *apis->APIptr == NULL ){
			FreeLibrary(hDLL);
			return NULL;
		}
		apis++;
	}
	return hDLL;
}

// address ( modulename [basename].funcname + offset 形式を出力する
void GetFailedModuleAddr(HANDLE hProcess, TAIL64(IMAGEHLP_SYMBOL) *syminfo, WCHAR *dest, void *addr)
{
	TAIL64T(IMAGEHLP_MODULE) mdlinfo;
	DWORD_PTR funcoffset;

	// address
	dest += wsprintf(dest, T(PTRPRINTFORMAT), addr);
	mdlinfo.SizeOfStruct = sizeof(mdlinfo);
	// modulename, basename
	if ( (DSymInitialize != NULL) && IsTrue(DSymGetModuleInfo(
			GetCurrentProcess(), (DWORD_PTR)addr, &mdlinfo)) ){
		dest += wsprintf(dest, T("(%s:") T(PTRPRINTFORMAT),
					mdlinfo.ModuleName, mdlinfo.BaseOfImage);
	}
	// funcname, offset
	if ( (DSymInitialize != NULL) && IsTrue(DSymGetSymFromAddr(
			hProcess, (DWORD_PTR)addr, &funcoffset, syminfo)) ){
		#ifdef UNICODE
		WCHAR funcnameW[256];

		AnsiToUnicode(syminfo->Name, funcnameW, 256);
		dest += wsprintf(dest, T(".%s+") T(PTRPRINTFORMAT), funcnameW, funcoffset);
		#else
		dest += wsprintf(dest, T(".%s+") T(PTRPRINTFORMAT), syminfo->Name, funcoffset);
		#endif
	}
	*dest++ = ')';
	*dest = '\0';
}

//-------------------------------------------------------------------- 例外処理
#define SHOWSTACKS 20 // 標準のスタック表示数
#define SHOWSTACKSBUFSIZE 0x200

LONG UnhandledExceptionFilterProc(EXCEPTION_POINTERS *ExceptionInfo)
{
	HANDLE hThread, hProcess;
	HMODULE hKernel32;

	EXCEPTION_RECORD *ExceptionRecord = ExceptionInfo->ExceptionRecord;
	DWORD_PTR checkaddr = (DWORD_PTR)ExceptionRecord->ExceptionAddress;
	DWORD_PTR targetaddr;
	DWORD ExceptionCode = ExceptionRecord->ExceptionCode;

	char symblebuffer[STACKWALKSIZE];
	TAIL64(IMAGEHLP_SYMBOL) *syminfo;

	WCHAR threadnamebuf[0x100];
	const WCHAR *threadname;

	WCHAR exceptionbuf[256], stackinfo[SHOWSTACKSBUFSIZE + 16], *stackptr;
	const WCHAR **Msg = msgstringsE;
	const WCHAR *Comment = NilStr;
	TAIL64(STACKFRAME) sframe;

	if ( ExceptionCode & B29 ){ // ユーザ定義
		return EXCEPTION_CONTINUE_SEARCH;
	}

	if ((ExceptionCode != EXCEPTION_ACCESS_VIOLATION) &&
//		(ExceptionCode != EXCEPTION_DATATYPE_MISALIGNMENT) && // 境界アクセス違反
		(ExceptionCode != EXCEPTION_FLT_INVALID_OPERATION) &&
		(ExceptionCode != EXCEPTION_IN_PAGE_ERROR) &&
		(ExceptionCode != EXCEPTION_STACK_OVERFLOW) &&
		(ExceptionCode != EXCEPTION_INT_DIVIDE_BY_ZERO) ){
		return EXCEPTION_CONTINUE_SEARCH;
	}
	// IsBadWritePtr/IsBadReadPtr 内は無視
	hKernel32 = GetModuleHandle(T("KERNEL32.DLL"));
	targetaddr = (DWORD_PTR)GetProcAddress(hKernel32, "IsBadWritePtr");
	if ( targetaddr != 0 ){
		if ( (checkaddr >= (targetaddr - 0x60)) &&
			 (checkaddr < targetaddr + 0x140) ){
			return EXCEPTION_CONTINUE_SEARCH;
		}
		targetaddr = (DWORD_PTR)GetProcAddress(hKernel32, "IsBadReadPtr");
		if ( (checkaddr >= (targetaddr - 0x60)) &&
			 (checkaddr < targetaddr + 0x140) ){
			return EXCEPTION_CONTINUE_SEARCH;
		}
	}
	if ( LOWORD(GetUserDefaultLCID()) == LCID_JAPANESE ) Msg = msgstringsJ;
	printoutf(Msg[msgstr_fault]);

	hProcess = GetCurrentProcess();
	hThread = GetCurrentThread();

	// デバッグ情報、シンボル情報取得の準備
	if ( DSymInitialize == NULL ){
		LoadWinAPI("IMAGEHLP.DLL", NULL, IMAGEHLPDLL);
	}
	if ( DSymInitialize != NULL ){
		DSymInitialize(hProcess, NULL, TRUE);
		syminfo = (TAIL64(IMAGEHLP_SYMBOL) *)symblebuffer;
		syminfo->SizeOfStruct = STACKWALKSIZE;
		syminfo->MaxNameLength = STACKWALKSIZE - sizeof(TAIL64(IMAGEHLP_SYMBOL));
	}

	// Thread name取得
	{
		DefineWinAPI(LONG, NtQueryInformationThread, (HANDLE, int, PVOID, ULONG, PULONG));

		GETDLLPROC(GetModuleHandleA("ntdll.dll"), NtQueryInformationThread);
		if ( DNtQueryInformationThread == NULL ){
			threadname = Msg[msgstr_unknownthread];
		}else{
			ULONG ulLength;
			PVOID pBeginAddress = NULL;

			DNtQueryInformationThread(hThread,
					defThreadQuerySetWin32StartAddress, &pBeginAddress,
					sizeof(pBeginAddress), &ulLength);

			GetFailedModuleAddr(hProcess, syminfo, threadnamebuf, pBeginAddress);
			threadname = threadnamebuf;
		}
	}
	// 例外詳細 exception
	ExceptionRecord->ExceptionFlags = EXCEPTION_NONCONTINUABLE;
	if ( (ExceptionCode == EXCEPTION_ACCESS_VIOLATION) ||
		 (ExceptionCode == EXCEPTION_IN_PAGE_ERROR) ){
		DWORD rwmode = msgstr_faultread;
		DWORD rwinfo = (DWORD)ExceptionRecord->ExceptionInformation[0];

		if ( rwinfo ){
			rwmode = (rwinfo == 8) ? msgstr_faultDEP : msgstr_faultwrite;
		}
		wsprintf(exceptionbuf, Msg[msgstr_memaddress],
				ExceptionRecord->ExceptionInformation[1], Msg[rwmode]);
	}else if ( ExceptionCode == EXCEPTION_INT_DIVIDE_BY_ZERO){
		tstrcpy(exceptionbuf, Msg[msgstr_div0]);
	}else{
		wsprintf(exceptionbuf, Msg[msgstr_except], ExceptionCode);
	}
	printoutf(L"Thread: %s, %s\r\n", threadname, exceptionbuf);

	// スタック/例外アドレス stackinfo
	if ( DStackWalk != NULL ){
		BOOL findapp = FALSE;
		CONTEXT targetThc;
		int stacks = SHOWSTACKS;
		int count = 0;

		targetThc = *ExceptionInfo->ContextRecord;
		targetThc.ContextFlags = CONTEXT_FULL;
		memset(&sframe, 0, sizeof(sframe));

#if defined(_M_ARM) || defined(_M_ARM64)
		sframe.AddrPC.Offset = ExceptionInfo->ContextRecord->REGPREFIX(Pc);
		sframe.AddrStack.Offset = ExceptionInfo->ContextRecord->REGPREFIX(Sp);
		sframe.AddrFrame.Offset = 0;
#else
		sframe.AddrPC.Offset = ExceptionInfo->ContextRecord->REGPREFIX(ip);
		sframe.AddrStack.Offset = ExceptionInfo->ContextRecord->REGPREFIX(sp);
		sframe.AddrFrame.Offset = ExceptionInfo->ContextRecord->REGPREFIX(bp);
#endif
		sframe.AddrPC.Mode = AddrModeFlat;
		sframe.AddrStack.Mode = AddrModeFlat;
		sframe.AddrFrame.Mode = AddrModeFlat;
		if ( ExceptionCode == EXCEPTION_STACK_OVERFLOW ){
			stacks += 3;
		}

		for (;;) {
			WCHAR addrstr[512];
			BOOL walkresult;

			stackptr = stackinfo;
			stackinfo[0] = '\0';

			count++;
			walkresult = DStackWalk(CPUTYPE, hProcess, hThread, &sframe,
					&targetThc, NULL, DSymFunctionTableAccess,
					DSymGetModuleBase, NULL);
			if ( !walkresult || (sframe.AddrPC.Offset == sframe.AddrReturn.Offset)){
				break;
			}
			// エラーアドレスが記載されていないときは追加
			if ( (stacks == SHOWSTACKS) &&
				((void *)sframe.AddrPC.Offset !=
					 (void *)ExceptionRecord->ExceptionAddress) ){
				GetFailedModuleAddr(hProcess, syminfo, addrstr,
						ExceptionRecord->ExceptionAddress);
				stackptr += wsprintf(stackptr, T("*%s\r\n"), addrstr);
			}
			// 指示アドレスの情報を記載
			GetFailedModuleAddr(hProcess, syminfo, addrstr,
					(void *)sframe.AddrPC.Offset);

			if ( stacks || !findapp ){ // 残り行数有りか、Appが見つかっていない
				if ( !findapp && (tstrstr(addrstr, T("(") AppExeName) != NULL)){
					findapp = TRUE;
					if ( stacks <= 1 ) stacks = 2;
				}

				if ( (stacks || findapp) &&
					 (((stackptr - stackinfo) + tstrlen(addrstr)) < (SHOWSTACKSBUFSIZE - 8)) ){
					stackptr += wsprintf(stackptr, T("%d:%s\r\n"), count, addrstr);
					if ( stacks ) stacks--;
				}
			}

			printoutf(L"%s", stackinfo);
		}
	}
	if ( stackinfo == stackptr ){
		GetFailedModuleAddr(hProcess, syminfo, stackinfo,
				ExceptionRecord->ExceptionAddress);
	}
	if ( DSymInitialize != NULL ) DSymCleanup(hProcess);

	printout(L"\r\nTerminate test.\r\n");
	ExitProcess(ExceptionCode);
	return EXCEPTION_EXECUTE_HANDLER;
}
