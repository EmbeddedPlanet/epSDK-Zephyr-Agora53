/****************************************************************************
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
 * @file    time_helper.h
 * @version 0.0.1
 * @author  Embedded Planet, Inc.
 * @author  Dan Maher
 * @date    31 JUL 2023
 *
 * @brief Utility to assist in epoch or runtime timekeeping
 *
 */

#ifndef TIME_HELPER_H
#define TIME_HELPER_H

#include <inttypes.h>
#ifdef ZEPHYROS
#include <zephyr/kernel.h>
#include <drivers/nrfx_errors.h>
#include "main.h"
#else
#include "nrfx.h"
#endif

/**
 * @brief Sets the system time initial timestamp
 *
 * @param seconds Can be anything, but should be synced with a time source to the current Unix epoch. Time in seconds elapsed since January 1, 1970.
 *                Often initialized to 0 prior to syncing to track time between start and sync.
 *
 * @return nrfx_err_t 1 for failure to create semaphore. For all others refer to nrfx_err_t codes
 */
nrfx_err_t set_time(uint32_t seconds);

/**
 * @brief Gets the current time in seconds
 *
 * @return uint32_t Time in seconds per the most recent timestamp set by set_time()
 */
uint32_t get_time_s();

/**
 * @brief Gets the current time in milliseconds
 *
 * @return uint32_t Time in milliseconds per the most recent timestamp set by set_time()
 */
uint64_t get_time_ms();

#endif