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

# September 28 - Final adapter version and first enclosure work

Today I finished another, nearly final version of the D6F-P bypass adapter.

I kept the possibility of testing different restriction diameters and prepared the variants needed for further experiments. The adapter itself was already developed enough that I could stop focusing on its basic geometry and move on to another part of the test setup.

I started designing the lid that will create a sealed connection between the tester and the enclosure being tested. It was a much larger part than the previous components and it quickly became clear that it required a different approach to modelling.

This part also helped me make a lot of progress in FreeCAD. Instead of treating sketches only as a way to draw a specific shape, I started using constraints and dimensional relationships much more deliberately. The model started becoming something that could be modified later without rebuilding everything from scratch.

I also cleaned up the mechanical side of the repository and added the models, printable files, documentation and an updated diagram of the tester.

**Total time spent 5 hours**

# September 29 - Cleaning up the firmware

After several days of adding new functions, the code still contained many parts that had been created while I was learning the nRF52840, Zephyr and C. Today I started turning the earlier prototype into more organised firmware.

I removed unnecessary fragments and leftovers from earlier tests. I changed the D6F-P flow conversion to use the polynomial approximation from the Omron documentation and changed the airflow value to a `float`.

I also moved the PID controller out of `main.c` into a separate module. This made the main loop much simpler and left it mostly responsible for the sequence of operations, including reading airflow, reading pressure, calculating the PID output and updating the PWM.

I also started improving the ADC and SPI error handling. Earlier versions of the code mostly assumed that every read would succeed, which was acceptable during the first tests but not for a more complete device.

**Total time spent 6 hours**

# September 30 - Firmware 1.0.0 and the airflow calibration problem

Today I finished a larger cleanup of the firmware. The code went through several versions before reaching `1.0.0`.

I cleaned up the ADC, SPI, PWM and PID modules, added separate error status values and improved hardware initialisation. I also moved the PID parameters into one structure so that different settings can later be used without changing the controller algorithm itself.

After bringing the firmware to this stage, a larger measurement problem still remained. The D6F-P measures only a small part of the airflow in the bypass, so the printed adapter and calculated restriction alone are not enough. I still need to determine the real relationship between the airflow through the D6F-P and the total airflow through the tester.

I started with a whiteboard brainstorming session and tried to develop a calibration method from first principles.

The first obvious ideas were a Pitot tube and a Venturi, but I rejected them fairly quickly. In a home-built setup there are too many additional variables related to geometry, flow profile and manufacturing accuracy for either method to be a reliable reference.

The next idea was a balloon with a known volume. The problem is that its walls stretch during inflation and create their own pressure, so the measurement conditions change as the balloon fills.

A large syringe seemed like a better direction. The problem there is the piston seal and static friction. At low pressure differences the piston does not move perfectly smoothly, so it is also not a good reference.

By the end of the day I had a much clearer idea of what the calibrator needed to do. It has to measure a known volume of air over a known time while adding as little and as predictable resistance to the airflow as possible.

**Total time spent 7 hours**

# October 1 - Parametric lid and bell prover

The first lid tests revealed another mechanical problem. The large flat surface was too flexible, which made it difficult to maintain even and sealed contact with the gasket.

I redesigned the lid into a cone-like shape. The geometry itself significantly increased its stiffness without requiring the entire part to become very thick.

I also wanted to be able to test different wall thicknesses easily. This forced me to define the constraints in FreeCAD more carefully. Critical geometry such as the gasket interface and the 22 mm connector can remain unchanged while the wall thickness is treated as a parameter that can be adjusted independently.

This was the point where I really understood the advantage of parametric modelling. Instead of correcting several dependent dimensions after every change, I can now change one value while keeping the important design assumptions intact. After solving this, FreeCAD started to feel more suitable for this kind of work than Fusion 360.

I also returned to the airflow calibration problem. I reversed the syringe idea. Instead of moving a sealed piston inside a cylinder, the whole container can move while water is used only as the seal.

In practice, this means using an inverted container of known volume partially submerged in water. Incoming air raises the container, and the airflow can be calculated from the displaced gas volume and the elapsed time. A larger container allows the same principle to be used for much higher flow rates.

I also added the idea of a counterweight that can nearly balance the moving container and reduce the additional pressure required to lift it.

Only later did I find out that this was not a new invention. A very similar method already exists in gas flow metrology and is known as a **bell prover**, a volumetric displacement standard used for calibrating gas flow.

**Total time spent 8 hours**

# October 2 - First complete electronics and PID tests

Today I started assembling the electronics in a form much closer to the actual tester and testing the whole system as one closed control loop.

On paper, all of the individual parts were already working. Pressure measurement over SPI, airflow measurement, PWM control and the PID controller had all been tested separately. Once they were combined in the real setup, several problems appeared that were not visible during individual module testing.

I fixed several issues related to initialisation and PWM control. The PID also behaved differently than it did during the earlier, more controlled tests. When the PWM output reached its maximum limit, the integral term could continue growing even though the system could no longer increase the control output. The physical setup also made it clear how important the correct controller direction is.

The most important change was the pressure sensor. The Honeywell ABP I had been using had a range that was much too large compared with the target pressure of around 50 Pa. I replaced it with a more suitable Honeywell HSC `002ND` differential pressure sensor with a much smaller measurement range.

Communication still uses SPI and the same basic Honeywell digital pressure format, but the physical pressure range represented by the output values is different. I therefore also started moving the pressure calculations from psi to pascals, which makes much more sense at such low pressures.

The electronics and the required components are now assembled together. The next step is to adapt the controller to the real setup. The earlier PID settings looked reasonable during simpler tests, but the complete system now needs a more systematic way of choosing the parameters instead of relying only on trial and error.

**Total time spent 8 hours**