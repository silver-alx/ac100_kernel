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

/**
 * @file
 * <b>NVIDIA APX ODM Kit::
 *         Implementation of the ODM Peripheral Discovery API</b>
 *
 * @b Description: Specifies the peripheral connectivity database NvOdmIoAddress entries
 *                 for the peripherals on E1162 module.
 */

#include "pmu/tps6586x/nvodm_pmu_tps6586x_supply_info_table.h"
#include "tmon/adt7461/nvodm_tmon_adt7461_channel.h"
#include "nvodm_tmon.h"


// RTC voltage rail
static const NvOdmIoAddress s_RtcAddresses[] = 
{
    { NvOdmIoModule_Vdd, 0x00, TPS6586xPmuSupply_LDO2, 0 }  /* VDD_RTC -> LD02 */
};

// Core voltage rail
static const NvOdmIoAddress s_CoreAddresses[] = 
{
    { NvOdmIoModule_Vdd, 0x00, TPS6586xPmuSupply_DCD0, 0 }  /* VDD_CORE -> SM0 */
};

// CPU voltage rail
static const NvOdmIoAddress s_ffaCpuAddresses[] = 
{
    { NvOdmIoModule_Vdd, 0x00, TPS6586xPmuSupply_DCD1, 0 }  /* VDD_CPU -> SM1 */
};

// PLLA voltage rail
static const NvOdmIoAddress s_PllAAddresses[] = 
{
    { NvOdmIoModule_Vdd, 0x00, TPS6586xPmuSupply_LDO1, 0 } /* AVDDPLLX_1V2 -> LDO1 */
};

// PLLM voltage rail
static const NvOdmIoAddress s_PllMAddresses[] = 
{
    { NvOdmIoModule_Vdd, 0x00, TPS6586xPmuSupply_LDO1, 0 } /* AVDDPLLX_1V2 -> LDO1 */
};

// PLLP voltage rail
static const NvOdmIoAddress s_PllPAddresses[] = 
{
    { NvOdmIoModule_Vdd, 0x00, TPS6586xPmuSupply_LDO1, 0 } /* AVDDPLLX_1V2 -> LDO1 */
};

// PLLC voltage rail
static const NvOdmIoAddress s_PllCAddresses[] = 
{
    { NvOdmIoModule_Vdd, 0x00, TPS6586xPmuSupply_LDO1, 0 } /* AVDDPLLX_1V2 -> LDO1 */
};

// PLLE voltage rail
static const NvOdmIoAddress s_PllEAddresses[] = 
{
    { NvOdmIoModule_Vdd, 0x00, Ext_TPS62290PmuSupply_BUCK, 0 } /* AVDD_PLLE -> VDD_1V05 */
};

// PLLU voltage rail
static const NvOdmIoAddress s_PllUAddresses[] = 
{
    { NvOdmIoModule_Vdd, 0x00, TPS6586xPmuSupply_LDO1, 0 } /* AVDD_PLLU -> LDO1 */
};

// PLLU1 voltage rail
static const NvOdmIoAddress s_ffaPllU1Addresses[] = 
{
    { NvOdmIoModule_Vdd, 0x00, TPS6586xPmuSupply_LDO1, 0 } /* AVDD_PLLU -> LDO1 */
};

// PLLS voltage rail
static const NvOdmIoAddress s_PllSAddresses[] = 
{
    { NvOdmIoModule_Vdd, 0x00, TPS6586xPmuSupply_LDO1, 0 } /* PLL_S -> LDO1 */
};

// PLLHD voltage rail
static const NvOdmIoAddress s_PllHdmiAddresses[] = 
{
    { NvOdmIoModule_Vdd, 0x00, TPS6586xPmuSupply_LDO8, 0 } /* AVDD_HDMI_PLL -> LDO8 */
};

// OSC voltage rail
static const NvOdmIoAddress s_VddOscAddresses[] = 
{
    { NvOdmIoModule_Vdd, 0x00, TPS6586xPmuSupply_LDO4, 0 } /* AVDD_OSC -> LDO4 */
};

// PLLX voltage rail
static const NvOdmIoAddress s_PllXAddresses[] = 
{
    { NvOdmIoModule_Vdd, 0x00, TPS6586xPmuSupply_LDO1, 0 } /* AVDDPLLX -> LDO1 */
};

// PLL_USB voltage rail
static const NvOdmIoAddress s_PllUsbAddresses[] = 
{
    { NvOdmIoModule_Vdd, 0x00, TPS6586xPmuSupply_LDO3, 0 } /* AVDD_USB_PLL -> derived from LDO3 (VDD_3V3) */
};

// SYS IO voltage rail
static const NvOdmIoAddress s_VddSysAddresses[] = 
{
    { NvOdmIoModule_Vdd, 0x00, TPS6586xPmuSupply_LDO4, 0 } /* VDDIO_SYS -> LDO4 */
};

// USB voltage rail
static const NvOdmIoAddress s_VddUsbAddresses[] = 
{
    { NvOdmIoModule_Vdd, 0x00, TPS6586xPmuSupply_LDO3, 0 } /* AVDD_USB -> derived from LDO3 (VDD_3V3) */
};

// HDMI voltage rail
static const NvOdmIoAddress s_VddHdmiAddresses[] = 
{
    { NvOdmIoModule_Vdd, 0x00, TPS6586xPmuSupply_LDO7, 0 } /* AVDD_HDMI -> LDO7 */
};

// MIPI voltage rail (DSI_CSI)
static const NvOdmIoAddress s_VddMipiAddresses[] = 
{
    { NvOdmIoModule_Vdd, 0x00, Ext_TPS72012PmuSupply_LDO, 0  } /* AVDD_DSI_CSI -> VDD_1V2 */
};

// LCD voltage rail
static const NvOdmIoAddress s_VddLcdAddresses[] = 
{
    // This is in the AON domain
    { NvOdmIoModule_Vdd, 0x00, TPS6586xPmuSupply_LDO4, 0 } /* VDDIO_LCD -> (LDO4PG) */
};

// Audio voltage rail
static const NvOdmIoAddress s_VddAudAddresses[] = 
{
    // This is in the AON domain
    { NvOdmIoModule_Vdd, 0x00, TPS6586xPmuSupply_LDO4, 0 } /* VDDIO_AUDIO -> (LDO4PG) */
};

// DDR voltage rail
static const NvOdmIoAddress s_VddDdrAddresses[] = 
{
    // This is in the AON domain
    { NvOdmIoModule_Vdd, 0x00, TPS6586xPmuSupply_LDO4, 0 }  /* VDDIO_DDR -> (LDO4PG) */
};

// DDR_RX voltage rail
static const NvOdmIoAddress s_VddDdrRxAddresses[] = 
{
    { NvOdmIoModule_Vdd, 0x00, TPS6586xPmuSupply_LDO9, 0 }  /* VDDIO_RX_DDR(2.7-3.3) -> LDO9 */
};

// NAND voltage rail
static const NvOdmIoAddress s_VddNandAddresses[] = 
{
    // This is in the AON domain
    { NvOdmIoModule_Vdd, 0x00, TPS6586xPmuSupply_LDO3, 0 }  /* VDDIO_NAND_3V3 -> derived from LDO3 (VDD_3V3) */
};

// UART voltage rail
static const NvOdmIoAddress s_VddUartAddresses[] = 
{
    // This is in the AON domain
    { NvOdmIoModule_Vdd, 0x00, TPS6586xPmuSupply_LDO4, 0 } /* VDDIO_UART -> (LDO4PG) */
};

// SDIO voltage rail
static const NvOdmIoAddress s_VddSdioAddresses[] = 
{
    { NvOdmIoModule_Vdd, 0x00, TPS6586xPmuSupply_LDO3, 0 } /* VDDIO_SDIO -> derived from LDO3 (VDD_3V3) */
};

// VDAC voltage rail
static const NvOdmIoAddress s_VddVdacAddresses[] = 
{
    { NvOdmIoModule_Vdd, 0x00, TPS6586xPmuSupply_LDO6, 0 } /* AVDD_VDAC -> LDO6 */
};

// VI voltage rail
static const NvOdmIoAddress s_VddViAddresses[] = 
{
    { NvOdmIoModule_Vdd, 0x00, TPS6586xPmuSupply_LDO3, 0 } /* VDDIO_VI -> derived from LDO3 (VDD_3V3) */
};

// BB voltage rail
static const NvOdmIoAddress s_VddBbAddresses[] = 
{
    // This is in the AON domain
    { NvOdmIoModule_Vdd, 0x00, TPS6586xPmuSupply_LDO4, 0 } /* VDDIO_BB -> (LDO4PG) */
};

// Super power voltage rail for the SOC
static const NvOdmIoAddress s_VddSocAddresses[]=
{
    { NvOdmIoModule_Vdd, 0x00, TPS6586xPmuSupply_SoC, 0 } /* VDD SOC */
};

// PEX_CLK voltage rail
static const NvOdmIoAddress s_VddPexClkAddresses[] = 
{
    { NvOdmIoModule_Vdd, 0x00, TPS6586xPmuSupply_LDO0, 0 }, /* VDDIO_PEX_CLK -> LDO0 */
    { NvOdmIoModule_Vdd, 0x00, Ext_TPS62290PmuSupply_BUCK, 0 }, /* AVDD_PLLE -> VDD_1V05 */
    { NvOdmIoModule_Vdd, 0x00, Ext_TPS74201PmuSupply_LDO, 0 }, /* PMU_GPIO-1 -> VDD_1V5 */
};

// PMU0
static const NvOdmIoAddress s_Pmu0Addresses[] = 
{
    { NvOdmIoModule_I2c_Pmu, 0x00, 0x68, 0 },
};

static const NvOdmIoAddress s_Vddio_Sd_En[] = {
    { NvOdmIoModule_Gpio, 'v'-'a', 1, 0 },
};

static const NvOdmIoAddress s_Vddio_Pnl_En[] = {
    { NvOdmIoModule_Gpio, 'c'-'a', 6, 0 },
};

// P1160 ULPI USB
static const NvOdmIoAddress s_UlpiUsbAddresses[] = 
{
    { NvOdmIoModule_ExternalClock, 1, 0, 0 }, /* ULPI PHY Clock -> DAP_MCLK2 */
};

//  LVDS LCD Display
static const NvOdmIoAddress s_LvdsDisplayAddresses[] = 
{
    { NvOdmIoModule_Display, 0, 0, 0 },
    { NvOdmIoModule_I2c, 0x00, 0xA0, 0 },
    { NvOdmIoModule_Pwm, 0x00, 0, 0 },
    { NvOdmIoModule_Vdd, 0x00, TPS6586xPmuSupply_LDO4, 0 },    /* VDDIO_LCD (AON:VDD_1V8) */
};

//  HDMI addresses based on Concorde 2 design
static const NvOdmIoAddress s_HdmiAddresses[] =
{
    { NvOdmIoModule_Hdmi, 0, 0, 0 },

    // Display Data Channel (DDC) for Extended Display Identification
    // Data (EDID)
    { NvOdmIoModule_I2c, 0x01, 0xA0, 0 },

    // HDCP ROM
    { NvOdmIoModule_I2c, 0x01, 0x74, 0 },

    // AVDD_HDMI
    { NvOdmIoModule_Vdd, 0x00, TPS6586xPmuSupply_LDO7, 0 },
    // AVDD_HDMI_PLL
    { NvOdmIoModule_Vdd, 0x00, TPS6586xPmuSupply_LDO8, 0 },

    // HDMI +5V for the pull-up for DDC (VDDIO_VID)
    { NvOdmIoModule_Vdd, 0x00, TPS6586xPmuSupply_LDO3, 0 },
};

//  Power for HDMI Hotplug
static const NvOdmIoAddress s_HdmiHotplug[] =
{
    // Power for Hotplug GPIO
    { NvOdmIoModule_Vdd, 0x00, TPS6586xPmuSupply_LDO4, 0 },
    // HDMI +5V for hotplug
    { NvOdmIoModule_Vdd, 0x00, Ext_TPS2051BPmuSupply_VDDIO_VID, 0 },
};

// Sdio
static const NvOdmIoAddress s_SdioAddresses[] =
{
    { NvOdmIoModule_Sdio, 0x0,  0x0 },                      /* SD Memory on SD Bus */
    { NvOdmIoModule_Sdio, 0x3,  0x0 },                      /* SD Memory on SD Bus */
    { NvOdmIoModule_Vdd,  0x00, Ext_SWITCHPmuSupply_VDDIO_SD }   /* EN_VDDIO_SD */
};

static const NvOdmIoAddress s_I2cSmbusAddresses[] = 
{
    { NvOdmIoModule_I2c, 2, 0x8A, 0 },
    { NvOdmIoModule_Gpio, (NvU32)'v'-'a', 2, 0} // EC_REQUEST# is Port V, Pin 2
};

static const NvOdmIoAddress s_Tmon0Addresses[] = 
{
    { NvOdmIoModule_I2c_Pmu, 0x00, 0x98, 0 },                  /* I2C bus */
    { NvOdmIoModule_Gpio, (NvU32)'n'-'a', 6, 0 },              /* GPIO Port N and Pin 6 */

    /* Temperature zone mapping */
    { NvOdmIoModule_Tsense, NvOdmTmonZoneID_Core, ADT7461ChannelID_Remote, 0 },   /* TSENSOR */
    { NvOdmIoModule_Tsense, NvOdmTmonZoneID_Ambient, ADT7461ChannelID_Local, 0 }, /* TSENSOR */
};

// Audio Codec on GEN1_I2C (I2C_1)
static const NvOdmIoAddress s_AudioCodecAddressesI2C_1[] = 
{
    { NvOdmIoModule_ExternalClock, 0, 0, 0 },       /* Codec MCLK -> APxx DAP_MCLK1 */
    { NvOdmIoModule_I2c, 0x00, 0x34, 0 },           /* Codec I2C ->  APxx PMU I2C, segment 0 */
                                                 /* Codec I2C address is 0x34 */
    { NvOdmIoModule_Gpio, (NvU32)'w'-'a', 0x02, 0 }, /* GPIO Port W and Pin 2 for HP_DET */

};
