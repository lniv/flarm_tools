/* send flarm data over udp to e,g, tophat
 * lots of hard coded stuff - ssid credentials.
 * mean to run using an external router - i currently use a hootoo which integrates a sizeable battery.
 * meant to run on an esp8266, obviously
 * as of jan 2nd 2016
 * 
 * compiled last May 4th 2018 , using arduino 1.6.8 board = node mcu 1.0 (ESP-12E)
 * flow to flarm is good, pretty much confirming we're on port 2 of the flarm (since 1 was configured as 19200, while 2 was set to 57600 baud)
 */

#define PROTOCOL_TCP
//#define PROTOCOL_UDP

#include <ESP8266WiFi.h> 

#define packTimeout 5 // ms (if nothing more on UART, then send packet)

//#include <ESP8266WebServer.h>

#define ARRLEN(a) (sizeof(a)/sizeof((a)[0]))

/* Set these to your desired credentials. */
const char *ssid = "TripMateSith-0C78";
const char *password = "11111111";

// flight computer
const char* host = "10.10.10.2"; // cellphone was 2, kobo 4,3
const int port = 4353; // flarm on 4352, vario on 4353

#ifdef PROTOCOL_TCP
#include <WiFiClient.h>
WiFiServer server(port);
WiFiClient client;
#endif

#ifdef PROTOCOL_UDP
#include <WiFiUdp.h>
WiFiUDP udp;
#endif


void send_packet(char *packetBuffer, int j) {
#ifdef PROTOCOL_TCP
	//client.write((char*)packetBuffer, j);
	client.write((char*) packetBuffer, j);
#endif

#ifdef PROTOCOL_UDP
    udp.beginPacket(host, port);
    udp.write(packetBuffer, j);
    udp.endPacket();
#endif
    return;
}

char packetBuffer[8192];
uint8_t wifi2serialBuffer [8192];

void setup() {
	delay(1000);
	Serial.begin(57600); // match flarm setup ; seems we're connected on port 2
	/* You can remove the password parameter if you want the AP to be open. */

	Serial.print("compiled on ");
	Serial.print(__DATE__);
	Serial.print("  ");
	Serial.println(__TIME__);
		
// operation in station mode; not sure why, but didn't seem necessary for UDP.
#ifdef PROTOCOL_TCP
        WiFi.mode(WIFI_STA);
#endif
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
	
#ifdef PROTOCOL_TCP
		Serial.println("Starting TCP Server");
		server.begin(); // start TCP server 
#endif

#ifdef PROTOCOL_UDP
	Serial.println("Starting UDP Server");
	udp.begin(port);
	Serial.print("Local port: ");
	Serial.println(udp.localPort());
#endif
	for (int i =0; i < ARRLEN(packetBuffer); i++) {
		packetBuffer[i] = 0;
	}
}

byte j =0 ;
void loop() {
    int incomingByte, packetSize = 0;
	IPAddress remoteIp;

// tcp - always insist on a client
#ifdef PROTOCOL_TCP
	if(!client.connected()) { // if client not connected
		client = server.available(); // wait for it to connect
		return;
	}
#endif

// old style - expects NEMA sentences, really, or at least newline terminated.
/*
    if (Serial.available() > 0) {
            incomingByte = Serial.read();
//             Serial.println(incomingByte, DEC);
            packetBuffer[j++] = incomingByte;
			// do we need to handle flight downloads, declaring tasks as something special? i have the feeling these are not NEMA
            if ((j >0 && incomingByte =='\n') || j == ARRLEN(packetBuffer)) {
                if (j < ARRLEN(packetBuffer)) {
                    packetBuffer[j+1] == 0;
                    //Serial.println(packetBuffer);
                }
                send_packet(packetBuffer, j);
                j = 0;
            }
    }
*/
// new style - from https://github.com/roboremo/ESP8266-WiFi-UART-Bridge/blob/master/v1.1/sketch_esp_WiFi_UART_Bridge.ino send on timeout.
// not clear how this will handle breaks etc;
// 20180505 - seems to work exactly like the older version; i.e. i still can't declare tasks or download flights. something is clearly wrong.
// TODO : try running on a laptop (at least easier to add error messages) ; maybe not adding a null terminator is a problem? seems we're getting the request, but our response is incorrect?
	if(Serial.available()) {

		// read the data until pause:
		// note that if we have a babbling idiot, we'll never send - but that might be the preferred method in any case.
		while(1) {
			if(Serial.available()) {
				packetBuffer[j] = (char)Serial.read(); // read char from UART
				if(j < ARRLEN(packetBuffer) - 1) {
					j++;
				}
			} else {
				delay(packTimeout);
				if(!Serial.available()) {
					break;
				}
			}
		}
		
		// now send to WiFi:
		send_packet(packetBuffer, j);
		j = 0;
	}

	//handle incoming data
	
#ifdef PROTOCOL_TCP

  // here we have a connected client

  if(client.available()) {
    while(client.available()) {
      wifi2serialBuffer[packetSize] = client.read();
	  // pretty odd - so just overwrite the last byte if we're too large?
      if(packetSize < ARRLEN(wifi2serialBuffer) - 1){
		  packetSize++;
	  }
    }
    // now send to UART:
    Serial.write(wifi2serialBuffer, packetSize);
    packetSize = 0;
  }
#endif

#ifdef PROTOCOL_UDP
    packetSize = udp.parsePacket();
	if (packetSize) {
		//Serial.print("got N= ");
		//Serial.println(packetSize);
		remoteIp = udp.remoteIP(); // store the ip of the remote device
		udp.read(wifi2serialBuffer, ARRLEN(wifi2serialBuffer));
		// now send to UART:
		Serial.write(wifi2serialBuffer, packetSize);
	}
#endif
}
