# ⚡ IoT Smart Plug (ESP32 + Supabase + AI Chat)

โปรเจคปลั๊กไฟอัจฉริยะที่สามารถวัดพลังงานไฟฟ้า ควบคุมผ่านหน้าเว็บแบบ Realtime และมีระบบผู้ช่วย AI ในตัว พัฒนาขึ้นเพื่อเป็น Term Project

## 🌟 Features (ความสามารถหลัก)

  * **Real-time Monitoring:** แสดงค่าแรงดันไฟฟ้า (Voltage), กระแสไฟ (Current), กำลังไฟฟ้า (Power) และพลังงานรวม (Energy)
  * **Remote Control:** เปิด-ปิด Relay ผ่านอินเทอร์เน็ตได้จากทุกที่
  * **Daily Scheduling:** ระบบตั้งเวลาเปิด-ปิดอัตโนมัติในทุกๆ วัน
  * **Data Visualization:** แสดงกราฟสถิติย้อนหลัง (Current, Power, Energy) เพื่อวิเคราะห์การใช้งาน
  * **AI Assistant:** หน้า Chat สำหรับพูดคุยสอบถามข้อมูลกับ AI เบื้องต้น
  * **Supabase Integration:** \* **Database:** เก็บข้อมูลการใช้พลังงานและประวัติการทำงาน
      * **Realtime:** อัปเดตสถานะอุปกรณ์และค่าที่อ่านได้ทันทีโดยไม่ต้อง Refresh หน้าเว็บ

-----

## 🏗️ Hardware & Software Stack

### Hardware

  * **ESP32** (Microcontroller หลัก)
  * **PZEM-004T** (เซนเซอร์วัดค่าพลังงานไฟฟ้า - หรือเซนเซอร์อื่นๆ ที่คุณใช้)
  * **Relay Module** (สำหรับตัด-ต่อวงจรไฟฟ้า)

### Software & Cloud

  * **Frontend:** html, css, js
  * **Backend/Database:** [Supabase](https://supabase.com/) (PostgreSQL + Realtime engine)
  * **Communication:** HTTP / WebSockets (ผ่าน Supabase SDK)

-----

## 📊 Database Schema (Supabase)

ระบบมีการจัดเก็บข้อมูลหลักๆ ดังนี้:

  * `measurements`: เก็บค่า V, I, P, E พร้อม Timestamp
  * `device_status`: เก็บสถานะ Relay (ON/OFF) และการตั้งเวลา (Schedule)
  * `chat_history`: (ถ้ามี) เก็บประวัติการคุยกับ AI

-----

## 🚀 Getting Started (วิธีการติดตั้ง)

1.  **Clone Repository:**

    ```bash
    git clone https://github.com/soraphu/iot-smartplug.git
    cd iot-smartplug
    ```

2.  **ESP32 Setup:**

      * เปิดโฟลเดอร์ Firmware ใน Arduino IDE
      * ติดตั้ง Library ที่จำเป็น (เช่น `Supabase-Arduino`, `PZEM004T`)
      * แก้ไฟล์ Config ใส่ `WiFi SSID`, `Password` และ `Supabase URL/Key`
      * Flash โปรแกรมลง ESP32

-----

## 👨‍💻 Author

  * **Soraphu** - [GitHub Profile](https://www.google.com/search?q=https://github.com/soraphu)

-----

**หมายเหตุ:** โปรเจคนี้เป็นส่วนหนึ่งของวิชา IoT ภาคเรียนที่ 2 ปีการศึกษา 2569