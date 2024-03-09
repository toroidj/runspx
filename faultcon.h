/*-----------------------------------------------------------------------------
	�ُ�I�����e�̕�	(c)TORO
-----------------------------------------------------------------------------*/
// WINAPI ���܂Ƃ߂ēǂݍ���
typedef struct {
	void (WINAPI **APIptr)();	// �擾����API�ւ̃|�C���^(�ŏI�Ȃ� NULL)
	const char *APIname;		// �擾����API�̖��O
} LOADWINAPISTRUCT;

// LoadWinAPI mode �Ɏg�p����l
#define LOADWINAPI_HANDLE		0	// HANDLE ���g���ēǂݍ���
#define LOADWINAPI_LOAD			1	// LoadLibrary ���g���� DLL �����[�h
#define LOADWINAPI_GETMODULE	2	// GetModuleHandle ���g���� DLL ���擾
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
