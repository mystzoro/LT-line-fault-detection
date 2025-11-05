# ⚡ Smart Line Fault Detection & Monitoring System (IoT Project)

This project presents a **Smart Line Fault Detection & Monitoring System** designed for **Low Tension (LT) power lines** using **ESP32 microcontrollers**.  
It detects line faults, locates their position, and automatically isolates the faulty section while sending real-time alerts to the control center.  

Developed by team Cypher Tech this project addresses the problem of detecting LT line breaks that traditional circuit breakers fail to identify.

---

## 📁 Project Files

| File Name | Description |
|------------|-------------|
| `pole1.ino` | Main ESP32 node code that monitors current, voltage, and vibration readings. Transmits data to the control center. |
| `pole2.ino` | Secondary ESP32 node that communicates with `pole1.ino` for fault synchronization and relay control. |

---

## ⚙️ Hardware Components Used
- ESP32 Microcontroller (x2)  
- Current Transformer (CT) Sensor  
- Voltage Sensor  
- Vibration Sensor  
- Relay Module  
- Jumper Wires & Breadboard  
- Power Supply / Battery / Solar Module  
- Optional: GPS Module for fault location tracking  

---

## 🔌 Working Principle
1. **Sensors** measure current, voltage, and vibration at each LT pole.  
2. **ESP32 Node 1 (`pole1.ino`)** detects abnormal readings and identifies potential line breaks.  
3. **ESP32 Node 2 (`pole2.ino`)** receives the signal, triggers the **relay** to isolate the faulty section, and logs data.  
4. Data is sent wirelessly to a **central dashboard** showing live fault status and location.  

Together, they form an **automated, low-cost, and scalable fault detection system** for smart power grids.

---

## 🛠️ Setup Instructions
1. Upload `pole1.ino` to the first ESP32 board.  
2. Upload `pole2.ino` to the second ESP32 board.  
3. Connect the sensors and relays according to your circuit diagram.  
4. Power both boards and observe communication and fault detection behavior.  
5. Optionally connect to a dashboard or IoT platform for live monitoring.  

---

## 💡 Features
- Real-time fault detection and location tracking  
- Automatic isolation of faulty section via relay  
- Wireless alerts using Wi-Fi / LoRa / GSM  
- Solar or battery-powered operation  
- Scalable for rural and urban grids  
- Cost-efficient prototype under ₹2000  

---

## 🧠 Future Enhancements
- Add AI-based signal filtering to avoid false triggers  
- Integrate a mobile app for remote fault alerts  
- Cloud-based analytics for predictive maintenance  
- Enhanced visualization dashboard with map integration  

---

## 📸 Preview

Here’s how the **Smart Line Fault Detection setup** looks:

<p align="center">
  <img src="circuit diagram.jpg" width="550" alt="Smart Line Fault Detection Circuit Setup">
</p>

---

## 🪪 License
This project is open-source under the [MIT License](LICENSE).

---

## 👤 Author
**Priyanshu Chand**  
Team: **Cypher Tech ** — * Hackathon 2025*  
📧 [priyanshuchand101@gmail.com]  
⭐ Don’t forget to star this repository if you find it helpful!
