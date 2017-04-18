
/*
 *  This sketch demonstrates how to set up a simple HTTP-like server.
 *  The server will set a GPIO pin depending on the request
 *    http://server_ip/gpio/0 will set the GPIO2 low,
 *    http://server_ip/gpio/1 will set the GPIO2 high
 *  server_ip is the IP address of the ESP8266 module, will be
 *  printed to Serial when the module is connected.
 */

#include <Wire.h>
#include <Adafruit_MLX90614.h>

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>

const char* defaultSSID = "HITRON-6770";
const char* defaultPassword = "5I668LPVIONP";
//const char* defaultSSID = "Fab Visitors";
//const char* defaultPassword = "Tonality";


String ssid = "";
String password = "";

// GLOBAL VARIABLES //

unsigned char debug = 1;              // to activate debugging stuff
unsigned char exitFlag = 0;
unsigned char connEstablished = 0;    // to ensure there is an initial connection.
unsigned char i = 0;                  // iterator
unsigned char timeOut = 30;           // time in seconds before we abort the connection
unsigned char analogVal = 0;          // to save the values coming from analog port.
unsigned char webID = 0;            // value to use in the switch case, find the way to make it local !!!
unsigned char notFound = 0;       // flag for the case of not found webpage.
unsigned char pos = 0;            // to cut the string containing the request.
double objectTemp = 0;            // contactless temperature measurements.

unsigned char sdaPin = D1;    // pins for te I2c communication
unsigned char sclPin = D2;


Adafruit_MLX90614 pyroSensor(0x5A);   // temperature sensor object (contactless);


// wifi server related //
// Create an instance of the server
// specify the port to listen on as an argument
WiFiServer server(80);

// WEBSERVER HTML SHIT ////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

String webPage = "";

String webStart = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE HTML>\r\n<html>\r\n";
String webEnd = "</html>\n";

String buttonEn = "\n<FORM METHOD='LINK' ACTION='0'><INPUT TYPE='submit' VALUE='disable gpio'>  </FORM>\n";
String buttonDis = "\n<FORM METHOD='LINK' ACTION='1'><INPUT TYPE='submit' VALUE='enable gpio'>  </FORM>\n";
String buttonD = "\n<a class='button' href='/0'>Write some random shit ButtonD</a>\n";
String analogStart = "\n<p><b> Value of the analog port:";
String analogEnd = "</b></p>";

String startParagraph = "<p>";
String endParagraph = "</p>";


////////////////////////////////////////////////////////////////////////////////

// FUNCTION DECLARATION //

void networkSelector(void);
void enumerateNetworks(void);
unsigned char connectToNetwork(void);
void constructWebpage(void);
String drawGraph(void);
void i2cScan();

// SETUP //

void setup() {

  // prepare GPIO2
  pinMode(2, OUTPUT);
  digitalWrite(2, LOW);

  pinMode(D3, OUTPUT);
  digitalWrite(D3, HIGH);


  //Serial port
  Serial.begin(115200);
  delay(1500);

  // i2c port
  Wire.begin(sdaPin, sclPin);   // i2c port over the pins D0(sda) and D1(scl)
  if(debug == 1) i2cScan(); // scans to know if devices connected for sure

  // temperature sensor

  if(debug == 1){
    objectTemp = pyroSensor.readObjectTempC();
    Serial.print("\nRemote temperature measurement:  ");
    Serial.print(objectTemp);
    Serial.print("ºC\n");
  }




  // prepare WiFi connection //
  // default ssid and password
  ssid = defaultSSID;           // works OK, just takes the char* and puts it inside the string
  password = defaultPassword;

  while(connEstablished == 0){
    unsigned char errFlag;           // just to save return value from connectToNetwork()
    errFlag = connectToNetwork();     // connects to the network defined with passwd and ssid(GLOBAL)
    if(errFlag == 0)
      connEstablished = 1;
    else
      networkSelector();

  }
    Serial.println("");
    Serial.println("WiFi connected");

    // Start the server
    server.begin();
    Serial.println("Server started");

    // Print the IP address
    Serial.println(WiFi.localIP());
}


void loop() {

// shit happening inside the microcontroller.

  // Check if a client has connected
  WiFiClient client = server.available();
  if (!client) {
    return;
  }



  // Read the first line of the request
  String request = client.readStringUntil('\r');
  /*
  Serial.print("String containing the request:               ");
  Serial.print('#');
  Serial.print(request.c_str());
  Serial.print('#');
  */
  pos = request.indexOf(' ');       // find the first space
  request = request.substring(pos+1);   // cut everything before the first space.
  pos = request.indexOf(' ');       // find the second space
  request = request.substring(0,pos);   // cut everything after the first space.



  //Serial.print("Processed string ready to use:               ");
  Serial.print('#');
  Serial.print(request.c_str());
  Serial.print('#');


  Serial.print('\n');
  client.flush();

  // Match the request

  if(request == "/0"){
    Serial.println("match /0");
    digitalWrite(2, LOW); // Set GPIO2 according to the request
    webID = 0;
  }
  else if(request == "/1"){
    Serial.println("match /1");
    digitalWrite(2, HIGH); // Set GPIO2 according to the request
    webID = 1;
  }
  else if(request == "/"){
    Serial.println("match /index");
    webID = 2;
  }
  else if(request == "/A0"){
    webID = 3;
    analogVal = analogRead(A0);
  }
  else if(request == "/Graph"){
    webID = 4;
  }
  else if(request == "/pyroSensor"){
    webID = 5;
  }
  else {
    Serial.println("invalid request");
    webID = -1;
  }

  analogVal = analogRead(A0);   //  not necessary, remove !!!

  // Send the response to the client
    constructWebpage();
    client.print(webPage);
    client.stop();                // ??? !!!
  delay(1);
  Serial.println("Client disonnected");

  // The client will actually be disconnected
  // when the function returns and 'client' object is detroyed
}

void networkSelector(){

  char c = 0;
  String networkN = "";   // saves the choosen network number.
  String passwd = "";     // saves the choosen network password.
  unsigned char n = 0;

  // show all available networks.

  Serial.print("\nLooking for networks: \n\n");
  enumerateNetworks();
  delay(400);

  // choose the network number.
  Serial.print("\nInsert network number: ");
  Serial.flush();
  while((Serial.available()) == 0);
  networkN = Serial.readString();
  n = networkN.toInt(); // convert it onto an integer.
  Serial.print("\nSelected network number:\n ");
  Serial.print(n);
  n = n-1;          // indexes are wrong, they start in 0, not in 1
  Serial.print("\n\nSelected network:\n ");
  ssid = WiFi.SSID(n);                      // THIS MAKES SOMETHING WRONG!!!
  Serial.print(ssid);

  if(debug == 1) Serial.print('#');Serial.print(ssid);Serial.print('#');


  // save the name of the network (or at least the number) somewhere.

  // ask for the password for the given network.

  Serial.print("\nInsert network password: ");
  Serial.flush();
  while((Serial.available()) == 0);
  password = Serial.readString(); // when reading,a \n gets added, and something else !!!
  //password.remove(8); // remove \n !!!
  password.remove(password.length()-2); // remove \n !!!
  if(debug == 1) Serial.print('#');Serial.print(password);Serial.print('#');
}

void enumerateNetworks(void){

  int n = WiFi.scanNetworks();
  if (n == 0)
    Serial.println("No available networks found");
  else{
    Serial.print(n);
    Serial.println(" networks found");
  }
  for (int i = 0; i < n; ++i){  // Print SSID y RSSI para cada una
    Serial.print(i + 1);
    Serial.print(": ");
    Serial.print(WiFi.SSID(i));
    Serial.print(" (");
    Serial.print(WiFi.RSSI(i));
    Serial.print(")");
    Serial.println((WiFi.encryptionType(i) == ENC_TYPE_NONE)?" ":"*");
    delay(10);
    }
}

unsigned char connectToNetwork(void){
  // Connect to WiFi network

  unsigned char timeOutFlag = 0;

  WiFi.disconnect();

  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid.c_str(),password.c_str());

  i = 0;
  while (WiFi.status() != WL_CONNECTED && timeOutFlag == 0) {
    delay(500);
    Serial.print(".");
    i++;
    if(i > 2*timeOut){
      timeOutFlag = 1;
      Serial.print("\nConnection timeout, choose new network.");
    }
  }
  return(timeOutFlag);    // if flag = 0, alles OK, if 1 no connection
}

void constructWebpage(void){
  notFound = 0;
  webPage = "";      // empty the string to fill with the webpage to be displayed.<
  String graph = "";
  webPage += webStart;

  switch(webID){
    case 0:
      if(debug == 1) Serial.print("Case 0 \n");
      webPage += "GPIO is now ";
      webPage += (webID)?"HIGH":"LOW";
      webPage += buttonDis;
      webPage += buttonD;

    break;

    case 1:
      if(debug == 1) Serial.print("Case 1 \n");
     webPage+= "GPIO is now ";
     webPage+= (webID)?"HIGH":"LOW";
     webPage+= buttonEn;
     webPage+= buttonD;
    break;

    case 2:
      if(debug == 1) Serial.print("Case 2 \n");
      webPage += "This is the index webpage, nothing interesting here";
    break;

    case 3:
      if(debug == 1) Serial.print("Case 3 \n");
      webPage += "Case3";
    break;

    case 4:
      if(debug == 1) Serial.print("Case 4");
      graph = drawGraph();
      webPage += graph;
    break;

    case 5:
    if(debug == 1) Serial.print("Case 5");
      objectTemp = pyroSensor.readObjectTempC();   // reading object temperature.
      webPage = webPage + startParagraph + "Remote temp reading  " + objectTemp + "ºC" + endParagraph;

    default:
      if(debug == 1) Serial.print("Case 3 \n");
      webPage += "  ERROR 404 NOT FOUND ...";
      notFound = 1;
    break;

  }

  if(notFound == 0){
    if(debug == 1)Serial.print("\nValue of the analog port: "); Serial.print(analogVal); Serial.print('\n');
    webPage += analogStart;   // writes the value of the analog port.
    webPage += analogVal;     // current analogvalue
    webPage += analogEnd;

  }

  //graph = drawGraph();
  //webPage += graph;

webPage += webEnd;   // closes the webpage before sending it to the client.

  //if(debug == 1) Serial.print("Webpage constructed"); Serial.print(webPage);
}

String drawGraph(void) {
	String out = "";
  out+= startParagraph;
	char temp[100];
	out += "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\" width=\"400\" height=\"150\">\n";
 	out += "<rect width=\"640\" height=\"320\" fill=\"rgb(255, 255, 255)\" stroke-width=\"10\" stroke=\"rgb(10, 10, 0)\" />\n";
 	out += "<g stroke=\"rgb(0,255,255)\">\n";
 	int y = rand() % 130;
 	for (int x = 10; x < 390; x+= 10) {
 		int y2 = rand() % 130;
 		sprintf(temp, "<line x1=\"%d\" y1=\"%d\" x2=\"%d\" y2=\"%d\" stroke-width=\"1\" />\n", x, 140 - y, x + 10, 140 - y2);
 		out += temp;
 		y = y2;
 	}
	out += "</g>\n</svg>\n";
  out+= endParagraph;

  return(out);
  //client.print(out);
	//WiFiserver.send ( 200, "image/svg+xml", out);
}


void i2cScan(void){

  byte error, address;
  int nDevices;

  Serial.println("Scanning...");

  nDevices = 0;
  for(address = 1; address < 127; address++ )
  {
    // The i2c_scanner uses the return value of
    // the Write.endTransmisstion to see if
    // a device did acknowledge to the address.
    Wire.beginTransmission(address);
    error = Wire.endTransmission();

    if (error == 0)
    {
      Serial.print("I2C device found at address 0x");
      if (address<16)
        Serial.print("0");
      Serial.print(address,HEX);
      Serial.println("  !");

      nDevices++;
    }
    else if (error==4)
    {
      Serial.print("Unknow error at address 0x");
      if (address<16)
        Serial.print("0");
      Serial.println(address,HEX);
    }
  }
  if (nDevices == 0)
    Serial.println("No I2C devices found\n");
  else
    Serial.println("done\n");

  delay(500);

}
