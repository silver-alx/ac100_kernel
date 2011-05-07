/*
 * Copyright (c) 2007-2009 NVIDIA Corporation.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * Neither the name of the NVIDIA Corporation nor the names of its contributors
 * may be used to endorse or promote products derived from this software
 * without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include "nvodm_query_gpio.h"
#include "nvodm_services.h"
#include "nvrm_drf.h"

#define NVODM_ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#define NVODM_PORT(x) ((x) - 'a')


static const NvOdmGpioPinInfo s_display[] = {
    /* DVT PAZ00 LVDS interface */
    { NVODM_PORT('m'), 6, NvOdmGpioPinActiveState_High },   // Enable (LVDS_SHTDN_N) (LO:OFF, HI:ON)
    { NVODM_PORT('u'), 3, NvOdmGpioPinActiveState_High },   // LCD_BL_PWM
    { NVODM_PORT('u'), 4, NvOdmGpioPinActiveState_High },   // LCD_BL_EN
    { NVODM_PORT('a'), 4, NvOdmGpioPinActiveState_High },   // EN_VDD_PNL
};

static const NvOdmGpioPinInfo s_hdmi[] =
{
    /* hdmi hot-plug interrupt pin */
    { NVODM_PORT('n'), 7, NvOdmGpioPinActiveState_High },    // HDMI HPD
};

static const NvOdmGpioPinInfo s_sdio[] = {
    {NVODM_PORT('v'), 5, NvOdmGpioPinActiveState_Low},    // SDIO1_CD#
    /* High for WP and low for read/write */
    {NVODM_PORT('h'), 1, NvOdmGpioPinActiveState_High},    // SDIO1_WP
};

static const NvOdmGpioPinInfo s_sdio3[] = {
    {NVODM_PORT('h'), 2, NvOdmGpioPinActiveState_Low},    // Card Detect for SDIO instance 3
    /* High for WP and low for read/write */
    {NVODM_PORT('h'), 3, NvOdmGpioPinActiveState_High},    // Write Protect for SDIO instance 3
};

static const NvOdmGpioPinInfo s_NandFlash[] = {
    {NVODM_PORT('c'), 7, NvOdmGpioPinActiveState_High}, // Raw NAND WP_N
};

static const NvOdmGpioPinInfo s_Bluetooth[] = {
    {NVODM_PORT('u'), 0, NvOdmGpioPinActiveState_Low}   // BT_RST#
};

static const NvOdmGpioPinInfo s_Wlan[] = {
    {NVODM_PORT('k'), 5, NvOdmGpioPinActiveState_Low},  // WF_PWDN#
    {NVODM_PORT('d'), 1, NvOdmGpioPinActiveState_Low},  // WF_RST#
    {NVODM_PORT('d'), 0, NvOdmGpioPinActiveState_Low}   // WF_LED#
};

/*
static const NvOdmGpioPinInfo s_Power[] = {
    // lid open/close, High = Lid Closed
    {NVODM_GPIO_INVALID_PORT, NVODM_GPIO_INVALID_PIN, NvOdmGpioPinActiveState_Low},
    // power button
    {NVODM_GPIO_INVALID_PORT, NVODM_GPIO_INVALID_PIN, NvOdmGpioPinActiveState_Low}
};
*/

static const NvOdmGpioPinInfo s_WakeFromKeyBoard[] = {
    {NVODM_PORT('j'), 7, NvOdmGpioPinActiveState_Low}   // EC Keyboard Wakeup - T20_WAKE#
};

const NvOdmGpioPinInfo *NvOdmQueryGpioPinMap(NvOdmGpioPinGroup Group,
    NvU32 Instance, NvU32 *pCount)
{
    switch (Group)
    {
        case NvOdmGpioPinGroup_Display:
            *pCount = NVODM_ARRAY_SIZE(s_display);
            return s_display;

        case NvOdmGpioPinGroup_Hdmi:
            *pCount = NVODM_ARRAY_SIZE(s_hdmi);
            return s_hdmi;

        case NvOdmGpioPinGroup_Crt:
            *pCount = 0;
            return NULL;

        case NvOdmGpioPinGroup_Sdio:
            if (Instance == 0)
            {
                *pCount = NVODM_ARRAY_SIZE(s_sdio);
                return s_sdio;
            }
            else if (Instance == 3)
            {
                *pCount = NVODM_ARRAY_SIZE(s_sdio3);
                return s_sdio3;
            }
            else
            {
                *pCount = 0;
                return NULL;
            }

        case NvOdmGpioPinGroup_NandFlash:
            *pCount = NVODM_ARRAY_SIZE(s_NandFlash);
            return s_NandFlash;

        case NvOdmGpioPinGroup_Bluetooth:
            *pCount = NVODM_ARRAY_SIZE(s_Bluetooth);
            return s_Bluetooth;

        case NvOdmGpioPinGroup_Wlan:
            *pCount = NVODM_ARRAY_SIZE(s_Wlan);
            return s_Wlan;

        case NvOdmGpioPinGroup_SpiEthernet:
            *pCount = 0;
            return NULL;
        
        case NvOdmGpioPinGroup_Vi:
            *pCount = 0;
            return NULL;

        case NvOdmGpioPinGroup_Power:
            *pCount = 0;
            return NULL;

        case NvOdmGpioPinGroup_EmbeddedController:
            *pCount = NVODM_ARRAY_SIZE(s_WakeFromKeyBoard);
            return s_WakeFromKeyBoard;

        case NvOdmGpioPinGroup_keypadMisc:
            *pCount = 0;
            return NULL;

        case NvOdmGpioPinGroup_Battery:
            *pCount = 0;
            return NULL;

        default:
            *pCount = 0;
            return NULL;
    }
}
