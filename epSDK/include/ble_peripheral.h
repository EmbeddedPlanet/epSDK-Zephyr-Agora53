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
 * @file    ble_peripheral
 * @version 1.0.0
 * @author  Embedded Planet, Inc.
 * @author  maherdan
 * @date    01 SEPT 2026
 * 
 * @brief Header for BLE peripheral APIs and configurations.
 * versions:
 *  1.0.0 - 28SEPT2026  Initial - maherdan
 * 
 */

/**
 * @file ble_peripheral.h
 */

#ifndef BLE_PERIPHERAL
#define BLE_PERIPHERAL

#include <stdbool.h>

/**
 * @brief Starts BLE advertising for set period of time
 *
 * @param seconds 32-bit number representing seconds for advertising to remain active after a power-up
 *
 * @return N/A
 */
void ble_periph_adv_start(uint32_t seconds);

/**
 * @brief Initializes BLE library and prepares advertising functionality
 *
 * @return 0 for success, <0 for failure
 */
int ble_periph_init(void);

/**
 * @brief Get the BLE advertising status
 *
 *
 * @return 0 for advertising disabled, 1 for advertising enabled
 */
bool ble_periph_adv_get_status(void);

#endif