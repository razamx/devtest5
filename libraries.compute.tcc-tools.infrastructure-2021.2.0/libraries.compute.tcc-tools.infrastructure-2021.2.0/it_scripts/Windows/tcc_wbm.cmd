@ECHO OFF

net session >nul 2>&1

if %errorLevel% == 0 (
        echo Success: Administrative permissions confirmed.
) else (
	echo Failure: %0 must be run with Administrative Permissions
	goto EOF
)

@ECHO.
ECHO SELECT TYPE DEPLOY CONFIG MACHINE
@echo.
echo BUILD MACHINE - [1]
@echo.
@echo.
set /p variant_deploy="WHICH TYPE DEPLOY?"
IF %variant_deploy% equ 1 (
		SET build_config=1
		goto mount
	)


:mount
@ECHO.
ECHO MOUNT DISTRIB SHARE
@echo.
echo SHARE FOLDER NNSTFSPERC02  - [1]
@echo.
@echo.
set /p variant="WHICH SHARE FOLDER MOUNT?"
IF %variant% equ 1 goto nnstfs
@echo.


:nnstfs
powershell Set-ExecutionPolicy -ExecutionPolicy BYPASS -Scope CurrentUser
set /p nnstfs="ENTER THE LETTER DISK FOR MOUNT NNSTFSPERC02:"
net use %nnstfs%: \\nnstfsperc02.inn.intel.com\TCC\common\distrib\windows_agent 
goto deploy_tcc

:deploy_tcc
@ECHO.
ECHO COPY DISTRIB FILES
robocopy %nnstfs%:\ c:\ins\ /E /Z /COPY:TDASO /DCOPY:T /R:2 /W:5  /MT:64
goto install_python37 

:install_python37
ECHO START DEPLOY PYTHON
c:\ins\python-3.7.0-amd64.exe /quiet InstallAllUsers=1 PrependPath=1 
ECHO END DEPLOY PYTHON
goto install_git

:install_git
ECHO START DEPLOY Git
c:\ins\Git-2.18.0-64-bit.exe /SILENT /NORESTART
ECHO END DEPLOY Git
goto install_7zip

:install_7zip
ECHO START DEPLOY 7Zip
msiexec /i "c:\ins\7z1604-x64.msi" /quiet /passive /norestart
ECHO END DEPLOY 7Zip
goto install_beyondcompare

:install_beyondcompare
ECHO START DEPLOY BC
C:\ins\BeyondCompare\BCompare-3.3.4.14431.exe /SP- /SILENT /NORESTART
ECHO END DEPLOY BC
goto install_MDtoHTML_tools

:install_MDtoHTML_tools
ECHO START DEPLOY MDtoHTML tools
C:\ins\MDtoHTML\pandoc-2.2.3.2-windows-x86_64.msi /quiet /norestart
C:\ins\MDtoHTML\jdk-10.0.2_windows-x64_bin.exe INSTALL_SILENT=Enable INSTALLDIR=C:\java\jdk-10.0.2 REBOOT=Disable
powershell.exe -nologo -noprofile -command "& { Add-Type -A 'System.IO.Compression.FileSystem'; [IO.Compression.ZipFile]::ExtractToDirectory('C:\ins\MDtoHTML\SaxonHE9-8-0-14J.zip', 'C:\SaxonHE9-8-0-14J'); }"
ECHO END DEPLOY MDtoHTML tools
goto install_tcagent

:install_tcagent
ECHO START DEPLOY TeamCITY Agent
powershell.exe -nologo -noprofile -command "& { Add-Type -A 'System.IO.Compression.FileSystem'; [IO.Compression.ZipFile]::ExtractToDirectory('c:\\ins\\BuildAgent.zip', 'c:\\BuildAgent\\');}"
	
SETLOCAL ENABLEDELAYEDEXPANSION
SET _SAMPLE=%COMPUTERNAME%

CALL :LCase _SAMPLE _RESULTS
cscript c:\ins\sed.vbs "C:\BuildAgent\conf\buildAgent.properties" "name=" "name=%_RESULTS%"

ENDLOCAL
GOTO setserver

::	cscript c:\ins\sed.vbs "C:\BuildAgent\conf\buildAgent.properties" "name=" "name=%_RESULT%"

:LCase
:UCase
SET _UCase=A B C D E F G H I J K L M N O P Q R S T U V W X Y Z
SET _LCase=a b c d e f g h i j k l m n o p q r s t u v w x y z
SET _Lib_UCase_Tmp=!%1!
IF /I "%0"==":UCase" SET _Abet=%_UCase%
IF /I "%0"==":LCase" SET _Abet=%_LCase%
FOR %%Z IN (%_Abet%) DO SET _Lib_UCase_Tmp=!_Lib_UCase_Tmp:%%Z=%%Z!
SET %2=%_Lib_UCase_Tmp%
GOTO:EOF

:setserver
cscript c:\ins\sed.vbs "C:\BuildAgent\conf\buildAgent.properties" "serverUrl=http\://buildserver" "serverUrl=https\://teamcity01-ir.devtools.intel.com"

powershell c:\ins\add_tccbuild.ps1

call cd c:\\BuildAgent\\bin\\
call service.install.bat
call service.start.bat
call cd c:\\Users\\tccbuild\\Desktop
	
SET TYPE_CONFIG=BUILD
ECHO END DEPLOY TeamCITY Agent
goto set_env_var




:set_env_var
powershell.exe -ExecutionPolicy Bypass -Command "[System.Environment]::SetEnvironmentVariable('BUILD_AGENT_PURPOSE', 'DEVELOPMENT', 'Machine')"
SETX /M PATH "%PATH%;C:\Program Files\Git\bin\;C:\Program Files (x86)\Pandoc;C:\Program Files\Python37;C:\Program Files\Python37\Scripts;C:\Program Files (x86)\Beyond Compare 3;C:\java\jdk-10.0.2\bin"

goto end

:end

net use %nnstfs%: /delete

ECHO.
ECHO.
ECHO #### 
ECHO # -I- Completed configuring %TYPE_CONFIG% on %COMPUTERNAME%.
ECHO #
ECHO #  	MACHINE WILL BE REBOOT AFTER 30 seconds
ECHO #		AFTER REBOOT SET USER CREDENTIALS FOR
ECHO #		             BUILD SERVICE
ECHO ####
ECHO.

timeout 30
::shutdown /r /t 0

:EOF