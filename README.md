# `pico2wota` - Raspberry Pi Pico 2W OTA Bootloader

> `pico2wota`, pronounced "picotwota"!

This project is a port of the original `picowota`, a bootloader for the Raspberry Pi Pico which allows upload of program code over WiFi ("Over The Air"). Originally it was made for the Pico W chip (RP2040), but this version supports the newer Pico 2W chip (RP2350). **Read the disclaimer at the bottom of the page!**

## Using in your project

First add `pico2wota` as a submodule to your project:
```
git submodule add https://github.com/loolirer/pico2wota
git submodule update --init pico2wota
git commit -m "Add pico2wota submodule"
```

Then modifiy your project's CMakeLists.txt to include the `pico2wota` directory:

```
add_subdirectory(pico2wota)
```

As the original, `pico2wota` either connects to an existing WiFi network (by default) or
creates one, in both cases with the given SSID and password.

You can either provide the following as environment variables, or set them
as CMake variables:

```
PICO2WOTA_WIFI_SSID # The WiFi network SSID
PICO2WOTA_WIFI_PASS # The WiFi network password
PICO2WOTA_WIFI_AP # Optional; 0 = connect to the network, 1 = create it
```

Then, you can either build just your standalone app binary (suitable for
updating via `pico2wota` when it's already on the Pico), or a combined binary
which contains the bootloader and the app (suitable for flashing the first
time):

```
pico2wota_build_standalone(my_executable_name)
pico2wota_build_combined(my_executable_name)
```

> Note: The combined target will also build the standalone binary.

To be able to update your app, you must provide a way to return to the
bootloader. By default, if GPIO15 is pulled low at boot time, then `pico2wota`
will stay in bootloader mode, ready to receive new app code.

You can also return to the bootloader from your app code - for example when a
button is pressed, or in response to some network request. The
`pico2wota_reboot` library provides a `pico2wota_reboot(bool to_bootloader)`
function, which your app can call to get back in to the bootloader.

```
CMakeLists.txt:

target_link_libraries(my_executable_name pico2wota_reboot)

your_c_code.c:

#include "pico2wota/reboot.h"

...

{

	...

	if (should_reboot_to_bootloader) {
		pico2wota_reboot(true);
	}

	...

}
```

## Uploading code via `pico2wota`

Once you've got the `pico2wota` bootloader installed on your Pico, you can use
the https://github.com/usedbytes/serial-flash tool to upload code to it.

As long as the Pico is "in" the `pico2wota` bootloader (i.e. because there's no
valid app code uploaded yet, or your app called `pico2wota_reboot(true);`), you
can upload an app `.elf` file which was built by `pico2wota_build_standalone()`:

If using the AP mode, the Pico's IP address will be (at the time of writing)
192.168.4.1/24, and the connected device's something in the same subnet.
Otherwise it depends on your network settings.

(Assuming your Pico's IP address is 192.168.1.123):
```
serial-flash tcp:192.168.1.123:4242 my_executable_name.elf
```

After uploading the code, if successful, the Pico will jump to the newly
uploaded app.

## How it works

This is derived from my Pico non-W bootloader, https://github.com/usedbytes/rp2040-serial-bootloader by Brian Starkey, which was converted to a WiFi version, and I adapted it for the RP2350 chip for leisure reasons.

The bootloader code attempts to avoid "bricking" by storing a CRC of the app
code which gets uploaded. If the CRC doesn't match, then the app won't get run
and the Pico will stay in `pico2wota` bootloader mode. This should make it fairly
robust against errors in transfers etc.

> **IMPORTANT DISCLAIMER:** this RP2350 port was done for another project, and AI was used in some pretty important parts. I tested this in my own Pico 2W and it worked for both WiFi and AP mode. **Use this at your own risk!**

---
