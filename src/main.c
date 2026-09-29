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
 * @file    main.c
 * @version See Version in main.h VER defines
 * @author  Embedded Planet, Inc.
 * @author  golobmichael, danmaher
 * @date    20 AUG 2026
 * 
 * @brief Agora53 Standalone main header
 * 
 * Built for use with the nRF Connect SDK
 *
 * This source file is private and confidential.
 * Unauthorized copying of this file is strictly prohibited.
 * 
 * Version 1.0 - 20AUG2026  Initial
 */

//Zephyr includes
#include <zephyr/kernel.h>

#include "main.h"
#include "cellularTask.h"
#include "agora53_bsp.h"
#include "ble_peripheral.h"
#include "led_helper.h"

#define STACK_SIZE 4096
#define THREAD0_PRIORITY 7
#define ADV_TIMEOUT     60      // 60s

// Define buffer and message queue variables for cell
char cell_queue_buffer[CELL_QUEUE_SIZE * sizeof(cell_queue_msg)];
K_MSGQ_DEFINE(xCellQueue, sizeof(cell_queue_msg), CELL_QUEUE_SIZE, 4);

/* Forward declaration of the thread function */
void cellular_task(void);
void LEDTask(void);

/* Define and initialize the thread statically */
K_THREAD_DEFINE(thread0_id, STACK_SIZE, cellular_task, NULL, NULL, NULL, THREAD0_PRIORITY, 0, SYS_FOREVER_MS);
K_THREAD_DEFINE(thread1_id, STACK_SIZE, sensor_task, NULL, NULL, NULL, THREAD0_PRIORITY, 0, SYS_FOREVER_MS);

int main(void)
{
    if(agora53_bsp_init() < 0){
        return -1;
    }

    miscInitialization();

    ble_periph_init();
    ble_periph_adv_start(ADV_TIMEOUT);

    /* Start sensor sample task */
    k_thread_start(thread1_id);
    /* Start Cell Task */
    k_thread_start(thread0_id);
}
