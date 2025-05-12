# 🌱 Smart Greenhouse Controller (IoT Project)

📄 This README is available in multiple languages:
🇭🇺 [Magyarul itt olvasható »](docs/README.hu.md)

This project is a smart greenhouse controller built with Arduino and ESP8266 for monitoring and controlling various environmental factors in a greenhouse. The system collects data from multiple sensors and can be monitored remotely via a web interface.

## Features
- **Sensors:**
  - DHT11 – Temperature and Humidity Sensor
  - Soil Moisture Sensor (Analog output)
  - Light Sensor (LDR)
- **ESP8266 WiFi Module** – Enables remote monitoring via a web page.
- **OLED Display** – Displays real-time data on temperature, humidity, soil moisture, and light intensity.
- **Joystick** – Allows manual control for adjusting parameters such as watering or fan speed.
- **Future Plans** – Adding CO2 monitoring capabilities.

📄 Documentation
The full project documentation is available in two languages (Hungarian and English) in the /docs folder. It includes hardware schematics, system overview, and usage instructions.

## Components
- **Microcontroller:**
  - Arduino (for sensor data processing and control)
  - ESP8266 (for WiFi and web server)
  
- **Sensors:**
  - DHT11 – For temperature and humidity measurement
  - Soil Moisture Sensor – For soil moisture monitoring
  - Light Sensor (LDR) – For light intensity measurement
  
- **Display:**
  - OLED SSD1306 – For displaying sensor data
  
- **Joystick:**
  - Two-axis analog joystick for manual input
  
- **Libraries:**
  - `#include <SPI.h>` – For SPI communication
  - `#include <Wire.h>` – For I2C communication
  - `#include <Adafruit_GFX.h>` – For graphics on the display
  - `#include <dht.h>` – For DHT11/DHT22 sensor handling

## PCB and Schematics
The project includes a custom PCB designed for the greenhouse controller. You can find the PCB files and the circuit schematic below:
- **PCB Files**: The PCB layout files (Gerber) can be found in the `/hardware/` folder.
- **Schematic Diagram**: The circuit schematic for the project is available as [PDF](docs/Schematic.pdf) [SVG](docs/Schematic.svg).

**Wiring and Assembly**:
- The wiring diagram is available under `/docs/wiring_diagram.png`, which illustrates how to connect the sensors and the microcontroller.

## 3D Printed Case
The project includes a 3D printed case to house the controller and sensors. This case helps protect the electronics and provides a clean and durable solution for mounting the components.

- **3D Printing Files**: The STL files for the 3D printed case can be found in the `/3d_case/` folder.
- You can print the case using any 3D printer.
- **Print Settings**: The recommended print settings are:
  - Layer height: 0.2mm
  - Infill: 20-30%
  - Material: PLA or ABS

## Setup and Installation
1. **Install Arduino IDE**: If you haven’t already, install the [Arduino IDE](https://www.arduino.cc/en/software).
2. **Install Libraries**: 
   - Adafruit GFX: `Sketch > Include Library > Manage Libraries > Adafruit GFX`
   - DHT Sensor Library: `Sketch > Include Library > Manage Libraries > DHT sensor library`
   
3. **Wiring**:
   - Connect your sensors to the Arduino as per the wiring diagram. You can find the wiring diagram under `/docs/wiring_diagram.png`.

4. **Upload Code**:
   - Upload the code from `firmware/SmartGreenhouse.ino` to your Arduino using the Arduino IDE.

## Future Work
1. **Integration of Additional Modules**
    Add CO2 sensor to monitor air quality.
    Add I2C clock module, for time based irrigation

2. **Memory Optimization**  
   The system currently operates with redundant, repeated text outputs, such as on the OLED display. These text strings should be organized into static character arrays or `const char*` variables, reducing SRAM usage.

3. **Eliminating AT Commands**  
   The ESP8266 is currently controlled via AT commands, which is not the most efficient approach. Instead of using the AT firmware, it would be better to use the NodeMCU (Lua) or the ESP8266 Arduino core, programming the microcontroller directly. This would remove the need for AT commands over the serial connection, allowing the ESP8266 to handle the control directly, while the Arduino Mega only transmits sensor data. This improves response time, reduces the chance of errors, and opens up further possibilities (e.g., HTTPS connections, JSON handling). Right now, when the controller is communicating, the hardware interface is freezd for a sec.

4. **External Network Accessibility**  
   The system is currently only accessible on the local network. A future development could involve making the web interface accessible over the internet.

5. **Data Security and Encryption**  
   The transmitted data is likely unencrypted over the network, making encryption a critical feature to implement.

6. **Fault Tolerance and Diagnostics**  
   Implementing a basic fault-tolerant system would be useful, such as using a watchdog (same used in the restart method) timer that restarts the system if it freezes or if no data is received within a certain time period. A logging feature for save errors or key events to an SD card.
## License
This project is licensed under the [MIT License](LICENSE).
You are free to use, modify, and distribute this code, even in commercial projects, as long as you include the original license and credit the author.

## Contact
If you have any questions or suggestions, feel free to contact me at:  
ata.kedves@gmail.com

---

🌱 Happy gardening with your smart greenhouse!
