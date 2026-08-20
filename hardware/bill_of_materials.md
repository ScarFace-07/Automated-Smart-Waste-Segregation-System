# Bill of Materials

| Component | Key Specs | Notes |
|---|---|---|
| Arduino Mega 2560 | ATmega2560, 16MHz, 54 digital I/O, 16 analog in | Main controller |
| ESP8266 NodeMCU | Tensilica L106, 80MHz, Wi-Fi 802.11 b/g/n | Cloud connectivity |
| YL-69 Moisture Sensor | 3.3–5V, analog + digital out | Wet waste detection |
| Custom inductive coil metal sensor | 5V DC, ~1–3cm detection range | Metal waste detection |
| HC-SR04 Ultrasonic Sensor ×4 | 2–400cm range, ±3mm accuracy | 1 end-of-belt + 3 per-bin |
| NEMA17 Stepper Motor | 1.8°/step, 40–52 N·cm holding torque | Bin platform rotation |
| A4988 Stepper Driver | 8–35V motor, up to 2A/coil, microstepping | Drives NEMA17 |
| MG995 Servo Motor | 4.8–7.2V, 9.4–11 kg·cm torque, 0–180° | Waste release mechanism |
| L298N Dual H-Bridge Driver | 5–35V motor, 2A/channel | Conveyor DC motor control |
| DC Geared Motor | 6–12V, 100–300 RPM | Drives conveyor belt |
| 16x2 I2C LCD | I2C (SDA/SCL), address 0x27/0x3F | Status display |
| 12V 5A SMPS | 100–240V AC in, 12V DC / 60W out | System power |
| Piezo Buzzer | 3.3kHz, 3–24V | Audio alerts |
| Ball bearings | — | Bin platform rotation support |

Full component descriptions, pinouts, and datasheet-level detail are in [`../docs/Project_Report.pdf`](../docs/Project_Report.pdf), Chapter 3.
