# WSN

This is my Master thesis in Computer Engineering developed with Gianfranco Costamagna.
We developed an energy aware wireless sensors network. The PCB of the network nodes is designed with Eagle, and it is made up of: a MSP430, a battery meter, a RTC (optional) and a CC2500 antenna.
The firmware of the microcontroller was developed with CCStudio, an IDE provided by TI.
The project could be built and tested with a MSP430 LaunchPad kit, provided by TI.
The library to manage communication between MSP and CC2500 via SPI are from alvarop (https://github.com/alvarop/msp430-cc2500)
See master_thesis.pdf (in Italian language) for further details on the PCB of the nodes, on the architecture of the network, and the communication protocol (written and designed from scratch) 
