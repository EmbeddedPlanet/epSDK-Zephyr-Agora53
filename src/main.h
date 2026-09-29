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
 * @file    main.h
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
#ifndef __MAIN_H__
#define __MAIN_H__

#include <stdlib.h>
#include <stdbool.h>

#include "cellularTask.h"

// Firmware version
#define VER_MAJOR   0
#define VER_MINOR   0
#define VER_PATCH   1

// Time between sensor sample cycles
#define SENSOR_SAMPLE_PERIOD_DEFAULT_MS  30000

// Enable a sensor by setting the corresponding define to 1
#define ICM20602_ACTIVE 1
#define HTU21D_ACTIVE 1
#define VL53L0X_ACTIVE 1
#define BME680_ACTIVE 1

#define INF_BLU_LED DT_NODELABEL(led2)
#define INF_RED_LED DT_NODELABEL(led1)
#define INF_GRN_LED DT_NODELABEL(led0)
#define INF_BAT_MON_EN DT_NODELABEL(bat_mon_en)
#define INF_BOARD_ID_EN DT_NODELABEL(board_id_en)
#define CELL_PWR_EN DT_NODELABEL(cell_pwr_en)
#define CELL_PWR_ON_OFF DT_NODELABEL(cell_on_off)
#define SENSOR_PWR_ENABLE DT_NODELABEL(sensor_pwr_en)
extern const struct gpio_dt_spec led_2;
extern const struct gpio_dt_spec led_1;
extern const struct gpio_dt_spec led_0;
extern const struct gpio_dt_spec gpio_bat_mon_en;
extern const struct gpio_dt_spec board_id_enable;
extern const struct gpio_dt_spec gpio_cell_pwr_en;
extern const struct gpio_dt_spec gpio_cell_on_off;
extern const struct gpio_dt_spec sens_pwr_en;

void miscInitialization( void );

//////////////////////////////////////////////////
/****                                        ****/
/****               CELLULAR                 ****/
/****                                        ****/
//////////////////////////////////////////////////

/**< Used to activate/deactivate cellular in the main application
 * 
 *   0 = Deactivated
 *   1 = Activated
 */
#define CELLULAR_ACTIVE     1

/* GPS status, true or false */
#define TELIT_GNSS_STATUS                    true

/* GPS fix timout in 10s of seconds */
#define GNSS_TIMEOUT                  18

/* GNSS interval for attempting fix in seconds */
#define GNSS_ATTEMPT_INTERVAL     3600U

/* Low satellite timeout status */
#define LOW_SAT_STATUS          true

/* Low satellite timeout threshold in 10s of seconds */
#define LOW_SAT_TIMEOUT         30

/* Low satellite num of satellite threshold */
#define LOW_SAT_NUMBER          3

/* Cellular transmission interval in seconds */
#define MIN_TRANS_INTERVAL    600U

/* Cellular queue size for transmissions */
#define CELL_QUEUE_SIZE                     7

/* Modem sleep state */
#define MODEM_SLEEP_STATE   MODEM_OFF

/* Enable GTP Location Functional */
#define GTP_STATUS          false

/* Select option for server, uncomment one of the following. Contact EP for additional options */
#define THINGSBOARD_HTTP_INTEGRATION    // Send to ThingsBoard HTPP Integration server

/* Both addresses below must be the same protocol for now */
/* Currently HTTP supported for POST */
/* If different endpoint location is required, contact EP for guidance */
#ifdef THINGSBOARD_HTTP_INTEGRATION
    #define IOT_BROKER_ADDRESS_POST "http://thingsboard.cloud:80"
#endif

/* Currently HTTP supported for GET */
#ifdef THINGSBOARD_HTTP_INTEGRATION
    #define IOT_BROKER_ADDRESS_GET "http://thingsboard.cloud:80"
#endif

/**
 * @brief Server path includes access token for post or get. Access token in post is also used for signifying if access token is in MQTT topic
 */
#ifdef THINGSBOARD_HTTP_INTEGRATION
    #define SERVER_PATH_ACCESS_TOKEN_POST_ENABLED   0
    #define SERVER_PATH_ACCESS_TOKEN_GET_ENABLED    1
#endif


/**
 * @brief Server Address Path Prefix for POST or sending data to server on HTTP and CoAP. Use as topic prefix for MQTT
 */
#ifdef THINGSBOARD_HTTP_INTEGRATION
    #define SERVER_ADDR_PREFIX_POST    "/api/v1/"
#endif


/**
 * @brief Server Address Path Prefix for GET or getting data from server. Used for HTTP and CoAP, ignored for MQTT
 */
#ifdef THINGSBOARD_HTTP_INTEGRATION
    #define SERVER_ADDR_PREFIX_GET    "/api/v1/"
#endif

/**
 * @brief Server Address for sending Telemetry Path Suffix. If SERVER_PATH_ACCESS_TOKEN_POST_ENABLED is equal to 0, 
 *        then the complete address for sending data to the cloud is IOT_BROKER_ADDRESS_POST+SERVER_ADDR_SUFFIX_TELE.
 *        If SERVER_PATH_ACCESS_TOKEN_POST_ENABLED is equal to 1, then the complete address is 
 *        IOT_BROKER_ADDRESS_POST+SERVER_ADDR_PREFIX_POST+IMEI+SERVER_ADDR_SUFFIX_TELE.
 *        Only used for CoAP and HTTP. Value ignored for MQTT.
 */
#ifdef THINGSBOARD_HTTP_INTEGRATION
    #define SERVER_ADDR_SUFFIX_TELE "/api/v1/integrations/http/6accb121-2cb5-787b-c645-2f5021538a25"
#endif

/**
 * @brief Server Address Attribute Path Suffix. Used for ThingsBoard HTTP GET or getting data from server. Value ignored for MQTT.
 */
#ifdef THINGSBOARD_HTTP_INTEGRATION
    #define SERVER_ADDR_SUFFIX_ATTR    "/attributes"
#endif


/**
 * @brief Server Address FOTA Path Suffix. Used for ThingsBoard HTTP. Value ignored if not using HTTP.
 */
#ifdef THINGSBOARD_HTTP_INTEGRATION
    #define SERVER_ADDR_SUFFIX_FOTA    "/firmware?"
#endif

/* Flag for signaling new data has been added to cell queue */
extern bool newDataAdded;
extern void cellular_task(void);
extern void sensor_task(void);
extern void LEDTask(void);

#endif