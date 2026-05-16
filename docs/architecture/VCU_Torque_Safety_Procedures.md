Outline Torque Safety Procedures 



Information comes from FSAE rules, and PM100DX datasheets BPPD check 

 EV4.7 from the rules states the BPPD check must monitor for the two conditions: The mechanical brakes are engaged, and the APPS \(Accelerator pedal position sensor\) signals more than 25% pedal travel. For our sensor scale, this would correlate to values of 1.125V \(S1, 0V-4.5V scale\) and 2.875V \(S2, 4.5V-0V scale\) 

 If the BPPD check detects these events occurring: 

o Power to the motors must be immediately and completely shut down o The Motor shut down must stay active until the APPS signals less than 5% 

pedal travel \(S1 = 0.225V, S2 = 3.775V\), with or without brakes operation 

 The team must be able to demonstrate this at the technical inspection Accelerator pedal inputs verification 

 Two sensors \(S1 and S2\) reading pedal voltage, working on scales of 4.5V to 0V and 0V to 4.5V. S1 and S2 should always sum to 5V \(They have a 0.5V o set\) Fault monitoring from PM100DX 

 If a DC bus overvoltage is detected \(~425V\) all IGBTs are latched o instantly disabling torque 



 Inverter Enable Lockout \(optional\) 

o This feature requires that before sending out an Inverter Enable command, the user must send out an Inverter Disable command. Once the inverter sees a Disable command, the lockout is removed, and controller can receive the Inverter Enable command. Located at byte 6, bit 7, a value of 0 = inverter can be enabled, a value of 1 = inverter can not be enabled. 



 Sudden Reversal Of Direction 

o This safety feature keeps the user from changing the direction command while the inverter is enabled. If the direction command is changed suddenly when the inverter is still enabled, inverter is disabled without triggering any 

faults. Also, the lockout condition is set again which will force the user to send an Inverter Disable command before re-enabling it 



 Motor-Over Temperature 

 This temperature parameter at address 113 sets the Motor temperature limit \(if the motor has a temperature sensor\). The temperature is set in degrees Celsius times 10 \(150°C is set as 1500\). If the temperature exceeds this value then the inverter will turn off and declare a fault. 



 Inverter-Over Temperature 

o This is an EEPROM parameter \(address 112\), that has a value of 10x the maximum temperature \(100 C max temp = parameter value of 1000\). If the temperature exceeds this amount the inverter will turn o and declare a fault. 



 Inverter Motor Over Speed 

o This parameter at address 111 contains an angular velocity, if this is exceeded by the motor, it will turn o and declare a “MOTOR OVERSPEED” 

fault. 



 ACCEL Pedal High 

o This Low Voltage parameter at address 125 sets a limit such that between that limit and ACCEL Pedal Max, torque command is set to a constant value of Motor Torque Limit. This should be set to a value that is higher than the highest possible acceleration position but less than 500. If the accelerator input goes above this value, torque command is set to 0, the inverter will turn off and declare the ACCEL OPEN fault 



 ACCEL Pedal Low 

o This Low Voltage parameter at address 120 sets a limit below which the torque command is 0. This should be set to a value that is lower than the lowest possible acceleration position but higher than 0. If the accelerator input goes below this value, torque command is set to 0, the inverter will turn off and declare the ACCEL SHORTED fault. 



 DC Under-Voltage limit 

o This high voltage parameter at address 104 sets the under-voltage limit. This limit should be set based on total voltage provided by the power 

supply/battery pack. A fault is generated when the voltage drops below this limit. To disable the under-voltage fault, set this limit to 0. 



 CAN Command Message Active 

o This Boolean parameter at address 146 enables CAN Command Timeout feature. The CAN Timeout feature if enabled will generate a fault if the CAN 

Command Message is not received within the time set by CAN Timeout parameter. It is recommended that this feature be enabled. 

0 = The CAN Timeout feature is disabled. 

1 = The CAN Timeout feature is enabled. 



 Fault Clear 

o If this Boolean parameter at address 20 is set to 0, active faults are cleared. It can be sent over CAN 





CAN fault monitoring 

 Each component will send a signal to the CAN, on a specific period. This will act as a “heartbeat monitor” where the signal is only sent if the system is functioning properly. If this signal is not received by the CAN when it should be, power must be immediately cut to the car.



