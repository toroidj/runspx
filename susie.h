/*-----------------------------------------------------------------------------
	Susie Plug-in 関連の定義			Copyright (c) TORO
 ----------------------------------------------------------------------------*/
//-------------------------------------- DLL 定数
#define SUSIEERROR_NOERROR       0
#define SUSIEERROR_NOTSUPPORT   -1
#define SUSIEERROR_USERCANCEL    1
#define SUSIEERROR_UNKNOWNFORMAT 2
#define SUSIEERROR_BROKENDATA    3
#define SUSIEERROR_EMPTYMEMORY   4
#define SUSIEERROR_FAULTMEMORY   5
#define SUSIEERROR_FAULTREAD     6
#define SUSIEERROR_RESERVED      7 // Susie 内部では Window 関係
#define SUSIEERROR_INTERNAL      8
#define SUSIEERROR_FILEWRITE     9 // Susie 内部で使用
#define SUSIEERROR_EOF           10 // Susie 内部で使用

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

// コールバック
typedef int (__stdcall *SUSIE_PROGRESS)(int nNum, int nDenom, LONG_PTR lData);

// Susie 用の UNIX 時刻
typedef ULONG_PTR susie_time_t;

// Susie 構造体
#pragma pack(push,1)
typedef struct PictureInfo {
	long   left, top;	// 画像を展開する位置
	long   width;	// 画像の幅(pixel)
	long   height;	// 画像の高さ(pixel)
	WORD   x_density;	// 画素の水平方向密度
	WORD   y_density;	// 画素の垂直方向密度
	short  colorDepth;	// １画素当たりのbit数
#ifdef _WIN64
	char   dummy[2]; // アラインメント
#endif
	HLOCAL hInfo;	// 画像内のテキスト情報
} SUSIE_PICTUREINFO;

typedef struct fileInfo {
	unsigned char  method[8];	// 圧縮法の種類
	ULONG_PTR      position;	// ファイル上での位置
	ULONG_PTR      compsize;	// 圧縮されたサイズ
	ULONG_PTR      filesize;	// 元のファイルサイズ
	susie_time_t   timestamp;	// ファイルの更新日時
	char           path[SUSIE_PATH_MAX];	// 相対パス
	char           filename[SUSIE_PATH_MAX];	// ファイル名
	unsigned long  crc;	// CRC
#ifdef _WIN64
	   // 64bit版の構造体サイズは444bytesですが、実際のサイズは
	   // アラインメントにより448bytesになります。環境によりdummyが必要です。
	char dummy[4]; // アラインメント
#endif
} SUSIE_FINFO;

typedef struct fileInfoW {
	unsigned char  method[8];		// 圧縮法の種類
	ULONG_PTR      position;		// ファイル上での位置
	ULONG_PTR      compsize;		// 圧縮されたサイズ
	ULONG_PTR      filesize;		// 元のファイルサイズ
	susie_time_t   timestamp;		// ファイルの更新日時
	WCHAR          path[SUSIE_PATH_MAX];		// 相対パス
	WCHAR          filename[SUSIE_PATH_MAX];	// ファイルネーム
	unsigned long  crc;			// CRC
#ifdef _WIN64
	   // 64bit版の構造体サイズは844bytesですが、実際のサイズは
	   // アラインメントにより848bytesになります。環境によりdummyが必要です。
	char dummy[4]; // アラインメント
#endif
} SUSIE_FINFOW;
#pragma pack(pop)
//-------------------------------------- DLL 関数
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
