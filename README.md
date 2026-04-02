# Atmospheric Transmittance (Simplified LOWTRAN)

A minimal C++ demo that computes atmospheric transmittance along a horizontal path using a simplified LOWTRAN-style model.

## Build

```pwsh
g++ c:\Users\zdk\Desktop\project\atm\src\main.cpp `
    c:\Users\zdk\Desktop\project\atm\src\LowtranModel.cpp `
    -std=c++23 -g -o c:\Users\zdk\Desktop\project\atm\src\atm.exe
```

## Run

```pwsh
c:\Users\zdk\Desktop\project\atm\src\atm.exe
```

## GUI (Qt)

Build (Qt6 required):

```pwsh
cmake -S c:\Users\zdk\Desktop\project\atm\qt_gui -B c:\Users\zdk\Desktop\project\atm\qt_gui\build
cmake --build c:\Users\zdk\Desktop\project\atm\qt_gui\build --config Release
```

Run:

```pwsh
c:\Users\zdk\Desktop\project\atm\qt_gui\build\Release\atm_gui_qt.exe
```

## Installer (Inno Setup)

1) Build Qt GUI first (see GUI section).

2) Run the packaging script (adjust Qt path if needed):

```pwsh
c:\Users\zdk\Desktop\project\atm\installer\build_installer.ps1 -QtDir D:\Qt\6.10.0\mingw_64
```

The installer will be generated in `atm\installer\ATM_Setup.exe`.

## Lowtran7 Auto Runner

Build:

```pwsh
g++ c:\Users\zdk\Desktop\project\atm\src\lowtran_auto.cpp -std=c++17 -o c:\Users\zdk\Desktop\project\atm\src\lowtran_auto.exe
```

Run (default uses `atm\lowtran7` and `Tape5` in that folder, and auto-presses Enter):

```pwsh
c:\Users\zdk\Desktop\project\atm\src\lowtran_auto.exe --coords 800 600
```

Custom paths:

```pwsh
c:\Users\zdk\Desktop\project\atm\src\lowtran_auto.exe --lowtran-dir c:\Users\zdk\Desktop\project\atm\lowtran7 --tape5 c:\path\to\Tape5 --coords 800 600 --timeout 30
```

## Compare ATM vs Lowtran7

Prepare cases in `compare_cases.csv`:

```csv
name,atm_input,tape5,coords_x,coords_y
case1,atm_input_case1.txt,c:\Users\zdk\Desktop\project\atm\lowtran7\example\Tape5,800,600
```

Run comparison:

```pwsh
C:/Users/zdk/AppData/Local/Microsoft/WindowsApps/python3.13.exe c:\Users\zdk\Desktop\project\atm\compare_lowtran.py
```

Generate random cases (also writes Tape5 files):

```pwsh
C:/Users/zdk/AppData/Local/Microsoft/WindowsApps/python3.13.exe c:\Users\zdk\Desktop\project\atm\compare_lowtran.py --generate --count 10 --seed 7
```

Optional ranges and plot:

```pwsh
C:/Users/zdk/AppData/Local/Microsoft/WindowsApps/python3.13.exe c:\Users\zdk\Desktop\project\atm\compare_lowtran.py --generate --count 10 --alt-min 0.2 --alt-max 8 --range-min 0.5 --range-max 2 --step-min 0.05 --step-max 0.2 --plot
```

Plot output: `compare_plot.png` (requires `matplotlib`).

Output columns: `name, atm, lowtran, rel_error`.

### Input format & meanings

1. `RH Temperature_C`
    - `RH`：相对湿度 (0–1)
    - `Temperature_C`：环境温度 (℃)
2. `path_type`
    - 0=水平传输，1=斜程传输
3. `Altitude_km Range_km`（当 `path_type=0`）
    - `Altitude_km`：传输高度 (km)
    - `Range_km`：水平传播距离 (km)
4. `H1_km H2_km GroundRange_km`（当 `path_type=1`）
    - `H1_km`：起始高度 (km)
    - `H2_km`：结束高度 (km)
    - `GroundRange_km`：两点间水平距离 (km)
5. `f0`
    - 5℃时饱和水汽密度 (g/m³)
6. `CO2_ppm`
    - CO2 体积分数 (ppm)
7. `scale_by_mixing_ratio`
    - 是否按混合比缩放系数 (1=是, 0=否)
8. `start_um end_um step_um`
    - 波段范围与积分步长 (μm)
9. `H2O_scale`
    - H2O 系数缩放因子（输入 `-1` 使用拟合默认值）
10. `CO2_scale`
    - CO2 系数缩放因子（输入 `-1` 使用拟合默认值）
11. `H2O_alt_scale`
    - H2O 高度指数缩放因子（输入 `-1` 使用拟合默认值）
12. `H2O_csv_path`
    - 水汽系数 CSV 路径，输入 `-` 使用默认路径
13. `CO2_csv_path`
    - CO2 系数 CSV 路径，输入 `-` 使用默认路径

### Example

Below example reproduces the 3–5 μm, RH=0.46, T=15℃, H=0.2 km, R=1 km case:

```text
0.46 15
0
0.2 1.0
6.8
400
1
3 5 0.05
-1
-1
-1
-
-

### Slant path example (vertical 1 km)

```text
0.46 15
1
0.0 1.0 0.0
6.8
400
1
3 5 0.05
-1
-1
-1
-
-
```
```
