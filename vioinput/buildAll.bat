@echo off

pushd hidpassthrough
call buildAll.bat
popd

pushd sys
call buildAll.bat
popd

mkdir Install
mkdir Install\W2K
mkdir Install\W2K\x86
copy hidpassthrough\objfre_w2K_x86\i386\viohidkmdf.pdb Install\W2K\x86
copy hidpassthrough\objfre_w2K_x86\i386\viohidkmdf.sys Install\W2K\x86
copy sys\objfre_w2K_x86\i386\vioinput.pdb Install\W2K\x86
copy sys\objfre_w2K_x86\i386\vioinput.sys Install\W2K\x86
copy sys\objfre_w2K_x86\i386\vioinput.inf Install\W2K\x86
copy C:\WinDDK\6001.18002\redist\wdf\x86\WdfCoInstaller01007.dll Install\W2K\x86
