#ifndef ATMOSPHERE_H
#define ATMOSPHERE_H

#include <cmath>
#include <vector>

namespace AtmosphereModel {

    // Constants for 1976 US Standard Atmosphere
    constexpr double EARTH_RADIUS = 6356766.0; // m (effective radius often used in lowtran, or use 6371km)
    constexpr double G0 = 9.80665; // m/s^2
    constexpr double MOLAR_MASS = 0.0289644; // kg/mol
    constexpr double R_GAS = 8.31432; // J/(mol K)

    struct AtmosphereState {
        double temperature; // Kelvin
        double pressure;    // Pascals
        double density;     // kg/m^3
    };

    /**
     * @brief Calculate atmospheric properties at a given geometric altitude using 1976 US Standard Atmosphere.
     * 
     * @param altitude Geometric altitude in meters (Z)
     * @return AtmosphereState Structure containing T, P, rho
     */
    AtmosphereState getState(double altitude);

}

#endif // ATMOSPHERE_H
