@echo "$Id: autoexec-testbox.cmd $"
@echo on
setlocal EnableExtensions
set exe=python.exe
for /f %%x in ('tasklist /NH /FI "IMAGENAME eq %exe%"') do if %%x == %exe% goto end
%SystemDrive%\Python27\python.exe %SystemDrive%\testboxscript\testboxscript\testboxscript.py --testrsrc-server-type=cifs --builds-server-type=cifs
pause
:end
