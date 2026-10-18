# Enclosure Leak Tester

An enclosure leak tester that measures the amount of airflow required to maintain a constant pressure inside the tested enclosure.

I am developing this project independently as part of an unpaid technical internship at a large company. It was created in response to a real engineering problem I encountered during the internship. Because the intended application is internal, I do not publish the company name or any details that could identify the specific use case.

The tester architecture, firmware, and mechanical design presented here are all developed by me.

<img src="./pictures/camera/final-leak-test-setup.jpg" alt="Final leak-test setup with a Panasonic flow meter" width="440" />

*Final test setup for measuring enclosure leakage.*

## How does it work?

A turbine pushes air into the tested enclosure, while a PID controller adjusts its output to maintain the target pressure.

Pressure is measured using a Honeywell sensor connected over SPI. Airflow is measured using an Omron D6F-P connected to the ADC.

Because the measurement range of the D6F-P is much lower than the total airflow of the system, the sensor operates in a separate bypass. Most of the air flows through the main channel and its restriction, while only a small portion passes through the D6F-P.

The final goal is to determine the enclosure leak rate from the airflow required to maintain a constant pressure.

<img src="./diagram/Enclosure-Leak-Sensor_1.0.0.svg" alt="Enclosure leak tester diagram, version 1.0.0" width="680" />

*Diagram of the tester’s pneumatic and electronic systems.*

## Main components

- Nordic nRF52840
- Zephyr / nRF Connect SDK
- Omron D6F-P
- Honeywell pressure sensor
- turbine control using PWM
- custom PID controller
- 3D-printed pneumatic parts designed in FreeCAD
- 22 mm conical connectors
- D6F-P bypass with a restriction in the main airflow path

<img src="./pictures/camera/integrated-turbine-electronics.jpg" alt="Integrated turbine electronics and sensor connections" width="580" />

*The turbine and electronics integrated into a single test setup.*

## Mechanical design

The pneumatic parts and lid are designed in FreeCAD. The lid uses a gasket to seal against the enclosure, while the adapters use 22 mm conical connections.

<img src="./pictures/camera/lid-gasket-closeup.jpg" alt="Close-up of the lid and gasket interface" width="400" />

*Lid and gasket sealing surface during testing on September 29.*

## Firmware

The firmware is split into separate modules:

- `adc` - airflow measurement
- `spi` - pressure measurement
- `pid` - pressure controller
- `pwm` - turbine control
- `main` - main device logic

The first complete firmware version was released as `1.0.0`.

The PID controller adjusts turbine PWM to maintain the pressure setpoint. Early tuning runs are documented in the journal.

<img src="./pictures/chart/pid-step-response.png" alt="PID controller step response during tuning" width="680" />

*Recorded PID controller response from September 22.*

## Airflow calibration

The D6F-P bypass needs calibration to relate the measured bypass flow to total airflow. A bell prover was explored using a moving inverted bottle sealed by water, with volume and elapsed time as the basis for the method.

<img src="./pictures/camera/bell-prover-counterweight-sketch.jpg" alt="Sketch of the bell prover concept with a counterweight" width="580" />

*Bell prover concept with a counterweight to reduce resistance to movement.*

The prototype used bottles, tubing and a drainage pipe. The bottle was counterbalanced by hand; the recorded results were too inconsistent for reliable calibration. The journal also documents the ADC conversion error discovered later.

<img src="./pictures/camera/bottle-calibration-tubing.jpg" alt="Bottle and tubing in the prototype calibration setup" width="400" />

*The prototype setup used to test the bell prover calibration method.*

## Journal

I document the full development process in [`JOURNAL.md`](JOURNAL.md).

It contains the successive stages of the project, problems encountered along the way, mechanical design changes, and firmware development starting from the first nRF52840 bring-up.

## Repository

- `code/release/src/` - firmware
- `3d-models/` - CAD models and printable files
- `diagram/` - system diagrams
- `docs/` - documentation for the components and standards used
- `JOURNAL.md` - project development journal

## License

AGPL-3.0