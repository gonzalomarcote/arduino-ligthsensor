/* Sketch - LigthSensor Poject - V. 1.0
Author: Gonzo - gonzalomarcote@gmail.com
*/

// Libraries to connect to SPI, Wifi, WiFiUDP, Time
#include <SPI.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <Time.h>
#include <Timezone.h>

// Libraries for Chronodot
#include <DS1307RTC.h>
#include <Wire.h>

// Wifi setup
char ssid[] = "casadebertayguta"; // Wifi SSID name
char pass[] = "1019gon$4";        // Wifi password
int status = WL_IDLE_STATUS;      // Wifi status

// Initialize the Wifi client library
WiFiClient client;

// Network setup
IPAddress ip(192, 168, 1, 114);
IPAddress dns(192, 168, 1, 17);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress checkIp;			    // Variable to store IP address
IPAddress checkGateway;			// Variable to store IP gateway

// Led and Ultrasonic HC-SR04 setup
const int leds = 6;      // Set Led in analogic PIN 6
const int echoPin = 8;   // Set Echo in PIN 8
const int trigPin = 9;   // Set Trigger in PIN 9
long duration;           // Set the duration of Ping for go and return and thus calculate the distance
long cm;                 // Distance in cm
int i = 0;               // Blink count i
int x = 0;               // Blink count x

// NTP setup
unsigned int localPort = 8888;		    // local port to listen for UDP packets
char timeServer[] = "192.168.1.2";	  // ntp.casadebertayguta.org NTP server
const int NTP_PACKET_SIZE = 48;		    // NTP time stamp is in the first 48 bytes of the message
byte packetBuffer[ NTP_PACKET_SIZE];	// buffer to hold incoming and outgoing packets
WiFiUDP Udp;				                  // A UDP instance to let us send and receive packets over UDP

// Piwik server address
//char server[] = "lightsensorws.gonzalomarcote.com";
//IPAddress server(192,168,1,16);

void setup() {
  
  // Initialize serial
  Serial.begin(9600);
  
  // Welcome message
  Serial.println("== Welcome to Ligthsensor V. 1.0 ==");
  Serial.println("");

  // PIN Setup for ultrasonic HC-SR04
  pinMode(trigPin, OUTPUT); // Definimos el pin para el trigger
  pinMode(echoPin, INPUT);  // Definimos el pin para el echo
  pinMode(leds, OUTPUT);    // Definimos que leds es un disp de salida

  // We connect to Wifi only on Arduino boot
  // Wifi setup
  WiFi.config(ip, dns, gateway, subnet);    // Configure network parameters
  Serial.println("Connecting to Wifi ...");
  status = WiFi.begin(ssid, pass);          // Trying to connect to wifi using WPA2 encryption

  // Failed wifi connection
  if ( status != WL_CONNECTED) { 
    Serial.println("ERROR: Failed to connect to Wifi!");
    Serial.println("");
    while(true);
  }
  
  // Succesfully connected:
  else {
    Serial.println("Connected to Wifi network!");
    // Print IP & Gateway addresses:
    checkIp = WiFi.localIP();
    checkGateway = WiFi.gatewayIP();
    //Serial.print("IP address is: ");
    //Serial.println(checkIp);
    //Serial.print("Gateway address is: ");
    //Serial.println(checkGateway);
    
    // If connected to the Wifi, we initializate Udp on specified port
    //Serial.print("Initializing UDP on port: ");
    //Serial.println(localPort);
    //Serial.println("");
    Udp.begin(localPort);
  }
    
  // After connecting to wifi we check time with NTP protocol
  Serial.println("Connecting to NTP server ...");
  setSyncProvider(getNtpTime);

  if (timeStatus() == 2) {
    Serial.println("Local time updated with NTP!");
    //Serial.print("Update status is: ");
    //Serial.println(timeStatus());
  } else {
    Serial.println("ERROR. Local time not updated with NTP");
    Serial.print("Update status is: ");
    Serial.println(timeStatus());
  }

  /*
  Serial.print("Local Date and Time updated with NTP is: ");
  time_t t = now();
  Serial.print(day(t));
  Serial.print(+ "/") ;
  Serial.print(month(t));
  Serial.print(+ "/") ;
  Serial.print(year(t)); 
  Serial.print( " ") ;
  Serial.print(hour(t));  
  Serial.print(+ ":") ;
  Serial.print(minute(t));
  Serial.print(":") ;
  Serial.println(second(t));
  */


  // We update the current Time and Date with DST
  time_t central, utc;
  TimeChangeRule esCEST = {"CEST", Last, Sun, Mar, 2, +120};  // UTC + 2 hours
  TimeChangeRule esCET = {"CET", Last, Sun, Oct, 3, +60};     // UTC + 1 hours
  Timezone esCentral(esCEST, esCET);
  utc = now();                                                // Current time from the Time Library
  central = esCentral.toLocal(utc);

  /*
  Serial.print("Local Date and Time updated with DST is: ");
  Serial.print(day(central));
  Serial.print(+ "/") ;
  Serial.print(month(central));
  Serial.print(+ "/") ;
  Serial.print(year(central)); 
  Serial.print( " ") ;
  Serial.print(hour(central));
  Serial.print(+ ":") ;
  Serial.print(minute(central));
  Serial.print(":") ;
  Serial.println(second(central));
  */

  // We set Chronodot with NTP & DST updated time only at boot
  Serial.println("Updating RTC Chronodot with updated with NTP & DST date...");
  if (RTC.set(esCentral.toLocal(utc))) {
    Serial.println("RTC Chronodot configured!");
  } else {
    Serial.println("WARNING. RTC Chronodot could not be configured");
  }

  // Initializate wire to write to Chronodot alarms
  Wire.begin();

  // Clear Chronodot alarms
  // Reset the register pointer to the 0F register
  Wire.beginTransmission(0x68);
  Wire.write(0x0F);
  Wire.endTransmission();
  Wire.requestFrom(0x68, 1);  // request 1 byte (Control/Status Register)
  //b0F = (Wire.read() & 0b11111110);

  Wire.beginTransmission(0x68);
  Wire.write(0x0F);        //control register 0F
  Wire.write((Wire.read() & 0b11111110)); 
  Wire.endTransmission();

  /*
  // Clear Chronodot alarms
  Wire.beginTransmission(0x68);
  Wire.write(0x0F); //select status registers,  
  Wire.write(0b00000000);//reset alarm
  Wire.endTransmission();
  */
  
  // Setting INTCN and A1IE to 1. A2IE to 0
  Wire.beginTransmission(0x68);
  Wire.write(0x0E);        // Control register 0E
  Wire.write(0b00000101);  // INTCN -> 1, A2IE -> 0 (disable), and A1IE set to 1 (enable)
  // Wire.write(0b00000000);  // INTCN -> 1, A2IE -> 0 (disable), and A1IE set to 1 (enable)
  Wire.endTransmission();
  Serial.println("Chronodot Alarm1 activated");

  // Set the time alarm
  Wire.beginTransmission(0x68);
  Wire.write( 0x07 );           //  start at the Alarm1 Seconds register
  Wire.write( 0x00 | 0x00 );    //  BCD 56 + bit7=0
  Wire.write( 0x00 | 0x00 );    //  BCD 34 + bit7=0
  Wire.write( 0x23 | 0x80 );    //  BCD 12 + bit7=1
  Wire.endTransmission();
  Serial.println("Chronodot Alarm1 set to 23:00:0");
   
  // End setup
  Serial.println("");
  Serial.println("===========================");
  Serial.println("");
}


void loop() {

  // Check Chronodot time
  // We read Chrnodot Time updated with NTP
  tmElements_t tm;

  if (RTC.read(tm)) {
    Serial.print("Ok, Chronodot Time is: ");
    print2digits(tm.Hour);
    Serial.write(':');
    print2digits(tm.Minute);
    Serial.write(':');
    print2digits(tm.Second);
    Serial.print(" - ");
    Serial.print(tm.Day);
    Serial.write('/');
    Serial.print(tm.Month);
    Serial.write('/');
    Serial.print(tmYearToCalendar(tm.Year));
    Serial.println();
  } else {
    if (RTC.chipPresent()) {
      Serial.println("The DS1307 is stopped.  Please run the SetTime");
      Serial.println();
    } else {
      Serial.println("DS1307 read error! Check circuitry");
      Serial.println();
    }
    delay(9000);
  }
  delay(1000);
  
  // Check distance with ultrasonic sensor
  digitalWrite(trigPin, LOW); 
  delayMicroseconds(2); 
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(5); 
  digitalWrite(trigPin, LOW);
  duration = pulseIn(echoPin, HIGH);
 
  cm = microsecondsToCentimeters(duration);
 
  //Serial.print("The ultrasonic in milisecs are: ");
  //Serial.println(duration);
  Serial.print("Distance is: ");
  Serial.print(cm);
  Serial.println(" cm");
  Serial.println("");
 
  // If the distance is lees than 50cm we turn on the lights leds
  if (cm < 50) {
    Serial.println("Presence detected! Light on");
    analogWrite(leds, 254);  // We turn on the light
    delay(60000);            // for 60 secs. Enough for sensor
    // We give a notice with 3 blinking lights that the minute is going over and the light is turning off
      do {
        for (x = 255; x > 0; x--) {
          analogWrite(leds, x);
          delay(10);
        }
        for (x = 0; x < 255; x++) {
          analogWrite(leds, x);
          delay(10);
        }
        i = ++i;
      } while (i < 3);

      x=0;
      i=0;

      /*
      // Send http request to piwik in lightsensorws.gonzalomarcote.com
      // First close any connection before send a new request.
      // This will free the socket on the WiFi shield
      client.stop();

      // if there's a successful connection:
      if (client.connect(server, 80)) {
        Serial.println("HTTP request");
        // send the HTTP PUT request:
        client.println("GET /index.html HTTP/1.1");
        client.println("Host: lightsensorws.gonzalomarcote.com");
        client.println("User-Agent: Mozilla/5.0 Arduino");
        //client.println("User-Agent: Arduino/1.1");
        client.println("Connection: close");
        client.println();
      } else {
        // if you couldn't make a connection:
        Serial.println("ERROR. HTTP request failed");
      }
      */

      delay(1000);

  } else {
    analogWrite(leds, 0);    // Keep light off
  }

  // Clear Chronodot alarms
  // Reset the register pointer to the 0F register
  Wire.beginTransmission(0x68);
  Wire.write(0x0F);
  Wire.endTransmission();
  Wire.requestFrom(0x68, 1);  // request 1 byte (Control/Status Register)
  //b0F = (Wire.read() & 0b11111110);

  Wire.beginTransmission(0x68);
  Wire.write(0x0F);        //control register 0F
  Wire.write((Wire.read() & 0b11111110)); 
  Wire.endTransmission();

  /*
  // Clear Chronodot alarms
  Wire.beginTransmission(0x68);
  Wire.write(0x0F); //select status registers,  
  Wire.write(0b00000000);//reset alarm
  Wire.endTransmission();
  */

  Serial.println("");
  delay(1000);
  
}


// --------- Functions ------------ //


// Function to calculate distance for the ultrasonic
long microsecondsToCentimeters(long microseconds) {
  // La velocidad del sonido a 20º de temperatura es 340 m/s o
  // 29 microsegundos por centrimetro.
  // La señal tiene que ir y volver por lo que la distancia a 
  // la que se encuentra el objeto es la mitad de la recorrida.
  return microseconds / 29 / 2 ;
}


// Function to send an NTP request to the time server at the given address
unsigned long sendNTPpacket(char* address) {
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;            // Stratum, or type of clock
  packetBuffer[2] = 6;            // Polling Interval
  packetBuffer[3] = 0xEC;         // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12] = 49;
  packetBuffer[13] = 0x4E;
  packetBuffer[14] = 49;
  packetBuffer[15] = 52;

  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  Udp.beginPacket(address, 123); //NTP requests are to port 123
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
}


// Function to get the NTP time in Unix time
unsigned long getNtpTime() {
  // After connecting to wifi we update date and time with NTP protocol
  sendNTPpacket(timeServer); // send an NTP packet to a time server

  // wait to see if a reply is available
  delay(1000);
  if (Udp.parsePacket()) {
    // We've received a packet, read the data from it
    Udp.read(packetBuffer, NTP_PACKET_SIZE); // read the packet into the buffer

    // the timestamp starts at byte 40 of the received packet and is four bytes,
    // or two words, long. First, extract the two words

    unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
    unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
    // combine the four bytes (two words) into a long integer
    // this is NTP time (seconds since Jan 1 1900)
    unsigned long secsSince1900 = highWord << 16 | lowWord;

    // now convert NTP time into everyday time
    // Unix time starts on Jan 1 1970. In seconds, that's 2208988800
    const unsigned long seventyYears = 2208988800UL;
    // subtract seventy years
    unsigned long epoch = secsSince1900 - seventyYears;
    Serial.println("Time synced with NTP server!");
    Serial.println("");
    Serial.println("Updating local Time with NTP...");
    // print Unix time
    return epoch;
    
  }
  Serial.println("ERROR. NTP update failed");
  return 0; // return 0 if unable to get the time
}


// Function to add one 0 to correct display time
void print2digits(int number) {
  if (number >= 0 && number < 10) {
    Serial.write('0');
  }
  Serial.print(number);
}
