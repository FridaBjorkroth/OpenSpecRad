# The OpenSpecRad system
This is the repository for files created during the development of an Open-source SpectroRadiometer (OpenSpecRad) system. The OpenSpecRad system is used for in situ measurement of the water-leaving reflectance, used in water quality monitoring of marine and lake water. All software and drawings are available and open-source, which means that anyone can download and use and/or make changes to the construction.

The water-leaving radiance is the spectrum of sunlight backscattered out of the ocean after interaction with the water and its constituents. Each component, as well as the water itself, has different inherent optical properties (IOPs) and absorbs and scatters different parts of the spectrum. Therefore the water-leaving radiance holds quantitative and qualitative information about the main optically active components of water, i.e. chlorophyll-a (Chl-a), coloured dissolved organic
matter (CDOM), and total suspended matter (TSM). Through bio-optical algorithms their concentrations can be derived.

## Using the OpenSpecRad system 
The downwelling irradiance Ed(z,λ) and upwelling radiance Lu(z,λ) is measured using an in-water instrument (Pic), as well as the downwelling irradiance just above the sea surface Ed(0+,λ) using a separate unit. The in-water instrument was built from two 3D-printed pressure and waterproof housings mounted onto a custom steel frame. The above-water unit was put in a plastic box to make it splash-proof. The measurement is started remotely by connecting a laptop/smartphone to the local WiFi network managed by the above-water unit. 

## Hardware
The hardware was based on the STS Developer's Kit from Ocean Optics (now Ocean Insight). The in-water units each include a STS-VIS miniature spectrometer, a Raspberry Pi 3B+ single-board computer, a Bar30 pressure sensor from Blue Robotics coupled via an I²C level converter, and a 10 Ah battery. The above-water unit was configured in the same way, except for the pressure sensor that was swapped for a real-time clock (RTC).

## Software
The open-source spectrometer device driver library SeaBreeze (version 3.0.11, Ocean Optics) was used for communicating with the spectrometers. A C program was written for making spectral measurements. A shell program was then developed for controlling the OpenSpecRad system from a laptop, e.g. starting the measurement programs, detecting errors, and transferring the collected data remotely via SSH. The measurement parameters, e.g. integration time, minimum depth for the first measurement, and depth resolution, were read from separate files when starting the three radiometric acquisitions, making changes of parameters easy.

![alt text](https://github.com/FridaBjorkroth/OpenSpecRad/blob/master/DSC_1105.JPG)

![alt text](https://github.com/FridaBjorkroth/OpenSpecRad/blob/master/Ed0sensorJPG.JPG)
