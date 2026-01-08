# 🚗 ECUMaster Black + ESP32 Bluetooth Display

[![ESP32](https://img.shields.io/badge/board-ESP32-blue.svg)](https://www.espressif.com/en/products/socs/esp32)
[![ECUMaster](https://img.shields.io/badge/device-ECUMaster-black.svg)](https://www.ecumaster.com)
[![LVGL](https://img.shields.io/badge/UI-LVGL-purple.svg)](https://lvgl.io/)
[![Arduino](https://img.shields.io/badge/framework-Arduino-blue.svg)](https://www.arduino.cc)
[![License: GPL v3](https://img.shields.io/badge/license-GPLv3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0.en.html)


### 📦 Description

This project interfaces the **ECUMaster Black ECU** with an **ESP32** to create a canbus-enabled dashboard display, 

### ✅ Tested On

- ESP32 JC2432W328

- Espressif esp32 Core v3.3.5
- ECUMaster Black 
- LVGL v8.3.0

---

### 📥 Installation

1. Clone this repo
2. Install dependencies (LVGL 8.3, eSPI, emucan, etc.)
3. Edit LVGL and eSPI  if needed.
4. Copy the mono-space fonts  to your project directory
5. Upload to ESP32
6. Optional - Connect an active 3.0-3.3V buzzer to GPIO Pin 22 (for warnings/CEL)
7. Enjoy real-time data on-screen!

---

### 📦 Dependencies

lvgl 8.3.0
esp32 3.3.5 by espressif (boardmanager)
emucan
driver/twai
--- 


Personal words: Honestly I despise lvgl . My coding skills are awful and chatgpt really sucks for this. LVGL is so damn sensitive that the code may not even work if you use lvgl 8.3.2 instead of 8.3.0. 
Everytime they release a new version its almost gonna guaranteed break your "old" code. They remove /replace lines of code between versions without caring at all. 
---

### 🧰 Hardware Used

- **ESP32 JC2432W328**  
  [🛒 AliExpress link](https://www.aliexpress.com/item/1005006729707613.html)
- **3D Printed Case**  
  [📦 Thingiverse STL](https://www.thingiverse.com/thing:6705691)
- **ECU**  
  ECUMaster Black
- **Active 3.3V buzzer**    
 [🛒 AliExpress link](https://www.aliexpress.com/item/1005008682347898.html)

  SN65HVD230 Can transceiver 
---

## 📺 Demo



---

### 📜 License

This project is licensed under **GPL v3**.  
See the [LICENSE](./LICENSE) file for more info.

---

### ❤️ Credits & Contributions

Made with ❤️ for petrolheads.  
Contributions, forks, and feature requests are welcome!
