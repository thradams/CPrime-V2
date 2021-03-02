cl printenum.c ..\cprime_lib.c /Zi /EHsc /Fe
cl printtest.c ..\cprime_lib.c /Zi /EHsc /Fe

REM To debug: devenv /DebugExe printenum.exe
REM This avoids a lot of vc++ projects for each test
