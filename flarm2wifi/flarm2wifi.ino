/* send flarm data over udp to e,g, tophat
 * lots of hard coded stuff - ssid credentials.
 * mean to run using an external router - i currently use a hootoo which integrates a sizeable battery.
 * meant to run on an esp8266, obviously
 * as of jan 2nd 2016
 * 
 * compiled last May 4th 2018 , using arduino 1.6.8 board = node mcu 1.0 (ESP-12E)
 * flow to flarm is good, pretty much confirming we're on port 2 of the flarm (since 1 was configured as 19200, while 2 was set to 57600 baud)
 */

#include <ESP8266WiFi.h> 
#include <WiFiUdp.h>
//#include <ESP8266WebServer.h>

#define ARRLEN(a) (sizeof(a)/sizeof((a)[0]))

/* Set these to your desired credentials. */
const char *ssid = "TripMateSith-0C78";
const char *password = "11111111";

// flight computer
const char* host = "10.10.10.2"; // cellphone was 2, kobo 4,3
const int port = 4352; // flarm on 4352, vario on 4353

WiFiUDP udp;


void send_packet(char *packetBuffer, int j) {
    udp.beginPacket(host, port);
    udp.write(packetBuffer, j);
    udp.endPacket();
    return;
}

char packetBuffer[128];
char wifi2serialBuffer [256];

void setup() {
        delay(1000);
        Serial.begin(57600); // match flarm setup ; seems we're connected on port 2
        /* You can remove the password parameter if you want the AP to be open. */
        
        WiFi.begin(ssid, password);
        int i =0;
        while (WiFi.status() != WL_CONNECTED) {
            delay(500);
            Serial.println(i);
            i++;
        }
        Serial.println("");
        
        Serial.println("WiFi connected");
        Serial.println("IP address: ");
        Serial.println(WiFi.localIP());
        
        //udp
        udp.begin(port);
        Serial.print("Local port: ");
        Serial.println(udp.localPort());
        
        for (int i =0; i < 128; i++) {
            packetBuffer[i] = 0;
        }
}

byte j =0 ;
void loop() {
    int incomingByte, packetSize;
	IPAddress remoteIp;
	
    if (Serial.available() > 0) {
            incomingByte = Serial.read();
//             Serial.println(incomingByte, DEC);
            packetBuffer[j++] = incomingByte;
            if ((j >0 && incomingByte =='\n') || j == 128) {
                if (j < ARRLEN(packetBuffer)) {
                    packetBuffer[j+1] == 0;
                    //Serial.println(packetBuffer);
                }
                send_packet(packetBuffer, j);
                j = 0;
            }
    }
	//handle incoming data
    packetSize = udp.parsePacket();
	if (packetSize > 0) {
		remoteIp = udp.remoteIP(); // store the ip of the remote device
		udp.read(wifi2serialBuffer, ARRLEN(wifi2serialBuffer));
		// now send to UART:
		Serial.write(wifi2serialBuffer, packetSize);
	}
  
}
