#include <driver/gpio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <esp_system.h>
#include <esp_log.h>
#include <esp_ipc.h>
#include <WiFi.h>
#include <PubSubClient.h>

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

  // Filas para cada tópico
  queue_vel = xQueueCreate(5, sizeof(int));
  queue_acel = xQueueCreate(5, sizeof(int));
  queue_freio = xQueueCreate(5, sizeof(int));
  queue_temp = xQueueCreate(5, sizeof(int));

  // Iniciar as tarefas
  // xTaskCreate(Função, "Nome(debug)", tamanho stack, entrada, prioridade, id(opcional));
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
  }
}

// Leitura do Pedal do Acelerador
void pedal(){
  for(;;){
    int acel = 1000*(analogRead(34)/ 4095.0);
    client.publish("acelerador", string(acel));
    osDelay(Sensor_DELAY);
  }
} 

// Leitura do Pedal do Freio
void freio(){
   for(;;){
    int freio = 1000*(analogRead(35)/ 4095.0);
    client.publish("acelerador", string(freio));
    osDelay(Sensor_DELAY);
  }
}

// Leitura do Sensor de Temperatura
void temperatura(){

}

// Leitura do Sensor de Distância
void distancia(){
  
}