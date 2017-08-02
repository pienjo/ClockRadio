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
LIBS:SI4702
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
L CONN_01X05 J2
U 1 1 5970F5B7
P 6300 1600
F 0 "J2" H 6300 1900 50  0000 C CNN
F 1 "CONN_01X05" V 6400 1600 50  0000 C CNN
F 2 "Pin_Headers:Pin_Header_Straight_1x05_Pitch2.54mm" H 6300 1600 50  0001 C CNN
F 3 "" H 6300 1600 50  0001 C CNN
	1    6300 1600
	1    0    0    -1  
$EndComp
$Comp
L CONN_01X05 J1
U 1 1 5970F684
P 4150 1600
F 0 "J1" H 4150 1900 50  0000 C CNN
F 1 "CONN_01X05" V 4250 1600 50  0000 C CNN
F 2 "Pin_Headers:Pin_Header_Straight_1x05_Pitch2.54mm" H 4150 1600 50  0001 C CNN
F 3 "" H 4150 1600 50  0001 C CNN
	1    4150 1600
	-1   0    0    1   
$EndComp
$Comp
L SI4702 U1
U 1 1 5970F76F
P 5200 1600
F 0 "U1" H 5200 1500 50  0000 C CNN
F 1 "SI4702" H 5200 1700 50  0000 C CNN
F 2 "mvb_si4702:mvb_SI4702" H 5200 1600 50  0001 C CNN
F 3 "DOCUMENTATION" H 5200 1600 50  0001 C CNN
	1    5200 1600
	1    0    0    -1  
$EndComp
Wire Wire Line
	4350 1400 4450 1400
Wire Wire Line
	4350 1500 4450 1500
Wire Wire Line
	4350 1600 4450 1600
Wire Wire Line
	4350 1700 4450 1700
Wire Wire Line
	4350 1800 4450 1800
Wire Wire Line
	5950 1400 6100 1400
Wire Wire Line
	5950 1500 6100 1500
Wire Wire Line
	5950 1600 6100 1600
Wire Wire Line
	5950 1700 6100 1700
Wire Wire Line
	5950 1800 6100 1800
$EndSCHEMATC
