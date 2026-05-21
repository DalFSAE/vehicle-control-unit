# Overview

The new VCU should contain a Thermocouple circuit to assist in the cooling. A Thermocouple is a temperature measuring sensor that generates a voltage that changes over temperature. A thermocouple is able to read a wide temerature range at a fast response time, making it the ideal choice for temperature measurements.  


## Thermocouple Research 

A Thermocouple circuit is comprised of two metal leads made of different materials. These two leads are welded together to make a junction. When the temperature changes from the junction to the ends of the leads, thus a voltage is developed at the junction. To note, there are two junctions found on a thermocouple that are located at opposite ends. 

The one junction, which could be denoted as "the hot junction" or "the sensing junction" will be placed on the surface or near the source that requires measurement. At this specific junction the two leads will react differently to the heat based on their difference in material. 

The second junction, "the cold junction" will be reading the voltage produced at the hot junction and will be connecting the other end of the two different materials. At this second junction is where a multimeter can be attached to conduct any readings.  

Important considerations when creating the circuit include the choice of metals for the leads. Depending on which combination of metal it will yield a variety of different voltage responses. Choosing or creating the proper Thermocouple will be a function of the measurement temperature range required for its application. Other key considerations when deciding a Thermocouple include the temperature accuracy, durability and the expected service life.

To further expand on the reasoning for the difference of metals is that metals when heated will produce a change in their movements of electrons uniquely. We can quantify this generated thermoelectric voltage in a specfic material per unit temperature difference as the Seebeck effect. The higher the Seebeck the stronger that material is at converting high themperatures into an electric voltage.  

When selecting the best thermocouple it is worth considering how fast we expect our response time to be as the smaller the probe sheath diameter and an exposed junction offers the fastest response time. 

## Possible Thermocouples

The following are possible thermocouple options to purchase. From what I have gathered we would best suited to use a thermocouple that has a protected (sheathed) junction to keep it safe from our electrical ground loops. 

## Beaded Wire Thermocouple 

Simplest thermocouple, uses an exposed bead thus a fast response time, can not be used for any liquids and not recommended for metal surfaces, best suited for gas temperature measurement. 

## Thermocouple Probe 

A probe has the the thermocouple wire housed inside a metallic tube, the material of the tubing can vary depending on what the temperature range required is. 

## Surface Probe 

Measures a solid surface temperature, the entire measuremtn area of the sensor must be in contact with the surface which can be difficult depending on the surface. 

## Exposed Thermocouple

This will allow for direct heat transfer from the measured object giving the fastest sensor response time however only best for gas measurement. 

## Grounded Thermocouple 

Sensor is welded to the sheath, sheath is generally composed of metal to allow for best heat transfer. To note since the junction is welded to the sheath it allows there is electrical contact thus it makes it susceptible to noise from ground loops. 

## Ungrounded Thermocouple

Isolated from the sheath with a layer of insulation between the thermocouple and the measured object. Slowest response time but safest choice, with the isolation layer. 

## Multiplexing the Thermocouple 

To save space on the VCU it is suggested to use a multiplexer to attach to the ADC input of our Thermocouple. Per the web " a multiplexer can be used with the ADC input of a thermocouple, but careful selection of a low-resistance, low-leakage analog multiplexer is necessary to avoid significant measurement errors and preserve accuracy." 

## Current Support

VCU Hw3.0 does *not* support any thermocouple inputs. A few options are available for measuring temperatures.

- **AiM ACC2 Open** - Supports up to 4 thermocouples. Data logged over CAN
- **AiM MXm** - Supports up to 4 thermocouples. Data logged over CAN
- **PM100DX** - Reports motor and internal coolant temperature over CAN 
## References

[Texas Instruments: A Basic Guide to Thermocouple Measurements](https://www.ti.com/lit/an/sbaa274a/sbaa274a.pdf?ts=1758725949092)
