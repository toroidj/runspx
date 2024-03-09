/*-----------------------------------------------------------------------------
	異常終了内容の報告	(c)TORO
-----------------------------------------------------------------------------*/
// WINAPI をまとめて読み込む
typedef struct {
	void (WINAPI **APIptr)();	// 取得したAPIへのポインタ(最終なら NULL)
	const char *APIname;		// 取得するAPIの名前
} LOADWINAPISTRUCT;

// LoadWinAPI mode に使用する値
#define LOADWINAPI_HANDLE		0	// HANDLE を使って読み込み
#define LOADWINAPI_LOAD			1	// LoadLibrary を使って DLL をロード
#define LOADWINAPI_GETMODULE	2	// GetModuleHandle を使って DLL を取得
#define LOADWINAPI(ptr,name)	{(void (WINAPI **)())&ptr,name}
#define LOADWINAPI1(name)	{(void (WINAPI **)())&(D ## name),#name}
#define LOADWINAPI1ND(name)	{(void (WINAPI **)())&name,#name}
#define LOADWINAPI1T(name)	{(void (WINAPI **)())&(D ## name),#name TAPITAIL}

extern HANDLE LoadWinAPI(const char *DLLname, HMODULE hDLL, LOADWINAPISTRUCT *apis/*,int mode*/);

#ifdef __cplusplus
extern "C" {
#endif
LONG UnhandledExceptionFilterProc(EXCEPTION_POINTERS *ExceptionInfo);
#ifdef __cplusplus
}
#endif
