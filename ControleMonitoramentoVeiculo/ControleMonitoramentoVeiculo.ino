#include <driver/gpio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <esp_system.h>
#include <esp_log.h>
#include <esp_ipc.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>

// WiFi
const char *id = "xxxxx";     // Rede wifi
const char *senha = "xxxxx";  // Senha

// MQTT Broker
const char *mqtt_broker = "broker.emqx.io";
const int mqtt_port = 1883;

WiFiClient espClient;
PubSubClient client(espClient);

// Tópicos
const char *velocidade = "velocidade";
const char *acelerador = "acelerador";
const char *freio = "freio";
const char *temperatura = "temperatura";

// Fila
QueueHandle_t buffer;

// Definições
#define Sensor_DELAY 50
#define Temp_DELAY 1000
#define LM75_ADDR 0x48

void setup() {

  // Conectar ao wifi
  WiFi.begin(id, senha);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Estabelecendo conexão...");
  }
  Serial.println("Wifi conectado.");

  //Configuração Mqtt
  client.setServer(mqtt_broker, mqtt_port);  // Define broker e porta
  client.setCallback(callback);              // Define função de recebimento

  while (!client.connected()) {
    String client_id = "cliente";
    client_id += String(WiFi.macAddress());  // Gera um ID único baseado no MAC
    Serial.printf("Conectando ao broker como %s...\n", client_id.c_str());

    if (client.connect(client_id.c_str())) {
        Serial.println("Conectado ao broker MQTT.");
    } else {
        Serial.print("Falha, estado: ");
        Serial.print(client.state());  // Mostra o erro
        delay(2000);  // Espera 2s antes de tentar novamente
    }
  }

  // Tópicos Mqtt
  client.subscribe(velocidade);
  client.subscribe(acelerador);
  client.subscribe(freio);
  client.subscribe(temperatura);
  client.subscribe(distancia);

  // Filas para cada tópico
  queue_vel = xQueueCreate(5, sizeof(int));
  queue_acel = xQueueCreate(5, sizeof(int));
  queue_freio = xQueueCreate(5, sizeof(int));
  queue_temp = xQueueCreate(5, sizeof(int));
  queue_dist = xQueueCreate(5, sizeof(int));

  // Iniciar as tarefas
  xTaskCreate(pedal, "pedal", 2048, NULL, 3, NULL);
  xTaskCreate(freio, "freio", 2048, NULL, 3, NULL);
  xTaskCreate(temperatura, "temperatura", 2048, NULL, 1, NULL);
  xTaskCreate(distancia, "distancia", 2048, NULL, 2, NULL);

  //Comunicação I2C
  Wire.begin();

  // Sensor Ultrassônico
  pinMode(23, OUTPUT); // emissor
  pinMode(22, INPUT); // receptor
}

void app_main() {
}


// Callback
void callback(char *topic, byte *payload, unsigned int length) {
  String msg;
  // Converte o array de bytes para string
  for (int i = 0; i < length; i++){
    msg += (char)payload[i];
  }
  // Filtar por tópico
  if(strcmp(topic, velocidade) == 0){
    xQueueSend(queue_vel, &msg, 0);
  } else if(strcmp(topic, acelerador) == 0){
    xQueueSend(queue_acel, &msg, 0);
  } else if(strcmp(topic, freio) == 0){
    xQueueSend(queue_freio, &msg, 0);
  } else if(strcmp(topic, distancia) == 0){
    xQueueSend(queue_dist, &msg, 0);
  }
}

// Leitura do Pedal do Acelerador
void pedal(){
  for(;;){
    int acel = 1000*(analogRead(34)/ 4095);
    client.publish("acelerador", string(acel));
    osDelay(Sensor_DELAY);
  }
} 

// Leitura do Pedal do Freio
void freio(){
   for(;;){
    int freio = 1000*(analogRead(35)/ 4095);
    client.publish("acelerador", string(freio));
    osDelay(Sensor_DELAY);
  }
}

// Leitura do Sensor de Temperatura
void temperatura(){
  int16_t temp;
  for(;;){
    Wire.beginTransmission(LM75_ADDR);
    Wire.write(0x00);
    Wire.endTransmission();
  
    Wire.requestFrom(LM75_ADDR, 2);   
  
    if (Wire.available() >= 2) {
      temp = ( ( (Wire.read() << 8) | Wire.read() )/ 256 )*10;
    }else{
      temp = -1000;
    }
    client.publish("temperatura", string(temp));
    osDelay(Temp_DELAY);
  }
}

// Leitura do Sensor de Distância
void distancia(){
  unsigned long tempo;
  int dist;
  for(;;){
    // Envia um pulso de 10 µs no Trig
    digitalWrite(23, LOW);
    delayMicroseconds(2);
    digitalWrite(23, HIGH);
    delayMicroseconds(10);
    digitalWrite(23, LOW);

    tempo = pulseIn(22, HIGH);
    dist = duration * 343 / 2; // distância *10^-4
    client.publish("distancia", string(dist));
    osDelay(Sensor_DELAY);
  }
}