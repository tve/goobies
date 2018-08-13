target extended-remote oc1.voneicken.com:2200
monitor swdp_scan
attach 1
#set mem inaccessible-by-default off
#kill
#load
compare-sections
dir /home/tve/.platformio/packages/framework-arduinostm32l0/system/STM32L0xx/Source
