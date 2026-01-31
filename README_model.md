# 低层大气红外吸收透过率计算过程说明

本文档说明 `atm/src` 中模型的计算流程、输入输出含义，并给出与论文公式的对应关系。

> 说明：论文中的 $u_1(\lambda)$ 与 $\delta(\lambda)$ 需要查表；当前代码使用分段常数做近似，以便展示完整流程。

## 1. 输入数据与物理意义

程序交互输入两行：

1. `h1_km h2_km ground_range_km visibility_km`
   - `h1_km`：起始高度（km）
   - `h2_km`：终止高度（km）
   - `ground_range_km`：两点水平距离（km）
   - `visibility_km`：能见度（km）

2. `start_um end_um step_um`
   - 波长范围，单位为微米（$\mu m$）

当前实现按论文主要公式计算吸收透过率（H2O + CO2），气溶胶消光未参与计算（接口保留，返回 1）。

## 2. 核心公式与代码对应

### 2.1 水汽吸收（水平路程）

论文给出：

- 水汽吸收透过率：

$$
\tau_{H_2O}(\lambda) = \exp\left[-u_1(\lambda) \cdot \frac{f}{f_0} \cdot H_r \cdot R \cdot e^{-0.5154H}\right] \tag{10}
$$

代码位置：`LowtranTransmittance::calculateTransmittance`、`calculateMolecularAbsorption`

- 对应变量：
  - $R$ → `range_km`
  - $H$ → `altitude_km`
  - $H_r$ → `DEFAULT_HR`
  - $f/f_0$ → `F_RATIO`
  - $u_1(\lambda)$ → `h2o_u1(wavelength_um)`

### 2.2 CO2 吸收（水平路程）

论文给出：

$$
\tau_{CO_2}(\lambda) = \exp\left[-\delta(\lambda) \cdot R \cdot e^{-0.313H}\right] \tag{18}
$$

代码位置：`LowtranTransmittance::calculateTransmittance`、`calculateMolecularAbsorption`

- 对应变量：
  - $\delta(\lambda)$ → `co2_delta(wavelength_um)`

### 2.3 总吸收透过率

论文给出：

$$
\tau_{z}(\lambda) = \tau_{H_2O}(\lambda) \cdot \tau_{CO_2}(\lambda) \tag{21}
$$

代码位置：`LowtranTransmittance::calculateTransmittance`

### 2.4 倾斜路程（数值积分）

论文给出解析形式（式(11)、(19)），代码采用离散积分近似：

$$
\tau(\lambda) = \exp\left( -\int \left[ u_1(\lambda)\frac{f}{f_0}H_r e^{-0.5154h} + \delta(\lambda)e^{-0.313h} \right] \, dr \right)
$$

代码位置：`LowtranTransmittance::calculateSlantTransmittance`

## 3. 计算流程（代码执行步骤）

1. 读取几何参数与波长范围。
2. 对每个波长 $\lambda$：
   - 计算水汽吸收系数 $u_1(\lambda)$ 与 CO2 吸收系数 $\delta(\lambda)$。
   - 按水平路径或斜路径公式计算透过率 $\tau(\lambda)$。
3. 输出波长-透过率表。

## 4. 输出结果含义

程序输出的 `Transmittance` 是大气透过率（$0~1$）：
- 接近 1：大气衰减弱，红外能量几乎不被吸收。
- 接近 0：大气衰减强，红外能量几乎被吸收。

该结果可用于评估不同波段、不同高度和路径几何条件下的红外传播能力。

## 5. 可改进方向（如果有实验数据）

- 用论文或实验的查表数据替换 `h2o_u1` 与 `co2_delta` 的分段常数。
- 将相对湿度、温度、能见度作为用户输入。
- 加入气溶胶消光模型，合并为更完整的透过率模型。
