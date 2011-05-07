#ifndef _NVODM_EC_H
#define _NVODM_EC_H

#define NV_EC_IOC_MAGIC 0x30

/* DMI command */
#define DMI_CMD _IOWR(NV_EC_IOC_MAGIC, 0, int)
#define READ_EC_VERSION         0
#define WRITE_UUID              1
#define READ_UUID               3
#define WRITE_MANUFACTURE       4
#define READ_MANUFACTURE        5
#define WRITE_PRODUCT_NAME      6
#define READ_PRODUCT_NAME       7
#define WRITE_VERSION           8
#define READ_VERSION            9
#define WRITE_SERIAL_NUMBER     10
#define READ_SERIAL_NUMBER      11
#define WRITE_OEM_STRINGS       12
#define READ_OEM_STRINGS        13
#define WRITE_KEYBOARD_INFO     14
#define READ_KEYBOARD_INFO      15
#define WRITE_COUNTRY_CODE      16
#define READ_COUNTRY_CODE       17
#define WRITE_CONFIG_FOR_TEST   18
#define READ_CONFIG_FOR_TEST    19

/* Keyboard command */
#define KEYBOARD_CMD _IOWR(NV_EC_IOC_MAGIC, 1, int)
#define WRITE_KEYBOARD_TYPE     0
#define READ_KEYBOARD_TYPE      1
#define ENABLE_CAPSLOCK         2
#define DISABLE_CAPSLOCK        3

/* Touchpad operation command */
#define TOUCHPAD_CMD _IOWR(NV_EC_IOC_MAGIC, 3, int)
#define CHECK_TOUCHPAD_STATUS           0
#define ENABLE_TOUCHPAD                 1
#define DISABLE_TOUCHPAD                2
#define CHECK_TOUCHPAD_SCROLL_STATUS    3
#define ENABLE_TOUCHPAD_SCROLL          4       
#define DISABLE_TOUCHPAD_SCROLL         5
#define SETTING_TOUCHPAD_SPEED          6  

/* LED command */
#define LED_CMD _IOWR(NV_EC_IOC_MAGIC, 2, int)
#define SET_LED_PATTERN_FIRST       0
#define SET_LED_PATTERN_SECOND      1
#define SET_LED_PATTERN_THIRD       2
#define SET_LED_PATTERN_FOURTH      3
#define SET_LED_PATTERN_FIFTH       4
#define SET_LED_PATTERN_SIXTH       5
#define SET_LED_PATTERN_SEVENTH     6
#define SET_LED_PATTERN_EIGHTH      7
#define DISABLE_LED                 8

/* EC operation command */
#define EC_CMD _IOWR(NV_EC_IOC_MAGIC, 4, int)
#define GET_RECOVERY_PIN_STATUS         0
#define RECOVERY_PIN_UP                 1
#define RECOVERY_PIN_DOWN               2
#define SUSPEND                         3
#define RESUME                          4
#define SET_POWER_LED                   5
#define SET_BATTERY_LOW_LED             6
#define SET_BATTERY_FULL_LED            7
#define DISABLE_POWER_BATTERY_LED       8
#define EC_CONTROL_LED                  9
#define POWER_BUTTON_CONTROL_LCD        10
#define POWER_BUTTON_CONTROL_SUSPEND    11
#define LID_SWITCH_CONTROL_LCD          12
#define LID_SWITCH_CONTROL_SUSPEND      13

/* EC event config command */
#define EVENT_CONFIG _IOWR(NV_EC_IOC_MAGIC, 5, int)
/* EC system event config command */
#define SYSTEM_EVENT_CONFIG         0
#define ENABLE_AC_PRESENT_EVENT     1
#define DISABLE_AC_PRESENT_EVENT    2
#define ENABLE_LID_SWITCH_EVENT     3
#define DISABLE_LID_SWITCH_EVENT    4
#define ENABLE_POWER_BUTTON_EVENT   5
#define DISABLE_POWER_BUTTON_EVENT  6
#define ENABLE_USB_OVER_CURRENT     7
#define DISABLE_USB_OVER_CURRENT    8
/* EC wakeup event config command */
#define WAKEUP_EVENT_CONFIG         9
#define ENABLE_POWER_BUTTON_WAKEUP_EVENT    10
#define DISABLE_POWER_BUTTON_WAKEUP_EVENT   11
#define ENABLE_LID_SWITCH_WAKEUP_EVENT      12
#define DISABLE_LID_SWITCH_WAKEUP_EVENT     13
#define ENABLE_HOMEKEY_WAKEUP_EVENT         14
#define DISABLE_HOMEKEY_WAKEUP_EVENT        15

/* Append operation command */
#define APPEND_CMD _IOWR(NV_EC_IOC_MAGIC, 6, int)
#define SHUT_DOWN           0
#define ENABLE_LCD          1
#define DISABLE_LCD         2
#define MUTE                3
#define UNMUTE              4
#define AUTO_RESUME         5

typedef struct {
    int operation;
    int write_buf;
    int read_buf[60];
    char str_write_buf[70];
    char str_read_buf[70];
} ec_odm_dmi_params;

typedef struct {
    int operation;
    char write_buf;
    char read_buf;
} ec_odm_keyboard_params;

typedef struct {
    int operation;
    int write_buf;
    int read_buf;
} ec_odm_touchpad_params;

typedef struct {
    int operation;
    int write_buf;
    int read_buf;
    char string_buf[60];
} ec_odm_led_params;

typedef struct {
    int operation;
    int write_buf;
    int read_buf;
    char string_buf[60];
} ec_odm_ec_params;

typedef struct {
    int operation;
    int sub_operation;
    int write_buf;
    int read_buf;
    char string_buf[60];
} ec_odm_event_params;

typedef struct {
    int operation;
    int write_buf;
    int read_buf;
    char str_buf[60];
} ec_odm_append_params;

#endif
