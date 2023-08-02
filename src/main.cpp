#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include <PubSubClient.h>
#include <EasyButton.h>

// WEB PAGE
const char TITLE[] PROGMEM = "<!DOCTYPE html><html lang=\"en\"><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1, user-scalable=no\"/><title>";
const char HEAD[] PROGMEM = "</title><style>body{background-color:black;font-family: Arial, sans-serif;margin:0}::placeholder{color: #1e1e1e; font-weight: 500;}a{text-decoration:none}.wrap-link{text-decoration:none;margin-left:15px}hr{height:1.3px;border:none;background-color:#3b3b3d;margin-top:20px;margin-bottom:20px}.header{background-color:#121212;border-bottom:solid 1.5px #252525;padding:1.8rem}.body{padding:1.4rem}.footer{padding:1rem}.font{font-size:1.3rem;color:#DADADC}.attr{background-color:#1C1C1E;border-top:solid 1.7px #3b3b3d;border-bottom:solid 1.7px #3b3b3d}.auto{margin:0 auto}.left{text-align:left}.center{text-align:center}.q{height:16px;margin:0;margin-left:10px;padding:0 5px;text-align:right;min-width:38px}.q.q-0:after{background-position-x:0}.q.q-1:after{background-position-x:-16px}.q.q-2:after{background-position-x:-32px}.q.q-3:after{background-position-x:-48px}.q.q-4:after{background-position-x:-64px}.q.l:before{background-position-x:-80px;padding-right:5px}.p-r{float:right}.top{margin-top:45px}.q:after,.q:before{content:'';width:16px;height:16px;display:inline-block;background-repeat:no-repeat;background-position:16px 0;background-image:url('data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAGAAAAAQCAMAAADeZIrLAAAAJFBMVEX///8AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAADHJj5lAAAAC3RSTlMAIjN3iJmqu8zd7vF8pzcAAABsSURBVHja7Y1BCsAwCASNSVo3/v+/BUEiXnIoXkoX5jAQMxTHzK9cVSnvDxwD8bFx8PhZ9q8FmghXBhqA1faxk92PsxvRc2CCCFdhQCbRkLoAQ3q/wWUBqG35ZxtVzW4Ed6LngPyBU2CobdIDQ5oPWI5nCUwAAAAASUVORK5CYII=');filter:invert(100%)}input{width: 100%; padding: 12px 20px; margin: 5px 0; display: inline-block; border: solid 1.7px #3b3b3d; border-radius: 8px; box-sizing: border-box;font-size:1.3rem;}button{ width: 100%; background-color:#1C1C1E; border-top: 0px; border-bottom: solid 1.7px #3b3b3d; border-left: 0px; border-right: 0px; color: #DADADC; font-size: 1.4rem; padding: 1.1rem; }</style></head><body><div class=\"auto\"><div class=\"header font center\">";
const char HEAD_END[] PROGMEM = "</div><div class=\"top\"></div><div class=\"body font attr left\">";
const char SSID_LINK[] PROGMEM = "<a href=\"connect?ssid=";
const char SSID_PASS[] PROGMEM = "&pass=";
const char SSID_NAME[] PROGMEM = "\" class=\"font\"><div class=\"wrap-link\">";
const char SSID_BARLOCK[] PROGMEM = "<div class=\"p-r q q-";
const char SSID_RSSI[] PROGMEM = "\"></div><div class=\"p-r\">";
const char SSID_END[] PROGMEM = "%</div></div></a>";
const char SPACE[] PROGMEM = "</div><div class=\"top\"></div>";
const char BUTTON_LINK[] PROGMEM = "<a href=\"";
const char BUTTON_NAME[] PROGMEM = "\"><div class=\"footer font attr center\">";
const char BUTTON_END[] PROGMEM = "</div></a>";
const char END[] PROGMEM = "</div></body></html>";

int value_led = 0;
unsigned long previousMillis = 0;

#define NYALA 1    // NYALA
#define MATI 0     // MATI

// Output Relay
#define RELAY1 16  // GPIO016
#define RELAY2 5   // GPIO05
#define RELAY3 4   // GPIO4

// MQTT Variabel
#define MQTT_RES "IMNDF_RES"
#define MQTT_RELAY "IMNDF_RELAY"
#define MQTT_STATUS "IMNDF_STATUS" 

// Variabel Access Point
const char *ssid = "IMNDF SMART HOME 1";
const char *password = "12345678";
const char* mqtt_server = "test.mosquitto.org";
const char* remote_host = "8.8.8.8";

// Variabel Indikator
int MERAH = 0; // GPIO0
int HIJAU = 2;    // GPIO02
int BIRU  = 15;    // GPIO15

// Alamat EEPROM
int s_addr = 3;
int p_addr = 4;
int start  = 5;

// Variabel SNFPRINT
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE	(50)
char msg[MSG_BUFFER_SIZE];

// Variabel Input Output
int dataRelay1, dataRelay2, dataRelay3;
int relayButton1, relayButton2, relayButton3;

// Variabel EEPROM WiFi
String ssid_memory, pass_memory;

int d_loops = 0;
int input_button = 0;

String mqtt_id = "imndfSmartHome";

WiFiClient espClient;
PubSubClient client(espClient);

const byte DNS_PORT = 53;
IPAddress apIP(192, 168, 1, 1);
DNSServer dnsServer;
ESP8266WebServer webServer(80);

EasyButton button1(14);
EasyButton button2(12);
EasyButton button3(13);

void setColor(int redValue, int greenValue, int blueValue) {
  analogWrite(MERAH, redValue);
  analogWrite(HIJAU, greenValue);
  analogWrite(BIRU, blueValue);
}

void eepromRELAY(){
    dataRelay1 = String(char(EEPROM.read(0))).toInt();
    dataRelay2 = String(char(EEPROM.read(1))).toInt();
    dataRelay3 = String(char(EEPROM.read(2))).toInt();
}

void outputRelay(int pub){
    eepromRELAY();
    Serial.println("\n# KONDISI RELAY");    
    if(dataRelay1 == 1){
      Serial.println("# 1. RELAY 1 NYALA");
      digitalWrite(RELAY1, NYALA);
    } else {
      Serial.println("# 1. RELAY 1 MATI"); 
      digitalWrite(RELAY1, MATI);
    } 

    if(dataRelay2 == 1){
      Serial.println("# 2. RELAY 2 NYALA");
      digitalWrite(RELAY2, NYALA);
    } else {
      Serial.println("# 2. RELAY 2 MATI"); 
      digitalWrite(RELAY2, MATI);
    } 

    if(dataRelay3 == 1){
      Serial.println("# 3. RELAY 3 NYALA\n");
      digitalWrite(RELAY3, NYALA);
    } else {
      Serial.println("# 3. RELAY 3 MATI\n"); 
      digitalWrite(RELAY3, MATI);
    } 

    if(pub == 1){
      delay(1000);
      snprintf (msg, MSG_BUFFER_SIZE, "%ld%ld%ld%ld%ld%ld%ld", 0, dataRelay1, dataRelay2, dataRelay3, dataRelay1, dataRelay2, dataRelay3);
      if (client.connected()) {
        client.publish(MQTT_RELAY, msg);
        setColor(0,0,0);
      }
    }
}

void btn1Pressed() {
    if(dataRelay1 == 0){
      relayButton1 = 1;
    }else{
      relayButton1 = 0;
    }
    
    snprintf (msg, MSG_BUFFER_SIZE, "%ld%ld%ld", relayButton1, dataRelay2, dataRelay3);
    for (int i = 0; i < 3; i++) {
      EEPROM.write(i , msg[i]);
    }

    EEPROM.commit();    
    outputRelay(1);
}

void btn2Pressed() {
    if(dataRelay2 == 0){
      relayButton2 = 1;
    }else{
      relayButton2 = 0;
    }

    snprintf (msg, MSG_BUFFER_SIZE, "%ld%ld%ld", dataRelay1, relayButton2, dataRelay3);
    for (int i = 0; i < 3; i++) {
      EEPROM.write(i , msg[i]);
    }

    EEPROM.commit();
    outputRelay(1);
}

void btn3Pressed() {
    if(dataRelay3 == 0){
      relayButton3 = 1;
    }else{
      relayButton3 = 0;
    }

    snprintf (msg, MSG_BUFFER_SIZE, "%ld%ld%ld", dataRelay1, dataRelay2, relayButton3);
    for (int i = 0; i < 3; i++){
      EEPROM.write(i , msg[i]);
    }

    EEPROM.commit();
    outputRelay(1);
}  

void button(){
    button1.read();
    button2.read();
    button3.read();
}

void eepromSSID(){
  int length_ssid = EEPROM.read(s_addr) + start;
  int length_pass = EEPROM.read(p_addr) + start;
  for (int i = start; i < length_ssid; i++){
    ssid_memory = ssid_memory + char(EEPROM.read(i));
  }

  for (int i = length_ssid; i < length_ssid - start + length_pass; i++){
    pass_memory = pass_memory + char(EEPROM.read(i));
  }
}

void Save(){
   String st;
   String s_ssid = webServer.arg("ssid");
   String s_pass = webServer.arg("pass");
   int delays = 0;

   if (webServer.hasArg("ssid") && webServer.hasArg("pass")){  
        st += FPSTR(TITLE);
        st += "Konfigurasi Berhasil";
        st += FPSTR(HEAD); 
        st += "Konfigurasi Berhasil";
        st += FPSTR(HEAD_END);
    
        if(s_pass != "") 
        {
          EEPROM.write(s_addr, s_ssid.length());
          EEPROM.write(p_addr, s_pass.length());
          
          for (int i = 0; i < s_ssid.length() + 1; i++) 
          {
            EEPROM.write(start + i , s_ssid[i]);
          }

          for (int i = 0; i < s_pass.length() + 1; i++) 
          {
            EEPROM.write(start + s_ssid.length() + i , s_pass[i]);
          }

          EEPROM.commit();
          
          st += "<div style=\"color: blue; text-align:center;\">Tunggu hingga Wi-Fi <b>" + webServer.arg("ssid") + "</b> terhubung, dengan ciri-ciri indikator pada perangkat berwarna hijau. Jika gagal, silahkan konfigurasi ulang</div>";
          st += FPSTR(SPACE);
          st += FPSTR(BUTTON_LINK);
          st += "/";
          st += FPSTR(BUTTON_NAME);
          st += "Konfigurasi Ulang";
          delays = 10000;
          setColor(255,0,255);
        }else{
          st += "<div style=\"color: red; text-align:center;\">Kata Sandi belum terisi</div>";
          st += FPSTR(SPACE);
          st += FPSTR(BUTTON_LINK);
          st += "/connect?ssid=";
          st += String(webServer.arg("ssid"));
          st += "&pass=";
          st += String(webServer.arg("pass"));
          st += FPSTR(BUTTON_NAME);
          st += "Masukan Password";
        }
        st += FPSTR(BUTTON_END);
        st += FPSTR(END);
   }

   webServer.send(200,"text/html", st);

   if(delays == 10000){
     delay(delays);
     delays = 0;
     setColor(255,0,255);
     setup();
   }
}

void Process(){
  String st;
  if (webServer.hasArg("ssid") && webServer.hasArg("pass")){  
    st += FPSTR(TITLE);
    st += "Menghubungkan ";
    st += String(webServer.arg("ssid"));
    st += FPSTR(HEAD); 
    st += "Otorisasi ";
    st += String(webServer.arg("ssid"));
    st += FPSTR(HEAD_END);
    if(String(webServer.arg("pass")).toInt() != 0)
    {
      st += "<form action=\"save\" method=\"get\"><input type=\"hidden\" name=\"ssid\" value=\"";
      st += String(webServer.arg("ssid"));
      st += "\"><input type=\"text\" name=\"pass\" placeholder=\"Kata Sandi\" maxlength=\"64\" required></div><button>Simpan</button></form>";
    }else{
      st += "<div style=\"color: red; text-align:center;\">Gunakan Wi-Fi yang memiliki otorisasi!</div>";
    }
    st += FPSTR(SPACE);
    st += FPSTR(BUTTON_LINK);
    st += "/";
    st += FPSTR(BUTTON_NAME);
    st += "Kembali";
    st += FPSTR(BUTTON_END);
    st += FPSTR(END);
  }
  webServer.send(200,"text/html", st);
}

int dBmtoPercentage(int dBm){
  int quality;
  if( dBm <= -100){ quality = 0; } 
  else if( dBm >= -50) { quality = 100; }
  else{ quality = 2 * (dBm + 100); } 
  return quality;
}

int percToBar(int perc){
  return perc / 25;
}

void ScanPage() {
    String st;
    int sNet = WiFi.scanNetworks();
    st += FPSTR(TITLE);
    st += "Pengaturan Wi-Fi";
    st += FPSTR(HEAD); 
    st += "Pengaturan Wi-Fi";
    st += FPSTR(HEAD_END);
    if(sNet == 0){ st += "<center>Tidak Ada Wi-Fi</center>"; }
    for (int i = 0; i < sNet; ++i)
    {
      st += FPSTR(SSID_LINK);
      st += WiFi.SSID(i);
      st += FPSTR(SSID_PASS);
      st += (WiFi.encryptionType(i) == ENC_TYPE_NONE) ? "0" : "1";
      st += FPSTR(SSID_NAME);
      st += WiFi.SSID(i);
      st += FPSTR(SSID_BARLOCK);
      st += percToBar(dBmtoPercentage(WiFi.RSSI(i)));
      st += (WiFi.encryptionType(i) == ENC_TYPE_NONE) ? "" : " l";
      st += FPSTR(SSID_RSSI);
      st += dBmtoPercentage(WiFi.RSSI(i));
      st += FPSTR(SSID_END);
      if(i < sNet - 1){ st += "<hr>"; }
    }
    st += FPSTR(SPACE);
    st += FPSTR(BUTTON_LINK);
    st += "/";
    st += FPSTR(BUTTON_NAME);
    st += "Pindai";
    st += FPSTR(BUTTON_END);
    st += FPSTR(END);
    webServer.send( 200 , "text/html", st);
}


void AccessPoint(){
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  WiFi.softAP(ssid, password);
  
  dnsServer.setTTL(60);
  dnsServer.setErrorReplyCode(DNSReplyCode::ServerFailure);
  dnsServer.start(DNS_PORT, "*", apIP);
        
  webServer.onNotFound(ScanPage);
  webServer.on("/save", Save);
  webServer.on("/connect", Process);
  webServer.begin();
}

void reboot(){
  d_loops = 0;
  setup();
}

void setup_wifi() {
  ssid_memory = "";
  pass_memory = "";
  eepromSSID();

  int loops = 0;
  while(d_loops == 5){
    loops++;
    if(loops == 1){
      AccessPoint();
    }
  
    setColor(255,0,0);
    button(); 
    dnsServer.processNextRequest();
    webServer.handleClient();
  }

  int counts = 0;
  while(ssid_memory == NULL & pass_memory == NULL){
    counts++;
    if(counts == 1){
      AccessPoint();
    }
  
    setColor(255,255,255);
    button(); 
    dnsServer.processNextRequest();
    webServer.handleClient();
  }
  
  delay(10);
  Serial.println();
  WiFi.mode(WIFI_STA);
  Serial.print("Menghubungkan ke ");
  Serial.print(ssid_memory);
  // Serial.println(pass_memory);
  Serial.println("...");

  WiFi.begin(ssid_memory.c_str(), pass_memory.c_str());

  int count = 0;
  while (WiFi.status() != WL_CONNECTED) {

    count++;
    if(count == 1){
      AccessPoint();
    }

    button(); 
    dnsServer.processNextRequest();
    webServer.handleClient();

    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= 300) {
      previousMillis = currentMillis;
      if (value_led == 0) {
        value_led = 255;
      } else {
        value_led = 0;
      }
      setColor(0,0,value_led);
    }
  }

  setColor(0,0,255);
  WiFi.mode(WIFI_STA);
  Serial.println();
  Serial.print(ssid_memory);
  Serial.println(" Terhubung");
  Serial.print("Alamat IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();
}

void callback(char* topic, byte* payload, unsigned int length) {
    int intopic;
    if(String(topic) == MQTT_RELAY){
      for(int i = 1; i < 4; i++){
        EEPROM.write(i - 1, (char)payload[i]);
      }
      EEPROM.commit();
      eepromRELAY();
      intopic = String(char(payload[0])).toInt();
    }

    if(intopic == 1){
        outputRelay(0);  
        snprintf (msg, MSG_BUFFER_SIZE, "%ld%ld%ld%ld%ld%ld%ld", 0, dataRelay1, dataRelay2, dataRelay3, dataRelay1, dataRelay2, dataRelay3);
        client.publish(MQTT_RELAY, msg);
    }  
}

void reconnect() {
  delay(1000);
  while (!client.connected()) {
    Serial.println("[#] Memeriksa konektivitas...");
    if(client.connect(mqtt_id.c_str())) {
      WiFi.mode(WIFI_STA);
      client.subscribe(MQTT_RELAY);
      snprintf (msg, MSG_BUFFER_SIZE, "%ld%ld%ld%ld%ld%ld%ld", 0, dataRelay1, dataRelay2, dataRelay3, dataRelay1, dataRelay2, dataRelay3);
      client.publish(MQTT_RELAY, msg);
      Serial.println("[#] UPDATE DATA SETELAH OFFLINE KE MQTT");
      Serial.println();
    }else{
      d_loops++;
      setColor(255,0,0);
      setup();
    }
  } 
}

void setup() {
  Serial.begin(115200);
  EEPROM.begin(512);
  
  button1.begin();
  button2.begin();
  button3.begin();
  
  button1.onPressed(btn1Pressed);
  button2.onPressed(btn2Pressed);
  button3.onPressed(btn3Pressed);

  button1.onPressedFor(2000, reboot);
  button2.onPressedFor(2000, reboot);
  button3.onPressedFor(2000, reboot);

  eepromRELAY();

  pinMode(MERAH, OUTPUT);
  pinMode(HIJAU, OUTPUT);
  pinMode(BIRU, OUTPUT);

  pinMode(RELAY1, OUTPUT);
  pinMode(RELAY2, OUTPUT);
  pinMode(RELAY3, OUTPUT);

  digitalWrite(RELAY1, dataRelay1);
  digitalWrite(RELAY2, dataRelay2);
  digitalWrite(RELAY3, dataRelay3);

  setup_wifi();

  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void loop() {
    if (!client.connected()) {
      reconnect();
    }
    


    client.loop();
    button();
    setColor(0,255,0);

    unsigned long now = millis();
    if (now - lastMsg > 2000) {
      lastMsg = now;
      snprintf (msg, MSG_BUFFER_SIZE, "%ld", 1);
      Serial.println("[#] Terhubung");
      client.publish(MQTT_STATUS, msg); 
    }
}