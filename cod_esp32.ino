#include <WiFi.h>
#include <ESPAsyncWebSrv.h>
#include "DHT.h"
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <IRrecv.h>
#include <IRutils.h>

#define DHTTYPE DHT11

// Conexão com o wifi
const char* ssid = "DJANIRA 2.4G";
const char* password = "92210906";

// pino da led(lampada)
const int relayPin = 18;

// pino do receptor IR
const uint16_t RECV_PIN = 14; 
IRrecv irrecv(RECV_PIN);
decode_results results;

// pino do emissor IR
const uint16_t IRpin = 32; 
IRsend irsend(IRpin);

// pino do sensor de temperatura e umidade
uint8_t DHTPin = 4;        
DHT dht(DHTPin, DHTTYPE);


// Códigos IR da TV 
const uint32_t energiaTV = 0x20DF10EF;
const uint32_t volMais = 0x20DF40BF;
const uint32_t volMenos = 0x20DFC03F;
const uint32_t canalMais = 0x20DF00FF;
const uint32_t canalMenos = 0x20DF807F;
const uint32_t mudo = 0x20DF906F; 

// Códigos IR do ar-condicionado
const uint32_t ligarAr = 0x8800347;
const uint32_t desligarAr = 0x88C0051;

const uint32_t temperatura[13] = {
  // Códigos IR referente a temperatura do ar
  0x880834F, // 18 graus
  0x8808440, // 19 graus
  0x8808541, // 20 graus
  0x8808642, // 21 graus
  0x8808743, // 22 graus
  0x8808844, // 23 graus
  0x8808945, // 24 graus
  0x8808A46, // 25 graus
  0x8808B47, // 26 graus
  0x8808C48, // 27 graus
  0x8808D49, // 28 graus
  0x8808E4A, // 29 graus
  0x8808F4B  // 30 graus
};

AsyncWebServer server(80);

void setup() {
  Serial.begin(115200);

  irrecv.enableIRIn();
  dht.begin();
  irsend.begin();

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print("");
  }
  Serial.println("WiFi conectado..!");
  Serial.print("IP obtido: ");
  Serial.println(WiFi.localIP());

  pinMode(relayPin, OUTPUT);  // Configura o pino do relé como saída
  digitalWrite(relayPin, HIGH); // Inicialmente, mantenha o relé desligado

  // endpoints para o sensor de temperatura e umidade
  server.on("/sensor", HTTP_GET, [](AsyncWebServerRequest *request){
    float Temperature = dht.readTemperature(); // Leia a temperatura
    float Humidity = dht.readHumidity();       // Leia a umidade
    String jsonResponse = "{\"temperature\":" + String(Temperature) + ",\"humidity\":" + String(Humidity) + "}";
    request->send(200, "application/json", jsonResponse);
  });
  
  // endpoints para o ar condicionado
  server.on("/ligar", HTTP_GET, [](AsyncWebServerRequest *request){
    irsend.sendLG2(ligarAr, 28);
    request->send(200, "text/plain", "Ar condicionado ligado");
  });

  server.on("/desligar", HTTP_GET, [](AsyncWebServerRequest *request){
    irsend.sendLG2(desligarAr, 28);
    request->send(200, "text/plain", "Ar condicionado desligado");
  });
  for (int i = 0; i < 13; i++) {
      String path = "/temperatura/" + String(i + 18);
      server.on(path.c_str(), HTTP_GET, [i](AsyncWebServerRequest *request){
        irsend.sendLG2(temperatura[i], 28);
        request->send(200, "text/plain", "Temperatura definida para: " + String(i + 18));
      });
  }
  
  // endpoints para tv
  // Endpoint para ligar/desligar a TV
  server.on("/tv/energia", HTTP_GET, [](AsyncWebServerRequest *request){
    Serial.println("Enviando comando de energia para a TV...");
    irsend.sendNEC(energiaTV, 32);
    Serial.println("Comando enviado!");
    String jsonResponse = "{\"status\":\"success\",\"message\":\"Comando de energia enviado para a TV\"}";
    request->send(200, "application/json", jsonResponse);
  });
  // Endpoints para controlar o volume
  server.on("/tv/volume/mais", HTTP_GET, [](AsyncWebServerRequest *request){
    irsend.sendNEC(volMais, 32);
    String jsonResponse = "{\"status\":\"success\",\"message\":\"Comando de aumentar volume enviado a TV\"}";
    request->send(200, "application/json", jsonResponse);
  });

  server.on("/tv/volume/menos", HTTP_GET, [](AsyncWebServerRequest *request){
    irsend.sendNEC(volMenos, 32);
    String jsonResponse = "{\"status\":\"success\",\"message\":\"Comando de diminuir volume enviado a TV\"}";
    request->send(200, "application/json", jsonResponse);
  });

  // Endpoints para mudar o canal
  server.on("/tv/canal/mais", HTTP_GET, [](AsyncWebServerRequest *request){
    irsend.sendNEC(canalMais, 32);
    String jsonResponse = "{\"status\":\"success\",\"message\":\"Comando de aumentar canal enviado a TV\"}";
    request->send(200, "application/json", jsonResponse);
  });

  server.on("/tv/canal/menos", HTTP_GET, [](AsyncWebServerRequest *request){
    irsend.sendNEC(canalMenos, 32);
    String jsonResponse = "{\"status\":\"success\",\"message\":\"Comando de diminuir canal enviado a TV\"}";
    request->send(200, "application/json", jsonResponse);
  });

  // Endpoint para ativar/desativar mudo
  server.on("/tv/mudo", HTTP_GET, [](AsyncWebServerRequest *request){
    irsend.sendNEC(mudo, 32);
    String jsonResponse = "{\"status\":\"success\",\"message\":\"Comando de mute/desmute enviado a TV\"}";
    request->send(200, "application/json", jsonResponse);
  });

  // Endpoint para ligar a LED
  server.on("/led/on", HTTP_GET, [](AsyncWebServerRequest *request){
    digitalWrite(relayPin, LOW); // Ativa o relé
    request->send(200, "text/plain", "LED ligado");
  });

  // Endpoint para desligar o LED
  server.on("/led/off", HTTP_GET, [](AsyncWebServerRequest *request){
    digitalWrite(relayPin, HIGH); // Desativa o relé
    request->send(200, "text/plain", "LED desligado");
  });
  server.begin();
}

void loop() {
  
    if(irrecv.decode(&results)){
      if (results.decode_type == UNKNOWN) {
        Serial.print("UNKNOWN: ");
        Serial.println(resultToTimingInfo(&results));
      } else {
        Serial.print(resultToHumanReadableBasic(&results));
      }
      Serial.println();
      irrecv.resume();
    }
    
}