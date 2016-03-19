#include <Wire.h>
#include <SPI.h>
#include <Adafruit_WINC1500.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_NeoPixel.h>

// NeoPixels
#define PIN 45
#define NUM_PIXELS 24 // (3 x 8-strips)
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_PIXELS, PIN, NEO_GRB + NEO_KHZ800);

#define OLED_RESET 4
Adafruit_SSD1306 display(OLED_RESET);

// SPI Mega2560<->ATWINC1500
#define WINC_CS   53
#define WINC_IRQ  7
#define WINC_RST  6

#define AUTHORIZATION_HEADER "Authorization: SharedAccessSignature sr=xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
#define HUB_POLLING_INTERVAL 5000L

// Setup the WINC1500 connection with the pins above and the default hardware SPI.
Adafruit_WINC1500 WiFi(WINC_CS, WINC_IRQ, WINC_RST);
int status = WL_IDLE_STATUS;

char ssid[] = "MSFTOPEN";   //  your network SSID (name)
// char pass[] = "ffffff";   // your network password (use for WPA, or use as key for WEP)

// Azure IoT Hub FQDN
char server[] = "HeresTheThing.azure-devices.net";

// Instantiate SSL client
Adafruit_WINC1500SSLClient client;

unsigned long lastConnectionTime = 0;            
const unsigned long pollingInterval = HUB_POLLING_INTERVAL;

void setup() {
  Serial.begin(9600);

  // NeoPixels
  strip.begin();
  clearStrip();
  strip.setBrightness(20);

  // Join i2c bus as MASTER
  Wire.begin();

  // Init OLED
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3C (for the 128x32)
  display.clearDisplay();
  display.display();
  

  // check for the presence of the shield:
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("ATWINC1500 not present");
    // don't continue:
    while (true);
  }
  
  // attempt to connect to Wifi network:
  while (status != WL_CONNECTED) {
    Serial.println();
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    status = WiFi.begin(ssid);
    // status = WiFi.begin(ssid, pass);

    // wait 7 seconds for connection:
    delay(7000);
    Serial.println("Waiting 7 sec to associate with AP...");
  }
  Serial.println("Connected to wifi");
  printWifiStatus();
  
  display.setRotation(2);
  display.setTextColor(WHITE);
      
} //end setup


void loop() {
  String response = "";

  // if there are incoming bytes available
  // from the server, read them and print them:
  while (client.available()) {
    char c = client.read();
    response.concat(c);
  }
     
  if (!response.equals("")) {
    Serial.println("\n---- start response ----");
    Serial.print(response);
    Serial.println("\n---- end response ----");
      
    if (response.startsWith("HTTP/1.1 200 OK")) {
      // write 200 OK to OLED
      display.clearDisplay();
      display.setCursor(0,0);
      display.setTextSize(3);
      display.print("200 OK");
      // Sending response to Uno on i2c bus
      display.display();
      Wire.beginTransmission(8);
      //Wire.write("200 OK BOSS.\n");
      char msg[100];
      strncpy(msg, strip_headers(response), 100);
      Serial.println();
      //Serial.println("==================");
      //Serial.println(msg);
      //Serial.println("==================");
      Wire.write(msg);
      //Wire.write("TEST");
      Wire.endTransmission();
    }
    else if (response.startsWith("HTTP/1.1 204")) {
      // write 200 OK to OLED
      display.clearDisplay();
      display.setCursor(0,0);
      display.setTextSize(2);
      display.print("204 NoContent");
      // Sending response to Uno on i2c bus
      display.display();
      //Wire.beginTransmission(8);
      //Wire.write("204 No Content");
      //Wire.endTransmission();
    }
    else{
      // write FAIL to OLED
      display.clearDisplay();
      display.setCursor(0,0);
      display.setTextSize(3);
      display.print("FAIL");
      display.display();
    }
  }
  
  // polling..if pollingInterval has passed
  if (millis() - lastConnectionTime > pollingInterval) {
    // if the server's disconnected, stop the client:
    if (!client.connected()) {
      Serial.println();
      Serial.println("Disconnecting from server.");
      client.stop();
    }
    // Clear OLED
    display.clearDisplay();
    display.setCursor(0,0);
    display.setTextSize(2);
    display.print("Connecting...");
    display.display();
    Serial.println("inside if millis: calling httpRequest from loop()");
    //Wire.beginTransmission(8);
    //Wire.write("Connecting to IoT Hub...");
    //Wire.endTransmission();
    for(byte j=0; j<10; j+=7) {
      knightRider(1, 20, 2, colorWheel(j)); // Cycles, Speed, Width, RGB Color
    }
    httpRequest();
  }
}


void printWifiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your WiFi shield's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}

void httpRequest() {
  client.stop();
  Serial.println("Start of GET request");
  Serial.println("\nStarting connection to server...");
  // if you get a connection, report back via serial:
  if (client.connect(server, 443)) {
    Serial.println("connected to server");
    // Make a HTTP request:
    client.println("GET /devices/Mega/messages/devicebound?api-version=2016-02-03 HTTP/1.1");
    client.println("Host: heresthething.azure-devices.net");
    client.println(AUTHORIZATION_HEADER);
    client.println("User-Agent: Atmel ATWINC1500");
    client.println("Connection: close");
    client.println();
  }
  // note the time that the connection was made:
  lastConnectionTime = millis();
}

char *strip_headers(String response) {
  // length of ETag value (GUID) == 36
  int etag_index = response.indexOf("ETag");
  String etag_id = response.substring(etag_index+7, etag_index+43);
  // RFC2616 - https://www.w3.org/Protocols/rfc2616/rfc2616-sec2.html#sec2.2
  // CR = <US-ASCII CR, carriage return (13)>
  // LF = <US-ASCII LF, linefeed (10)>
  // HTTP/1.1 defines the sequence CR LF as the end-of-line marker
  // for all protocol elements except the entity-body

  // Well apparently it's two times \r\n
  int endofheaders_index = response.indexOf("\r\n\r\n");
  String msg = response.substring(endofheaders_index+4, response.length());

  Serial.println();
  Serial.println("=======================================");
  Serial.print("ETag index="); Serial.println(response.indexOf("ETag"));
  Serial.print("etagid="); Serial.print(etag_id);
  Serial.println();
  Serial.print("indexofrnrn="); Serial.println(endofheaders_index);
  //Serial.print("msg="); Serial.println(msg);
  Serial.println("=======================================");
  
  //char buf[msg.length()+2];
  char buf[100];
  msg.toCharArray(buf, msg.length());
  //msg = "yo";

  //complete (delete) msg
  delete_msg(etag_id);
  return buf;
}

String delete_msg(String etag_id) {
  client.stop();
  Serial.println("DELETING MESSAGE...");
  if (client.connect(server, 443)) {
    Serial.println("connected to server");
    // Make a HTTP request:
    client.print("DELETE /devices/Mega/messages/devicebound/");
    client.print(etag_id);
    client.println("?api-version=2016-02-03 HTTP/1.1");
    client.println("User-Agent: Atmel ATWINC1500");
    client.println("Host: heresthething.azure-devices.net");
    //client.println("Accept: */*");
    client.println(AUTHORIZATION_HEADER);
    //client.println("Transfer-Encoding: Chunked");
    client.println("Connection: close");
    client.println();
  }
  String response = "";

  delay(2000);
  while (client.available()) {
      char c = client.read();
      response.concat(c);
  }
  
  //delay(2000);
  //Serial.println("RESPONSE FROM DELETE REQUEST:");
  //Serial.println(response);
  int iof = response.indexOf("HTTP");
  String statuscode = response.substring(response.indexOf("HTTP"), 20);
  //Serial.println("statuscode:");
  //Serial.println(iof);
  Serial.println(statuscode);
  return statuscode;
}

void clearStrip() {
  for( int i = 0; i<NUM_PIXELS; i++){
    strip.setPixelColor(i, 0x000000); strip.show();
  }
}

void knightRider(uint16_t cycles, uint16_t speed, uint8_t width, uint32_t color) {
  // Cycles - one cycle is scanning through all pixels left then right (or right then left)
  // Speed - how fast one cycle is (32 with 16 pixels is default KnightRider speed)
  // Width - how wide the trail effect is on the fading out LEDs.  The original display used
  //         light bulbs, so they have a persistance when turning off.  This creates a trail.
  //         Effective range is 2 - 8, 4 is default for 16 pixels.  Play with this.
  // Color - 32-bit packed RGB color value.  All pixels will be this color.
  // knightRider(cycles, speed, width, color);
  uint32_t old_val[NUM_PIXELS]; // up to 256 lights!
  // Larson time baby!
  for(int i = 0; i < cycles; i++){
    for (int count = 1; count<NUM_PIXELS; count++) {
      strip.setPixelColor(count, color);
      old_val[count] = color;
      for(int x = count; x>0; x--) {
        old_val[x-1] = dimColor(old_val[x-1], width);
        strip.setPixelColor(x-1, old_val[x-1]); 
      }
      strip.show();
      delay(speed);
    }
    for (int count = NUM_PIXELS-1; count>=0; count--) {
      strip.setPixelColor(count, color);
      old_val[count] = color;
      for(int x = count; x<=NUM_PIXELS ;x++) {
        old_val[x-1] = dimColor(old_val[x-1], width);
        strip.setPixelColor(x+1, old_val[x+1]);
      }
      strip.show();
      delay(speed);
    }
  }
}

uint32_t colorWheel(byte WheelPos) {
  // Using a counter and for() loop, input a value 0 to 251 to get a color value.
  // The colors transition like: red - org - ylw - grn - cyn - blue - vio - mag - back to red.
  // Entering 255 will give you white, if you need it.
  byte state = WheelPos / 21;
  switch(state) {
    case 0: return strip.Color(255, 0, 255 - ((((WheelPos % 21) + 1) * 6) + 127)); break;
    case 1: return strip.Color(255, ((WheelPos % 21) + 1) * 6, 0); break;
    case 2: return strip.Color(255, (((WheelPos % 21) + 1) * 6) + 127, 0); break;
    case 3: return strip.Color(255 - (((WheelPos % 21) + 1) * 6), 255, 0); break;
    case 4: return strip.Color(255 - (((WheelPos % 21) + 1) * 6) + 127, 255, 0); break;
    case 5: return strip.Color(0, 255, ((WheelPos % 21) + 1) * 6); break;
    case 6: return strip.Color(0, 255, (((WheelPos % 21) + 1) * 6) + 127); break;
    case 7: return strip.Color(0, 255 - (((WheelPos % 21) + 1) * 6), 255); break;
    case 8: return strip.Color(0, 255 - ((((WheelPos % 21) + 1) * 6) + 127), 255); break;
    case 9: return strip.Color(((WheelPos % 21) + 1) * 6, 0, 255); break;
    case 10: return strip.Color((((WheelPos % 21) + 1) * 6) + 127, 0, 255); break;
    case 11: return strip.Color(255, 0, 255 - (((WheelPos % 21) + 1) * 6)); break;
    default: return strip.Color(0, 0, 0); break;
  }
}

uint32_t dimColor(uint32_t color, uint8_t width) {
   return (((color&0xFF0000)/width)&0xFF0000) + (((color&0x00FF00)/width)&0x00FF00) + (((color&0x0000FF)/width)&0x0000FF);
}

