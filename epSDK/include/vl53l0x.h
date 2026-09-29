/****************************************************************************                                                                     *
 * Copyright (c) 2026 Embedded Planet, Inc.                                 *
 * SPDX-License-Identifier: Apache-2.0                                      *
 *                                                                          *
 * Licensed under the Apache License, Version 2.0 (the "License");          *
 * you may not use this file except in compliance with the License.         *
 * You may obtain a copy of the License at                                  *
 *                                                                          *
 *     http://www.apache.org/licenses/LICENSE-2.0                           *
 *                                                                          *
 * Unless required by applicable law or agreed to in writing, software      *
 * distributed under the License is distributed on an "AS IS" BASIS,        *
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. *
 * See the License for the specific language governing permissions and      *
 * limitations under the License.                                           *
 ****************************************************************************/

/**
 * @file    vl53l0x.h
 * @version 1.0.0
 * @author  Embedded Planet, Inc.
 * @author  Mark Timieski
 * @date    11 FEB 2024
 *
 * @brief Contains functions for the configuration and control of the vl53l0x time of flight sensor.
 *
 * @see Data sheet:     https://www.st.com/en/imaging-and-photonics-solutions/vl53l0x.html
 * @see Error codes:    https://infocenter.nordicsemi.com/index.jsp?topic=%2Fsdk_nrf5_v17.1.0%2Fgroup__nrfx__error__codes.html
 */

#ifndef VL53L0X_H
#define VL53L0X_H

#ifdef ZEPHYROS
#include <zephyr/drivers/i2c.h>
#include <drivers/nrfx_errors.h>
#include <zephyr/kernel.h>
#else
#include "nrfx_twi.h"
#endif

/** @brief Used to hold twi configuration and register data of a single vl53l0x. Required prior to call to vl53l0x_init.
*/
typedef struct
{
    #ifdef ZEPHYROS
    struct i2c_dt_spec dev_i2c;
    #else
    nrfx_twi_t twi;             /** < TWI interface that hosts the VL53L0X */
    #endif
    uint16_t address;           /** < I2C address */
} VL53L0X;


/***************************************
 * Public Functions
 ***************************************/
#ifdef ZEPHYROS
/**
 * @brief   Inits an vl53l0x struct with a device address & the i2c used.
 *
 * @param tof       Pointer to an VL53L0X struct
 * @param dev_i2c   The i2c interface being used
 *
 * @return  0 if successful. For all others, refer to the Global Error Codes document at the top of this file.
 */
int vl53l0x_init(VL53L0X *tof, struct i2c_dt_spec dev_i2c);
#else
/**
 * @brief   Inits an vl53l0x struct with a device address & the twi used.
 *
 * @param tof       Pointer to an VL53L0X struct
 * @param twi       The twi port of the nRF used
 *
 * @return  0 if successful. For all others, refer to the Global Error Codes document at the top of this file.
 */
int vl53l0x_init(VL53L0X *tof, nrfx_twi_t twi);
#endif

/**
 * @brief   Retrieves most recent measurement from the system, and populates a results struct.
 *
 * @param tof       Pointer to an VL53L0X struct
 * @param ret_code  0 if successful. For all others, refer to the Global Error Codes document at the top of this file.
 *
 * @return  vl53l0x_sensor_data in mm as a floating point value
 */
float vl53l0x_get_data(VL53L0X *tof, int *ret_code);

/**
 * @brief   Sets the return signal rate limit check value in MCPS (mega counts per second).
 * "This represents the amplitude of the signal reflected from the target and detected by the device";
 *  setting this limit presumably determines the minimum measurement necessary for the sensor to report
 *  a valid reading.  Setting a lower limit increases the potential range of the sensor but also seems
 *  to increase the likelihood of getting an inaccurate reading because of unwanted reflections from
 *  objects other than the intended target. Defaults to 0.25 MCPS as initialized by the ST API and this library.
 *
 * @param tof              Pointer to an VL53L0X struct
 * @param limit_Mcps       limit in MCPS
 *
 * @return  0 if successful. For all others, refer to the Global Error Codes document at the top of this file.
 */
int setSignalRateLimit(VL53L0X *tof, float limit_Mcps);

/**
 * @brief   Set the measurement timing budget in microseconds (uS), which is the time allowed
 *  for one measurement; this library handles splitting the timing budget among the
 *  sub-steps in the ranging sequence. A longer timing budget allows for more accurate
 *  measurements. Increasing the budget by a factor of N decreases the range measurement
 *  standard deviation by a factor of sqrt(N). Defaults to about 33000uS;
 *  the minimum is 20000uS, 200000uS for high accuracy. Based on VL53L0X_set_measurement_timing_budget_micro_seconds()
 *
 * @param tof             Pointer to an VL53L0X struct
 * @param budget_us       target measurement budget in uS
 *
 * @return  TRUE if successful.
 */
bool setMeasurementTimingBudget(VL53L0X *tof, uint32_t budget_us);


/**
 * @brief   Set the measurement timing budget in microseconds.
 *
 * @param tof             Pointer to an VL53L0X struct
 * @param ret_code  0 if successful. For all others, refer to the Global Error Codes document at the top of this file.
 *
 * @return  Measurement Timing budget in micro-seconds (uS).
 */
uint32_t getMeasurementTimingBudget(VL53L0X *tof, int *ret_code);

#endif