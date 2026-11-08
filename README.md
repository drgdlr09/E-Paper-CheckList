# E-Paper CheckList - A Brief Overview
For the final week of Half-Life, I decided to design the E-Paper CheckList, a simple to-do list device with a 2.9" e-paper screen and three buttons. As a student with a lot of my plate right now, I built the CheckList in order to stay more organized and make sure I get everything I need to do done. I chose e-paper over other types of screens because it keeps showing the list even with no power, meaning the device is power-efficient and can last for a long time on one charge. You can scroll through the list using the buttons and check off items as you complete them.

# How it Works
The E-Paper Checklist has a XIAO ESP32-S3 microcontroller, a 2.9" e-paper screen for showing the list, three buttons (UP, OK and DOWN) for moving through the list and checking items off, a LiPo battery with an SS12D00 on/off switch, and a voltage divider for reading the battery level.

The E-Paper Checklist is built around the Seeed XIAO ESP32-S3, the same microcontroller I used in my Virtual SandBox. I picked it because it has a built-in battery charger and a deep sleep mode that uses almost nearly no power.

The buttons each have a 10k pull-up resistor and are wired between a GPIO pin and ground. The UP button is on D0, OK is on D1, and DOWN is on D2. I put them on these pins on purpose, because only certain pins can wake the ESP32 from deep sleep. UP and DOWN move the highlight through the list, and OK checks or unchecks the highlighted item. Holding DOWN for 1.5 seconds unchecks everything so I can start over the next day.

The e-paper screen talks to the XIAO over SPI. The screen’s SCL pin goes to D8, SDA goes to D10, CS goes to D4, DC goes to D5, RES goes to D6, and BUSY goes to D7. The screen solders straight onto a 1x8 header on the front of the board, so I don’t need any wires. The screen shows up to 6 items per page, with a header that shows how many items are done and the battery percentage.

The battery is set up the same way as my Virtual SandBox. The positive wire goes into the BAT+IN pad, through the on/off switch, out the BAT+OUT pad, and then by wire to the BAT+ pad under the XIAO. The negative wire goes straight to BAT-. The XIAO charges the battery when it’s plugged into USB-C. Two 100k resistors cut the battery voltage in half so D3 can read it, which is how the screen shows the battery percentage. The resistors are after the switch, so they don’t drain the battery when it’s turned off.

# The Firmware
The firmware is written in Arduino C++ and uses the GxEPD2 library for the e-paper screen. After 4 seconds with no button presses, the device will enter sleep mode. Pressing any button will automatically take the device out of sleep mode. The button you press to exit sleep mode matters because as soon as you press that button, the associated action will be triggered automatically.

The list is hardcoded in the firmware, so it is not editable until you change the actual code. In the future, I want to create some sort of app or webpage that allows you to update a schedule from another device, like a smartphone.

# Bill of Materials

