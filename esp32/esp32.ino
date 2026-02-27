#include <WiFi.h>
#include <PZEM004Tv30.h>
#include <esp_task_wdt.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "time.h"

// --- ตั้งค่า WiFi ---
const char* ssid = "GU";
const char* password = "********";

const String SUPABASE_LOG_URL = "https://npjszdvrfqrllpqirmvd.supabase.co/rest/v1/energy_log";
const String SUPABASE_CONTROL_URL = "https://npjszdvrfqrllpqirmvd.supabase.co/rest/v1/control";
const String SUPABASE_KEY = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJzdXBhYmFzZSIsInJlZiI6Im5wanN6ZHZyZnFybGxwcWlybXZkIiwicm9sZSI6ImFub24iLCJpYXQiOjE3NzE5MjAyNTEsImV4cCI6MjA4NzQ5NjI1MX0.TgEerk3G3xoXprmaJgLsnQrkwpUqU77RWdhTrSw749g";

const char* NTP_SERVER = "pool.ntp.org";
const long  GMT_OFFSET_SEC = 7 * 3600; // ประเทศไทย UTC+7 (7 ชม. * 3600 วินาที)
const int   DAYLIGHT_OFFSET_SEC = 0;
const unsigned long POST_DELAY = 300000;
const unsigned long GET_DELAY = 2000;
const unsigned long PATCH_CURRENT_DELAY = 5000;
unsigned long _lastPostTime = 0;
unsigned long _lastGetTime = 0;
unsigned long _lastPatchTime = 0;

// --- ตั้งค่าขา Pin ---
#define RELAY_PIN 33
#define PZEM_RX_PIN 16
#define PZEM_TX_PIN 17
#define WDT_TIMEOUT 5
#define ACTIVE_LOW true

PZEM004Tv30 pzem(Serial2, PZEM_RX_PIN, PZEM_TX_PIN);

// --- ตัวแปรจัดการเวลา ---
unsigned long targetTime = 0;
bool _relayStateOn = false;

void setup() {
  // Serial.begin(115200);

  // เริ่มต้น Serial สำหรับ PZEM
  Serial2.begin(9600, SERIAL_8N1, PZEM_RX_PIN, PZEM_TX_PIN);

  // ตั้งค่า Relay (Active Low)
  pinMode(RELAY_PIN, OUTPUT);

  // --- ตั้งค่า Watchdog ---
  esp_task_wdt_config_t wdt_config = {
      .timeout_ms = WDT_TIMEOUT * 1000, // เวลาที่ตั้งไว้ (เช่น 5 วินาที)
      .idle_core_mask = (1 << portNUM_PROCESSORS) - 1, // ตรวจสอบทุก Core
      .trigger_panic = true // ถ้าค้างให้ Restart ทันที
  };
  esp_task_wdt_reconfigure(&wdt_config);
  esp_task_wdt_add(NULL);

  // เชื่อมต่อ WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    // Serial.print(".");
    esp_task_wdt_reset(); // Reset WDT ขณะรอ WiFi
  }
  // Serial.println("\nWiFi Connected! IP: " + WiFi.localIP().toString());
  
  configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, NTP_SERVER);
  // Serial.println("Waiting for time sync...");
  
  // รอจนกว่าเวลาจะซิงค์สำเร็จ (ตรวจสอบว่าปีไม่ใช่ 1970)
  struct tm timeinfo;
  while (!getLocalTime(&timeinfo)) {
    delay(500);
    // Serial.print(".");
  }
  // Serial.println("\nTime Synced!");
}

void loop() {
  esp_task_wdt_reset();

  getRealtimeControllerState();
  patchCurrentData();
  postData();
  
  delay(10); // ลดภาระ CPU
}

// --- ฟังก์ชันอ่านค่า PZEM ---
String getPZEMDataJSON() {
  float v = pzem.voltage();
  float c = pzem.current();
  float p = pzem.power();
  float e = pzem.energy();
  
  String json = "{";
  json += "\"voltage\":" + String(isnan(v) ? 0 : v) + ",";
  json += "\"current\":" + String(isnan(c) ? 0 : c) + ",";
  json += "\"power\":" + String(isnan(p) ? 0 : p) + ",";
  json += "\"energy\":" + String(isnan(e) ? 0 : e) ;
  json += "}";
  return json;
}

void postData() {
  if ((millis() - _lastPostTime) > POST_DELAY) {
    if (WiFi.status() == WL_CONNECTED) {

      HTTPClient http;
      http.begin(SUPABASE_LOG_URL);

      // Add Headers
      http.addHeader("Content-Type", "application/json");
      http.addHeader("apikey", SUPABASE_KEY);
      http.addHeader("Authorization", (String) "Bearer " + SUPABASE_KEY);

      // เอาค่าจากเซ็นเซอร์
      String json_payload = getPZEMDataJSON() ;

      // Serial.print("POST DATA: ");
      // Serial.print(json_payload);
      // Serial.println("--------------------------------------------------");

      int httpResponseCode = http.POST(json_payload);

      if (httpResponseCode > 0) {
        // Serial.print(" -> RES ");
        // Serial.println(httpResponseCode);  // 201 คือสำเร็จ (Created)
      } else {
        // Serial.print("Error code: ");
        // Serial.println(httpResponseCode);
      }
      http.end();
    }
    _lastPostTime = millis();
  }
}  //postData

void patchCurrentData() {
  if ((millis() - _lastPatchTime) > PATCH_CURRENT_DELAY) {
    if (WiFi.status() == WL_CONNECTED) {
      HTTPClient http;

      // ระบุเงื่อนไขใน URL: ?id=eq.1 (แก้ไขเฉพาะแถวที่ id เท่ากับ 1)
      http.begin(SUPABASE_CONTROL_URL + "?id=eq.1");

      http.addHeader("Content-Type", "application/json");
      http.addHeader("apikey", SUPABASE_KEY);
      http.addHeader("Authorization", (String) "Bearer " + SUPABASE_KEY);

      // ส่งเฉพาะข้อมูลที่ต้องการแก้ไข
      String patchData = getPZEMDataJSON() ;

      // ใช้คำสั่ง .PATCH()
      int httpResponseCode = http.PATCH(patchData);
      // Serial.print("PATCH DATA: ");
      // Serial.println(patchData);
      // Serial.println("--------------------------------------------------");

      if (httpResponseCode > 0) {
        // Serial.print(" -> RES ");
        // Serial.println(httpResponseCode);  // ปกติจะได้ 204 (No Content) แปลว่าสำเร็จ
      } else {
        // Serial.print("Error: ");
        // Serial.println(httpResponseCode);
      }
      http.end();
    }
    _lastPatchTime = millis();
  }
}  //patchCurrentData

void getRealtimeControllerState() {
  if ((millis() - _lastGetTime) > GET_DELAY) {
    if (WiFi.status() == WL_CONNECTED) {
      HTTPClient http;
      http.begin(SUPABASE_CONTROL_URL);

      // Headers เหมือนเดิม
      http.addHeader("apikey", SUPABASE_KEY);
      http.addHeader("Authorization", (String) "Bearer " + SUPABASE_KEY);

      const int httpResponseCode = http.GET();  // เปลี่ยนเป็น GET

      if (httpResponseCode > 0) {
        String payload = http.getString();

        // --- ตัวอย่างการแกะข้อมูล JSON ---
        DynamicJsonDocument doc(1024);
        deserializeJson(doc, payload);

        // เนื่องจากผลลัพธ์จาก Supabase จะมาเป็น Array [ ] เสมอแม้จะมีแถวเดียว
        const bool get_state = doc[0]["turn_on"];
        const bool timer_enable = doc[0]["timer_enable"];
        const int turn_on_time = doc[0]["turn_on_time"];
        const int turn_off_time = doc[0]["turn_off_time"];

        // Serial.print("GET TIMER ENABLE: ");
        // Serial.println(timer_enable);
        // Serial.print("GET TURN ON TIME: ");
        // Serial.println(turn_on_time);
        // Serial.print("GET TURN OFF TIME: ");
        // Serial.println(turn_off_time);
        // Serial.print("GET RELAY STATE: ");
        // Serial.println(get_state);
        // Serial.println("--------------------------------------------------");
        if( timer_enable ) 
          timerController( turn_on_time, turn_off_time ) ;
        else if (get_state != _relayStateOn) {
          _relayStateOn = get_state;
          digitalWrite( RELAY_PIN, _relayStateOn ) ;
          // Serial.print("CHANGE RELAY STATE TO: ");
          // Serial.println(_relayStateOn);
        } ;
      } else {
        // Serial.print("Error on GET: ");
        // Serial.println(httpResponseCode);
      }
      http.end();
    }
    _lastGetTime = millis();
  }
}  //getRealtimeState

void timerController(const int turn_on_time, const int turn_off_time) {

  const int currentTime = getNowAsInt();
  if (currentTime == -1) return; // ดึงเวลาไม่ได้ ให้ข้ามไปก่อน

  bool shouldBeOn = false;

  // 2. Logic ตรวจสอบช่วงเวลา (ครอบคลุมทุก Test Case)
  if (turn_on_time < turn_off_time) {
    // กรณีปกติ (ไม่ข้ามเที่ยงคืน) เช่น On 08:00 (480) -> Off 17:00 (1020)
    shouldBeOn = (currentTime >= turn_on_time && currentTime < turn_off_time);
  } else {
    // กรณีข้ามเที่ยงคืน เช่น On 23:00 (1380) -> Off 02:00 (120)
    // จะเปิดเมื่อ "เวลาตอนนี้มากกว่าเวลาเปิด" OR "น้อยกว่าเวลาปิด"
    shouldBeOn = (currentTime >= turn_on_time || currentTime < turn_off_time);
  }

  shouldBeOn = ACTIVE_LOW ? !shouldBeOn : shouldBeOn ;
  // 3. ตรวจสอบสถานะปัจจุบันกับสถานะที่ควรจะเป็น
  // ถ้าสถานะไม่ตรงกัน ถึงจะส่งค่าไป Patch เพื่อประหยัด Bandwidth และไม่โดน Supabase Block
  if (shouldBeOn != _relayStateOn) {
    _relayStateOn = shouldBeOn; // อัปเดตตัวแปรในเครื่อง
    digitalWrite( RELAY_PIN, _relayStateOn ) ;
    
    // Serial.print("TIMER TRIGGERED! NOW: ");
    // Serial.print(currentTime);
    // Serial.println(shouldBeOn ? " -> TURN ON" : " -> TURN OFF");
    
    patchState(); // ส่งค่าไปอัปเดตบน Supabase
  }
}//timerController

int getNowAsInt() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    return -1; // ถ้าดึงเวลาไม่ได้ให้คืนค่า -1
  }
  // เลือกวิธีที่ต้องการ (ในที่นี้ใช้แบบนาทีรวม 0-1439)
  return (timeinfo.tm_hour * 60) + timeinfo.tm_min;
}

void patchState() {
    HTTPClient http;
    http.begin(SUPABASE_CONTROL_URL + "?id=eq.1");

    http.addHeader("Content-Type", "application/json");
    http.addHeader("apikey", SUPABASE_KEY);
    http.addHeader("Authorization", (String) "Bearer " + SUPABASE_KEY);

    String patchData = "{";
    patchData += "\"turn_on\":" + String( _relayStateOn ? "true" : "false" ) ;
    patchData += "}";

    // ใช้คำสั่ง .PATCH()
    int httpResponseCode = http.PATCH(patchData);
    // Serial.print("PATCH STATE: ");
    // Serial.print(patchData);

    if (httpResponseCode > 0) {
      // Serial.print(" -> RES ");
      // Serial.println(httpResponseCode);  // 204 (No Content) แปลว่าสำเร็จ
      // Serial.println("--------------------------------------------------");
    } else {
      // Serial.print("Error: ");
      // Serial.println(httpResponseCode);
      // Serial.println("--------------------------------------------------");
    }
    http.end();
}
