#define ONLINE 1
#ifdef ONLINE
#include <PubSubClient.h>
#else
#include <SerialPubSubClient.h>
#endif

#include <UIPEthernet.h>

#include <SPI.h>
#include <utility/logging.h>
#include <Ultrasonic.h>

// Atualizar ultimo valor para ID do seu Kit para evitar duplicatas
const byte mac[] = { 0xDE, 0xED, 0xBA, 0xFE, 0xFE, 0x06 };

// Endereço do Cloud MQTT
const char* BROKER_MQTT = "test.mosquitto.org";

// Valor da porta do servidor MQTT. 1883 é o valor padrão. 18575
const int BROKER_PORT = 1883;

const char* TOPICO = "senai-code-xp/vagas/06";

#define ID_MQTT  "arduino-06"     //id mqtt (para identificação de sessão)

#define pinoVermelho A0
#define pinoVerde    A1
#define pinoConexao  A2
#define pinoEnvio    A3

Ultrasonic ultrasonic(8, 9);

int estadoPinoVerde = HIGH;
int estadoPinoVermelho = LOW;

long time = 0;
long timeCon = 0;
int intervalo = 1000;
int intervaloCon = 5000;

int estadoVaga = 0;

void callback(char* topic, byte* payload, unsigned int length) {

}

void turnLed(uint8_t pino, int state) {
  if (state) {
    digitalWrite(pino, HIGH);
  } else {
    digitalWrite(pino, LOW);
  }
}

EthernetClient ethClient;
PubSubClient client(BROKER_MQTT, BROKER_PORT, callback, ethClient);

void setup() {
  pinMode(pinoVermelho, OUTPUT);
  pinMode(pinoVerde, OUTPUT);
  pinMode(pinoConexao, OUTPUT);
  pinMode(pinoEnvio, OUTPUT);

  initSerial();
#ifdef ONLINE
  initEthernet();
#endif
  reconnectMQTT();

  if (client.connected()) { // Teste
    turnLed(pinoConexao, 1);
  }
}

void loop() {
  intervaloLeitura();
  verificaConexaoEMQTT(); //garante funcionamento da conexão ao broker MQTT
  client.loop();
}


void acionarLed(uint8_t pino) {
  digitalWrite(pino, HIGH);
  delay(20);
  digitalWrite(pino, LOW);
  delay(20);
  digitalWrite(pino, HIGH);
  delay(20);
  digitalWrite(pino, LOW);
}

void intervaloLeitura() {
  if (millis() - time > intervalo) {
    lerSensorUltrassonico();
    time = millis();
  }
}


void lerSensorUltrassonico() {
  int distancia = ultrasonic.distanceRead();
  int estadoLeitura;
  Serial.print("Distancia: "); Serial.println(distancia);

  if (distancia != 0) {
    if (distancia < 10) {
      estadoLeitura = 0;
    } else {
      estadoLeitura = 1;
    }
    if (estadoLeitura != estadoVaga) {
      enviarEstado(estadoLeitura);
      ligarLedVaga(estadoLeitura);
      estadoVaga = estadoLeitura;
    }
  }
}

void enviarEstado(int state) {
  if (client.connected()) {
    if (state) {
      if (client.publish(TOPICO, "1", 1)) {
        acionarLed(pinoEnvio);
      }
    } else {
      if (client.publish(TOPICO, "0", 1)) {
        acionarLed(pinoEnvio);
      }
    }
  }
}

void ligarLedVaga (int estadoVaga) {
  if (estadoVaga) {
    turnLed(pinoVermelho, 0);
    turnLed(pinoVerde, 1);
  } else {
    turnLed(pinoVermelho, 1);
    turnLed(pinoVerde, 0);
  }
}

//Função: inicializa comunicação serial com baudrate 9600 (para fins de monitorar no terminal serial o que está acontecendo).
void initSerial() {
  Serial.begin(9600);
  while (!Serial) {}
  Serial.println("Serial OK!");
}

//Função: inicializa e conecta-se na internet
void initEthernet() {
  
  if (!Ethernet.begin(mac)) {
    Serial.println("DHCP Falhou!");
  } else {
    Serial.println(Ethernet.localIP());
  }
}

//Função: reconecta-se ao broker MQTT (caso ainda não esteja conectado ou em caso de a conexão cair) em caso de sucesso na conexão ou reconexão, o subscribe dos tópicos é refeito.
void reconnectMQTT() {
  if (millis() - timeCon > intervaloCon) {
    Serial.print("* Tentando se conectar ao Broker MQTT: ");
    Serial.println(BROKER_MQTT);

    if (client.connect(ID_MQTT, NULL, NULL, TOPICO, 0, 1, "")) {
      Serial.println("Conectado com sucesso ao broker MQTT!");
      turnLed(pinoConexao, 1);
      Serial.flush();
    } else {
      Serial.println("Falha ao reconectar no broker.");
      Serial.println("Havera nova tentatica de conexao em 2s");
      Serial.flush();

      timeCon = millis();
      turnLed(pinoConexao, 0);
    }
  }
}

//Função: verifica o estado das conexões Ethernet e ao broker MQTT. Em caso de desconexão (qualquer uma das duas), a conexão é refeita.
void verificaConexaoEMQTT() {
  if (!client.connected()) {
    turnLed(pinoConexao, 0);
    reconnectMQTT(); //se não há conexão com o Broker, a conexão é refeita
  }
}
