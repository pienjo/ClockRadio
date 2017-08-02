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
LIBS:levelshifter-cache
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
L CONN_01X04 J2
U 1 1 597263A7
P 3750 3100
F 0 "J2" H 3750 3350 50  0000 C CNN
F 1 "CONN_01X04" V 3850 3100 50  0000 C CNN
F 2 "Pin_Headers:Pin_Header_Straight_1x04_Pitch2.54mm" H 3750 3100 50  0001 C CNN
F 3 "" H 3750 3100 50  0001 C CNN
	1    3750 3100
	-1   0    0    -1  
$EndComp
$Comp
L CONN_01X04 J1
U 1 1 5972648A
P 3700 2150
F 0 "J1" H 3700 2400 50  0000 C CNN
F 1 "CONN_01X04" V 3800 2150 50  0000 C CNN
F 2 "Pin_Headers:Pin_Header_Straight_1x04_Pitch2.54mm" H 3700 2150 50  0001 C CNN
F 3 "" H 3700 2150 50  0001 C CNN
	1    3700 2150
	-1   0    0    -1  
$EndComp
$Comp
L GND #PWR01
U 1 1 59726537
P 4000 2300
F 0 "#PWR01" H 4000 2050 50  0001 C CNN
F 1 "GND" H 4000 2150 50  0000 C CNN
F 2 "" H 4000 2300 50  0001 C CNN
F 3 "" H 4000 2300 50  0001 C CNN
	1    4000 2300
	1    0    0    -1  
$EndComp
$Comp
L GND #PWR02
U 1 1 5972654F
P 4050 3250
F 0 "#PWR02" H 4050 3000 50  0001 C CNN
F 1 "GND" H 4050 3100 50  0000 C CNN
F 2 "" H 4050 3250 50  0001 C CNN
F 3 "" H 4050 3250 50  0001 C CNN
	1    4050 3250
	-1   0    0    -1  
$EndComp
Text Label 4050 3150 0    60   ~ 0
SDA_33
Text Label 4050 3050 0    60   ~ 0
SCL_33
$Comp
L PWR_FLAG #FLG03
U 1 1 597265C9
P 3950 2700
F 0 "#FLG03" H 3950 2775 50  0001 C CNN
F 1 "PWR_FLAG" H 3950 2850 50  0000 C CNN
F 2 "" H 3950 2700 50  0001 C CNN
F 3 "" H 3950 2700 50  0001 C CNN
	1    3950 2700
	-1   0    0    -1  
$EndComp
Text Label 4000 2200 0    60   ~ 0
SDA_5
Text Label 4000 2100 0    60   ~ 0
SCL_5
$Comp
L +5V #PWR04
U 1 1 59726609
P 4350 2000
F 0 "#PWR04" H 4350 1850 50  0001 C CNN
F 1 "+5V" H 4350 2140 50  0000 C CNN
F 2 "" H 4350 2000 50  0001 C CNN
F 3 "" H 4350 2000 50  0001 C CNN
	1    4350 2000
	1    0    0    -1  
$EndComp
Text Label 6150 1900 0    60   ~ 0
SDA_5
$Comp
L +5V #PWR05
U 1 1 5972674C
P 5850 1400
F 0 "#PWR05" H 5850 1250 50  0001 C CNN
F 1 "+5V" H 5850 1540 50  0000 C CNN
F 2 "" H 5850 1400 50  0001 C CNN
F 3 "" H 5850 1400 50  0001 C CNN
	1    5850 1400
	1    0    0    -1  
$EndComp
$Comp
L R R3
U 1 1 59726766
P 5850 1650
F 0 "R3" V 5930 1650 50  0000 C CNN
F 1 "4K7" V 5850 1650 50  0000 C CNN
F 2 "Resistors_SMD:R_0603_HandSoldering" V 5780 1650 50  0001 C CNN
F 3 "" H 5850 1650 50  0001 C CNN
	1    5850 1650
	1    0    0    -1  
$EndComp
$Comp
L R R1
U 1 1 59726805
P 5050 1650
F 0 "R1" V 5130 1650 50  0000 C CNN
F 1 "2K7" V 5050 1650 50  0000 C CNN
F 2 "Resistors_SMD:R_0603_HandSoldering" V 4980 1650 50  0001 C CNN
F 3 "" H 5050 1650 50  0001 C CNN
	1    5050 1650
	1    0    0    -1  
$EndComp
Text Label 4850 1900 2    60   ~ 0
SDA_33
$Comp
L BSS138 Q1
U 1 1 59726916
P 5450 1800
F 0 "Q1" H 5650 1875 50  0000 L CNN
F 1 "BSS138" H 5650 1800 50  0000 L CNN
F 2 "TO_SOT_Packages_SMD:SOT-23_Handsoldering" H 5650 1725 50  0001 L CIN
F 3 "" H 5450 1800 50  0001 L CNN
	1    5450 1800
	0    1    1    0   
$EndComp
Text Label 6250 3450 0    60   ~ 0
SCL_5
$Comp
L +5V #PWR06
U 1 1 59726AC0
P 5950 2950
F 0 "#PWR06" H 5950 2800 50  0001 C CNN
F 1 "+5V" H 5950 3090 50  0000 C CNN
F 2 "" H 5950 2950 50  0001 C CNN
F 3 "" H 5950 2950 50  0001 C CNN
	1    5950 2950
	1    0    0    -1  
$EndComp
$Comp
L R R4
U 1 1 59726AC6
P 5950 3200
F 0 "R4" V 6030 3200 50  0000 C CNN
F 1 "4K7" V 5950 3200 50  0000 C CNN
F 2 "Resistors_SMD:R_0603_HandSoldering" V 5880 3200 50  0001 C CNN
F 3 "" H 5950 3200 50  0001 C CNN
	1    5950 3200
	1    0    0    -1  
$EndComp
$Comp
L R R2
U 1 1 59726ACC
P 5150 3200
F 0 "R2" V 5230 3200 50  0000 C CNN
F 1 "2K7" V 5150 3200 50  0000 C CNN
F 2 "Resistors_SMD:R_0603_HandSoldering" V 5080 3200 50  0001 C CNN
F 3 "" H 5150 3200 50  0001 C CNN
	1    5150 3200
	1    0    0    -1  
$EndComp
Text Label 4950 3450 2    60   ~ 0
SCL_33
$Comp
L BSS138 Q2
U 1 1 59726AD9
P 5550 3350
F 0 "Q2" H 5750 3425 50  0000 L CNN
F 1 "BSS138" H 5750 3350 50  0000 L CNN
F 2 "TO_SOT_Packages_SMD:SOT-23_Handsoldering" H 5750 3275 50  0001 L CIN
F 3 "" H 5550 3350 50  0001 L CNN
	1    5550 3350
	0    1    1    0   
$EndComp
$Comp
L PWR_FLAG #FLG07
U 1 1 59726BCB
P 3900 1800
F 0 "#FLG07" H 3900 1875 50  0001 C CNN
F 1 "PWR_FLAG" H 3900 1950 50  0000 C CNN
F 2 "" H 3900 1800 50  0001 C CNN
F 3 "" H 3900 1800 50  0001 C CNN
	1    3900 1800
	1    0    0    -1  
$EndComp
$Comp
L PWR_FLAG #FLG08
U 1 1 59726C18
P 3950 3500
F 0 "#FLG08" H 3950 3575 50  0001 C CNN
F 1 "PWR_FLAG" H 3950 3650 50  0000 C CNN
F 2 "" H 3950 3500 50  0001 C CNN
F 3 "" H 3950 3500 50  0001 C CNN
	1    3950 3500
	1    0    0    1   
$EndComp
$Comp
L R R5
U 1 1 597275D2
P 4900 2900
F 0 "R5" V 4980 2900 50  0000 C CNN
F 1 "0" V 4900 2900 50  0000 C CNN
F 2 "Resistors_THT:R_Axial_DIN0204_L3.6mm_D1.6mm_P5.08mm_Horizontal" V 4830 2900 50  0001 C CNN
F 3 "" H 4900 2900 50  0001 C CNN
	1    4900 2900
	0    1    1    0   
$EndComp
$Comp
L R R6
U 1 1 59745D51
P 4200 2000
F 0 "R6" V 4280 2000 50  0000 C CNN
F 1 "R" V 4200 2000 50  0000 C CNN
F 2 "Resistors_THT:R_Axial_DIN0204_L3.6mm_D1.6mm_P5.08mm_Horizontal" V 4130 2000 50  0001 C CNN
F 3 "" H 4200 2000 50  0001 C CNN
	1    4200 2000
	0    1    1    0   
$EndComp
$Comp
L R R7
U 1 1 597460CD
P 4250 2900
F 0 "R7" V 4330 2900 50  0000 C CNN
F 1 "0" V 4250 2900 50  0000 C CNN
F 2 "Resistors_THT:R_Axial_DIN0204_L3.6mm_D1.6mm_P7.62mm_Horizontal" V 4180 2900 50  0001 C CNN
F 3 "" H 4250 2900 50  0001 C CNN
	1    4250 2900
	0    1    1    0   
$EndComp
Wire Wire Line
	3900 2300 4000 2300
Wire Wire Line
	4050 3250 3950 3250
Wire Wire Line
	4050 3150 3950 3150
Wire Wire Line
	4050 3050 3950 3050
Wire Wire Line
	4050 2950 3950 2950
Wire Wire Line
	3950 2950 3950 2700
Wire Wire Line
	3900 2200 4000 2200
Wire Wire Line
	3900 2100 4000 2100
Wire Wire Line
	3900 2000 4050 2000
Wire Wire Line
	5850 1400 5850 1500
Wire Wire Line
	5850 1800 5850 1900
Wire Wire Line
	5650 1900 6150 1900
Wire Wire Line
	4850 1900 5250 1900
Wire Wire Line
	5050 1900 5050 1800
Connection ~ 5050 1900
Connection ~ 5850 1900
Wire Wire Line
	4650 1450 5400 1450
Wire Wire Line
	5400 1450 5400 1600
Connection ~ 5050 1450
Wire Wire Line
	5950 2950 5950 3050
Wire Wire Line
	5950 3350 5950 3450
Wire Wire Line
	5750 3450 6250 3450
Wire Wire Line
	4950 3450 5350 3450
Wire Wire Line
	5150 3450 5150 3350
Connection ~ 5150 3450
Connection ~ 5950 3450
Wire Wire Line
	5150 3000 5500 3000
Wire Wire Line
	5500 3000 5500 3150
Connection ~ 5150 3000
Wire Wire Line
	3900 1800 3900 2000
Wire Wire Line
	3950 3250 3950 3500
Wire Wire Line
	4050 2950 4050 2900
Wire Wire Line
	4050 2900 4100 2900
Wire Wire Line
	5150 2900 5150 3050
Wire Wire Line
	4650 1450 4650 2900
Connection ~ 4650 2900
Wire Wire Line
	5050 1450 5050 1500
Wire Wire Line
	4400 2900 4750 2900
Wire Wire Line
	5050 2900 5150 2900
$EndSCHEMATC
