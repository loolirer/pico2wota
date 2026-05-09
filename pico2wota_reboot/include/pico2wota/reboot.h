/**
 * Copyright (c) 2022 Brian Starkey <stark3y@gmail.com>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __PICO2WOTA_REBOOT_H__
#define __PICO2WOTA_REBOOT_H__

#include <stdbool.h>

#define PICO2WOTA_BOOTLOADER_ENTRY_MAGIC 0xb105f00d

void pico2wota_reboot(bool to_bootloader);

#endif /* __PICO2WOTA_REBOOT_H__ */
