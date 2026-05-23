#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>


const char* ssid = "Wokwi-GUEST";           // Rede padrão do simulador Wokwi
const char* password = "";
const char* mqtt_broker = "broker.hivemq.com"; // Broker público para testes
const int mqtt_port = 1883;


LiquidCrystal_I2C lcd(0x27, 20, 4);

const int PIN_TRIG = 5;
const int PIN_ECHO = 19;
const int PIN_CHUVA = 34; 
const int PIN_SOLO  = 35; 
const int PIN_BUZZER = 13;

const int DISTANCIA_LEITO_VAZIO = 200; 
const int LIMITE_ALERTA_ENCHENTE = 40;  


WiFiClient espClient;
PubSubClient client(espClient);

unsigned long ultimaPublicacao = 0;
const long INTERVALO_PUBLICACAO = 2000; // Envia dados a cada 2 segundos

void setupWifi() {
  delay(10);
  lcd.setCursor(0, 2); lcd.print("Conectando WiFi...  ");
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Conectado!");
  lcd.setCursor(0, 2); lcd.print("WiFi: CONECTADO     ");
}

void reconnectMQTT() {
  // Loop até conectar ao Broker
  while (!client.connected()) {
    Serial.print("Tentando conexão MQTT...");
    // Cria um ID único para o cliente ESP32
    String clientId = "ESP32-RioClient-";
    clientId += String(random(0xffff), HEX);
    
    if (client.connect(clientId.c_str())) {
      Serial.println("Conectado ao Broker MQTT!");
      lcd.setCursor(0, 3); lcd.print("MQTT: CONECTADO     ");
    } else {
      Serial.print("Falhou, rc=");
      Serial.print(client.state());
      Serial.println(" Tentando novamente em 3 segundos...");
      lcd.setCursor(0, 3); lcd.print("MQTT: ERRO CONEXAO  ");
      delay(3000);
    }
  }
}

long lerUltrassonico() {
  digitalWrite(PIN_TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(PIN_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_TRIG, LOW);
  
  long duracao = pulseIn(PIN_ECHO, HIGH, 30000);
  long distancia = duracao * 0.034 / 2;
  
  return (distancia == 0) ? DISTANCIA_LEITO_VAZIO : distancia;
}

void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22);
  
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0); lcd.print("  SISTEMA SMART-FLOW ");
  lcd.setCursor(0, 1); lcd.print(" MONITORAMENTO IOT   ");

  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);
  pinMode(PIN_BUZZER, OUTPUT);
  
  setupWifi();
  client.setServer(mqtt_broker, mqtt_port);
}

void loop() {

  if (!client.connected()) {
    reconnectMQTT();
  }
  client.loop(); // Mantém o stack interno do MQTT vivo

  if (millis() - ultimaPublicacao >= INTERVALO_PUBLICACAO) {
    ultimaPublicacao = millis();
    
   
    long distanciaSensor = lerUltrassonico();
    int nivelRioCm = DISTANCIA_LEITO_VAZIO - distanciaSensor;
    if (nivelRioCm < 0) nivelRioCm = 0;
    
    int pctChuva = map(analogRead(PIN_CHUVA), 4095, 0, 0, 100);
    int pctSolo = map(analogRead(PIN_SOLO), 4095, 0, 0, 100);
    
    // 2. Lógica de Status de Alerta
    String statusAtual = "OK";
    bool riscoEnchente = (distanciaSensor <= LIMITE_ALERTA_ENCHENTE);
    bool riscoDesabamento = (pctSolo > 75 && pctChuva > 60);

    if (riscoEnchente) {
      statusAtual = "ALERTA_ENCHENTE";
      tone(PIN_BUZZER, 2500, 200); 
    } else if (riscoDesabamento) {
      statusAtual = "ALERTA_ENCOSTA";
      tone(PIN_BUZZER, 1200, 100);
    } else {
      noTone(PIN_BUZZER);
    }
    
    // 3. Publicação dos Dados via MQTT (Convertendo valores para String)
    client.publish("rio/telemetria/nivel", String(nivelRioCm).c_str());
    client.publish("rio/telemetria/chuva", String(pctChuva).c_str());
    client.publish("rio/telemetria/solo", String(pctSolo).c_str());
    client.publish("rio/telemetria/status", statusAtual.c_str());
    
    // 4. Atualização Local no LCD para validação visual na bancada
    lcd.setCursor(0, 0);
    lcd.print("Rio: "); lcd.print(nivelRioCm); lcd.print("cm | Chuva: "); lcd.print(pctChuva); lcd.print("%  ");
    lcd.setCursor(0, 1);
    lcd.print("Solo: "); lcd.print(pctSolo); lcd.print("% | St: "); lcd.print(statusAtual.substring(0,6)); lcd.print("  ");
    
    // Debug na Serial
    Serial.printf("Dados Publicados -> Rio: %d cm | Chuva: %d%% | Status: %s\n", nivelRioCm, pctChuva, statusAtual.c_str());
  }
}