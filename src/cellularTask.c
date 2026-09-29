/**
 * Created on: Sept 1, 2022
 * Created by: golobmichael
 * 
 * Copyright (c) Embedded Planet, Inc - All rights reserved
 *
 * This source file is private and confidential.
 * Unauthorized copying of this file is strictly prohibited.
 * 
 * Version 1.0 - 01SEPT22  Initial, golobmichael
 */

#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include "main.h"
#include "cellularTask.h"
#include "led_helper.h"

// Enable logging for app task
LOG_MODULE_REGISTER(CELL_TASK, LOG_LEVEL_INF);

#define LED_THREAD_PRIORITY     7
#define LED_THREAD_STACK_SIZE   1024
K_THREAD_DEFINE(led_task_id, LED_THREAD_STACK_SIZE, LEDTask, NULL, NULL, NULL, LED_THREAD_PRIORITY, 0, SYS_FOREVER_MS);

cellParam cellParams;
CellRegStatus_t regStatusFlag;
CellStatus_t cellStatus;

/*-----------------------------------------------------------*/
void cellular_task(void)
{
    #define REG_TIMEOUT      900    // 900s
    bool retCellular = false;
    static bool first_run = true;
    uint32_t lastGnssTime = 0, lastGtpTime = 0, sasTokenTimer = 0, currentTime = 0;
    bool fotaAvailable = false;
    int32_t httpsRespSize = 0;
    uint8_t *httpsRespBody;

    /* Start and suspend LED task */
    led_init();
    k_thread_start(led_task_id);
    k_thread_suspend(led_task_id);
    /* pause LEDs */
    led_pause();

    while(1){
        uint8_t satellites = 0;
        static uint8_t gnssFix = 0;
        cellStatus = CellGeneralFail;
        
        /* Wait for item in queue */
        if(newDataAdded == true){
            // Enable LEDs
            led_resume();
            /* Flash LED */
            k_thread_resume(led_task_id);

            /* Clear flag */
            newDataAdded = false;

            /* Initialize cell library and startup modem hardware, don't need to call on first run */
            if( cellInit() != 0 ){
                LOG_ERR("Failed to initialize cell, delay and retry\r\n");
                closeCell(MODEM_SLEEP_STATE);
                k_sleep(K_MSEC(5000));
                continue;
            }

            /* Get current time */
            currentTime = k_uptime_get();

            /* Is it time to attain GNSS fix, will run on second pass */
            if(TELIT_GNSS_STATUS == true && (currentTime - lastGnssTime) > GNSS_ATTEMPT_INTERVAL){
                if( setGNSS(true) != 0 ){
                    LOG_ERR("Failed to enable GNSS, delay and retry\r\n");
                    closeCell(MODEM_SLEEP_STATE);
                    k_sleep(K_MSEC(5000));
                    continue;
                }

                /* Check registration status, advance when registered or timeout */
                for(uint16_t i = 0; i < GNSS_TIMEOUT; i++){
                    gnssFix = statusGNSS(&satellites);
                    if(gnssFix >= 2){
                        /* App callback to add GNSS fix if desired */
                        addGnssQueueMsg(true);
                        break;
                    }
                    /* Not enough satellites aquired after a specified amount of time, break loop */
                    if(LOW_SAT_STATUS == true && satellites < LOW_SAT_NUMBER && i == LOW_SAT_TIMEOUT){
                        LOG_WRN( ">>>  GNSS aquire satellite timeout  <<<\r\n" );
                        break;
                    }
                    k_sleep(K_MSEC(10000));
                }

                /* If fix failed, then add to queue*/
                if(gnssFix < 2){
                    /* App callback to add GNSS fix if desired */
                    addGnssQueueMsg(false);
                }

                /* Return back to cell comms */
                setGNSS(false);

                /* Update last gnss time */
                lastGnssTime = k_uptime_get();
            }

            /* Clear flag */
            first_run = false;

            /* Enable eDRX and register cell */
            if( registerCellular(CELLULAR_CARRIER, REG_TIMEOUT) != 0 ){
                LOG_ERR("Failed to start cell registration process, delay and retry\r\n");
                closeCell(MODEM_SLEEP_STATE);
                k_sleep(K_MSEC(5000));
                continue;
            }

            /* Check registration status */
            regStatusFlag = regStatus();
            if(regStatusFlag != REG_STATUS_REGISTERED_HOME && regStatusFlag != REG_STATUS_ROAMING_REGISTERED){
                /* Wait 10s and check again */
                k_sleep(K_MSEC(10000));
                regStatusFlag = regStatus();
                if(regStatusFlag != REG_STATUS_REGISTERED_HOME && regStatusFlag != REG_STATUS_ROAMING_REGISTERED){
                    LOG_ERR("Failed to register to cell network, delay and retry\r\n");
                    cellStatus = CellRegisterFail;
                    closeCell(MODEM_SLEEP_STATE);
                    k_sleep(K_MSEC(5000));
                    continue;
                }
            }

            /* Is it time to attain GTP fix, will run on second pass */
            if(GTP_STATUS == true && (currentTime - lastGtpTime) > GNSS_ATTEMPT_INTERVAL){
                if(connectCell(IOT_BROKER_ADDRESS_POST, GTP_STATUS) != cellStatus){
                    LOG_ERR("Failed to connect to cell socket, delay and retry\r\n");
                    closeCell(MODEM_SLEEP_STATE);
                    k_sleep(K_MSEC(5000));
                    continue;
                }
                
                /* Update last gnss time */
                lastGtpTime = k_uptime_get();
            }
            else{
                if(connectCell(IOT_BROKER_ADDRESS_POST, false) != cellStatus){
                    LOG_ERR("Failed to connect to cell socket, delay and retry\r\n");
                    closeCell(MODEM_SLEEP_STATE);
                    k_sleep(K_MSEC(5000));
                    continue;
                }
            }
            
            
            if(strstr( IOT_BROKER_ADDRESS_GET, "http://" ) != NULL){
                /* Send HTTP Post (without TLS) */
                sendHttpMsg(METHOD_GET, IOT_BROKER_ADDRESS_GET, SERVER_ADDR_PREFIX_GET, SERVER_ADDR_SUFFIX_ATTR, SERVER_PATH_ACCESS_TOKEN_GET_ENABLED, &fotaAvailable);
            }

            /* Check registration status */
            regStatusFlag = regStatus();
            if(regStatusFlag != REG_STATUS_REGISTERED_HOME && regStatusFlag != REG_STATUS_ROAMING_REGISTERED){
                /* Wait 10s and check again */
                k_sleep(K_MSEC(10000));
                regStatusFlag = regStatus();
                if(regStatusFlag != REG_STATUS_REGISTERED_HOME && regStatusFlag != REG_STATUS_ROAMING_REGISTERED){
                    LOG_ERR("Failed to register to cell network, delay and retry\r\n");
                    cellStatus = CellRegisterFail;
                    closeCell(MODEM_SLEEP_STATE);
                    k_sleep(K_MSEC(5000));
                    continue;
                }
            }

            /* Clear flag again in case new data has arrived */
            newDataAdded = false;

            if(k_msgq_num_used_get(&xCellQueue) != 0){
                if(strstr( IOT_BROKER_ADDRESS_POST, "http://" ) != NULL){
                    /* Send HTTP Post (without TLS) */
                    sendHttpMsg(METHOD_POST, IOT_BROKER_ADDRESS_POST, SERVER_ADDR_PREFIX_POST, SERVER_ADDR_SUFFIX_TELE, SERVER_PATH_ACCESS_TOKEN_POST_ENABLED, NULL);
                }
            }

            closeCell(MODEM_SLEEP_STATE);

            k_thread_suspend(led_task_id);
            // Enable debug out for LED
            led_pause();
        }
        /* Delay between checking queue*/
        k_sleep(K_MSEC(5000));
    }
}

/*-----------------------------------------------------------*/
void LEDTask( void )
{
    for(;;)
    {
        if(regStatusFlag == REG_STATUS_REGISTERED_HOME || regStatusFlag == REG_STATUS_ROAMING_REGISTERED)
        {
            led_mode(LED_SINGLE_BLINK, true, 1);
        }
        else
        {
            led_mode(LED_TRIPLE_BLINK, true, 1);
        }

        k_sleep(K_MSEC(5000));
    }
}