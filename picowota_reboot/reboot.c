/**
 * Copyright (c) 2022 Brian Starkey <stark3y@gmail.com>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "hardware/structs/watchdog.h"
#include "hardware/watchdog.h"

#if defined(PICO_RP2350)
#include "hardware/structs/powman.h"
#endif

#include "picowota/reboot.h"

void picowota_reboot(bool to_bootloader)
{
    hw_clear_bits(&watchdog_hw->ctrl, WATCHDOG_CTRL_ENABLE_BITS);
    
    if (to_bootloader) {
        #if defined(PICO_RP2350)
                powman_hw->scratch[5] = PICOWOTA_BOOTLOADER_ENTRY_MAGIC;
                powman_hw->scratch[6] = ~PICOWOTA_BOOTLOADER_ENTRY_MAGIC;
        #else
                watchdog_hw->scratch[5] = PICOWOTA_BOOTLOADER_ENTRY_MAGIC;
                watchdog_hw->scratch[6] = ~PICOWOTA_BOOTLOADER_ENTRY_MAGIC;
        #endif
    } else {
        #if defined(PICO_RP2350)
                powman_hw->scratch[5] = 0;
                powman_hw->scratch[6] = 0;
        #else
                watchdog_hw->scratch[5] = 0;
                watchdog_hw->scratch[6] = 0;
        #endif
    }
    
    watchdog_reboot(0, 0, 0);
    while (1) {
        tight_loop_contents();
        asm("");
    }
}