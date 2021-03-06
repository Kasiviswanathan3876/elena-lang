@echo off

set pre=Microsoft.VisualStudio.Product.
set ids=%pre%Community %pre%Professional %pre%Enterprise %pre%BuildTools
for /f "usebackq tokens=1* delims=: " %%i in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere" -latest -products %ids% -requires Microsoft.Component.MSBuild`) do (
  if /i "%%i"=="installationPath" set InstallDir=%%j
)

IF NOT EXIST %InstallDir%nul goto MissingMSBuildToolsPath
IF NOT EXIST %InstallDir%\MSBuild\15.0\Bin\MSBuild.exe goto MissingMSBuildExe

ECHO =========== Starting Release Compile ==================

ECHO Command line Compiler compiling....
ECHO -----------------------------------
"%InstallDir%\MSBuild\15.0\Bin\MSBuild.exe" %1\elenasrc2\elc\vs\elc15.vcxproj /p:configuration=release
IF NOT %ERRORLEVEL%==0 GOTO CompilerError

ECHO Virtual Machine compiling....
ECHO -----------------------------
"%InstallDir%\MSBuild\15.0\Bin\MSBuild.exe" %1\elenasrc2\elenavm\vs\elenavm15.vcxproj /p:configuration=release
IF NOT %ERRORLEVEL%==0 GOTO CompilerError

ECHO IDE compiling....
ECHO -----------------
"%InstallDir%\MSBuild\15.0\Bin\MSBuild.exe" %1\elenasrc2\ide\vs\elide15.vcxproj /p:configuration=release
IF NOT %ERRORLEVEL%==0 GOTO CompilerError

ECHO Run-Time Engine compiling....
ECHO -----------------------------
"%InstallDir%\MSBuild\15.0\Bin\MSBuild.exe" %1\elenasrc2\elenart\vs\elenart15.vcxproj /p:configuration=release
IF NOT %ERRORLEVEL%==0 GOTO CompilerError

ECHO Script Engine compiling....
ECHO ----------------------------
"%InstallDir%\MSBuild\15.0\Bin\MSBuild.exe" %1\elenasrc2\elenasm\vs\elenasm15.vcxproj /p:configuration=release
IF NOT %ERRORLEVEL%==0 GOTO CompilerError

ECHO Simplified Assembler compiling....
ECHO -----------------------------------
"%InstallDir%\MSBuild\15.0\Bin\MSBuild.exe" %1\elenasrc2\tools\asm2bin\vs\asm2binx15.vcxproj /p:configuration=release
IF NOT %ERRORLEVEL%==0 GOTO CompilerError

ECHO ECODES viewer compiling....
ECHO ---------------------------
"%InstallDir%\MSBuild\15.0\Bin\MSBuild.exe" %1\elenasrc2\tools\ecv\vs\ecv15.vcxproj /p:configuration=release
IF NOT %ERRORLEVEL%==0 GOTO CompilerError

ECHO Virtual Machine Terminal compiling....
ECHO --------------------------------------
"%InstallDir%\MSBuild\15.0\Bin\MSBuild.exe" %1\elenasrc2\tools\elt\vs\elt15.vcxproj /p:configuration=release
IF NOT %ERRORLEVEL%==0 GOTO CompilerError

ECHO Optimization Rule Generator compiling....
ECHO ------------------------------------------
"%InstallDir%\MSBuild\15.0\Bin\MSBuild.exe" %1\elenasrc2\tools\og\vs\og15.vcxproj /p:configuration=release
IF NOT %ERRORLEVEL%==0 GOTO CompilerError

ECHO Syntax Parse Table Generator compiling....
ECHO ------------------------------------------
"%InstallDir%\MSBuild\15.0\Bin\MSBuild.exe" %1\elenasrc2\tools\sg\vs\sg15.vcxproj /p:configuration=release
IF NOT %ERRORLEVEL%==0 GOTO CompilerError

ECHO =========== Release Compiled ==================

ECHO =========== Compiling ELENA files ==================

%1\bin\sg %1\dat\sg\syntax.txt
move %1\dat\sg\syntax.dat %1\bin

%1\bin\og %1\dat\og\rules.txt
move %1\dat\og\rules.dat %1\bin

md lib40
%1\bin\asm2binx %1\src40\core\core_routines.esm lib40\system

%1\bin\asm2binx %1\asm\x32\core.asm %1\bin\x32
%1\bin\asm2binx %1\asm\x32\corex.asm %1\bin\x32
%1\bin\asm2binx %1\asm\x32\coreapi.asm %1\bin\x32
%1\bin\asm2binx %1\asm\x32\core_vm.asm %1\bin\x32
%1\bin\asm2binx %1\asm\x32\core_win.asm %1\bin\x32

echo ========== copying bin files.. ===============

md bin

copy %1\bin\asm2binx.exe bin
copy %1\bin\elc.exe bin
copy %1\bin\elide.exe bin
copy %1\bin\sg.exe bin
copy %1\bin\elt.exe bin
copy %1\bin\ecv.exe bin
copy %1\bin\og.exe bin

copy %1\bin\elenavm.dll bin
copy %1\bin\elenart.dll bin
copy %1\bin\elenasm.dll bin
copy %1\bin\winstub.ex_ bin
copy %1\bin\syntax.dat bin
copy %1\bin\rules.dat bin
copy %1\bin\elc.cfg bin
copy %1\bin\elenavm.cfg bin
copy %1\bin\elenart.cfg bin

echo copying bin\templates files

md bin\templates
copy %1\bin\templates\*.cfg bin\templates\

echo copying bin\x32 files

md bin\x32
copy %1\bin\x32\*.bin bin\x32\

echo copying bin\scripts files

md bin\scripts
copy %1\bin\scripts\*.es bin\scripts\

echo copying dat files

md dat
md dat\sg
copy %1\dat\sg\syntax.txt dat\sg

md dat\og
copy %1\dat\og\rules.txt dat\og

echo copying doc files

md doc
copy %1\doc\license doc\
copy %1\doc\roadmap.txt doc\
copy %1\doc\contributors doc\
copy %1\doc\readme.txt .

rem md doc\api
rem copy %1\doc\api\*.html doc\api

md doc\lang
copy %1\doc\lang\spec.txt  doc\lang

md doc\tech 
copy %1\doc\tech\bytecode.txt doc\tech 
copy %1\doc\tech\knowhow.txt doc\tech 

echo copying elenasrc2 files

md elenasrc2
copy %1\elenasrc2\elena2.workspace elenasrc2

md elenasrc2\common
copy %1\elenasrc2\common\*.cpp elenasrc2\common
copy %1\elenasrc2\common\*.h elenasrc2\common

md elenasrc2\elc
copy %1\elenasrc2\elc\*.cpp elenasrc2\elc
copy %1\elenasrc2\elc\*.h elenasrc2\elc

md elenasrc2\elc\vs
copy %1\elenasrc2\elc\vs\elc15.vcxproj elenasrc2\elc\vs
copy %1\elenasrc2\elc\vs\elc.rc elenasrc2\elc\vs
copy %1\elenasrc2\elc\vs\resource.h elenasrc2\elc\vs

md elenasrc2\elc\win32 
copy %1\elenasrc2\elc\win32\*.cpp elenasrc2\elc\win32
copy %1\elenasrc2\elc\win32\*.h elenasrc2\elc\win32

md elenasrc2\elenavm
copy %1\elenasrc2\elenavm\*.cpp elenasrc2\elenavm
copy %1\elenasrc2\elenavm\*.h elenasrc2\elenavm

md elenasrc2\elenavm\vs
copy %1\elenasrc2\elenavm\vs\elenavm15.vcxproj elenasrc2\elenavm\vs
copy %1\elenasrc2\elenavm\vs\elenavm11.rc elenasrc2\elenavm\vs
copy %1\elenasrc2\elenavm\vs\resource.h elenasrc2\elenavm\vs

md elenasrc2\elenavm\win32 
copy %1\elenasrc2\elenavm\win32\*.cpp elenasrc2\elenavm\win32
copy %1\elenasrc2\elenavm\win32\*.h elenasrc2\elenavm\win32

md elenasrc2\elenart
copy %1\elenasrc2\elenart\*.cpp elenasrc2\elenart
copy %1\elenasrc2\elenart\*.h elenasrc2\elenart

md elenasrc2\elenart\vs
copy %1\elenasrc2\elenart\vs\elenart15.vcxproj elenasrc2\elenart\vs
copy %1\elenasrc2\elenart\vs\elenart.rc elenasrc2\elenart\vs
copy %1\elenasrc2\elenart\vs\resource.h elenasrc2\elenart\vs

md elenasrc2\elenart\win32 
copy %1\elenasrc2\elenart\win32\*.cpp elenasrc2\elenart\win32
copy %1\elenasrc2\elenart\win32\*.h elenasrc2\elenart\win32

md elenasrc2\engine
copy %1\elenasrc2\engine\*.cpp elenasrc2\engine
copy %1\elenasrc2\engine\*.h elenasrc2\engine

md elenasrc2\engine\win32 
copy %1\elenasrc2\engine\win32\*.cpp elenasrc2\engine\win32
copy %1\elenasrc2\engine\win32\*.h elenasrc2\engine\win32

md elenasrc2\gui
copy %1\elenasrc2\gui\*.cpp elenasrc2\gui
copy %1\elenasrc2\gui\*.h elenasrc2\gui

md elenasrc2\gui\winapi32 
copy %1\elenasrc2\gui\winapi32\*.cpp elenasrc2\gui\winapi32 
copy %1\elenasrc2\gui\winapi32\*.h elenasrc2\gui\winapi32 

md elenasrc2\ide
copy %1\elenasrc2\ide\*.cpp elenasrc2\ide
copy %1\elenasrc2\ide\*.h elenasrc2\ide

md elenasrc2\ide\eng
copy %1\elenasrc2\ide\eng\*.h elenasrc2\ide\eng

md elenasrc2\ide\vs
copy %1\elenasrc2\ide\vs\elide15.vcxproj elenasrc2\ide\vs

md elenasrc2\ide\winapi32 
copy %1\elenasrc2\ide\winapi32\*.cpp elenasrc2\ide\winapi32 
copy %1\elenasrc2\ide\winapi32\*.h elenasrc2\ide\winapi32 
copy %1\elenasrc2\ide\winapi32\*.rc elenasrc2\ide\winapi32 

md elenasrc2\ide\winapi32\icons
copy %1\elenasrc2\ide\winapi32\icons\*.bmp elenasrc2\ide\winapi32\icons 
copy %1\elenasrc2\ide\winapi32\icons\*.ico elenasrc2\ide\winapi32\icons 

md elenasrc2\elenasm 
copy %1\elenasrc2\elenasm\*.cpp elenasrc2\elenasm
copy %1\elenasrc2\elenasm\*.h elenasrc2\elenasm

md elenasrc2\elenasm\win32
copy %1\elenasrc2\elenasm\win32\*.cpp elenasrc2\elenasm\win32
copy %1\elenasrc2\elenasm\win32\*.h elenasrc2\elenasm\win32

md elenasrc2\elenasm\vs
copy %1\elenasrc2\elenasm\vs\*.vcxproj elenasrc2\elenasm\vs
copy %1\elenasrc2\elenasm\vs\resource.h elenasrc2\elenasm\vs
copy %1\elenasrc2\elenasm\vs\elenasm.rc elenasrc2\elenasm\vs

md elenasrc2\tools 

md elenasrc2\tools\asm2bin
copy %1\elenasrc2\tools\asm2bin\*.cpp elenasrc2\tools\asm2bin
copy %1\elenasrc2\tools\asm2bin\*.h elenasrc2\tools\asm2bin

md elenasrc2\tools\asm2bin\vs
copy %1\elenasrc2\tools\asm2bin\vs\*.vcxproj elenasrc2\tools\asm2bin\vs
copy %1\elenasrc2\tools\asm2bin\vs\*.h elenasrc2\tools\asm2bin\vs
copy %1\elenasrc2\tools\asm2bin\vs\*.rc elenasrc2\tools\asm2bin\vs

md elenasrc2\tools\sg
copy %1\elenasrc2\tools\sg\*.cpp elenasrc2\tools\sg
copy %1\elenasrc2\tools\sg\*.h elenasrc2\tools\sg
md elenasrc2\tools\sg\win32
copy %1\elenasrc2\tools\sg\win32\*.h elenasrc2\tools\sg\win32

md elenasrc2\tools\sg\vs
copy %1\elenasrc2\tools\sg\vs\*.vcxproj elenasrc2\tools\sg\vs

md elenasrc2\tools\elt
copy %1\elenasrc2\tools\elt\*.cpp elenasrc2\tools\elt
copy %1\elenasrc2\tools\elt\*.h elenasrc2\tools\elt

md elenasrc2\tools\elt\vs
copy %1\elenasrc2\tools\elt\vs\*.vcxproj elenasrc2\tools\elt\vs

md elenasrc2\tools\ecv
copy %1\elenasrc2\tools\ecv\*.cpp elenasrc2\tools\ecv
copy %1\elenasrc2\tools\ecv\*.h elenasrc2\tools\ecv

md elenasrc2\tools\ecv\winapi
copy %1\elenasrc2\tools\ecv\winapi\*.cpp elenasrc2\tools\ecv\winapi
copy %1\elenasrc2\tools\ecv\winapi\*.h elenasrc2\tools\ecv\winapi

md elenasrc2\tools\ecv\vs
copy %1\elenasrc2\tools\ecv\vs\*.vcxproj elenasrc2\tools\ecv\vs

md elenasrc2\tools\og
copy %1\elenasrc2\tools\og\*.cpp elenasrc2\tools\og
copy %1\elenasrc2\tools\og\*.h elenasrc2\tools\og

md elenasrc2\tools\og\vs
copy %1\elenasrc2\tools\og\vs\*.vcxproj elenasrc2\tools\og\vs

md install
copy %1\install\*.iss install
md install\redist
copy %1\install\redist\VC_redist.x86.exe install\redist

echo copying examples files

md examples 

rem md examples\gui\agenda
rem copy %1\examples\gui\agenda\*.l examples\gui\agenda
rem copy %1\examples\gui\agenda\*.prj examples\gui\agenda

md examples\console\binary 
copy %1\examples\console\binary\*.l examples\console\binary
copy %1\examples\console\binary\*.prj examples\console\binary

md examples\console\bsort
copy %1\examples\console\bsort\*.l examples\console\bsort
copy %1\examples\console\bsort\*.prj examples\console\bsort

rem md examples\gui\c_a_g
rem copy %1\examples\gui\c_a_g\*.l examples\gui\c_a_g
rem copy %1\examples\gui\c_a_g\*.prj examples\gui\c_a_g

rem md examples\gui\c_a_g\formulas

rem md examples\gui\c_a_g\formulas\Circulo
rem copy %1\examples\gui\c_a_g\formulas\Circulo\*.bmp examples\gui\c_a_g\formulas\Circulo

rem md examples\gui\c_a_g\formulas\Paralelogramos 
rem copy %1\examples\gui\c_a_g\formulas\Paralelogramos\*.bmp examples\gui\c_a_g\formulas\Paralelogramos 

rem md examples\gui\c_a_g\formulas\Trapezio 
rem copy %1\examples\gui\c_a_g\formulas\Trapezio\*.bmp examples\gui\c_a_g\formulas\Trapezio

rem md examples\c_a_g\gui\formulas\Triangulos 
rem copy %1\examples\gui\c_a_g\formulas\Triangulos\*.bmp examples\gui\c_a_g\formulas\Triangulos 

rem md examples\gui\c_a_g\inf
rem md examples\gui\c_a_g\obj
rem md examples\gui\c_a_g\bin

md examples\script\calculator 
copy %1\examples\script\calculator\*.l examples\script\calculator 
copy %1\examples\script\calculator\*.prj examples\script\calculator 
copy %1\examples\script\calculator\*.es examples\script\calculator 

md examples\console\datetime 
copy %1\examples\console\datetime\*.l examples\console\datetime 
copy %1\examples\console\datetime\*.prj examples\console\datetime  

rem md examples\dices
rem copy %1\examples\dices\*.l examples\dices
rem copy %1\examples\dices\*.prj examples\dices

rem md examples\dices\kniffel
rem copy %1\examples\dices\kniffel\*.l examples\dices\kniffel

rem md examples\eldoc 
rem copy %1\examples\eldoc\*.l examples\eldoc
rem copy %1\examples\eldoc\*.prj examples\eldoc

md examples\console\goods 
copy %1\examples\console\goods\*.l examples\console\goods 
copy %1\examples\console\goods\*.txt examples\console\goods 
copy %1\examples\console\goods\*.prj examples\console\goods 

rem md examples\gui\graphs
rem copy %1\examples\gui\graphs\*.l examples\gui\graphs
rem copy %1\examples\gui\graphs\*.prj examples\gui\graphs

md examples\console\helloworld 
copy %1\examples\console\helloworld\*.l examples\console\helloworld
copy %1\examples\console\helloworld\*.prj examples\console\helloworld
rem copy %1\examples\helloworld\*.es examples\helloworld

md examples\script\interpreter
copy %1\examples\script\interpreter\*.l examples\script\interpreter
copy %1\examples\script\interpreter\*.prj examples\script\interpreter
copy %1\examples\script\interpreter\*.txt examples\script\interpreter
copy %1\examples\script\interpreter\*.es examples\script\interpreter

rem md examples\console\matrix 
rem copy %1\examples\console\matrix\*.l examples\console\matrix 
rem copy %1\examples\console\matrix\*.prj examples\console\matrix 

rem md examples\opencalc
rem copy %1\examples\opencalc\*.bat examples\opencalc
rem copy %1\examples\opencalc\*.vl examples\opencalc

md examples\console\pi
copy %1\examples\console\pi\*.l examples\console\pi
copy %1\examples\console\pi\*.prj examples\console\pi

md examples\console\replace
copy %1\examples\console\replace\*.l examples\console\replace
copy %1\examples\console\replace\*.prj examples\console\replace

md examples\console\sum
copy %1\examples\console\sum\*.l examples\console\sum
copy %1\examples\console\sum\*.prj examples\console\sum

md examples\console\random
copy %1\examples\console\random\*.l examples\console\random
copy %1\examples\console\random\*.prj examples\console\random

md examples\files\textdb
copy %1\examples\files\textdb\*.l examples\files\textdb
copy %1\examples\files\textdb\*.prj examples\files\textdb
copy %1\examples\files\textdb\*.txt examples\files\textdb

md examples\files\textfile
copy %1\examples\files\textfile\*.l examples\files\textfile
copy %1\examples\files\textfile\*.prj examples\files\textfile
copy %1\examples\files\textfile\*.txt examples\files\textfile

md examples\console\words
copy %1\examples\console\words\*.l examples\console\words
copy %1\examples\console\words\*.prj examples\console\words

rem md examples\upndown 
rem md examples\upndown\bin
rem copy %1\examples\upndown\*.l examples\upndown 
rem copy %1\examples\upndown\*.prj examples\upndown 

rem md examples\upndown\gui
rem copy %1\examples\upndown\gui\*.l examples\upndown\gui

rem md examples\upndown\upndown
rem copy %1\examples\upndown\upndown\*.l examples\upndown\upndown 

rem md examples\upndown\dictionary
rem copy %1\examples\upndown\dictionary\*.l examples\upndown\dictionary

rem md examples\gui\notepad 
rem md examples\gui\notepad\bin
rem copy %1\examples\gui\notepad\*.l examples\gui\notepad
rem copy %1\examples\gui\notepad\*.prj examples\gui\notepad 

md examples\console\trans
copy %1\examples\console\trans\*.l examples\console\trans
copy %1\examples\console\trans\*.prj examples\console\trans

rem md examples\timer
rem copy %1\examples\timer\*.l examples\timer
rem copy %1\examples\timer\*.prj examples\timer

md examples\db\sqlite
copy %1\examples\db\sqlite\*.l examples\db\sqlite
copy %1\examples\db\sqlite\*.prj examples\db\sqlite

md examples\script\js
copy %1\examples\script\js\*.l examples\script\js
copy %1\examples\script\js\*.prj examples\script\js
copy %1\examples\script\js\*.xprj examples\script\js
copy %1\examples\script\js\*.txt examples\script\js
copy %1\examples\script\js\*.es examples\script\js
copy %1\examples\script\js\*.js examples\script\js

echo copying src4 files

md src40

md asm
md asm\x32
copy %1\asm\x32\*.asm asm\x32
copy %1\asm\*.esm asm

md src40\core
xcopy %1\src40\core\*.esn src40\core /s

md src40\system
xcopy %1\src40\system\*.l src40\system /s
xcopy %1\src40\system\*.prj src40\system /s

md src40\extensions
xcopy %1\src40\extensions\*.l src40\extensions /s
xcopy %1\src40\extensions\*.prj src40\extensions /s

md src40\net
xcopy %1\src40\net\*.l src40\net /s
xcopy %1\src40\net\*.prj src40\net /s

md src40\forms
xcopy %1\src40\forms\*.l src40\forms /s
xcopy %1\src40\forms\*.prj src40\forms /s

md src40\sqlite
xcopy %1\src40\sqlite\*.l src40\sqlite /s
xcopy %1\src40\sqlite\*.prj src40\sqlite /s

md src40\cellular
xcopy %1\src40\cellular\*.l src40\cellular /s
xcopy %1\src40\cellular\*.prj src40\cellular /s

copy %1\recompile15.bat 
copy %1\rebuild_lib.bat 
copy %1\rebuild_examples.bat 
copy %1\whatsnew.txt

echo copying rosetta files

md examples\rosetta
md examples\rosetta\accumulator
copy %1\examples\rosetta\accumulator\*.l examples\rosetta\accumulator
copy %1\examples\rosetta\accumulator\*.prj examples\rosetta\accumulator

md examples\rosetta\ackermann 
copy %1\examples\rosetta\ackermann\*.l examples\rosetta\ackermann
copy %1\examples\rosetta\ackermann\*.prj examples\rosetta\ackermann

md examples\rosetta\addfield
copy %1\examples\rosetta\addfield\*.l examples\rosetta\addfield 
copy %1\examples\rosetta\addfield\*.prj examples\rosetta\addfield 

md examples\rosetta\amb 
copy %1\examples\rosetta\amb\*.l examples\rosetta\amb
copy %1\examples\rosetta\amb\*.prj examples\rosetta\amb

md examples\rosetta\aplusb
copy %1\examples\rosetta\aplusb\*.l examples\rosetta\aplusb
copy %1\examples\rosetta\aplusb\*.prj examples\rosetta\aplusb

md examples\rosetta\applycallback 
copy %1\examples\rosetta\applycallback\*.l examples\rosetta\applycallback 
copy %1\examples\rosetta\applycallback\*.prj examples\rosetta\applycallback 

md examples\rosetta\arithmeticint
copy %1\examples\rosetta\arithmeticint\*.l examples\rosetta\arithmeticint
copy %1\examples\rosetta\arithmeticint\*.prj examples\rosetta\arithmeticint

md examples\rosetta\arithmmean
copy %1\examples\rosetta\arithmmean\*.l examples\rosetta\arithmmean 
copy %1\examples\rosetta\arithmmean\*.prj examples\rosetta\arithmmean 

md examples\rosetta\associativearrays
copy %1\examples\rosetta\associativearrays\*.l examples\rosetta\associativearrays
copy %1\examples\rosetta\associativearrays\*.prj examples\rosetta\associativearrays

md examples\rosetta\arithmeval
copy %1\examples\rosetta\arithmeval\*.l examples\rosetta\arithmeval
copy %1\examples\rosetta\arithmeval\*.prj examples\rosetta\arithmeval

md examples\rosetta\arrays 
copy %1\examples\rosetta\arrays\*.l examples\rosetta\arrays
copy %1\examples\rosetta\arrays\*.prj examples\rosetta\arrays

md examples\rosetta\arrayconcat
copy %1\examples\rosetta\arrayconcat\*.l examples\rosetta\arrayconcat
copy %1\examples\rosetta\arrayconcat\*.prj examples\rosetta\arrayconcat

md examples\rosetta\smavg 
copy %1\examples\rosetta\smavg\*.l examples\rosetta\smavg 
copy %1\examples\rosetta\smavg\*.prj examples\rosetta\smavg 

md examples\rosetta\arraymode
copy %1\examples\rosetta\arraymode\*.l examples\rosetta\arraymode
copy %1\examples\rosetta\arraymode\*.prj examples\rosetta\arraymode

md examples\rosetta\anonymrec
copy %1\examples\rosetta\anonymrec\*.l examples\rosetta\anonymrec
copy %1\examples\rosetta\anonymrec\*.prj examples\rosetta\anonymrec

md examples\rosetta\median
copy %1\examples\rosetta\median\*.l examples\rosetta\median
copy %1\examples\rosetta\median\*.prj examples\rosetta\median

md examples\rosetta\bitwise
copy %1\examples\rosetta\bitwise\*.l examples\rosetta\bitwise
copy %1\examples\rosetta\bitwise\*.prj examples\rosetta\bitwise

md examples\rosetta\anagram
copy %1\examples\rosetta\anagram\*.l examples\rosetta\anagram
copy %1\examples\rosetta\anagram\*.prj examples\rosetta\anagram
copy %1\examples\rosetta\anagram\*.txt examples\rosetta\anagram

md examples\rosetta\brackets
copy %1\examples\rosetta\brackets\*.l examples\rosetta\brackets
copy %1\examples\rosetta\brackets\*.prj examples\rosetta\brackets

md examples\rosetta\bestshuffle 
copy %1\examples\rosetta\bestshuffle\*.l examples\rosetta\bestshuffle
copy %1\examples\rosetta\bestshuffle\*.prj examples\rosetta\bestshuffle

md examples\rosetta\bullscows
copy %1\examples\rosetta\bullscows\*.l examples\rosetta\bullscows
copy %1\examples\rosetta\bullscows\*.prj examples\rosetta\bullscows

md examples\rosetta\binary
copy %1\examples\rosetta\binary\*.l examples\rosetta\binary
copy %1\examples\rosetta\binary\*.prj examples\rosetta\binary

md examples\rosetta\caesar
copy %1\examples\rosetta\caesar\*.l examples\rosetta\caesar
copy %1\examples\rosetta\caesar\*.prj examples\rosetta\caesar

md examples\rosetta\charmatch
copy %1\examples\rosetta\charmatch\*.l examples\rosetta\charmatch
copy %1\examples\rosetta\charmatch\*.prj examples\rosetta\charmatch

md examples\rosetta\combinations
copy %1\examples\rosetta\combinations\*.l examples\rosetta\combinations
copy %1\examples\rosetta\combinations\*.prj examples\rosetta\combinations

md examples\rosetta\calendar
copy %1\examples\rosetta\calendar\*.l examples\rosetta\calendar
copy %1\examples\rosetta\calendar\*.prj examples\rosetta\calendar

md examples\rosetta\doors
copy %1\examples\rosetta\doors\*.l examples\rosetta\doors
copy %1\examples\rosetta\doors\*.prj examples\rosetta\doors

md examples\rosetta\twentyfour
copy %1\examples\rosetta\twentyfour\*.l examples\rosetta\twentyfour
copy %1\examples\rosetta\twentyfour\*.prj examples\rosetta\twentyfour

md examples\rosetta\simple_windowed_app
copy %1\examples\rosetta\simple_windowed_app\*.l examples\rosetta\simple_windowed_app
copy %1\examples\rosetta\simple_windowed_app\*.prj examples\rosetta\simple_windowed_app

md examples\rosetta\gui_component_interaction
copy %1\examples\rosetta\gui_component_interaction\*.l examples\rosetta\gui_component_interaction
copy %1\examples\rosetta\gui_component_interaction\*.prj examples\rosetta\gui_component_interaction
		
md examples\rosetta\string_append
copy %1\examples\rosetta\string_append\*.l examples\rosetta\string_append
copy %1\examples\rosetta\string_append\*.prj examples\rosetta\string_append

md examples\rosetta\string_case
copy %1\examples\rosetta\string_case\*.l examples\rosetta\string_case
copy %1\examples\rosetta\string_case\*.prj examples\rosetta\string_case

md examples\rosetta\string_comparison
copy %1\examples\rosetta\string_comparison\*.l examples\rosetta\string_comparison
copy %1\examples\rosetta\string_comparison\*.prj examples\rosetta\string_comparison

md examples\rosetta\string_concatenation
copy %1\examples\rosetta\string_concatenation\*.l examples\rosetta\string_concatenation
copy %1\examples\rosetta\string_concatenation\*.prj examples\rosetta\string_concatenation

md examples\rosetta\string_interpolation
copy %1\examples\rosetta\string_interpolation\*.l examples\rosetta\string_interpolation
copy %1\examples\rosetta\string_interpolation\*.prj examples\rosetta\string_interpolation

md examples\rosetta\string_matching
copy %1\examples\rosetta\string_matching\*.l examples\rosetta\string_matching
copy %1\examples\rosetta\string_matching\*.prj examples\rosetta\string_matching

md examples\rosetta\string_prepend
copy %1\examples\rosetta\string_prepend\*.l examples\rosetta\string_prepend
copy %1\examples\rosetta\string_prepend\*.prj examples\rosetta\string_prepend

md examples\rosetta\ninetynine
copy %1\examples\rosetta\ninetynine\*.l examples\rosetta\ninetynine
copy %1\examples\rosetta\ninetynine\*.prj examples\rosetta\ninetynine

md examples\rosetta\evolutionary
copy %1\examples\rosetta\evolutionary\*.l examples\rosetta\evolutionary
copy %1\examples\rosetta\evolutionary\*.prj examples\rosetta\evolutionary

md examples\rosetta\compare_str_list
copy %1\examples\rosetta\compare_str_list\*.l examples\rosetta\compare_str_list
copy %1\examples\rosetta\compare_str_list\*.prj examples\rosetta\compare_str_list

md examples\rosetta\dynamic_var
copy %1\examples\rosetta\dynamic_var\*.l examples\rosetta\dynamic_var
copy %1\examples\rosetta\dynamic_var\*.prj examples\rosetta\dynamic_var

md examples\rosetta\firstclass
copy %1\examples\rosetta\firstclass\*.l examples\rosetta\firstclass
copy %1\examples\rosetta\firstclass\*.prj examples\rosetta\firstclass

md examples\rosetta\loop_multiple_arrays
copy %1\examples\rosetta\loop_multiple_arrays\*.l examples\rosetta\loop_multiple_arrays
copy %1\examples\rosetta\loop_multiple_arrays\*.prj examples\rosetta\loop_multiple_arrays

md examples\rosetta\knutalg
copy %1\examples\rosetta\knutalg\*.l examples\rosetta\knutalg
copy %1\examples\rosetta\knutalg\*.prj examples\rosetta\knutalg

md examples\rosetta\manboy
copy %1\examples\rosetta\manboy\*.l examples\rosetta\manboy
copy %1\examples\rosetta\manboy\*.prj examples\rosetta\manboy

md examples\rosetta\reverse_words_in_string
copy %1\examples\rosetta\reverse_words_in_string\*.l examples\rosetta\reverse_words_in_string
copy %1\examples\rosetta\reverse_words_in_string\*.prj examples\rosetta\reverse_words_in_string

md examples\rosetta\tokenizer
copy %1\examples\rosetta\tokenizer\*.l examples\rosetta\tokenizer
copy %1\examples\rosetta\tokenizer\*.prj examples\rosetta\tokenizer

md examples\rosetta\trigonometric
copy %1\examples\rosetta\trigonometric\*.l examples\rosetta\trigonometric
copy %1\examples\rosetta\trigonometric\*.prj examples\rosetta\trigonometric

md examples\rosetta\toppergroup
copy %1\examples\rosetta\toppergroup\*.l examples\rosetta\toppergroup
copy %1\examples\rosetta\toppergroup\*.prj examples\rosetta\toppergroup

md examples\rosetta\twelvestats
copy %1\examples\rosetta\twelvestats\*.l examples\rosetta\twelvestats
copy %1\examples\rosetta\twelvestats\*.prj examples\rosetta\twelvestats

md examples\rosetta\truncprime
copy %1\examples\rosetta\truncprime\*.l examples\rosetta\truncprime
copy %1\examples\rosetta\truncprime\*.prj examples\rosetta\truncprime

md examples\rosetta\treeview
copy %1\examples\rosetta\treeview\*.l examples\rosetta\treeview
copy %1\examples\rosetta\treeview\*.prj examples\rosetta\treeview

md examples\net\chat
copy %1\examples\net\chat\*.l examples\net\chat
copy %1\examples\net\chat\*.prj examples\net\chat

bin\elc src40\system\system.prj
@echo off 
if %ERRORLEVEL% EQU -2 GOTO CompilerError
@echo on

bin\elc src40\extensions\extensions.prj
@echo off 
if %ERRORLEVEL% EQU -2 GOTO CompilerError
@echo on

rem bin\elc src40\net\net.prj

bin\elc src40\forms\forms.prj
@echo off 
if %ERRORLEVEL% EQU -2 GOTO CompilerError
@echo on

rem bin\elc src40\sqlite\sqlite.prj

bin\elc src40\cellular\cellular.prj
@echo off 
if %ERRORLEVEL% EQU -2 GOTO CompilerError
@echo on

rem bin\elc src40\graphics\graphics.prj
rem bin\elc src40\xforms\xforms.prj

goto:eof
::ERRORS
::---------------------
:MissingMSBuildRegistry
echo Cannot obtain path to MSBuild tools from registry
goto:eof
:MissingMSBuildToolsPath
echo The MSBuild tools path from the registry '%InstallDir%' does not exist
goto:eof
:MissingMSBuildExe
echo The MSBuild executable could not be found at '%InstallDir%'
goto:eof
:CompilerError
echo The MSBuild returns error %ERRORLEVEL%
goto:eof
