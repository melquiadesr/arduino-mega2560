#include <TimerOne.h>
#include <SD.h>
#include <SPI.h>
#include <Ethernet.h>

/****************************************
 *************  DADOS DO SD  ************
 ****************************************/
const int chip = 4;
File dataFile;

/****************************************
 * Endereço do servidor arduino na rede *
 ****************************************/
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
IPAddress ip(192,168,0,25);
IPAddress gateway(192,168,0,1);
IPAddress subnet(255, 255, 255, 0);

EthernetClient client;

/****************************************
 ****** Endereço do servidor php   ******
 ****************************************/
IPAddress serverIp (192,168,0,11);
int serverPort = 8000;


/****************************************
 ******* INFORMAÇÕES DO SENSOR   ********
 ****************************************/
int sensorPin = 21;
int pulses = 0;
const float coefficient = 0.0021243;

/****************************************
 **********  DADOS CALCULADOS  **********
 ****************************************/
float flowRate;
float liters = 0;
int seconds = 0;
int minutes = 0;

/****************************************
 ************  CONFIGURAÇÃO  ************
 ****************************************/
const int sendPeriod = 5; //minutos
const String fileName = "data.txt";

void setup() {
  Serial.begin(9600);

  if (!SD.begin(chip)) {
    Serial.println("ERRO NO CARTÃO SD!");
    return;
  }
  removeData();
  Ethernet.begin(mac, ip, gateway, subnet);
  if (Ethernet.begin(mac) == 0) {
    Serial.println("ERRO NA CONEXÃO!");
    return;
  }

  if ( ! sendData("{\"initializing\":\"true\"}")) {
    Serial.println("ERRO NO ENVIO!");
    return;
  }

  pinMode(sensorPin, INPUT);
  attachInterrupt(digitalPinToInterrupt(sensorPin), pulse, RISING);
  sei();

  Timer1.initialize();
  Timer1.attachInterrupt(calculateData);
  Serial.println("MONITORAMENTO INICIALIZADO!");
}

void loop() {
    if (seconds == 0 && minutes > 0) {
      
      recordData();
      
      if (
        minutes >= sendPeriod
        && sendAllData()
      )
      {
        removeData();
        minutes = 0;
      }
      
      delay(1000);
    }
}

boolean sendData (String data) {
  if( ! client.connect(serverIp, serverPort)) {
    return false;
  }
  client.connect(serverIp, serverPort);
  client.println("POST /sensor_data HTTP/1.1");
  client.print("Host: ");
  client.print(serverIp);
  client.print(":");
  client.println(serverPort);
  client.println("Content-Type: application/json");
  client.print("Content-Length: ");
  client.println(data.length());
  client.println("Connection: keep-alive");
  client.println();
  client.println(data);

  return true;
}

boolean sendAllData() {
  if( ! client.connect(serverIp, serverPort)) {
    return false;
  }
  client.connect(serverIp, serverPort);
  client.println("POST /sensor_data HTTP/1.1");
  client.print("Host: ");
  client.print(serverIp);
  client.print(":");
  client.println(serverPort);
  client.println("Content-Type: application/json");
  client.print("Content-Length: ");
  client.println(getDataLenght(),0);
  client.println("Connection: keep-alive");
  client.println();
  client.print("{\"data\":[");
  
  dataFile = SD.open(fileName);
  while (dataFile.available()) {
    char c = (char) dataFile.read();
    client.print(c);
  }
  dataFile.close();
  
  client.print("]}");
  client.println();

  return true;
}

void calculateData () {
  seconds++;
  
  if(seconds == 60) {
    minutes++;
    seconds = 0;
  }
  
  if(pulses > 0) {
    flowRate = pulses * coefficient;
    liters = liters + flowRate;
  }
  
  pulses = 0;
}

void recordData() {
  boolean firstLine = true;
  
  if (SD.exists(fileName)) {
    firstLine = false;
  }
  
  dataFile = SD.open(fileName, FILE_WRITE);
  if ( ! dataFile) {
    Serial.println("ERRO AO ACESSAR O SD...");
    return;
  }
  
  if ( ! firstLine) {
    dataFile.println(",");
  }
  
  dataFile.print("{\"minute\":\"");
  dataFile.print(minutes);
  dataFile.print("\",\"liters\":\"");
  dataFile.print(liters);
  dataFile.print("\"}");
  dataFile.close();
  
  liters = 0;
}

float getDataLenght() {
  float length = 11;
  
  dataFile = SD.open(fileName);
  while (dataFile.available()) {
    char c = (char) dataFile.read();
    length++;
  }
  dataFile.close();
  
  return length;
}

void removeData() {
  if (SD.exists(fileName)) {
    SD.remove(fileName);
  }
}

void pulse () {
    pulses++;
}
