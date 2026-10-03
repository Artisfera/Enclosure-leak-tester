# Enclosure Leak Tester

An enclosure leak tester that measures the amount of airflow required to maintain a constant pressure inside the tested enclosure.

I am developing this project independently as part of an unpaid technical internship at a large company. It was created in response to a real engineering problem I encountered during the internship. Because the intended application is internal, I do not publish the company name or any details that could identify the specific use case.

The tester architecture, firmware, and mechanical design presented here are all developed by me.

## How does it work?

A turbine pushes air into the tested enclosure, while a PID controller adjusts its output to maintain the target pressure.

Pressure is measured using a Honeywell sensor connected over SPI. Airflow is measured using an Omron D6F-P connected to the ADC.

Because the measurement range of the D6F-P is much lower than the total airflow of the system, the sensor operates in a separate bypass. Most of the air flows through the main channel and its restriction, while only a small portion passes through the D6F-P.

The final goal is to determine the enclosure leak rate from the airflow required to maintain a constant pressure.

## Main components

- Nordic nRF52840
- Zephyr / nRF Connect SDK
- Omron D6F-P
- Honeywell ABP pressure sensor
- turbine control using PWM
- custom PID controller
- 3D-printed pneumatic parts designed in FreeCAD
- 22 mm conical connectors
- D6F-P bypass with a restriction in the main airflow path

## Firmware

The firmware is split into separate modules:

- `adc` - airflow measurement
- `spi` - pressure measurement
- `pid` - pressure controller
- `pwm` - turbine control
- `main` - main device logic

The first complete firmware version was released as `1.0.0`.

## Journal

I document the full development process in [`JOURNAL.md`](JOURNAL.md).

It contains the successive stages of the project, problems encountered along the way, mechanical design changes, and firmware development starting from the first nRF52840 bring-up.

## Repository

- `src/` - firmware
- `3d-models/` - CAD models and printable files
- `Diagram/` - system diagrams
- `docs/` - documentation for the components and standards used
- `JOURNAL.md` - project development journal

## License

AGPL-3.0