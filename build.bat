echo off
cls
set flags=-wd4201 -wd4389 -wd4018 -wd4100 -wd4505 -nologo -MT -GR- -EHa- -Oi 
IF %1 == system_build (
cl %flags% -nologo win_app.cpp /Z7 /Fespranimator /Fa /link -opt:ref -incremental:no user32.lib Dsound.lib D2d1.lib Dwrite.lib Ole32.lib Comdlg32.lib
cl %flags% spranimator.cpp /Fa /Z7 /link -opt:ref -incremental:no /DLL /EXPORT:updateApplication /OUT:lib_spranimator.dll 
)
IF %1 == application_build (
cl %flags% spranimator.cpp /Z7 /link -opt:ref -incremental:no /DLL /EXPORT:updateApplication /OUT:lib_spranimator.dll
)
IF %1 == application_run (
spranimator
)