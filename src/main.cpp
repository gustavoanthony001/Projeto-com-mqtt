//=======================================================
//Projeto: MQTT Duplas
//Descrição: seguir as intruções do classroom
//Autores: Gustavo Anthony e Eduardo do Carmo 
//Data: 04/11/2026
//Versão: 0.1
//=======================================================

#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <LiquidCrystal_I2C.h>
#include <Botao.h>


void aoReceberMsg(char* topico, byte* payLoad, unsigned int tamanho);
void tratarJson(String mensagem);

LiquidCrystal_I2C lcd(0x27, 20, 4);

const char* SSID = "SENAI EDUCACIONAL";
const char* SENHA = "info@134";

const char* BROKER = "broker.empx.io";
const int PORTA = 1833;
const char* TOPICO_SUB = "senai134/esp32/eduardoEgustavo/comando";
const char* TOPICO_PUB = "senai134/esp32/eduardoEgustavo/status";

bool estadoLed = true;
 String mensagemErro = "Sem Erros";

WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);

void setup() 
{
  Serial.begin(115200);
  Serial.print("");

  Serial.printf("Conectando a rede %s ", SSID);
  WiFi.begin(SSID, SENHA);

  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print("#");
    delay(500);
  }

  delay(1000);

  Serial.printf("\n\r Conectado a rede %s com sucesso!\n\r", SSID);
  Serial.printf("Conectado ao Broker %s\n\r", BROKER);

  mqtt.setServer(BROKER, PORTA);
  mqtt.setCallback(aoReceberMsg);

  String id = "esp32-" +
                String((uint32_t)ESP.getEfuseMac(), HEX);
  Serial.printf("ID do dispositivo: %s\n\r", id.c_str());

  bool conectadoBroker;
  conectadoBroker = mqtt.connect(id.c_str());

  if(conectadoBroker)
  {
    Serial.printf("Conectado ao Broker %s com sucesso!", BROKER);
    mqtt.subscribe(TOPICO_SUB);
    Serial.printf("Inscrito no tópico: %s\n\r", TOPICO_SUB);
  }
  else
  {
    Serial.printf("Falha ao conectar ao Broker %s\n\r", BROKER);
    Serial.printf("Falha: ");
    Serial.println(mqtt.state());

  }

}

void loop() 
{
 mqtt.loop();

 static unsigned long ultimoEnvio = 0;
 const unsigned long tempoEntreEnvios = 10000;

 if(millis() - ultimoEnvio > tempoEntreEnvios)
 {
    ultimoEnvio = millis();

    JsonDocument doc;
    doc["Leds ligados"] = estadoLed;

    String mensagem;
    serializeJson(doc, mensagem);

    mqtt.publish(TOPICO_PUB, mensagem.c_str());
 }
}

void aoReceberMsg(char* topico, byte* payLoad, unsigned int tamanho)
{
    String mensagem;
    mensagem.reserve(tamanho);

    for(unsigned int i=0; i<tamanho; i++)
    {
        mensagem += (char)payLoad[i];
    }

    Serial.printf("Mensagem recebida no tópico: %s\n\rMensagem:\n\r%s\n\n\r");
    tratarJson(mensagem);
}

void tratarJson(String mensagem)
{
    JsonDocument doc;

    DeserializationError erro = deserializeJson(doc, mensagem);

    if (erro)
    {
        Serial.println("Erro no JSON");
        return;
    }

    estadoLed = doc["Led Azul"].as<bool>();

    if(estadoLed)
    {
        Serial.println("Led Azul Ligado");
        rgbLedWrite(48, 0, 0, 255);
    }
    else
    {
        Serial.println("Led Desligado");
        rgbLedWrite(48, 0, 0, 0);
    }
}

   
