cd \users\trent\code\crystal-picnic\build\steam
call signall.bat
cd \users\trent\code\steamworks\tools\contentbuilder
builder\steamcmd.exe +login trentg FIXME +run_app_build ..\scripts\build_cp.vdf
