# NRF24-total-control
NRF24L01+ (and clones) registers at your fingertips  
![roboremo_interface_screenshot](https://raw.githubusercontent.com/roboremo/NRF24-total-control/master/RoboRemo-interface-screenshot.png "RoboRemo interface screenshot")  
To use it, copy the interface file to the roboremo folder on the sdcard of your tablet, then open RoboRemo and select menu -> interface -> import and choose the file.  
## Working principle
One or more NRF24L01+ (or clones) module is connected to the Arduino UNO using any 6 digital pins for each transceiver (I use software SPI, so any digital pin can be used)  
The Arduino UNO is connected to a Galaxy Tab 4 using USB cable and OTG adapter  
The tablet runs RoboRemo app.  
To connect, select menu -> connect -> USB -> 115200 -> OK  
Note: please use original Arduino UNO, since RoboRemo does not support yet the ch340 usb chip found in Arduino clones.
I created a RoboRemo interface with many buttons for configuring different registers of the NRF. When you press a button, the app sends a specific command to the Arduino, which controls the transceiver(s).
You can easily add more buttons, according to your needs.
You can select menu -> edit ui, and then click on any button and select "set press action" in order to view and/or modify the command associated with that button.  
## Available commands
| Command           | Examples                   | Description  |
| ----------------- | -------------------------- | ------------ |
| "use pins [...]"  | "use pins 2 12 11 13 10 8" | Switch between the transceivers. Must provide exactly 6 pins (IRQ, MISO, MOSI, SCK, CSN, CE), separated by space. |
| "spi init"        |                            | Initialize the software SPI pins |
| "ce high"         |                            | Switch CE pin to HIGH state |
| "ce low"          |                            | Switch CE pin to LOW state |
| "sb XX b"         | "sb 00 1"                  | Set bit b in the register XX. (XX is the register address, a 2-digit hex number) |
| "cb XX b"         |                            | Clear bit b in the register XX. |
| "rb XX b"         |                            |  Read bit b from the register XX. |
|"write RR DD[...]" | "write 05 00", "write 1f cd3f7f9c20" | Write data to register. (RR is the register address, a 2-digit hex number, DD[...] is the data, even-digit hex number) |
| "wrcmd CC DD[...]" | "wrcmd 50 73", "wrcmd b0 0000000000000000" | Write command. CC is the command number, a 2-digit hex number, DD[...] is the data, even-digit hex number |
| "read NN RR" | "read 05 0a", "read 01 05" | Read NN bytes from register RR. NN is the byte count to read, a 2-digit hex number, RR si the regidter address, a 2-digit hex number |
| "bank 1"          |                            | Select bank 1 (for RFM73) |
| "bank 0"          |                            | Select bank 0 (for RFM73) |

This project, together with this: https://github.com/roboremo/NRF24-demodulator
form a complete tool for debugging and demystifying some Chinese NRF24L01 clones :)
