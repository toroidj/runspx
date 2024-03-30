name = run

UseDebug = 0	# 1 �Ȃ� debug �w��
!ifndef UseBCC
UseBCC = 1	# 1 �Ȃ� BCC32 , 0 �Ȃ� CL
!endif
UseTASM = 1	# 1 �Ȃ� TASM�g�p
UseIlink = 0	# 1 �Ȃ� ilink32(BCC 5.5�ȍ~) ���g�p
!ifndef ARM
ARM = 0		# 1 �Ȃ� Arm/Arm64��(���UNICODE��)
!endif
UseUnicode = 1
#=======================================
!ifndef X64
X64 = 0		# 1 �Ȃ� x64��(���UNICODE��)
!else
X64 = 1
!endif

!ifndef UseUnicode
UseUnicode = 0	# 1 �Ȃ� UNICODE ��
!else
  UnicodeCopt	= -DUNICODE
!endif

!if $(X64)
UseBCC	= 0
UseTASM = 0
#UseUnicode	= 1
TAIL	= 64
spi	= sph
!else
spi	= spi
!endif

!if $(ARM)
UseUnicode	= 0
TAIL	= $(TAIL)A
spi	= $(spi)a
!endif

!if $(UseUnicode)
  UnicodeCopt	= -DUNICODE
!else
!endif
#=============================================================== Borland �p�ݒ�
!if $(UseBCC)
.autodepend
!if $(UseDebug)
  DebugCopt	= -R -v	# �ǉ� C �R���p�C���I�v�V����
  DebugLopt	= -v
!endif
!if $(UseTASM)
 C0D	= TINYC0D
 C0DOBJ	= TINYC0D.OBJ
!else
 C0D	= $(C0DLL)
!endif
#-------------------------------------- �R���p�C�����ʎw��
cc	= @BCC32 -4 -c -C -d -H -Hc -w -O -O1 -RT- -x- -DWINVER=0x400 $(bc56opt) $(DebugCopt) $(UnicodeCopt) $(ExCopt) -DRUNSPX=$(spi)
#-------------------------------------- �����J���ʎw��
!if $(UseIlink)
 bc56opt = -Oc -q -a4
 linkopt = @ilink32 -c -m -P -V4.0 -Gn -q $(DebugLopt)
!else
 linkopt = @tlink32 -c -m -P -V4.0 $(DebugLopt)
!endif
  C0CON	= C0X32
  C0GUI	= C0W32
  C0DLL	= C0D32X
#-------------------------------------- �����J(GUI)
linkexe	= $(linkopt) -Tpe -aa
#-------------------------------------- �����J(CONSOLE)
linkcon	= $(linkopt) -Tpe -ap
#-------------------------------------- �����J(DLL)
linkdll	= $(linkopt) -Tpd -aa

#-------------------------------------- �C���|�[�g
implib  = @implib
#-------------------------------------- ���\�[�X�R���p�C��
rc	= @Brc32 -r
#-------------------------------------- �Öق̎w��
.c.obj:
  $(cc) -o$@ $<

.cpp.obj:
  $(cc) -o$@ $<

.rc.res:
  $(rc) -fo$@ $<
#============================================================= Microsoft �p�ݒ�
!else
!if $(UseDebug)
Copt	= /Od /Zi /GS #/analyze
DebugLopt	= /DEBUG /PDB:$(name)$(spi).pdb
!else # UseDebug
!ifdef RELEASE
Copt	= /Gz /O2 /Os /Oy /DRELEASE
!else # RELEASE
Copt	= /Gz /O2 /Os /Oy
!endif # RELEASE
!endif # UseDebug

!if $(X64) # x64, ARM64
X64Copt	= /wd4244 /wd4267
# ���T�C�Y�Ⴂ�x��������(���ł��邾�����̎w����g��Ȃ�����)
!if !$(ARM) # x64
LINK64	= /MACHINE:AMD64 /LARGEADDRESSAWARE
RCOPT	= /d_WIN64
!else # ARM64
LINK64	= /MACHINE:ARM64 /LARGEADDRESSAWARE
Copt	= /O2 /Os /Oy /DRELEASE
X64Copt	= /wd4244 /wd4267
RCOPT	= /d_WIN64 /d_ARM64_ /d_M_ARM64
!endif # ARM64
!else # !$(X64), x86, ARM32
!if $(ARM) # ARM32
LINK64	= /MACHINE:ARM /LARGEADDRESSAWARE
RCOPT	= /d_M_ARM
!endif # ARM32
!endif # X64

#-------------------------------------- �R���p�C��
cc	= @"cl" /GF /nologo /c /DWINVER=0x400 $(Copt) $(UnicodeCopt) $(X64Copt) /W3 /wd4068 /wd4996 /DRUNSPX=$(spi)
# 4068:�s���� #pragma	4996:�Â��֐����g�p����	/Wp64:64bit�ڐA���`�F�b�N

#-------------------------------------- �����J���ʎw��
linkopt = @"link" /NOLOGO /INCREMENTAL:NO /MAP /OPT:REF /OPT:ICF /NXCOMPAT /DYNAMICBASE $(DebugLopt) $(LINK64)
C0DLL	= Kernel32.Lib AdvAPI32.Lib Ole32.Lib Gdi32.Lib \
	  Shell32.Lib User32.Lib comctl32.lib Comdlg32.lib
C0GUI	= $(C0DLL)
C0CON	= $(C0DLL)
#-------------------------------------- �����J(GUI)
linkexe	= $(linkopt) /SUBSYSTEM:WINDOWS
#-------------------------------------- �����J(CONSOLE)
linkcon	= $(linkopt) /SUBSYSTEM:CONSOLE
#-------------------------------------- �����J(DLL)
linkdll	= $(linkopt) /DLL
#-------------------------------------- ���\�[�X�R���p�C��
rc	= @%COMSPEC% /C RC /dSDKRC $(RCOPT) $(UnicodeCopt)

#-------------------------------------- �Öق̎w��
.c.obj:
  $(cc) /Fo$@ $<

.cpp.obj:
  $(cc) /Fo$@ $<

.rc.res:
  $(rc) $<
!endif

#------------------------------------------------------------------------------
allFiles:	makefile  CODE$(TAIL).OBJ  $(name)$(spi).exe
obj = $(name).obj run_code.obj run_test.obj run_sub.obj faultcon.obj

#------------------------------------------------------ code�̌n�؊��p
CODE$(TAIL).OBJ:
  -@del *.obj 2> nul
  -@del *.res 2> nul
  -@if exist %CSM% del %CSM%\*.CSM
  @copy nul CODE$(TAIL).OBJ > nul

#------------------------------------------------------------------------------
$(name)$(spi).exe: $(obj) app.res
!if $(UseBCC)
  $(linkcon) $(C0CON) $(obj),$<,$*,$(DebugLib) NOEH32 IMPORT32 CW32,,app.res
!else
  $(linkcon) $(C0CON) $(obj) /OUT:$&.exe app.res
!endif

$(name).obj: $(name).cpp $(name).h susie.h torowin.h faultcon.h
run_code.obj: run_code.cpp $(name).h susie.h torowin.h
run_test.obj: run_test.cpp $(name).h susie.h torowin.h
run_sub.obj: run_sub.cpp $(name).h susie.h torowin.h
