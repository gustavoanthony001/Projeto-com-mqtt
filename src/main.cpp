/*
===================================================================
ATIVIDADE: Desafio de Projeto IoT - Comunicação entre ESP32
Data de Inicio: 04/11/26
Dupla: Gustavo Anthony & Eduardo do Carmo
===================================================================
*/

#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <LiquidCrystal_I2C.h>
#include <Botao.h>

LiquidCrystal_I2C lcd(0x27, 20, 4);

Botao btnVermelho(7);
Botao btnVerde(5);
Botao btnAzul(6);

#define PINO_RGB 48

void aoReceberMsg(char* topico, byte* payLoad, unsigned int tamanho);
void tratarJson(String mensagem);
void controlarRGB(String cor);
void publicarStatus(String cor, String estado);
void conectarMQTT();

const char* SSID  = "SENAI EDUCACIONAL";
const char* SENHA = "info@134";

const char* BROKER = "broker.emqx.io";
const int   PORTA  = 1883;

const char* TOPICO_SUB = "senai134/esp32/eduardoEgustavo/comando";
const char* TOPICO_PUB = "senai134/esp32/eduardoEgustavo/status";

String corAtual = "DESLIGADO";
String idDispositivo;

WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);

void setup()
{
  btnVermelho.iniciar();
  btnVerde.iniciar();
  btnAzul.iniciar();

  Serial.begin(115200);

  lcd.init();
  lcd.backlight();
  lcd.setCursor(3, 0);
  lcd.print("LED DESLIGADO");

  rgbLedWrite(PINO_RGB, 0, 0, 0);

  Serial.printf("Conectando a rede %s ", SSID);
  WiFi.begin(SSID, SENHA);

  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print("#");
    delay(500);
  }

  Serial.printf("\n\rConectado a rede %s com sucesso!\n\r", SSID);

  mqtt.setServer(BROKER, PORTA);
  mqtt.setCallback(aoReceberMsg);
  mqtt.setBufferSize(256);

  idDispositivo = "esp32-" + String((uint32_t)ESP.getEfuseMac(), HEX);
  Serial.printf("ID do dispositivo: %s\n\r", idDispositivo.c_str());

  conectarMQTT();
}

void loop()
{
  if (!mqtt.connected())
  {
    Serial.println("Reconectando ao MQTT...");
    conectarMQTT();
  }
  mqtt.loop();

  btnAzul.atualizar();
  btnVerde.atualizar();
  btnVermelho.atualizar();

  if (btnAzul.pressionou())
  {
    Serial.println("BOTAO AZUL CLICADO");
    controlarRGB("AZUL");
  }

  if (btnVerde.pressionou())
  {
    Serial.println("BOTAO VERDE CLICADO");
    controlarRGB("VERDE");
  }

  if (btnVermelho.pressionou())
  {
    Serial.println("BOTAO VERMELHO CLICADO");
    controlarRGB("VERMELHO");
  }
}

void controlarRGB(String cor)
{
  cor.toUpperCase();
  cor.trim();

  lcd.clear();

  if (cor == "VERMELHO")
  {
    rgbLedWrite(PINO_RGB, 255, 0, 0);
    lcd.setCursor(4, 0);  lcd.print("LED LIGADO");
    lcd.setCursor(0, 2);  lcd.print("COR VERMELHA LIGADA");
    corAtual = "VERMELHO";
  }
  else if (cor == "VERDE")
  {
    rgbLedWrite(PINO_RGB, 0, 255, 0);
    lcd.setCursor(4, 0);  lcd.print("LED LIGADO");
    lcd.setCursor(0, 2);  lcd.print("COR VERDE LIGADA");
    corAtual = "VERDE";
  }
  else if (cor == "AZUL")
  {
    rgbLedWrite(PINO_RGB, 0, 0, 255);
    lcd.setCursor(4, 0);  lcd.print("LED LIGADO");
    lcd.setCursor(0, 2);  lcd.print("COR AZUL LIGADA");
    corAtual = "AZUL";
  }
  else if (cor == "DESLIGADO" || cor == "OFF")
  {
    rgbLedWrite(PINO_RGB, 0, 0, 0);
    lcd.setCursor(3, 0);  lcd.print("LED DESLIGADO");
    corAtual = "DESLIGADO";
  }
  else
  {
    Serial.printf("Cor desconhecida: %s\n\r", cor.c_str());
    lcd.setCursor(0, 0);  lcd.print("COR INVALIDA");
    return;
  }

  publicarStatus(corAtual, corAtual == "DESLIGADO" ? "desligado" : "ligado");
}

void publicarStatus(String cor, String estado)
{
  JsonDocument doc;
  doc["cor"]    = cor;
  doc["estado"] = estado;

  char buffer[128];
  serializeJson(doc, buffer);

  mqtt.publish(TOPICO_PUB, buffer);
  Serial.printf("Status publicado: %s\n\r", buffer);
}

void tratarJson(String mensagem)
{
  mensagem.trim();
  Serial.printf("Payload bruto: [%s]\n\r", mensagem.c_str());

  JsonDocument doc;
  DeserializationError erro = deserializeJson(doc, mensagem);

  if (erro)
  {
    Serial.printf("Nao e JSON valido (%s), usando texto puro\n\r", erro.c_str());
    controlarRGB(mensagem);
    return;
  }

  JsonVariant valor = doc["cor"];
  if (valor.isNull()) valor = doc["color"];
  if (valor.isNull()) valor = doc["led"];

  if (valor.isNull())
  {
    Serial.println("JSON sem o campo 'cor'");
    return;
  }

  String cor = valor.as<String>();
  cor.trim();
  cor.toUpperCase();

  if (cor == "VERMELHA" || cor == "RED")        cor = "VERMELHO";
  else if (cor == "GREEN")                      cor = "VERDE";
  else if (cor == "BLUE")                       cor = "AZUL";
  else if (cor == "APAGADO" || cor == "OFF")    cor = "DESLIGADO";

  Serial.printf("Cor interpretada: %s\n\r", cor.c_str());
  controlarRGB(cor);
}

// Callback: chamado quando chega mensagem no tópico inscrito
// ---------------------------------------------------------------
void aoReceberMsg(char* topico, byte* payLoad, unsigned int tamanho)
{
  String mensagem;
  for (unsigned int i = 0; i < tamanho; i++)
  {
    mensagem += (char)payLoad[i];
  }

  Serial.printf("Mensagem recebida [%s]: %s\n\r", topico, mensagem.c_str());
  tratarJson(mensagem);
}

void conectarMQTT()
{
  while (!mqtt.connected())
  {
    Serial.printf("Conectando ao Broker %s...\n\r", BROKER);

    if (mqtt.connect(idDispositivo.c_str()))
    {
      Serial.println("Conectado ao Broker com sucesso!");
      mqtt.subscribe(TOPICO_SUB);
      Serial.printf("Inscrito no topico: %s\n\r", TOPICO_SUB);
      publicarStatus(corAtual, "online");
    }
    else
    {
      Serial.printf("Falha, rc=%d. Nova tentativa em 3s\n\r", mqtt.state());
      delay(3000);
    }
  }
}
