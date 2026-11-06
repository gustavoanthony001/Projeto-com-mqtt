//=======================================================
// Projeto: MQTT Duplas
// Descrição: Controle RGB via MQTT e JSON
// Autores: Gustavo Anthony e Eduardo do Carmo
// Data: 04/11/2026
// Versão: 1.0
//=======================================================

#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>


void aoReceberMsg(char* topico, byte* payLoad, unsigned int tamanho);
void tratarJson(String mensagem);
void controlarRGB(String cor);
void publicarStatus(String cor, String estado);
void conectarMQTT();

const char* SSID = "SENAI EDUCACIONAL";
const char* SENHA = "info@134";

const char* BROKER = "broker.emqx.io";
const int PORTA = 1883;

const char* TOPICO_SUB =
"senai134/esp32/eduardoEgustavo/comando";

const char* TOPICO_PUB =
"senai134/esp32/eduardoEgustavo/status";

#define PINO_RGB 48

String corAtual = "DESLIGADO";

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

    Serial.printf("\n\rConectado a rede %s com sucesso!\n\r", SSID);
    Serial.printf("Conectado ao Broker %s\n\r", BROKER);

    mqtt.setServer(BROKER, PORTA);
    mqtt.setCallback(aoReceberMsg);

    mqtt.setBufferSize(256);

    String id = "esp32-" +
                String((uint32_t)ESP.getEfuseMac(), HEX);

    Serial.printf("ID do dispositivo: %s\n\r", id.c_str());

    bool conectadoBroker;
    conectadoBroker = mqtt.connect(id.c_str());

    if (conectadoBroker)
    {
        Serial.printf("Conectado ao Broker %s com sucesso!\n", BROKER);
        mqtt.subscribe(TOPICO_SUB);

        Serial.printf("Inscrito no tópico: %s\n\r", TOPICO_SUB);
    }
    else
    {
        Serial.printf("Falha ao conectar ao Broker %s\n\r", BROKER);
        Serial.printf("Falha: ");
        Serial.println(mqtt.state());
    }

    rgbLedWrite(PINO_RGB, 0, 0, 0);
}

void loop()
{
    mqtt.loop();

    if (!mqtt.connected())
    {
        Serial.println("Reconectando ao MQTT...");
        conectarMQTT();
    }
}

void aoReceberMsg(char* topico, byte* payLoad, unsigned int tamanho)
{
    String mensagem;
    mensagem.reserve(tamanho);

    for (unsigned int i = 0; i < tamanho; i++)
    {
        mensagem += (char)payLoad[i];
    }

    Serial.printf("Mensagem recebida no tópico: %s\n", topico);
    Serial.printf("Mensagem: %s\n", mensagem.c_str());

    tratarJson(mensagem);
}


void tratarJson(String mensagem)
{
    JsonDocument doc;

    DeserializationError erro = deserializeJson(doc, mensagem);

    if (erro)
    {
        Serial.println("Erro no JSON!");
        return;
    }

    if (!doc["cor"].is<const char*>())
    {
        Serial.println("Campo cor inválido!");
        return;
    }

    String cor = doc["cor"].as<String>();

    controlarRGB(cor);
}

void controlarRGB(String cor)
{
    cor.toUpperCase();

    if (cor == "VERMELHO")
    {
        rgbLedWrite(PINO_RGB, 255, 0, 0);
        corAtual = "VERMELHO";
        Serial.println("LED VERMELHO LIGADO");
    }
    else if (cor == "AZUL")
    {
        rgbLedWrite(PINO_RGB, 0, 0, 255);
        corAtual = "AZUL";
        Serial.println("LED AZUL LIGADO");
    }
    else if (cor == "VERDE")
    {
        rgbLedWrite(PINO_RGB, 0, 255, 0);
        corAtual = "VERDE";
        Serial.println("LED VERDE LIGADO");
    }
    else if (cor == "OFF")
    {
        rgbLedWrite(PINO_RGB, 0, 0, 0);
        corAtual = "DESLIGADO";
        Serial.println("LED DESLIGADO");
    }
    else
    {
        Serial.println("Cor invalida!");
        publicarStatus(cor, "ERRO");
        return;
    }

    publicarStatus(corAtual, "LIGADO");
}

void publicarStatus(String cor, String estado)
{
    JsonDocument doc;

    doc["dispositivo"] = "ESP32_RGB";
    doc["cor"] = cor;
    doc["estado"] = estado;

    String mensagem;

    serializeJson(doc, mensagem);

    if (mqtt.publish(TOPICO_PUB, mensagem.c_str()))
    {
        Serial.println("Status publicado:");
        Serial.println(mensagem);
    }
    else
    {
        Serial.println("Erro ao publicar status!");
    }
}

void conectarMQTT()
{
    while (!mqtt.connected())
    {
        String id = "esp32-" +
                    String((uint32_t)ESP.getEfuseMac(), HEX);

        if (mqtt.connect(id.c_str()))
        {
            Serial.println("Reconectado ao Broker!");

            mqtt.subscribe(TOPICO_SUB);
        }
        else
        {
            Serial.print("Falha MQTT: ");
            Serial.println(mqtt.state());

            delay(1000);
        }
    }
}