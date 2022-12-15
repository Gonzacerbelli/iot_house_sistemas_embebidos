//Librerias
//WiFi
#include "WiFiEsp.h"
//RFID
#include <SPI.h>
#include <MFRC522.h>
//Display 16x02
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
//Sensor DHT11 para medir temperatura y humedad
#include <DHT.h>

#ifndef HAVE_HWSERIAL1
#include "SoftwareSerial.h"
SoftwareSerial ESPserial(2, 3); // RX, TX
#endif

// Definimos el pin digital donde se conecta el sensor
#define DHTPIN 5
// Dependiendo del tipo de sensor
#define DHTTYPE DHT11
// Inicializamos el sensor DHT11
DHT dht(DHTPIN, DHTTYPE);

//Crear el objeto lcd  dirección  0x27, 16 columnas x 2 filas
LiquidCrystal_I2C lcd(0x27, 16, 2);

//RFID
#define RST_PIN         9
#define SS_PIN          10
MFRC522 rfid(SS_PIN, RST_PIN);


//Variables globales
bool acceso = 0;
long uid_tarjeta;
float tanque_de_agua;
bool bomba_encendida = 0;

//Leds
const int LED_ACCESO = 7;
const int LED_BOMBA = 6;


//Credenciales WiFi
char ssid[] = "sc-1244";            // your network SSID (name)
char pass[] = "UJZNTY5KTY3Q";        // your network password
int status = WL_IDLE_STATUS;     // the Wifi radio's status

IPAddress server(192, 168, 0, 150);

// Initialize the Ethernet client object
WiFiEspClient client;

unsigned long lastConnectionTime = 0;         // last time you connected to the server, in milliseconds
const unsigned long postingInterval = 10000L; // delay between updates, in milliseconds

void setup()
{
  // initialize serial for debugging
  Serial.begin(9600);
  // Start the software serial for communication with the ESP8266
  ESPserial.begin(9600);

  //Escribimos por el monitor serie mensaje de inicio
  Serial.println("Inicio de sistema");

  WiFi.init(&ESPserial);

  WiFi.disconnect();

  // check for the presence of the shield
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("WiFi shield not present");
    // don't continue
    while (true);
  }

  // attempt to connect to WiFi network
  while ( status != WL_CONNECTED) {
    Serial.print("Attempting to connect to WPA SSID: ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network
    status = WiFi.begin(ssid, pass);
  }

  // you're connected now, so print out the data
  Serial.println("You're connected to the network");
  
  printWifiStatus();

  //dht11
  dht.begin();

  //RFID
  SPI.begin();      // Init SPI bus
  rfid.PCD_Init();   // Init MFRC522

  //LCD
  lcd.init();
  //Encender la luz de fondo.
  lcd.backlight();

  //LEDS
  pinMode(LED_ACCESO, OUTPUT);
  pinMode(LED_BOMBA, OUTPUT);

}

void make_request(float temperatura, float tanque_de_agua, float humedad, float indice_de_calor){
  client.stop();
  
  Serial.println("Starting connection to server...");
  // if you get a connection, report back via serial
  if (client.connect(server, 5000)) {
    String endpoint = "GET /carga?temperatura=" + String(temperatura) + "&humedad=" + String(humedad) + "&indiceDeCalor=" + String(indice_de_calor) + "&nivelTanque=" + String(tanque_de_agua) + " HTTP/1.1";
    Serial.println("Connected to server");
    // Make a HTTP request
    client.println(endpoint);
    client.println("Accept: */*");
    client.println("Connection: close");
    client.println();
    // note the time that the connection was made
    lastConnectionTime = millis();
  }

}

void resetear_todo() {
  digitalWrite(LED_ACCESO, LOW);
  digitalWrite(LED_BOMBA, LOW);
  lcd.clear();
  acceso = 0;
  uid_tarjeta = 0;
}

void calcular_tanque_de_agua() {

  tanque_de_agua = analogRead(A1);
  Serial.print("Nivel de agua: ");
  Serial.println(tanque_de_agua);

  if (tanque_de_agua < 30) {
    bomba_encendida = 1;
    digitalWrite(LED_BOMBA, HIGH);
  }
  if (tanque_de_agua > 190) {
    bomba_encendida = 0;
    digitalWrite(LED_BOMBA, LOW);
  }
}

float leer_humedad() {
  return dht.readHumidity();
}

float leer_temperatura() {
  return dht.readTemperature();
}

float calcular_indice_de_calor(float temperatura, float humedad) {
  return dht.computeHeatIndex(temperatura, humedad, false);
}

void probar_conexion_rfid() {
  bool result = rfid.PCD_PerformSelfTest();
  Serial.print(F("\nPrueba de conexion con RFID: "));
  if (result)
    Serial.println(F("Conectado"));
  else
    Serial.println(F("Desconectado"));
}

long obtener_uid() {
  if ( ! rfid.PICC_ReadCardSerial()) { //Cortar una vez que se obtiene el Serial
    return -1;
  }
  unsigned long hex_num;
  hex_num =  rfid.uid.uidByte[0] << 24;
  hex_num += rfid.uid.uidByte[1] << 16;
  hex_num += rfid.uid.uidByte[2] <<  8;
  hex_num += rfid.uid.uidByte[3];
  rfid.PICC_HaltA(); // Dejar de leer
  return hex_num;
}

void entrar() {
  if (rfid.PICC_IsNewCardPresent()) {
    unsigned long uid = obtener_uid();
    if (uid != -1) {
      Serial.print("Tarjeta detectada, UID: " + String(uid));
      uid_tarjeta = uid;
      acceso = 1;
      digitalWrite(LED_ACCESO, HIGH);
      lcd.clear();
      lcd.print("Bienvenido");
    }
  }
}

void salir() {
  if (rfid.PICC_IsNewCardPresent()) {
    unsigned long uid = obtener_uid();
    if (uid != -1) {
      Serial.print("Tarjeta detectada, UID: " + String(uid));
      if (uid_tarjeta == uid) {
        resetear_todo();
        lcd.print("Adios");
        Serial.print("Saliendo");
      }
    }
  }
}

void loop()
{

  probar_conexion_rfid();

  if (!acceso) {
    lcd.setCursor(0, 0);
    lcd.print("Ingrese llave");

    Serial.println("Bienvenido. Escanear llave magnetica");

    entrar();

  } else {
    calcular_tanque_de_agua();
    float temperatura = leer_temperatura();
    float humedad = leer_humedad();
    float indice_de_calor = calcular_indice_de_calor(temperatura, humedad);

    Serial.println("El tanque de agua tiene " + String(tanque_de_agua) + " litros");
    Serial.println("La temperatura es de " + String(temperatura) + "°C");
    Serial.println("La humedad es del " + String(humedad) + "%");
    Serial.println("El índice de calor es de " + String(indice_de_calor) + "%");
    Serial.println();

    lcd.setCursor(0, 0);
    lcd.print("Temp: " + String(temperatura) + "C - Ind Cal: " + String(indice_de_calor));
    lcd.setCursor(0, 1);
    lcd.print("Hum: " + String(humedad) + "% - Agua: " + String(tanque_de_agua) + "l");

    

    if (millis() - lastConnectionTime > postingInterval) {
      make_request(temperatura, tanque_de_agua, humedad, indice_de_calor);
    }

    for (int i = 0; i < 7; i++) {
      lcd.scrollDisplayLeft();
    }

    salir();
  }


  // if there are incoming bytes available
  // from the server, read them and print them
  while (client.available()) {
    char c = client.read();
    Serial.write(c);
  }

  delay(2000);

}


void printWifiStatus()
{
  // print the SSID of the network you're attached to
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your WiFi shield's IP address
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength
  long rssi = WiFi.RSSI();
  Serial.print("Signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}
