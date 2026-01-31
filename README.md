# Atmospheric Transmittance (Simplified LOWTRAN)

A minimal C++ demo that computes atmospheric transmittance along a slant path using a simplified LOWTRAN-style model.

## Build

```pwsh
g++ c:\Users\zdk\Desktop\project\atm\src\main.cpp `
    c:\Users\zdk\Desktop\project\atm\src\Atmosphere.cpp `
    c:\Users\zdk\Desktop\project\atm\src\LowtranModel.cpp `
    -o c:\Users\zdk\Desktop\project\atm\src\output\atm.exe
```

## Run

```pwsh
c:\Users\zdk\Desktop\project\atm\src\output\atm.exe
```

### Input format

1. `h1_km h2_km ground_range_km visibility_km`
2. `start_um end_um step_um`

Example:
```
0.1 1.0 5.0 23
8 12 0.5
```
