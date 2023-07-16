# IoT dashboard
The IoT dashboard was created for a university project to familiarize me with embedded development under the FREE RTOS SDK. The device collects data from temperature/pressure sensor and the open Weather API using HTTP requests in order to display them on the e-ink display connected via SPI. The hardware used in the project is esp8266 microcontroller, bmp280 temperature and pressure sensor, waveshare e-ink 2.9 inch V2 display. The components were placed on a PCB that I designed in Altium Designer.

For the needs of the project, I wrote drivers for the e-ink display and the bmp280 sensor. You can find them in the components dricetory:
```
├── components
│   ├── i2c_bmp280
│   │   ├── component.mk
│   │   ├── i2c_bmp280.c
│   │   └── include
│   │       ├── bmp280_defs.h
│   │       └── i2c_bmp280.h
│   └── waveshare_2in9
│       ├── component.mk
│       ├── fonts.c
│       ├── include
│       │   ├── fonts.h
│       │   ├── w_2in9_defs.h
│       │   ├── w_2in9.h
│       │   └── w_2in9_paint.h
│       ├── w_2in9.c
│       └── w_2in9_paint.c
```
The IoT dashboard also uses the frozen library for JSON serialization and deserialization.

# Photos
![alt text](https://github.com/Cyberdom123/iot-dashboard/blob/main/imag/front.jpg)

![alt text](https://github.com/Cyberdom123/iot-dashboard/blob/main/imag/pcb.jpg)

# Schematic

![alt text](https://github.com/Cyberdom123/iot-dashboard/blob/main/imag/schematic.png)
