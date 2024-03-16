/*=============================================================================
=============================================================================*/
#define Q_Version 1,0,0,0
#define InfoVer "1.0"
#define InfoCopy "(c)2024"

#ifndef _WIN64
 #ifdef _M_ARM
  #define APPNAME "Runspia"
  #define P_Processor "ARM32"
 #else
  #define APPNAME "Runspi"
  #define P_Processor "x86"
 #endif
#else
 #ifdef _M_ARM64
  #define APPNAME "Runspha"
  #define P_Processor "ARM64"
 #else
  #define APPNAME "Runsph"
  #define P_Processor "AMD64"
 #endif
#endif

#define Copyright		"Copyright " InfoCopy " TORO"
#define P_ProductName	APPNAME
#define P_Description	APPNAME
#define P_Comments		""
#define P_Company_NAME	"toroid"
#define P_Copyright		Copyright
#define X_Version InfoVer

#define CFG_COMPANY	"TOROID"	// AppData ‚Ì’¼‰º‚ÉŽg—p‚·‚é–¼‘O

