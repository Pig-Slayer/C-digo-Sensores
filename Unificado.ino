#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BNO055.h>
#include <utility/imumaths.h>
#include <Adafruit_BMP280.h>
#include <TinyGPSPlus.h>
#include <HardwareSerial.h>

// --- Definições de Pinos ---
// BNO055 (I2C Principal - Wire)
#define I2C_SDA_BNO 21
#define I2C_SCL_BNO 22

// BMP280 (I2C Secundário - Wire1)
#define I2C_SDA_BMP 25
#define I2C_SCL_BMP 26

// GPS Neo 6M (UART2)
#define RXD2 16
#define TXD2 17

// --- Instanciação dos Objetos ---
// Passamos o endereço de Wire para o BNO e Wire1 para o BMP
Adafruit_BNO055 bno = Adafruit_BNO055(55, 0x28, &Wire);
Adafruit_BMP280 bmp(&Wire1); 

TinyGPSPlus gps;
HardwareSerial gpsSerial(2);

// --- Variáveis Globais ---
float altitudeInicial = 0.0;
unsigned long tempoAnterior = 0;
const unsigned long intervalo = 10; // Taxa de atualização de 10ms (100Hz)

void setup() {
  Serial.begin(115200); 
  while (!Serial) delay(10);

  // 1. Inicialização do BNO055 (Wire)
  Wire.begin(I2C_SDA_BNO, I2C_SCL_BNO);
  if (!bno.begin()) {
    Serial.println(F("Erro: Nenhum sensor BNO055 detectado!"));
    while (1);
  }
  bno.setExtCrystalUse(true);

  // 2. Inicialização do BMP280 (Wire1)
  Wire1.begin(I2C_SDA_BMP, I2C_SCL_BMP);
  Wire1.setClock(400000);
  if (!bmp.begin(0x76)) {
    Serial.println(F("Erro: Sensor BMP280 nao encontrado!"));
    while (1);
  }
  
  // Configuração do filtro do BMP280
  bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,     
                  Adafruit_BMP280::SAMPLING_X1,   
                  Adafruit_BMP280::SAMPLING_X1,     
                  Adafruit_BMP280::FILTER_X4,      
                  Adafruit_BMP280::STANDBY_MS_1);
                  
  // Descarta as primeiras 20 leituras para o filtro estabilizar
  for(int i = 0; i < 20; i++) {
    bmp.readAltitude(1013.25);
    delay(10);
  }
  // Define o marco zero da altitude
  altitudeInicial = bmp.readAltitude(1013.25);

  // 3. Inicialização do GPS
  gpsSerial.begin(9600, SERIAL_8N1, RXD2, TXD2);
  
  Serial.println(F("Inicialização Concluída. Iniciando telemetria..."));
  Serial.println(F("Yaw,Pitch,Roll,AltitudeRelativa,Lat,Lng")); // Cabeçalho
}

void loop() {
  // Alimenta o objeto GPS em segundo plano continuamente
  while (gpsSerial.available() > 0) {
    gps.encode(gpsSerial.read());
  }

  // Bloco de disparo a cada 10ms
  if (millis() - tempoAnterior >= intervalo) {
    tempoAnterior = millis();

    // --- Leitura do BNO055 ---
    sensors_event_t event;
    bno.getEvent(&event);
    float yaw = event.orientation.x;
    float pitch = event.orientation.y;
    float roll = event.orientation.z;

    // --- Leitura do BMP280 ---
    float alturaRelativa = bmp.readAltitude(1013.25) - altitudeInicial;

    // --- Impressão dos Dados (Formato CSV) ---
    Serial.print(yaw);   Serial.print(",");
    Serial.print(pitch); Serial.print(",");
    Serial.print(roll);  Serial.print(",");
    Serial.print(alturaRelativa); Serial.print(",");

    // --- Leitura do GPS ---
    if (gps.location.isValid()) {
      Serial.print(gps.location.lat(), 6);
      Serial.print(",");
      Serial.println(gps.location.lng(), 6); // Quebra de linha encerra o pacote
    } else {
      // Caso não tenha fixado satélite, imprime zeros para manter a estrutura do CSV
      Serial.println("0.000000,0.000000"); 
    }
  }
}