#include <SPI.h> // Inclui a biblioteca SPI

// Definição dos pinos SPI do ESP32
// Verifique a pinagem específica do seu modelo de ESP32.
// Para muitos ESP32 DevKitC, os pinos padrão SPI são:
// SCK: GPIO18
// MISO: GPIO19
// MOSI: GPIO23
// SS (Chip Select): Escolha um GPIO não utilizado (ex: GPIO5)

const int CS_PIN = 5; // Pino Chip Select (SS) do ESP32 conectado ao CS do LTC2874

// Endereços dos registradores do LTC2874 (baseado na folha de dados [cite: 301, 304])
const byte REG_MODE2 = 0x9;   // Registrador MODE2 para SLEW e RETRY_CQ [cite: 301]
const byte REG_TMRCTRL = 0xC; // Registrador TMRCTRL para RETRYTC [cite: 304]
const byte REG_CTRL2 = 0xE;   // Registrador CTRL2 para SIO_MODE e ENL+ [cite: 304]
const byte REG_CTRL1 = 0xD;   // Registrador CTRL1 para DRVEN e WKUP [cite: 304]


// Comandos SPI do LTC2874 (baseado na folha de dados [cite: 278])
const byte CMD_WRITE = 0b001;  // Comando Write Register (No Update) [cite: 278]
const byte CMD_WRTUPD = 0b011; // Comando Write One Register and Update All [cite: 278]

void setup() {
  Serial.begin(115200);
  Serial.println("Inicializando configuracao do LTC2874...");

  pinMode(CS_PIN, OUTPUT);
  digitalWrite(CS_PIN, HIGH); // Garante que o CS esteja HIGH inicialmente (LTC2874 inativo)

  // Inicializa a comunicação SPI
  // Frequência de clock (até 20MHz para o LTC2874 [cite: 31]), MSB primeiro, SPI Mode 0 (CPOL=0, CPHA=0)
  SPI.begin();
  SPI.beginTransaction(SPISettings(20000000, MSBFIRST, SPI_MODE0));

  // --- Configuração do LTC2874 para a Porta 1 ---

  // 1. Configurar Modo SIO para Porta 1 (SIO_MODE1 = 1) [cite: 144]
  // Registrador CTRL2 (0xE), bit D0 (SIO_MODE1)
  // Valor default do CTRL2 é 0000,0000[cite: 304]. Queremos apenas SIO_MODE1 em 1.
  byte ctrl2_data = 0b00000001; // SIO_MODE1 = 1 (para Porta 1) [cite: 304]
  sendSPICommand(CMD_WRTUPD, REG_CTRL2, ctrl2_data);
  Serial.println("Porta 1 configurada para SIO Mode.");

  // 2. Definir taxa de Slew para Lenta (SLEW1 = 0) [cite: 144]
  // Registrador MODE2 (0x9), bit D4 (SLEW1)
  // Valor default do MODE2 é 1111,0100[cite: 301]. Precisamos mudar o bit D4 para 0.
  // Vamos definir apenas os bits relevantes para o exemplo e manter outros no padrão ou como 0.
  // Bits D7:D4 são para SLEW4:SLEW1. Se quisermos apenas SLEW1=0 e o resto como default (1)
  // O default de MODE2 é 0xF4, onde os bits SLEW são 1111.
  // Para SLEW1=0, precisamos que o bit D4 seja 0. Se o resto for 1, seria 1110.
  byte mode2_data = 0b11101111; // SLEW1=0, SLEW2=1, SLEW3=1, SLEW4=1. [cite: 301, 144] O resto dos bits é mantido como default.
  sendSPICommand(CMD_WRTUPD, REG_MODE2, mode2_data);
  Serial.println("Taxa de Slew da Porta 1 configurada para Lenta (3us).");

  // 3. Habilitar auto-retry para CQ (RETRY_CQ = 1) [cite: 247]
  // Registrador MODE2 (0x9), bit D0 (RETRY_CQ)
  // O valor default de MODE2 é 1111,0100[cite: 301].
  // Para habilitar RETRY_CQ (bit D0), o novo valor seria 1111,0101.
  mode2_data = 0b11110101; // RETRY_CQ = 1, resto dos bits conforme default [cite: 301, 247]
  sendSPICommand(CMD_WRTUPD, REG_MODE2, mode2_data);
  Serial.println("Auto-retry habilitado para a Porta 1 CQ.");

  // 4. Definir atraso de auto-retry para o mais rápido (RETRYTC = 000 = 0.12s) [cite: 250, 304]
  // Registrador TMRCTRL (0xC), bits D2:D0 (RETRYTC)
  // Valor default do TMRCTRL é 1000,0101[cite: 304]. O default de RETRYTC é 101 (3.9s)[cite: 304].
  // Queremos 000 (0.12s) para RETRYTC.
  byte tmrctrl_data = 0b10000000; // LPTC=1000, RETRYTC=000 [cite: 304]
  sendSPICommand(CMD_WRTUPD, REG_TMRCTRL, tmrctrl_data);
  Serial.println("Atraso de auto-retry configurado para 0.12s.");

  // 5. Habilitar o driver da Porta 1 (DRVEN1 = 1) [cite: 131]
  // Registrador CTRL1 (0xD), bit D0 (DRVEN1)
  // Valor default do CTRL1 é 0000,0000[cite: 304]. Queremos apenas DRVEN1 em 1.
  byte ctrl1_data = 0b00000001; // DRVEN1 = 1 (para Porta 1) [cite: 304]
  sendSPICommand(CMD_WRTUPD, REG_CTRL1, ctrl1_data);
  Serial.println("Driver da Porta 1 habilitado.");

  SPI.endTransaction();
  Serial.println("Configuracao do LTC2874 concluida.");
}

void loop() {
  // Ligar o LED (TXD1 = HIGH, pois a polaridade é invertida no CQ [cite: 62])
  // Para ligar o LED no CQ1, precisamos que o TXD1 seja HIGH, o que fará o CQ1 ir para LOW.
  // A folha de dados indica que o CQ é invertido em polaridade em relação ao TXD[cite: 62].
  // Portanto, para ter um "HIGH" na linha CQ e acender o LED (ânodo no CQ, cátodo no resistor -> GND),
  // o TXD precisa ser "LOW". Se o LED for ligado entre CQ e VDD com resistor, e for de alta corrente,
  // ou entre CQ e GND, a lógica pode variar.
  // Assumindo um LED com ânodo no CQ1 e cátodo via resistor ao GND:
  // Para ligar o LED, CQ1 deve ir HIGH (proximo a VDD). Isso requer TXD1 LOW.
  // Para desligar o LED, CQ1 deve ir LOW (proximo a GND). Isso requer TXD1 HIGH.
  // VAMOS ASSUMIR QUE LIGAR O LED SIGNIFICA PUXAR O CQ PARA BAIXO (conexão padrão de um sensor IO-Link)
  // E que o LED está conectado entre L+ e CQ com um resistor.
  // L+ estará habilitado via o Hot Swap Controller.
  // Portanto, para acender o LED, o pino CQ deve ir para LOW (TXD para HIGH).

  // A maneira mais simples de testar um LED é entre CQ e GND com um resistor.
  // Neste caso: TXD HIGH -> CQ LOW (LED OFF se pino TXEN1 está controlando o LED)
  // TXD LOW -> CQ HIGH (LED ON se TXEN1 está controlando o LED)

  // Vamos reinterpretar o controle do LED.
  // O pino CQ é uma saída referenciada a GND, invertida em polaridade com respeito ao TXD[cite: 62].
  // Se você tem um LED com ânodo no CQ e cátodo para GND via resistor:
  //   Para acender o LED (CQ = VDD, ou alto): TXD deve ser LOW.
  //   Para apagar o LED (CQ = GND, ou baixo): TXD deve ser HIGH.

  // Escrevendo no registrador CTRL1 (0xD), bit D0 (DRVEN1) [cite: 304]
  // Este é o método de controle via SPI, o que é mais robusto para a maioria das aplicações.
  // Se o TXEN1 (pino 22/18) estiver strapado para GND (desabilitado via pino),
  // então o DRVEN1 (bit D0 do CTRL1) controla a habilitação do driver.

  // Ligar o LED: Definir DRVEN1 como 1.
  byte ctrl1_data_on = 0b00000001; // DRVEN1 = 1 [cite: 304]
  sendSPICommand(CMD_WRTUPD, REG_CTRL1, ctrl1_data_on);
  Serial.println("LED ligado (Porta 1 CQ: HIGH, assumindo TXD=LOW por default ou strapado)");
  delay(1000);

  // Desligar o LED: Definir DRVEN1 como 0.
  byte ctrl1_data_off = 0b00000000; // DRVEN1 = 0 [cite: 304]
  sendSPICommand(CMD_WRTUPD, REG_CTRL1, ctrl1_data_off);
  Serial.println("LED desligado (Porta 1 CQ: HIGH-Z)");
  delay(1000);
}

// Função para enviar um comando SPI para o LTC2874
void sendSPICommand(byte command, byte address, byte data) {
  uint16_t command_word = 0;

  // Monta a palavra de 16 bits: [C2:C0] [A3:A0] [X] [D7:D0]
  // C2:C0 (3 bits de comando)
  command_word |= (command << 13); // Desloca o comando para os bits mais significativos

  // A3:A0 (4 bits de endereço)
  command_word |= (address << 9);  // Desloca o endereço para a posição correta

  // X (1 bit don't care, 0 ou 1) - aqui definimos como 0
  // command_word |= (0 << 8); // Já é 0 por default

  // D7:D0 (8 bits de dados)
  command_word |= data;

  digitalWrite(CS_PIN, LOW); // Ativa o Chip Select
  SPI.transfer16(command_word); // Envia a palavra de 16 bits
  digitalWrite(CS_PIN, HIGH); // Desativa o Chip Select
}
