#include <SPI.h>

const int CS_PIN = 5; // Pino Chip Select (SS) do ESP32 conectado ao CS do LTC2874

// Endereços dos registradores do LTC2874
const byte REG_MODE2 = 0x9;   // Registrador MODE2 para SLEW e RETRY_CQ 
const byte REG_TMRCTRL = 0xC; // Registrador TMRCTRL para RETRYTC 
const byte REG_CTRL2 = 0xE;   // Registrador CTRL2 para SIO_MODE e ENL+ 
const byte REG_CTRL1 = 0xD;   // Registrador CTRL1 para DRVEN e WKUP


// Comandos SPI do LTC2874 
const byte CMD_WRITE = 0b001;  // Comando Write Register 
const byte CMD_WRTUPD = 0b011; // Comando Write One Register and Update All 

void setup() {
  Serial.begin(115200);
  Serial.println("Inicializando configuracao do LTC2874...");

  pinMode(CS_PIN, OUTPUT);
  digitalWrite(CS_PIN, HIGH); // Garante que o CS esteja HIGH inicialmente 

  // Inicializa a comunicação SPI
  SPI.begin();
  SPI.beginTransaction(SPISettings(20000000, MSBFIRST, SPI_MODE0));

  byte ctrl2_data = 0b00000001;
  sendSPICommand(CMD_WRTUPD, REG_CTRL2, ctrl2_data);
  Serial.println("Porta 1 configurada para SIO Mode.");

  byte mode2_data = 0b11101111;
  sendSPICommand(CMD_WRTUPD, REG_MODE2, mode2_data);
  Serial.println("Taxa de Slew da Porta 1 configurada para Lenta (3us).");

   mode2_data = 0b11110101;
  sendSPICommand(CMD_WRTUPD, REG_MODE2, mode2_data);
  Serial.println("Auto-retry habilitado para a Porta 1 CQ.");

  byte tmrctrl_data = 0b10000000;
  sendSPICommand(CMD_WRTUPD, REG_TMRCTRL, tmrctrl_data);
  Serial.println("Atraso de auto-retry configurado para 0.12s.");

  byte ctrl1_data = 0b00000001; 
  sendSPICommand(CMD_WRTUPD, REG_CTRL1, ctrl1_data);
  Serial.println("Driver da Porta 1 habilitado.");

  SPI.endTransaction();
  Serial.println("Configuracao do LTC2874 concluida.");
}

void loop() {
  // Ligar o LED: Definir DRVEN1 como 1.
  byte ctrl1_data_on = 0b00000001; // DRVEN1 = 1
  sendSPICommand(CMD_WRTUPD, REG_CTRL1, ctrl1_data_on);
  Serial.println("LED ligado (Porta 1 CQ: HIGH, assumindo TXD=LOW por default ou strapado)");
  delay(1000);

  // Desligar o LED: Definir DRVEN1 como 0.
  byte ctrl1_data_off = 0b00000000; // DRVEN1 = 0
  sendSPICommand(CMD_WRTUPD, REG_CTRL1, ctrl1_data_off);
  Serial.println("LED desligado (Porta 1 CQ: HIGH-Z)");
  delay(1000);
}

// Função para enviar um comando SPI para o LTC2874
void sendSPICommand(byte command, byte address, byte data) {
  uint16_t command_word = 0;

  command_word |= (command << 13); // Desloca o comando para os bits mais significativos

  command_word |= (address << 9);  // Desloca o endereço para a posição correta

  command_word |= data;

  digitalWrite(CS_PIN, LOW); // Ativa o Chip Select
  SPI.transfer16(command_word); // Envia a palavra de 16 bits
  digitalWrite(CS_PIN, HIGH); // Desativa o Chip Select
}
