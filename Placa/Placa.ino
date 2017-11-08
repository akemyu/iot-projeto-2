#include <PubSubClient.h>
#include <UIPEthernet.h>
#include <utility/logging.h>
#include <SPI.h>
#include <LiquidCrystal.h>

#define ID_MQTT  "arduino-05"     //id mqtt (para identificação de sessão)

#define pinoConexao  A2  // Led conexão
#define pinoRecebe   A3

// Atualizar ultimo valor para ID do seu Kit para evitar duplicatas
const byte mac[] = { 0xDE, 0xED, 0xBA, 0xFE, 0xFE, 0x05 };

// Endereço do Cloud MQTT
const char* BROKER_MQTT = "test.mosquitto.org";

// Valor da porta do servidor MQTT. 1883 é o valor padrão. 18575
const int BROKER_PORT = 1883;

// Tópico inscrito
const char* TOPICO = "senai-code-xp/vagas/06";
String TOPICO_GERAL = "senai-code-xp/vagas/";

// Pinos LCD
const int en = 6, d4 = 5, d5 = 4, d6 = 3, d7 = 2;
#define rs A5
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

// Tempo para reconexão
long timeCon = 0;
int intervaloCon = 2000;

// Tempo para desligar display
long timeDisplay = 0;
int intervaloDisplay = 10000;

// Tempo para verificar vagas
long timeVagas = 0;
int intervaloVagas = 1000;


// Vagas
int numeroVagas = 0;
int estadoVaga06 = -1;

char message_buff[100];

void callback(char* topic, byte* payload, unsigned int length) {

  Serial.println("Recebeu da fila: ");
  acionarLed(pinoRecebe);

  Serial.println(topic);
  String str_topic = topic;
  //str_topic = str_topic.substring(TOPICO_GERAL.length()-1);
  Serial.println("Topico convertido:");
  Serial.println(str_topic);

  if (strcmp(topic,TOPICO)==0){
    Serial.println("Topico OK!");
  }
  
  char* payloadAsChar = payload;
  // Workaround para pequeno bug na biblioteca
  payloadAsChar[length] = 0;

  // Converter em tipo String para conveniência
  String msg = String(payloadAsChar);
  estadoVaga06 = msg.toInt();


}

EthernetClient ethClient;
PubSubClient client(BROKER_MQTT, BROKER_PORT, callback, ethClient);


void setup() {
  initSerial();
  initEthernet();

  pinMode(pinoConexao, OUTPUT);
  pinMode(pinoRecebe, OUTPUT);

  reconnectMQTT();

  if (client.connected()) { // Teste
    turnLed(pinoConexao, 1);
  }

  // Inicializando LCD (Colunas e linhas)
  lcd.begin(16, 2);
  // Print a message to the LCD.

}

void loop() {
  verificaConexaoEMQTT(); //garante funcionamento da conexão ao broker MQTT
  client.loop();
  // set the cursor to column 0, line 1
  // (note: line 1 is the second row, since counting begins with 0):

  imprimeVagas();
}

void imprimeVagas() {
  if (millis() - timeVagas > intervaloVagas) {
    if (estadoVaga06 == 1) {
      numeroVagas = 1;
    }
    else if (estadoVaga06 == 0) {
      numeroVagas = 0;
    }
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Vaga 06:");
    lcd.setCursor(0, 1);
    lcd.print(numeroVagas);
    timeVagas = millis();
  }
}

//Função: inicializa comunicação serial com baudrate 9600 (para fins de monitorar no terminal serial o que está acontecendo).
void initSerial() {
  Serial.begin(9600);
  while (!Serial) {}
  Serial.println("Serial OK!");
}


//Função: reconecta-se ao broker MQTT (caso ainda não esteja conectado ou em caso de a conexão cair) em caso de sucesso na conexão ou reconexão, o subscribe dos tópicos é refeito.
void reconnectMQTT() {
  if (millis() - timeCon > intervaloCon) {
    Serial.print("* Tentando se conectar ao Broker MQTT: ");
    Serial.println(BROKER_MQTT);

    if (client.connect(ID_MQTT, NULL, NULL)) {
      Serial.println("Conectado com sucesso ao broker MQTT!");
      turnLed(pinoConexao, 1);
      Serial.flush();

      if (client.subscribe(TOPICO)) {
        Serial.print("Inscrito em: "); Serial.println(TOPICO);
      }

    } else {
      Serial.println("Falha ao reconectar no broker.");
      Serial.println("Havera nova tentatica de conexao em 2s");
      Serial.flush();
      timeCon = millis();
      turnLed(pinoConexao, 0);

    }
  }
}

// Liga ou desliga led
void turnLed(uint8_t pino, int state) {
  if (state) {
    digitalWrite(pino, HIGH);
  } else {
    digitalWrite(pino, LOW);
  }
}

// Pisca led
void acionarLed(uint8_t pino) {
  digitalWrite(pino, HIGH);
  delay(20);
  digitalWrite(pino, LOW);
  delay(20);
  digitalWrite(pino, HIGH);
  delay(20);
  digitalWrite(pino, LOW);
}


//Função: verifica o estado das conexões Ethernet e ao broker MQTT. Em caso de desconexão (qualquer uma das duas), a conexão é refeita.
void verificaConexaoEMQTT() {
  if (!client.connected()) {
    turnLed(pinoConexao, 0);
    reconnectMQTT(); //se não há conexão com o Broker, a conexão é refeita
  }
}

void initEthernet() {
  Serial.println("Iniciando Ethernet!");
  if (!Ethernet.begin (mac)) {
    Serial.println("DHCP falhou!");
  } else {
    Serial.println(Ethernet.localIP());
  }
}

