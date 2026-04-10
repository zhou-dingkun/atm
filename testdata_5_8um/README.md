# 5-8um 标准对比测试数据

本目录用于 ATM 与 lowtran7 的 5-8um 波段对比，保证可复现、字段固定、便于批量统计。

## 数据文件

- `compare_cases_5_8um.csv`（位于仓库根目录）
	- 供 `compare_lowtran.py` 直接读取。
	- 字段：`name, atm_input, tape5, coords_x, coords_y`
- `case_profile_5_8um.csv`
	- 记录每个样本的完整物理参数。
	- 字段：`name, rh, temperature_c, path_type, altitude_km, range_km, f0_g_m3, co2_ppm, band_start_um, band_end_um, step_um, h2o_scale, co2_scale, h2o_alt_scale`
- `atm_inputs/`
	- 每个 case 对应 ATM 输入文件（固定 5-8um）。
- `Tape5/`
	- 每个 case 对应 lowtran7 的 Tape5 文件。

## 规范设置

- 波段固定：`5.000-8.000 um`
- 步长固定：`0.050 um`
- 路径类型：`horizontal`
- 湿度与温度：`RH=0.46, T=15.0C`
- 其他固定参数：`f0=6.8 g/m^3, CO2=400 ppm`
- 系数参数：`H2O_scale=-1, CO2_scale=-1, H2O_alt_scale=-1`

## 复现实验

1. 生成标准数据（如需重建）：

```powershell
python generate_5_8um_cases.py
```

2. 运行 ATM vs lowtran7 对比：

```powershell
python compare_lowtran.py --cases-file compare_cases_5_8um.csv --plot
```

