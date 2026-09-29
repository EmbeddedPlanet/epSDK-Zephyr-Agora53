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
 * @file    app_task.c
 * @version See Version in main.h VER defines
 * @author  Embedded Planet, Inc.
 * @author  golobmichael, danmaher
 * @date    20 AUG 2026
 *
 * @brief Agora53 Standalone application task source code
 *
 * Built for use with the nRF Connect SDK
 *
 * This source file is private and confidential.
 * Unauthorized copying of this file is strictly prohibited.
 *
 * Version 1.0 - 20AUG2026  Initial
 */

//Project specific includes
//Zephyr includes
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/i2c.h>
#include <hal/nrf_gpio.h>
#include <zephyr/pm/pm.h>
#include <zephyr/pm/device.h>
#include <zephyr/device.h>
#include "main.h"
#include "cellularTask.h"

#include <zephyr/drivers/spi.h>

//EPSDK includes
#include "icm20602.h"
#include "htu21d.h"
#include "vl53l0x.h"
#include "bme680.h"
#include "time_helper.h"

#include "compact_payload_z.h"
#include "agora53_bsp.h"

// Enable logging for app task
LOG_MODULE_REGISTER(APP_C, LOG_LEVEL_INF);

const struct device *i2c_dev = DEVICE_DT_GET(DT_NODELABEL(i2c1));

#if ICM20602_ACTIVE
#define ICM20602_NODE DT_NODELABEL(icm20602)
static const struct i2c_dt_spec icm20602_i2c = I2C_DT_SPEC_GET(ICM20602_NODE);
ICM20602 icm;
#endif

#if HTU21D_ACTIVE
#define HTU21D_NODE DT_NODELABEL(htu21d)
static const struct i2c_dt_spec htu21d_i2c = I2C_DT_SPEC_GET(HTU21D_NODE);
#endif

#if VL53L0X_ACTIVE
#define VL53L0X_NODE DT_NODELABEL(vl53l0x)
static const struct i2c_dt_spec vl53l0x_i2c = I2C_DT_SPEC_GET(VL53L0X_NODE);
VL53L0X tof;
#endif

#if BME680_ACTIVE
#define BME680_NODE DT_NODELABEL(bme680)
static const struct i2c_dt_spec bme680_i2c = I2C_DT_SPEC_GET(BME680_NODE);
#endif

const struct gpio_dt_spec led_2 = GPIO_DT_SPEC_GET(INF_BLU_LED, gpios);
const struct gpio_dt_spec led_1 = GPIO_DT_SPEC_GET(INF_RED_LED, gpios);
const struct gpio_dt_spec led_0 = GPIO_DT_SPEC_GET(INF_GRN_LED, gpios);
const struct gpio_dt_spec gpio_bat_mon_en = GPIO_DT_SPEC_GET(INF_BAT_MON_EN, gpios);
const struct gpio_dt_spec board_id_enable = GPIO_DT_SPEC_GET(INF_BOARD_ID_EN, gpios);
const struct gpio_dt_spec gpio_cell_pwr_en = GPIO_DT_SPEC_GET(CELL_PWR_EN, gpios);
const struct gpio_dt_spec gpio_cell_on_off = GPIO_DT_SPEC_GET(CELL_PWR_ON_OFF, gpios);
const struct gpio_dt_spec sens_pwr_en = GPIO_DT_SPEC_GET(SENSOR_PWR_ENABLE, gpios);

// Frame counter for payload
static uint8_t frameCounter = 0;
bool newDataAdded;

/* Cellular Transmission Interval */
static uint32_t cellTransInt = MIN_TRANS_INTERVAL;

K_SEM_DEFINE(xSensPwrEnSemaphore, 1, 1);

/*-----------------------------------------------------------*/
void sensorPwrEnConfig( bool status )
{
    static uint8_t counter = 0;

    //Take semaphore
    if(k_sem_take(&xSensPwrEnSemaphore, K_NO_WAIT) != 0)
    {
        return;
    }

    // Configure GPIO to an output
    gpio_pin_configure_dt(&sens_pwr_en, GPIO_OUTPUT);

    //If status is false, and all tasks want this pin disabled, then clear pin
    if(status == false)
    {
        //Decrement counter if above zero
        if(counter > 0)
        {
            counter--;
        }
        //If counter is now 0, then disable
        if(counter == 0)
        {
            gpio_pin_set_dt(&sens_pwr_en, false);
        }
    }

    //If status is true then set pin and increase counter
    if(status == true)
    {
        gpio_pin_set_dt(&sens_pwr_en, true);
        if(counter < 0xFF)
        {
            counter++;
        }
    }

    //Give semaphore
    k_sem_give(&xSensPwrEnSemaphore);
}

/*-----------------------------------------------------------*/
void miscInitialization( void )
{
    int err = 0;

    printk("\r\n------------------------\r\n");
    printk("*     Agora53 epSDK    *\r\n");
    printk("*       V%d.%d.%d         *\r\n", VER_MAJOR, VER_MINOR, VER_PATCH);
    printk("------------------------\r\n");

    //Verify that the Agora has the I/O set to to 3.3V
    if ((NRF_UICR_S->VREGHVOUT & UICR_VREGHVOUT_VREGHVOUT_Msk) != (UICR_VREGHVOUT_VREGHVOUT_3V0 & UICR_VREGHVOUT_VREGHVOUT_Msk))
    {

        k_sleep(K_MSEC(1000));

        NRF_NVMC_S->CONFIG = NVMC_CONFIG_WEN_Wen << NVMC_CONFIG_WEN_Pos;
        while (NRF_NVMC_S->READY == NVMC_READY_READY_Busy) {}
        
        NRF_UICR_S->VREGHVOUT = (UICR_VREGHVOUT_VREGHVOUT_3V0 | ~UICR_VREGHVOUT_VREGHVOUT_Msk);
        while (NRF_NVMC_S->READY == NVMC_READY_READY_Busy) {}

        NRF_NVMC_S->CONFIG = NVMC_CONFIG_WEN_Ren;

        k_sleep(K_MSEC(2000));
        NVIC_SystemReset();

    }

    // CELL_ON_OFF
    err = gpio_pin_configure_dt(&gpio_cell_on_off, (GPIO_INPUT|GPIO_DISCONNECTED));
    if (err) 
    {
        LOG_ERR("CELL ON OFF ERROR");
    }

    // CELL_PWR_EN
    err = gpio_pin_configure_dt(&gpio_cell_pwr_en, GPIO_OUTPUT);
    if (err) 
    {
        LOG_ERR("CELL PWR EN ERROR");
    }
    //Turn off the cell power enable power
    gpio_pin_set_dt(&gpio_cell_pwr_en, 0);

    // BAT_MON_EN
    err = gpio_pin_configure_dt(&gpio_bat_mon_en, GPIO_OUTPUT);
    if (err) 
    {
        LOG_ERR("BATT MON EN ERROR");
    }
    //Turn off the battery monitor enable power
    gpio_pin_set_dt(&gpio_bat_mon_en, 0);

    // BOARD_ID_EN#
    err = gpio_pin_configure_dt(&board_id_enable, GPIO_OUTPUT);
    if (err) 
    {
        LOG_ERR("BOARD ID EN ERROR");
    }
    //Turn off the battery monitor enable power
    gpio_pin_set_dt(&board_id_enable, 0);

    // Start RTC epoch timer
    set_time(0);
}

/*-----------------------------------------------------------*/
void sensor_task(void)
{
    int err = 0;
    cell_queue_msg cell_msg;

    // SENSOR_PWR_EN enable
    sensorPwrEnConfig(true);
    k_msleep(500);

    /* Force modem to run on first pass */
    newDataAdded = true;

    while(1){
        cellDiag parameters = {'\0'};

        //Get latest cell diagnostic values
        if( queryCellularDiag(&parameters) != 0 )
        {
            /* Do not send if unable to attain values */
            continue;
        }

        //Loop and give time for IMEI during power up
        uint8_t loopCnt = 0;
        LOG_INF("Waiting for IMEI to be received");
        while( parameters.imei[0] == 0 && parameters.imei[15] == 0 )
        {
            k_msleep(500);
            queryCellularDiag(&parameters);
            loopCnt++;
            if(loopCnt > 240)
            {
                break;
            }
        }
        //Cannot send data on this loop if IMEI is unavailable
        if(loopCnt > 240)
        {
            LOG_ERR("No IMEI to receive");
            // Force modem to restart and attempt to get IMEI
            newDataAdded = true;
            // Let IMEI be 0 so data continues being sent to cloud
            //continue;
        }
        LOG_INF("IMEI received");

        // Get epoch time
        uint32_t ts = get_time_s();
        LOG_INF("TIME:%u",ts);

        #if BME680_ACTIVE
            // Initialize BME680
            if (!device_is_ready(bme680_i2c.bus))
            {
                printk("I2C bus %s is not ready!\n\r", bme680_i2c.bus->name);
            }
            else
            {
                printk("I2C bus %s is ready\n\r", bme680_i2c.bus->name);
            }
            bme680_init_return_values ret_val;
            ret_val = bme680_init(bme680_i2c, BSEC_SAMPLE_RATE_LP, 0.0f);
            if(ret_val.bme680_status != 0){
                LOG_ERR("bme680_init failure, ret_val:%i", ret_val.bme680_status);
            }
            if(ret_val.bsec_status != 0){
                LOG_ERR("bme680_init failure, ret_val:%i", ret_val.bsec_status);
            }
            if(ret_val.semaphore_status != 0){
                LOG_ERR("bme680_init failure, ret_val:%i", ret_val.semaphore_status);
            }
            if(ret_val.task_status != 0){
                LOG_ERR("bme680_init failure, ret_val:%i", ret_val.task_status);
            }
            if(ret_val.timer_status != 0){
                LOG_ERR("bme680_init failure, ret_val:%i", ret_val.timer_status);
            }
        #endif

        #if ICM20602_ACTIVE
            // Initialize ICM20602
            if (!device_is_ready(icm20602_i2c.bus))
            {
                printk("I2C bus %s is not ready!\n\r", icm20602_i2c.bus->name);
            }
            else
            {
                printk("I2C bus %s is ready\n\r", icm20602_i2c.bus->name);
            }
            icm20602_init(&icm, icm20602_i2c);
        #endif

        #if HTU21D_ACTIVE
            // Initialize HTU21D
            if (!device_is_ready(htu21d_i2c.bus))
            {
                printk("I2C bus %s is not ready!\n\r", htu21d_i2c.bus->name);
            }
            else
            {
                printk("I2C bus %s is ready\n\r", htu21d_i2c.bus->name);
            }
            htu21_init(htu21d_i2c);
        #endif

        #if VL53L0X_ACTIVE
            // Initialize VL53L0X
            if (!device_is_ready(vl53l0x_i2c.bus))
            {
                printk("I2C bus %s is not ready!\n\r", vl53l0x_i2c.bus->name);
            }
            else
            {
                printk("I2C bus %s is ready\n\r", vl53l0x_i2c.bus->name);
            }
            vl53l0x_init(&tof, vl53l0x_i2c);
        #endif

        //Increment frame counter or roll over if needed
        frameCounter = (uint8_t) (frameCounter + 1);

        //Initialize compact payload
        compact_payload_init();

        //Set up data converter for compact payload
        union epcp_convert_type ct;

        //Create system subpacket
        uint8_t epcp_ver = EPCP_VERSION;
        uint8_t updateVerMaj = VER_MAJOR;
        uint8_t updateVerMin = VER_MINOR;
        uint8_t updateVerBui = VER_PATCH;
        uint8_t ep_serial[7] = {"654321\0"};

        ct.ui32 = (ep_bsp_read_battery_voltage()/10);
        LOG_INF("BatV:%umV",(ct.ui32*10));
        if(frameCounter == 1){
            compact_payload_add_data(EPCP_SYSTEM_FULL, EPCP_VER, &epcp_ver);
            compact_payload_add_data(EPCP_SYSTEM_FULL, EPCP_FW_MAJOR, &updateVerMaj);
            compact_payload_add_data(EPCP_SYSTEM_FULL, EPCP_FW_MINOR, &updateVerMin);
            compact_payload_add_data(EPCP_SYSTEM_FULL, EPCP_FW_PATCH, &updateVerBui);
            compact_payload_add_data(EPCP_SYSTEM_FULL, EPCP_MSG_CNT, &frameCounter);
            compact_payload_add_data(EPCP_SYSTEM_FULL, EPCP_SN, ep_serial);
            compact_payload_add_data(EPCP_SYSTEM_FULL, EPCP_TIME, &ts);
            compact_payload_add_data(EPCP_SYSTEM_FULL, EPCP_BATT, &(ct.ui32));
        }else{
            compact_payload_add_data(EPCP_SYSTEM_CONDENSED, EPCP_VER, &epcp_ver);
            compact_payload_add_data(EPCP_SYSTEM_CONDENSED, EPCP_MSG_CNT, &frameCounter);
            compact_payload_add_data(EPCP_SYSTEM_CONDENSED, EPCP_TIME, &ts);
            compact_payload_add_data(EPCP_SYSTEM_CONDENSED, EPCP_BATT, &(ct.ui32));
        }

        //Add Cell data to subpacket
        char* endptr;
        uint64_t hexIMEI = strtoull(parameters.imei,&endptr,10); //Convert IMEI from string to number
        compact_payload_add_data(EPCP_CELL, EPCP_IMEI, &(hexIMEI));
        compact_payload_add_data(EPCP_CELL, EPCP_RSSI, &parameters.rssi);
        compact_payload_add_data(EPCP_CELL, EPCP_RSRQ, &parameters.rsrq);

    #if ICM20602_ACTIVE
        bool icm_data_ready = icm20602_data_ready(&icm, &err);
        if(err)
        {
            LOG_ERR("ICM20602 failure");
        }
        else if(icm_data_ready)
        {
            static icm20602_sensor_data icm_data;
            if (err)
            {
                LOG_ERR("ICM20602 failure");
            }
            else
            {
                icm_data = icm20602_get_data(&icm, &err);
                LOG_INF("ICM20602 Data:");
                LOG_INF("Accel X: %2.2f, Accel Y: %2.2f, Accel Z: %2.2f", (double)icm_data.accel_x, (double)icm_data.accel_y, (double)icm_data.accel_z);
                LOG_INF("Gyro X: %2.2f, Gyro Y: %2.2f, Gyro Z: %2.2f", (double)icm_data.gyro_x, (double)icm_data.gyro_y, (double)icm_data.gyro_z);
                // Add ICM data to compact payload
                ct.i32 = icm_data.accel_x * 100;
                compact_payload_add_data(EPCP_ICM20602, EPCP_ACCEL_X, &(ct.ui32));
                ct.i32 = icm_data.accel_y * 100;
                compact_payload_add_data(EPCP_ICM20602, EPCP_ACCEL_Y, &(ct.ui32));
                ct.i32 = icm_data.accel_z * 100;
                compact_payload_add_data(EPCP_ICM20602, EPCP_ACCEL_Z, &(ct.ui32));
                ct.i32 = icm_data.gyro_x * 100;
                compact_payload_add_data(EPCP_ICM20602, EPCP_GYRO_X, &(ct.ui32));
                ct.i32 = icm_data.gyro_y * 100;
                compact_payload_add_data(EPCP_ICM20602, EPCP_GYRO_Y, &(ct.ui32));
                ct.i32 = icm_data.gyro_z * 100;
                compact_payload_add_data(EPCP_ICM20602, EPCP_GYRO_Z, &(ct.ui32));
            }
        }
    #endif

    #if HTU21D_ACTIVE
        err =  htu21_is_connected();
        if(err){
            LOG_ERR("HTU connection err %d", err);
        }

        float temp = 0;
        float hum = 0;
        htu21_read_temperature_and_relative_humidity(&temp, &hum);
        LOG_INF("HTU21D Data:");
        LOG_INF("Temperature: %2.2f", (double) temp);
        LOG_INF("Humidity: %2.2f", (double) hum);
        //Convert TEMP to int per spec
        ct.i32 = temp * 100;
        compact_payload_add_data(EPCP_SI7021, EPCP_TEMP, &(ct.ui32));
        //Convert HUM to int per spec
        ct.i32 = hum * 100;
        compact_payload_add_data(EPCP_SI7021, EPCP_HUM, &(ct.ui32));
    #endif

    #if VL53L0X_ACTIVE
        static float vl5_data;
        vl5_data = vl53l0x_get_data(&tof, &err);
        if(err){
            LOG_ERR("Unable to get device data! Error %d\r\n", err);
        }

        /* Data is automatically converted and scaled */
        LOG_INF("VL53L0X Data:");
        LOG_INF("Distance: %3.2fmm", (double) vl5_data);
        compact_payload_add_data(EPCP_VL53L0X, EPCP_DIST, &(vl5_data));
    #endif

    #if BME680_ACTIVE
        if(k_sem_take(&xBme680DataReadySemaphore, K_MSEC(100)) == 0){
            /* Get a struct containg the latest bme680 data. Print the data to console */
            bme680_sensor_data read_data = bme680_get_latest_data();
            printk("\r\n");
            LOG_INF("BME680 Data:");
            LOG_INF("Raw Temp:\t\t%2.2f", (double) read_data.raw_temp);
            LOG_INF("Compensated Temp:\t%2.2f", (double) read_data.temp);
            LOG_INF("Raw rH:\t\t%2.2f", (double) read_data.raw_humidity);
            LOG_INF("Compensated rH:\t%2.2f", (double) read_data.humidity);
            LOG_INF("kPa:\t\t\t%2.2f", (double) read_data.raw_pressure);
            LOG_INF("Gas Stab Status:\t%d", read_data.stabStatus);
            LOG_INF("Gas Run-in Status:\t%d", read_data.runInStatus);
            LOG_INF("Raw Gas:\t\t%2.2f", (double) read_data.raw_gas);
            LOG_INF("Gas :\t\t%2.2f", (double) read_data.gas_percentage);
            LOG_INF("co2:\t\t\t%2.2f",(double) read_data. co2_equivalent);
            LOG_INF("co2 Accuracy:\t%d",read_data. co2_accuracy);
            LOG_INF("Breath VOC:\t\t%2.2f", (double) read_data.breath_voc_equivalent);
            LOG_INF("Breath VOC Accuracy:\t%d", read_data.breath_voc_accuracy);
            LOG_INF("Static IAQ:\t\t%2.2f", (double) read_data.static_iaq);
            LOG_INF("Static IAQ Accuracy:\t%d", read_data.static_iaq_accuracy);
            LOG_INF("IAQ:\t\t\t%2.2f", (double) read_data.iaq);
            LOG_INF("IAQ Accuracy:\t%d", read_data.iaq_accuracy);
            // Add BME data to compact payload
            ct.i32 = read_data.temp * 100;
            compact_payload_add_data(EPCP_BME680, EPCP_TEMP, &(ct.ui32));
            ct.i32 = read_data.raw_pressure * 100;
            compact_payload_add_data(EPCP_BME680, EPCP_PRES, &(ct.ui32));
            ct.ui32 = read_data.humidity * 100;
            compact_payload_add_data(EPCP_BME680, EPCP_HUM, &(ct.ui32));
            ct.ui32 = read_data.raw_gas * 100;
            compact_payload_add_data(EPCP_BME680, EPCP_GAS, &(ct.ui32));
            ct.ui32 = read_data.co2_equivalent * 100;
            compact_payload_add_data(EPCP_BME680, EPCP_CO2, &(ct.ui32));
            ct.ui32 = read_data.breath_voc_equivalent * 100;
            compact_payload_add_data(EPCP_BME680, EPCP_BREATH, &(ct.ui32));
            ct.ui32 = read_data.iaq;
            compact_payload_add_data(EPCP_BME680, EPCP_IAQ_SCORE, &(ct.ui32));
            ct.ui32 = read_data.iaq_accuracy;
            compact_payload_add_data(EPCP_BME680, EPCP_IAQ_ACCURACY, &(ct.ui32));
        }
    #endif

        //Receive and print compact payload
        cell_msg.size = compact_payload_get_packet(cell_msg.data);
        LOG_INF("Payload Size: %d", cell_msg.size);
        k_msgq_put(&xCellQueue, &cell_msg.data, K_SECONDS(5));

        newDataAdded = true;

        // Stop and delete the BME thread
        k_thread_abort(&bme_thread_data);
        sensorPwrEnConfig(false);
        k_msleep(cellTransInt*1000);
        sensorPwrEnConfig(true);
    }
}

void addGnssQueueMsg( bool status )
{
    //Create message to send to queue
    static uint32_t prevFixTime = 0;

    cellDiag parameters = {'\0'};
    //Get latest cell values
    if( queryCellularDiag(&parameters) != 0 )
    {
        /* retry once */
        if( queryCellularDiag(&parameters) != 0 )
        {
            LOG_ERR("Failed to query cell diagnostic data");
            return;
        }
    }

    /* Ensure new fix by change in timestamp */
    if(parameters.fixTime == prevFixTime || status == false){
        LOG_ERR("No new fix detected");
        return;
    }

    //Initialize compact payload
    compact_payload_init();

    //Set up data converter
    union epcp_convert_type ct;

    // Increment frame counter
    frameCounter = (uint8_t) (frameCounter + 1);

    uint8_t epcp_ver = EPCP_VERSION;
    uint32_t ts = get_time_s();
    ct.ui32 = (ep_bsp_read_battery_voltage()/10);
    LOG_INF("BatV:%umV",(ct.ui32*10));

    compact_payload_add_data(EPCP_SYSTEM_CONDENSED, EPCP_VER, &epcp_ver);
    compact_payload_add_data(EPCP_SYSTEM_CONDENSED, EPCP_MSG_CNT, &frameCounter);
    compact_payload_add_data(EPCP_SYSTEM_CONDENSED, EPCP_TIME, &ts);
    compact_payload_add_data(EPCP_SYSTEM_CONDENSED, EPCP_BATT, &(ct.ui32));

    //Add Cell data to subpacket
    char* endptr;
    uint64_t hexIMEI = strtoull(parameters.imei,&endptr,10); //Convert IMEI from string to number
    compact_payload_add_data(EPCP_CELL, EPCP_IMEI, &(hexIMEI));
    compact_payload_add_data(EPCP_CELL, EPCP_RSSI, &parameters.rssi);
    compact_payload_add_data(EPCP_CELL, EPCP_RSRQ, &parameters.rsrq);

    if(parameters.fixTime != prevFixTime || status == false){
        //Add satellites
        compact_payload_add_data(EPCP_GNSS, EPCP_SAT, &(parameters.satellites));
        //If failed fix then set values to 0
        if(parameters.fix < 2){
            parameters.lat_float = 0;
            parameters.long_float = 0;
            compact_payload_set_error(EPCP_GNSS,true);
        }
        //Add GNSS data to subpacket
        prevFixTime = parameters.fixTime;
        //Convert LAT from float to int per spec
        ct.i32 = (int32_t) (parameters.lat_float * 100000);
        compact_payload_add_data(EPCP_GNSS, EPCP_LAT, &(ct.ui32));
        //Convert LON from float to int per spec
        ct.i32 = (int32_t) (parameters.long_float * 100000);
        compact_payload_add_data(EPCP_GNSS, EPCP_LON, &(ct.ui32));
    }

    //Generate completed packet
    cell_queue_msg gnss_data_msg;

    //Receive and print compact payload
    gnss_data_msg.size = compact_payload_get_packet(gnss_data_msg.data);
    LOG_INF("Payload Size: %d", gnss_data_msg.size);
    k_msgq_put(&xCellQueue, &gnss_data_msg.data, K_NO_WAIT);
}