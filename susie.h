/*-----------------------------------------------------------------------------
	Susie Plug-in �֘A�̒�`			Copyright (c) TORO
 ----------------------------------------------------------------------------*/
//-------------------------------------- DLL �萔
#define SUSIEERROR_NOERROR       0
#define SUSIEERROR_NOTSUPPORT   -1
#define SUSIEERROR_USERCANCEL    1
#define SUSIEERROR_UNKNOWNFORMAT 2
#define SUSIEERROR_BROKENDATA    3
#define SUSIEERROR_EMPTYMEMORY   4
#define SUSIEERROR_FAULTMEMORY   5
#define SUSIEERROR_FAULTREAD     6
#define SUSIEERROR_RESERVED      7 // Susie �����ł� Window �֌W
#define SUSIEERROR_INTERNAL      8
#define SUSIEERROR_FILEWRITE     9 // Susie �����Ŏg�p
#define SUSIEERROR_EOF           10 // Susie �����Ŏg�p

#define SUSIE_SOURCE_MASK 7
#define SUSIE_SOURCE_DISK 0
#define SUSIE_SOURCE_MEM 1
#define SUSIE_SOURCE_IGNORECASE 0x80
#define SUSIE_DEST_MASK 0x700
#define SUSIE_DEST_DISK 0
#define SUSIE_DEST_MEM 0x100
#define SUSIE_DEST_REJECT_UNKNOWN_TYPE 0x800
#define SUSIE_DEST_EXTRA_OPTION 0x1000

#define SUSIE_CHECK_SIZE (2 * 1024)
#define SUSIE_PATH_MAX 200

// �R�[���o�b�N
typedef int (__stdcall *SUSIE_PROGRESS)(int nNum, int nDenom, LONG_PTR lData);

// Susie �p�� UNIX ����
typedef ULONG_PTR susie_time_t;

// Susie �\����
#pragma pack(push,1)
typedef struct PictureInfo {
	long   left, top;	// �摜��W�J����ʒu
	long   width;	// �摜�̕�(pixel)
	long   height;	// �摜�̍���(pixel)
	WORD   x_density;	// ��f�̐����������x
	WORD   y_density;	// ��f�̐����������x
	short  colorDepth;	// �P��f�������bit��
#ifdef _WIN64
	char   dummy[2]; // �A���C�������g
#endif
	HLOCAL hInfo;	// �摜���̃e�L�X�g���
} SUSIE_PICTUREINFO;

typedef struct fileInfo {
	unsigned char  method[8];	// ���k�@�̎��
	ULONG_PTR      position;	// �t�@�C����ł̈ʒu
	ULONG_PTR      compsize;	// ���k���ꂽ�T�C�Y
	ULONG_PTR      filesize;	// ���̃t�@�C���T�C�Y
	susie_time_t   timestamp;	// �t�@�C���̍X�V����
	char           path[SUSIE_PATH_MAX];	// ���΃p�X
	char           filename[SUSIE_PATH_MAX];	// �t�@�C����
	unsigned long  crc;	// CRC
#ifdef _WIN64
	   // 64bit�ł̍\���̃T�C�Y��444bytes�ł����A���ۂ̃T�C�Y��
	   // �A���C�������g�ɂ��448bytes�ɂȂ�܂��B���ɂ��dummy���K�v�ł��B
	char dummy[4]; // �A���C�������g
#endif
} SUSIE_FINFO;

typedef struct fileInfoW {
	unsigned char  method[8];		// ���k�@�̎��
	ULONG_PTR      position;		// �t�@�C����ł̈ʒu
	ULONG_PTR      compsize;		// ���k���ꂽ�T�C�Y
	ULONG_PTR      filesize;		// ���̃t�@�C���T�C�Y
	susie_time_t   timestamp;		// �t�@�C���̍X�V����
	WCHAR          path[SUSIE_PATH_MAX];		// ���΃p�X
	WCHAR          filename[SUSIE_PATH_MAX];	// �t�@�C���l�[��
	unsigned long  crc;			// CRC
#ifdef _WIN64
	   // 64bit�ł̍\���̃T�C�Y��844bytes�ł����A���ۂ̃T�C�Y��
	   // �A���C�������g�ɂ��848bytes�ɂȂ�܂��B���ɂ��dummy���K�v�ł��B
	char dummy[4]; // �A���C�������g
#endif
} SUSIE_FINFOW;
#pragma pack(pop)
//-------------------------------------- DLL �֐�
typedef int (__stdcall *GETPLUGININFO)(int infono, LPSTR buf, int buflen);
typedef int (__stdcall *GETPLUGININFOW)(int infono, LPWSTR buf, int buflen);
typedef int (__stdcall *ISSUPPORTED)(LPCSTR filename, void *dw);
typedef int (__stdcall *ISSUPPORTEDW)(LPCWSTR filename, void *dw);
typedef int (__stdcall *CONFIGURATIONDLG)(HWND hWnd, int function);
typedef int (__stdcall *GETPICTUREINFO)(LPCSTR buf, LONG_PTR len, unsigned int flag, struct PictureInfo *lpInfo);
typedef int (__stdcall *GETPICTUREINFOW)(LPCWSTR buf, LONG_PTR len, unsigned int flag, struct PictureInfo *lpInfo);
typedef int (__stdcall *GETPICTURE)(LPCSTR buf, LONG_PTR len, unsigned int flag, HANDLE *pHBInfo, HANDLE *pHBm, FARPROC lpPrgressCallback, LONG_PTR lData);
typedef int (__stdcall *GETPICTUREW)(LPCWSTR buf, LONG_PTR len, unsigned int flag, HANDLE *pHBInfo, HANDLE *pHBm, FARPROC lpPrgressCallback, LONG_PTR lData);
typedef int (__stdcall *GETPREVIEW)(LPCSTR buf, LONG_PTR len, unsigned int flag, HANDLE *pHBInfo, HANDLE *pHBm, FARPROC lpPrgressCallback, LONG_PTR lData);
typedef int (__stdcall *GETPREVIEWW)(LPCWSTR buf, LONG_PTR len, unsigned int flag, HANDLE *pHBInfo, HANDLE *pHBm, FARPROC lpPrgressCallback, LONG_PTR lData);
typedef int (__stdcall *GETARCHIVEINFO)(LPCSTR buf, LONG_PTR len, unsigned int flag, HLOCAL *lphInf);
typedef int (__stdcall *GETARCHIVEINFOW)(LPCWSTR buf, LONG_PTR len, unsigned int flag, HLOCAL *lphInf);
typedef int (__stdcall *GETFILE)(LPCSTR src, LONG_PTR len, LPSTR dest, unsigned int flag, FARPROC prgressCallback, LONG_PTR lData);
typedef int (__stdcall *GETFILEW)(LPCWSTR src, LONG_PTR len, LPWSTR dest, unsigned int flag, FARPROC prgressCallback, LONG_PTR lData);
typedef int (__stdcall *GETFILEINFO)(LPCSTR buf, LONG_PTR len, LPCSTR filename, unsigned int flag, SUSIE_FINFO *lpInfo);
typedef int (__stdcall *GETFILEINFOW)(LPCWSTR buf, LONG_PTR len, LPCWSTR filename, unsigned int flag, SUSIE_FINFOW *lpInfo);
typedef int (__stdcall *CONFIGURATIONDLG)(HWND hWnd, int function);
typedef int (__stdcall *CREATEPICTURE)(LPCSTR filepath, unsigned int flag, HANDLE *pHBInfo, HANDLE *pHBm, void *lpInfo, void *progressCallback, LONG_PTR lData);
typedef int (__stdcall *CREATEPICTUREW)(LPCWSTR filepath, unsigned int flag, HANDLE *pHBInfo, HANDLE *pHBm, void *lpInfo, void *progressCallback, LONG_PTR lData);