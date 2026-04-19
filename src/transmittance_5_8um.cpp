#include <algorithm>
#include <array>
#include <cmath>
#include <iomanip>
#include <iostream>
#include "LowtranModel.h"

namespace {
constexpr double kStartUm = 5.0;
constexpr double kEndUm = 8.0;

static const std::array<double, 61> kWavelengthUm = {
    5.00, 5.05, 5.10, 5.15, 5.20, 5.25, 5.30, 5.35, 5.40, 5.45,
    5.50, 5.55, 5.60, 5.65, 5.70, 5.75, 5.80, 5.85, 5.90, 5.95,
    6.00, 6.05, 6.10, 6.15, 6.20, 6.25, 6.30, 6.35, 6.40, 6.45,
    6.50, 6.55, 6.60, 6.65, 6.70, 6.75, 6.80, 6.85, 6.90, 6.95,
    7.00, 7.05, 7.10, 7.15, 7.20, 7.25, 7.30, 7.35, 7.40, 7.45,
    7.50, 7.55, 7.60, 7.65, 7.70, 7.75, 7.80, 7.85, 7.90, 7.95,
    8.00,
};

// Fixed from HITRAN-derived hapi_data_5_8um.txt (interpolated on 0.05 um grid).
static const std::array<double, 61> kAlphaH2OCmInv = {
    8.14708692e-04, 1.02239212e-05, 3.50218445e-03, 4.89111547e-02, 6.11671210e-01,
    6.16297750e-03, 2.69033473e-03, 8.61406122e-01, 1.11168225e-03, 1.71063094e-03,
    1.56984031e-03, 3.39991596e-02, 1.32509786e-02, 4.06745068e-02, 6.15567546e-03,
    1.90734772e-01, 2.03229684e-02, 2.22159178e-02, 3.12569786e-01, 3.30922495e-01,
    2.41189410e-02, 2.40108421e+00, 2.82227746e-02, 3.15249520e-02, 1.61186378e-02,
    2.27529635e-03, 6.26542490e-04, 1.32239891e-01, 6.70046723e-02, 7.28064094e-01,
    9.66333145e-01, 1.08916559e-01, 5.17211394e-01, 7.01697285e-02, 2.53249027e-02,
    1.52265658e-01, 5.42130292e-02, 8.95995572e-02, 1.95151319e-02, 2.47896106e-02,
    2.64658125e-02, 1.44543657e-01, 1.05110115e-02, 1.01789617e-01, 1.48559882e-02,
    1.27115675e-02, 2.48559484e-02, 2.24754166e-02, 1.28648285e-03, 2.79630633e-03,
    9.20066450e-04, 6.92350875e-04, 3.09310621e-03, 7.93987978e-04, 5.41530814e-05,
    3.82516536e-03, 2.09882063e-04, 3.72941693e-04, 1.93276370e-03, 3.04642285e-04,
    9.32769935e-06,
};

static const std::array<double, 61> kAlphaCO2CmInv = {
    8.03356864e-07, 1.99568477e-05, 1.49162746e-05, 4.49051162e-06, 7.34030126e-04,
    2.25426908e-04, 8.25082075e-06, 5.64225843e-06, 9.39819427e-07, 2.75609634e-07,
    6.52735098e-07, 3.28319141e-08, 1.02079598e-08, 2.18357296e-09, 1.87136864e-09,
    1.87083415e-09, 1.02450728e-09, 9.54594970e-10, 2.22206761e-09, 3.26173498e-11,
    0.00000000e+00, 0.00000000e+00, 7.62768050e-12, 1.89357770e-10, 2.86776684e-10,
    1.04244494e-09, 2.33544952e-10, 6.48799448e-10, 1.36825928e-10, 0.00000000e+00,
    0.00000000e+00, 0.00000000e+00, 0.00000000e+00, 0.00000000e+00, 0.00000000e+00,
    1.79025796e-13, 8.68118769e-12, 5.75031620e-12, 4.93324239e-12, 7.44584357e-12,
    1.33267661e-11, 1.26722086e-13, 0.00000000e+00, 0.00000000e+00, 0.00000000e+00,
    0.00000000e+00, 0.00000000e+00, 0.00000000e+00, 0.00000000e+00, 0.00000000e+00,
    0.00000000e+00, 0.00000000e+00, 0.00000000e+00, 0.00000000e+00, 0.00000000e+00,
    0.00000000e+00, 0.00000000e+00, 0.00000000e+00, 0.00000000e+00, 0.00000000e+00,
    0.00000000e+00,
};

static double interp_coef(const std::array<double, 61>& values, double wavelength_um) {
    if (wavelength_um <= kWavelengthUm.front()) return values.front();
    if (wavelength_um >= kWavelengthUm.back()) return values.back();

    auto it = std::lower_bound(kWavelengthUm.begin(), kWavelengthUm.end(), wavelength_um);
    size_t idx = static_cast<size_t>(it - kWavelengthUm.begin());
    if (kWavelengthUm[idx] == wavelength_um) return values[idx];

    size_t i0 = idx - 1;
    size_t i1 = idx;
    double t = (wavelength_um - kWavelengthUm[i0]) / (kWavelengthUm[i1] - kWavelengthUm[i0]);
    return values[i0] + t * (values[i1] - values[i0]);
}

static double saturation_vapor_density_g_m3(double temp_c) {
    double es_hpa = 6.112 * std::exp((17.67 * temp_c) / (temp_c + 243.5));
    return 216.7 * es_hpa / (temp_c + 273.15);
}

static double saturation_vapor_pressure_pa(double temp_c) {
    double es_hpa = 6.112 * std::exp((17.67 * temp_c) / (temp_c + 243.5));
    return es_hpa * 100.0;
}
}  // namespace

int main() {
    std::cout << "=== Atmospheric Transmittance (5-8um, fixed HITRAN coef) ===\n";
    std::cout << "Input: RH(0~1)  Temperature(C)\n";
    std::cout << "Example: 0.46 15\n\n";

    double hr = 0.46;
    double temp_c = 15.0;
    if (!(std::cin >> hr >> temp_c)) {
        return 0;
    }

    std::cout << "\nPath type (0=horizontal, 1=slant):\n\n";
    int path_type = 0;
    if (!(std::cin >> path_type)) {
        return 0;
    }

    double altitude_km = 0.2;
    double range_km = 1.0;
    double h1_km = 0.0;
    double h2_km = 1.0;
    double ground_range_km = 1.0;
    if (path_type == 0) {
        std::cout << "\nInput: Altitude_km  Range_km\n";
        std::cout << "Example: 0.2 1.0\n\n";
        if (!(std::cin >> altitude_km >> range_km)) {
            return 0;
        }
    } else {
        std::cout << "\nInput: H1_km H2_km GroundRange_km\n";
        std::cout << "Example: 0.0 1.0 0.0\n\n";
        if (!(std::cin >> h1_km >> h2_km >> ground_range_km)) {
            return 0;
        }
    }

    std::cout << "\nInput f0 (saturated water vapor density at 5C, g/m^3):\n";
    std::cout << "Example: 6.8\n\n";
    double f0 = 6.8;
    if (!(std::cin >> f0)) {
        return 0;
    }

    std::cout << "\nInput CO2 ppm (e.g., 400):\n\n";
    double co2_ppm = 400.0;
    if (!(std::cin >> co2_ppm)) {
        return 0;
    }

    std::cout << "\nInput wavelength step for fixed 5-8 um band (e.g., 0.05):\n\n";
    double step_um = 0.05;
    if (!(std::cin >> step_um)) {
        return 0;
    }
    if (step_um <= 0.0) {
        std::cout << "Invalid step_um.\n";
        return 0;
    }

    const double default_h2o_scale = 1.0;
    const double default_co2_scale = 1.0;
    const double default_h2o_alt_scale = 1.0;

    std::cout << "\nInput H2O coefficient scale (enter -1 for default):\n\n";
    double h2o_scale = default_h2o_scale;
    if (!(std::cin >> h2o_scale)) {
        return 0;
    }
    if (h2o_scale < 0.0) {
        h2o_scale = default_h2o_scale;
    }

    std::cout << "\nInput CO2 coefficient scale (enter -1 for default):\n\n";
    double co2_scale = default_co2_scale;
    if (!(std::cin >> co2_scale)) {
        return 0;
    }
    if (co2_scale < 0.0) {
        co2_scale = default_co2_scale;
    }

    std::cout << "\nInput H2O altitude exponent scale (enter -1 for default):\n\n";
    double h2o_alt_scale = default_h2o_alt_scale;
    if (!(std::cin >> h2o_alt_scale)) {
        return 0;
    }
    if (h2o_alt_scale < 0.0) {
        h2o_alt_scale = default_h2o_alt_scale;
    }

    double f = saturation_vapor_density_g_m3(temp_c);
    double f_ratio = (f0 > 0.0) ? (f / f0) : 1.0;

    // Keep consistent with existing 3-5/8-12 path in src/main.cpp (scale_by_mixing_ratio = 1).
    double e_ref_pa = saturation_vapor_pressure_pa(5.0);
    double ref_h2o_mixing_ratio = e_ref_pa / 101325.0;
    double ref_co2_mixing_ratio = co2_ppm * 1.0e-6;

    LowtranTransmittance model;
    model.setHumidity(hr, f_ratio);

    double integral = 0.0;
    double prev_lambda = 0.0;
    double prev_tau = 0.0;
    bool first = true;

    for (double lambda = kStartUm; lambda <= kEndUm + 1e-12; lambda += step_um) {
        double alpha_h2o_cm_inv = interp_coef(kAlphaH2OCmInv, lambda);
        double alpha_co2_cm_inv = interp_coef(kAlphaCO2CmInv, lambda);

        double u1 = alpha_h2o_cm_inv * 1.0e5 * h2o_scale * ref_h2o_mixing_ratio;
        double delta = alpha_co2_cm_inv * 1.0e5 * co2_scale * ref_co2_mixing_ratio;

        double tau = 1.0;
        if (path_type == 0) {
            tau = model.calculateTransmittance(range_km, altitude_km, lambda, u1, delta, h2o_alt_scale);
        } else {
            tau = model.calculateSlantTransmittance(h1_km, h2_km, ground_range_km, lambda, u1, delta, h2o_alt_scale);
        }

        if (!first) {
            double dl = lambda - prev_lambda;
            integral += 0.5 * (prev_tau + tau) * dl;
        } else {
            first = false;
        }

        prev_lambda = lambda;
        prev_tau = tau;
    }

    double band_width = kEndUm - kStartUm;
    double avg_tau = (band_width > 0.0) ? (integral / band_width) : 0.0;

    std::cout << "\nResults:\n";
    std::cout << "  f(T) (g/m^3): " << std::fixed << std::setprecision(4) << f << "\n";
    std::cout << "  f_ratio (f/f0): " << std::fixed << std::setprecision(4) << f_ratio << "\n";
    std::cout << "  Band-average transmittance (5-8um): " << std::fixed << std::setprecision(6) << avg_tau << "\n";

    std::cout << "\n===== CURVE DATA =====\n";
    std::cout << "Wavelength_um,Transmittance,H2O_Alpha_cm_inv,CO2_Alpha_cm_inv\n";

    for (double lambda = kStartUm; lambda <= kEndUm + 1e-12; lambda += step_um) {
        double alpha_h2o_cm_inv = interp_coef(kAlphaH2OCmInv, lambda);
        double alpha_co2_cm_inv = interp_coef(kAlphaCO2CmInv, lambda);

        double u1 = alpha_h2o_cm_inv * 1.0e5 * h2o_scale * ref_h2o_mixing_ratio;
        double delta = alpha_co2_cm_inv * 1.0e5 * co2_scale * ref_co2_mixing_ratio;

        double tau = 1.0;
        if (path_type == 0) {
            tau = model.calculateTransmittance(range_km, altitude_km, lambda, u1, delta, h2o_alt_scale);
        } else {
            tau = model.calculateSlantTransmittance(h1_km, h2_km, ground_range_km, lambda, u1, delta, h2o_alt_scale);
        }

        std::cout << std::fixed << std::setprecision(4) << lambda << ","
                  << std::fixed << std::setprecision(6) << tau << ","
                  << std::scientific << std::setprecision(6) << alpha_h2o_cm_inv << ","
                  << std::scientific << std::setprecision(6) << alpha_co2_cm_inv << "\n";
    }

    std::cout << "===== END CURVE DATA =====\n";
    return 0;
}
