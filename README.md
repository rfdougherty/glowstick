# glowstick

An 8-channel high power LED driver for light-art projects. This work was inspired by a [project](https://github.com/rfdougherty/ledFlicker) that I built many years ago to do visual experiments in an MRI scanner. Some [old python code from that work](https://github.com/vistalab/vistadisp/tree/master/ledFlicker/python) might be useful. E.g., here's [code](https://github.com/vistalab/vistadisp/blob/master/ledFlicker/python/ledFlickComputeXform.py) to compute human cone LMS stimulation from LED color power spectra.

<p align="center">
  <img src="enclosure/glowstick.png" height="500" alt="Glowstick">
</p>

## Hardware

The LED drivers use a simple [constant-current driver](https://www.instructables.com/Power-LED-s---simplest-light-with-constant-current/) with the LED current determined by a current-sense resistor Rcs. The LED current is approximately equal to 0.5 / Rcs and the power dissipated by the resistor is approximately 0.25 / Rcs. In the schematic here, I set Rcs to 0.75Ω to achieve an LED current of about 0.67A, safely under the 700mA rating for the [Luxeon Rebel LEDs](https://luxeonstar.com/wp-content/uploads/documentation/ds68.pdf) that I used. With this value, Rcs will dissipate up to 333mW, so be sure to use at least a 1/2W resistor. The MOSFETs will also dissipate some power, with the amount a function of the LED power and the voltage difference between the source supply and the LED forward voltage. E.g., if the LEDs are drawing 700mA at a Vf of 3V using a 5V supply, the MOSFETs will dissipate about 2*0.7=1.4W. At these levels, the MOSFETs will get quite hot and should be attached to a small heatsink. While not nearly as efficient as a buck driver, this driver circuit is simple, reliable, and cheap.

Note that to consistently drive the LEDs at or near 700mA you will also need adequate thermal management for the LED module. From the LuxeonStar site:
> A suitable finned heatsink or other cooling method must always be used with the SP-02 LED module to ensure that the junction temperature of the LEDs is kept well below the maximum rating specified in the Lumileds datasheet. The size of the heatsink will depend on the ambient temperature and the current used to power the LEDs. Bench testing that we have conducted with SP-02 modules powered at 700mA in an open-air environment and an ambient temperature of 25°C has indicated that a heat sink with a thermal rating of 9 C°/W or lower should be adequate.

The schematic and layout was designed in [EasyEDA](https://easyeda.com/) and the [EasyEDA json files](hardware/) are included. After getting the board made and testing it, I realized that I forgot to add pull-down resistors to keep the LEDs off when the microcontroller is not connected or is rebooting. For my build, I squeezed 10kΩ resistors in with the 1kΩ resistor leads and the ground end of the 0.75Ω resistor leads. But it would be best to update the schematic to include these pull-downs.

<div style="display: flex; gap: 20px;">
    <img src="hardware/glowstick_schematic.svg" width="500" alt="Schematic">
    <img src="hardware/glowstick_layout.svg" width="500" alt="Layout">
</div>

The circuit includes a voltage regulator so a microcontroller can be powered by the LED supply. If you use a 5V supply and your microcontroller has a 5V input, then you can omit the voltage regulator and just add a bypass wire. Or, you could populate it with a 3.3V regulator if your input supply is less than 5V but greater than 3.3V. Also, as noted above, the power dissipation of the MOSFETs is affected by the voltage difference between the source supply and the LED forward voltage. So you could make it more efficient by using a supply that is closer to the maximum LED forward voltage. Just be sure to account for the 0.5V dropout in the circuit. E.g., if you are using LEDs with a Vf of no more than 3.2V, then you could use a 3.7V supply to get the best efficiency.

I used a [Wemos S3 Mini](https://www.wemos.cc/en/latest/s3/s3_mini.html) for the microcontroller.

## Firmware

The firmware is written in C/C++ using [Arduino](https://www.arduino.cc/) and the [aWOT framework](https://github.com/rbaron/aWOT). Note that I used a 7-LED module for my build, so the firmware currently only supports 7 channels. If you do use the 8th channel, you will need to update the firmware.

The code runs a web server with the following API endpoints:

- `GET /config` - Retrieve the current configuration as JSON
- `POST /config` - Update the configuration (expects JSON payload)
- `GET /leds` - Get the current LED PWM values for all 7 channels
- `PUT /leds/:l0/:l1/:l2/:l3/:l4/:l5/:l6` - Set the LED PWM values (0-1023 for each channel)
- `PUT /iscale/:scale` - Set the intensity scale (0.0-1.0)
- `PUT /mod/:amp/:step` - Set modulation parameters (amplitude and step, 0.0-1.0)
- `PUT /update/:interval` - Set update interval (10-9999999 microseconds)
- `GET /sensors` - Get sensor data (temperature, time, LED states)
- `GET /reboot` - Trigger system reboot

TODO:
- Add a web GUI to allow easy configuration
- MQTT support
- Add timer functions, including sunrise/sunset support
- Add LED spectra (measured or from the datasheet) and allow HSV color control

## Enclosure

An [enclosure](enclosure/) for the pcb, microcontroller module, and a 7-chip [LuxeonStar](https://luxeonstar.com/) LED module was designed in [OpenSCAD](https://www.openscad.org/). The seven [Luxen Rebel](https://luxeonstar.com/wp-content/uploads/documentation/ds68.pdf) LEDs illuminate a large [selenite](https://en.wikipedia.org/wiki/Selenite_(mineral)) tower crystal. The LED module that I used had the following chips [custom-mounted to a 40mm round FR4 Coolbase board](https://luxeonstar.com/product/sp-02/):
 * Royal-Blue (LXML-PR02-A900)
 * Blue (LXML-PB02)
 * Cyan (LXML-PE01-0070)
 * Green (LXML-PM01-0100)
 * Lime (LXML-PX02-0000)
 * Amber (LXM5-PL01)
 * Deep Red (LXM3-PD01)

The LEDs were diffused with a [Polymer Optics 12° 39 mm Circular Beam Diffused Optic](https://luxeonstar.com/product/264/). A summary of the relevant specs (all driven at 350mA, except royal blue at 700mA) from the datasheet.
| Color      | Flux/Power  | Vf (typ) | Wavelength |
|------------|----------------|-------|------------|
| Royal-Blue | 940 mW         | 2.9V  | 440-460 nm |
| Blue       | 41 lm          | 2.95V | 460-485 nm |
| Cyan       | 76 lm          | 3.17V | 490-515 nm |
| Green      | 102 lm         | 3.21V | 520-540 nm |
| Lime       | 184 lm         | 2.75V | 566-569 nm |
| Amber      | 99 lm          | 2.1V  | 585-595 nm |
| Deep Red   | 360 mw         | 2.1V  | 650-670 nm |

## Measurements
The MOSFETs used in this build are the most temperature critical parts, as you can easily fry them with a good thermal design. The IRL2703 MOSFETs that I used have a junction-to-case thermal resistance of 3.3 W/°C and maximum junction temperature of 175°C, the [maximim safe case temerature](https://community.infineon.com/t5/Knowledge-Base-Articles/Calculating-the-case-temperature-Tc-of-MOSFET-and-controller-in-CCG7DC/ta-p/401569#.) is 170°C for the Vf 2.1V LEDs driven at 700mA. That's pretty toasty, so I try to keep it under 100°C. 

To test the safe operating range, I first measured everything in open-air (room temp ~20°C) with no heatsinks at a 25% PWM duty cycle (LED PWM value of 256) and noted that the peak board temperature (ie, MOSFET case temp) was under 55°C. 

I made individual LED measurements at a 50% duty cycle (LED value of 512), and observed the following temperatures after about 5 minutes (that's when temps seem to asymptote). At this duty cycle, the total system curent draw was about 300 mA for one LED.

| Color      | Vf (typ) | Temp |
|------------|----------|------|
| Royal-Blue | 2.9V     | 42°C |
| Blue       | 2.95V    | 42°C |
| Cyan       | 3.17V    | 41°C |
| Green      | 3.21V    | 42°C |
| Lime       | 2.75V    | 43°C |
| Amber      | 2.1V     | 65°C |
| Deep Red   | 2.1V     | 62°C |

With small stick-on heatsinks (~5x5x4mm), I tested running all 7 LEDs at 50% duty cycle. This drew a total of about 1.6A and resulted in the following thermals for the board (left) and LED module (right):

<div style="display: flex; gap: 20px;">
    <img src="hardware/img_thermal_1731793438477.jpg" width="400" alt="Board thermal image">
    <img src="hardware/img_thermal_1731793451250.jpg" width="400" alt="LED board thermal image">
</div>

With further testing with the small heatsinks on the MOSFETs, I would be comfortable driving all the LEDs except Amber and Deep Red at 100% duty cycle and keep the Amber and Deep Red at 75% or less and only drive them higher for brief periods of time. I'd also avoid driving lime at 100%. I will try using larger heatsinks and revisit these limits. For the LED module, you could get away not using a heatsink if only driving a couple of LEDs at full power for long periods of time. But your LEDs will be happier to be keep cooler with a modest heatsink. 