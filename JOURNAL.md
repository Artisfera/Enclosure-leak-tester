---
title: "Enclosure Leak Tester"
author: "Patryk Ankudowicz"
description: "An enclosure leak tester based on pressure and airflow measurements."
created_at: "2026-09-15"
---

# September 15: First work with the nRF52840 and Zephyr

Today I started the actual work on the project.

I chose the Nordic nRF52840 and Zephyr even though I had not built a larger project on this platform or in C before. I wanted to get the whole workflow working first, from writing the code to building it, flashing the board and debugging it.

I wrote a few simple test programs, including basic GPIO control and a blink test. That gave me a practical way to understand how a Zephyr project is structured and where the hardware configuration fits into it.

By the end of the day I had the development environment working properly and a basic code structure ready for the actual tester instead of just standalone examples.

**Total time spent: 5 hours**

# September 16: Flow measurement and first turbine control

Today I connected the first real sensor used in the project, an Omron D6F-P0001A1 flow sensor.

I started by configuring the ADC on the nRF52840 and reading the analog output from the sensor. Raw ADC values were not very useful on their own, so I added conversion to millivolts and then converted the voltage into airflow in ml/min.

After a few corrections the readings became stable enough to use in the rest of the program.

Later I moved on to turbine control. I first tested PWM on a simple load and only then moved the output to the pin intended for the turbine.

By the end of the day I had both parts working together. The firmware could measure airflow and control the device generating it.

**Total time spent: 7 hours**

# September 17: Reading the pressure sensor over SPI

Today I added the Honeywell pressure sensor and started working with SPI.

The communication itself looked simple at first, but getting it configured correctly in Zephyr took more work than I expected. I had to set up the bus, pin assignments, chip select and the sensor configuration before the received data became reliable.

The first bytes coming from the sensor were not a ready pressure value either. The sensor includes status information together with the measurement, so I had to separate those bits first and then convert the remaining data into a physical pressure value.

I also moved the pressure sensor code into its own source file. At this point the project already included ADC, PWM and SPI, so keeping everything inside one `main.c` file would have become messy very quickly.

At the end of the day pressure and airflow measurement were both working in the same project.

**Total time spent: 7 hours**

# September 18: First complete tester software

Today I combined the separate pieces from the previous days into one working demonstration program.

Until now the flow sensor, pressure sensor and turbine control had mostly been tested independently. I connected them into the first complete measurement sequence.

The program can start the turbine and monitor the pressure rise inside the enclosure. Once the target pressure is reached, the turbine can be stopped and the software continues monitoring pressure and airflow for a defined period.

It is still an early version of the measurement method, but this was the first time the whole sequence could run automatically from start to finish.

The test also showed a weakness in the original control approach. A fixed PWM value does not give the same airflow under every condition. The turbine operating point changes together with pressure and the resistance of the pneumatic system.

That meant fixed PWM control would not be enough if I wanted repeatable measurements.

**Total time spent: 7 hours**

# September 21: Controlling the turbine with PID

Today I replaced the simple turbine control with my first PID controller.

Instead of using a fixed PWM duty, the firmware now compares the measured value with the setpoint and continuously corrects the turbine output.

I implemented the controller myself rather than using a ready library. I wanted to understand how the proportional, integral and derivative parts behaved in the real system and not just treat the controller as a black box.

I also added real timing between control loop iterations. The integral and derivative terms depend on time, so assuming that every loop takes exactly the same amount of time would make the controller less predictable.

For the first tests I limited the maximum turbine output as well. With badly chosen parameters, a controller can react far too aggressively to a large error, so keeping the available range limited made the early tests much safer and easier to understand.

By the end of the day the closed loop was working and the measured value directly affected the next PWM setting.

**Total time spent: 7 hours**

# September 22: PID tuning and first physical test setup

Today I focused on tuning the controller on a real setup.

I tested different values of Kp, Ki and Kd and watched how they changed the response time, overshoot and overall stability.

Reading everything directly from the terminal quickly became inconvenient, so I changed the serial output to make plotting easier. I printed the measured value, setpoint, error, individual PID terms and current PWM duty together.

Seeing the response on a graph made the tuning much easier. It was immediately visible when one part of the controller started dominating or when the system began oscillating.

I also assembled the first temporary test setup with the turbine and sensors, so from this point the controller was being tested with real airflow rather than only as software.

This was also where the main limitation of the D6F became obvious. The sensor is very useful for small airflow because its range is only around 0 to 100 ml/min, but that same range becomes a problem when the enclosure leaks more heavily.

I wanted to keep the sensitivity of the D6F without limiting the entire tester to such a small total airflow.

**Total time spent: 7 hours**

# September 23: First mechanical design and the 22 mm connection

Today most of the work moved from firmware to mechanics.

I needed a proper airflow path for the tester, so I started working in FreeCAD. This was my first real project in the program and I decided to learn it directly on the part I actually needed instead of spending hours making unrelated tutorial models.

For the pneumatic connection I did not want to invent another arbitrary connector. I chose a standardized 22 mm conical connection based on geometry used in medical breathing systems.

I went through the available dimensions and started reproducing the geometry in CAD. One of the important details was that the 22 mm connection is not simply a straight cylinder. The mating surface has a defined taper, which had to be included in the model.

FreeCAD worked much better for this than I expected. Once I understood the basic workflow it felt much more natural than the other CAD tools I had tried before.

This tutorial helped me a lot when getting started.

https://www.youtube.com/watch?v=JjFh8vtMBC8

At the same time I started working on a way to solve the D6F range problem. The sensor itself measures only around 100 ml/min, while the main pneumatic path may eventually need to handle airflow in the tens of liters per minute and possibly close to 100 L/min.

The idea was to measure only a small fraction of the total airflow instead of sending everything through the D6F.

**Total time spent: 7 hours**

# September 24: Modelling the airflow path in FreeCAD

Today almost all of the work was CAD.

I continued developing the 22 mm connector and started building the main airflow path around it.

I added the first version of a restriction in the main channel and prepared locations for a separate measurement path. The model quickly became more than just a simple adapter.

I also ran into several FreeCAD problems along the way. Some sketches looked closed but were not actually valid closed profiles, and the solver occasionally reported conflicting constraints that were not obvious at first.

After several failed sketches, a few crashes and more time spent understanding how FreeCAD expects the model to be structured, things finally started to make sense.

A lot of the problems were not really caused by the operations themselves. They came from how I had built the sketches and relationships between features.

By the end of the day I had the basic geometry ready for the actual D6F bypass.

**Total time spent: 5 hours**

# September 25: 22 mm adapter and D6F-P bypass

Today I worked on the most important mechanical part of the measurement system so far.

Passing the whole airflow through the D6F would not make sense because the sensor reaches its useful limit at around 100 ml/min. Instead, I designed a bypass.

Most of the air travels through the main channel while only a small fraction passes through the D6F. I added a restriction in the main path so that a pressure difference appears between two points. That pressure difference drives airflow through the bypass and through the sensor.

This allows the D6F to stay inside its normal measurement range while the complete pneumatic system handles a much larger total airflow.

I expanded the 22 mm adapter with the main restriction, pressure pickup locations, bypass channels and the sensor mounting geometry.

The first restriction dimensions are only a calculated starting point. I do not expect the theoretical geometry alone to give an accurate final result. The printed surface, real channel dimensions, bypass resistance and the sensor itself will all affect the relationship between bypass flow and total flow.

The finished assembly will therefore need real calibration.

The model had also become large enough that I had to start separating and organizing the solids with printing and later revisions in mind.

I sent the parts to the printer for the weekend, so the next useful test will be with the actual hardware rather than another CAD screenshot.

I also added a 45 degree transition inside the female side of the connector. This lets the part print without internal supports. I wanted to avoid support marks inside the airflow path because they could change the surface and make the flow less repeatable.

**Total time spent: 8 hours**