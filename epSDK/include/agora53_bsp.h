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
 * @file    agora53_bsp.h
 * @version 1.0.0
 * @author  Embedded Planet, Inc.
 * @author  golobmichael
 * @date    01 SEPT 2026
 * 
 * @brief Header for bsp library APIs and configurations.
 * versions:
 *  1.0.0 - 28SEPT2023  Initial - golobmichael
 * 
 */

/**
 * @file agora53_bsp.h
 */

#ifndef __AGORA53_BSP_H__
#define __AGORA53_BSP_H__

/* *INDENT-OFF* */
#ifdef __cplusplus
    extern "C" {
#endif
/* *INDENT-ON* */

/**
 * @brief Intializes the Agora53 board.
 * 
 * @return 0 for success, <0 for failure
 */
int agora53_bsp_init( void );

/**
 * @brief Retrieves the battery voltage
 * 
 * @return int representing battery voltage in millivolts
 */
int ep_bsp_read_battery_voltage( void );

/* *INDENT-OFF* */
#ifdef __cplusplus
    }
#endif
/* *INDENT-ON* */

#endif /* __AGORA53_BSP_H__ */