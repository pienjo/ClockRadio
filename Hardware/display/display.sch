EESchema Schematic File Version 2
LIBS:power
LIBS:device
LIBS:transistors
LIBS:conn
LIBS:linear
LIBS:regul
LIBS:74xx
LIBS:cmos4000
LIBS:adc-dac
LIBS:memory
LIBS:xilinx
LIBS:microcontrollers
LIBS:dsp
LIBS:microchip
LIBS:analog_switches
LIBS:motorola
LIBS:texas
LIBS:intel
LIBS:audio
LIBS:interface
LIBS:digital-audio
LIBS:philips
LIBS:display
LIBS:cypress
LIBS:siliconi
LIBS:opto
LIBS:atmel
LIBS:contrib
LIBS:valves
LIBS:my_display
EELAYER 25 0
EELAYER END
$Descr A4 11693 8268
encoding utf-8
Sheet 1 1
Title ""
Date ""
Rev ""
Comp ""
Comment1 ""
Comment2 ""
Comment3 ""
Comment4 ""
$EndDescr
$Comp
L CONN_01X08 J2
U 1 1 59A2E045
P 5500 3850
F 0 "J2" H 5500 4300 50  0000 C CNN
F 1 "CONN_01X08" V 5600 3850 50  0000 C CNN
F 2 "Pin_Headers:Pin_Header_Straight_1x08_Pitch2.54mm" H 5500 3850 50  0001 C CNN
F 3 "" H 5500 3850 50  0001 C CNN
	1    5500 3850
	1    0    0    -1  
$EndComp
$Comp
L CONN_01X08 J1
U 1 1 59A2E0FD
P 4200 3900
F 0 "J1" H 4200 4350 50  0000 C CNN
F 1 "CONN_01X08" V 4300 3900 50  0000 C CNN
F 2 "Pin_Headers:Pin_Header_Straight_1x08_Pitch2.54mm" H 4200 3900 50  0001 C CNN
F 3 "" H 4200 3900 50  0001 C CNN
	1    4200 3900
	-1   0    0    -1  
$EndComp
$Comp
L LED D1
U 1 1 59A2E1C4
P 5250 3050
F 0 "D1" H 5250 3150 50  0000 C CNN
F 1 "LED" H 5250 2950 50  0000 C CNN
F 2 "LEDs:LED_D5.0mm" H 5250 3050 50  0001 C CNN
F 3 "" H 5250 3050 50  0001 C CNN
	1    5250 3050
	-1   0    0    -1  
$EndComp
$Comp
L LED D2
U 1 1 59A2E1E5
P 6650 3050
F 0 "D2" H 6650 3150 50  0000 C CNN
F 1 "LED" H 6650 2950 50  0000 C CNN
F 2 "LEDs:LED_D5.0mm" H 6650 3050 50  0001 C CNN
F 3 "" H 6650 3050 50  0001 C CNN
	1    6650 3050
	-1   0    0    -1  
$EndComp
Text Label 5000 1250 1    60   ~ 0
A5
$Comp
L CA56-12 AFF1
U 1 1 59A2E563
P 5700 1950
F 0 "AFF1" H 5700 2750 50  0000 C CNN
F 1 "CA56-12" H 5700 2650 50  0000 C CNN
F 2 "my_display:SMA413064" H 5200 1950 50  0001 C CNN
F 3 "" H 5200 1950 50  0001 C CNN
	1    5700 1950
	1    0    0    -1  
$EndComp
Text Label 5500 1250 1    60   ~ 0
A3
Text Label 6000 1250 1    60   ~ 0
A2
Text Label 6500 1250 1    60   ~ 0
A8
Text Label 5400 2650 3    60   ~ 0
C6
Text Label 5500 2650 3    60   ~ 0
C7
Text Label 5600 2650 3    60   ~ 0
C5
Text Label 5700 2650 3    60   ~ 0
C1
Text Label 5800 2650 3    60   ~ 0
C3
Text Label 5900 2650 3    60   ~ 0
C8
Text Label 6000 2650 3    60   ~ 0
C2
Text Label 6100 2650 3    60   ~ 0
C4
Text Label 4400 3550 0    60   ~ 0
C1
Text Label 4400 3650 0    60   ~ 0
A4
Text Label 4400 3750 0    60   ~ 0
A6
Text Label 4400 3850 0    60   ~ 0
C4
Text Label 4400 3950 0    60   ~ 0
A1
Text Label 4400 4050 0    60   ~ 0
C2
Text Label 4400 4150 0    60   ~ 0
A7
Text Label 4400 4250 0    60   ~ 0
A8
Text Label 5300 3500 2    60   ~ 0
C3
Text Label 5300 3600 2    60   ~ 0
C6
Text Label 5300 3700 2    60   ~ 0
A5
Text Label 5300 3800 2    60   ~ 0
C8
Text Label 5300 3900 2    60   ~ 0
A3
Text Label 5300 4000 2    60   ~ 0
A2
Text Label 5300 4100 2    60   ~ 0
C7
Text Label 5300 4200 2    60   ~ 0
C5
Text Label 5100 3050 2    60   ~ 0
A4
Text Label 6500 3050 2    60   ~ 0
A7
Text Label 5400 3050 0    60   ~ 0
C1
Text Label 6800 3050 0    60   ~ 0
C2
$EndSCHEMATC
