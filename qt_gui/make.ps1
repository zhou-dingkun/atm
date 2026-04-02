Remove-Item -Recurse -Force c:\Users\zdk\Desktop\project\atm\qt_gui\build
cmake -S c:\Users\zdk\Desktop\project\atm\qt_gui -B c:\Users\zdk\Desktop\project\atm\qt_gui\build `
  -G "MinGW Makefiles" `
  -DCMAKE_CXX_COMPILER=D:\Qt\Tools\mingw1310_64\bin\g++.exe `
  -DCMAKE_MAKE_PROGRAM=D:\Qt\Tools\mingw1310_64\bin\mingw32-make.exe `
  -DQt6_DIR=D:\Qt\6.10.0\mingw_64\lib\cmake\Qt6
cmake --build c:\Users\zdk\Desktop\project\atm\qt_gui\build
cd build
./atm_gui_qt.exe
cd ..