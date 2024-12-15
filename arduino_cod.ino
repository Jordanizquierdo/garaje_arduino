#include <SPI.h>
#include <MFRC522.h>
#include <WiFi.h>
#include <ESP32Servo.h>
#include <WebServer.h>
#include <HTTPClient.h>

// Pines RC522
#define SS_PIN 5
#define RST_PIN 22

// Pines del servomotor
#define SERVO_PIN 2 // GPIO2 para la señal del servomotor

// Crear instancia del RC522
MFRC522 rfid(SS_PIN, RST_PIN);

// Configuración WiFi
const char* ssid = "Jordan";
const char* password = "cualquiera1";

// Crear instancia del servomotor
Servo servo;

// Crear instancia del servidor web en el puerto 80
WebServer server(80);

// UID válido para activar el servomotor
String uidValido = "3AF6AE2";

// API Key de ThingSpeak
const String apiKey = "N07RF2DD66BDYTL8";
const String thingSpeakServer = "http://api.thingspeak.com/update";

// Función para conectar al WiFi
void conectarWiFi() {
  Serial.print("Conectando a WiFi");
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nConectado a la red WiFi");
  Serial.print("Dirección IP: ");
  Serial.println(WiFi.localIP());
}

// Funciones para mover el servomotor
void moverServo() {
  Serial.println("Comando recibido: Abrir servomotor.");
  for (int angulo = 0; angulo <= 90; angulo += 10) {
    servo.write(angulo);
    delay(100);
  }
  server.send(200, "text/plain", "Servo movido a posición abierta");
}

void cerrarServo() {
  Serial.println("Comando recibido: Cerrar servomotor.");
  for (int angulo = 90; angulo >= 0; angulo -= 10) {
    servo.write(angulo);
    delay(100);
  }
  server.send(200, "text/plain", "Servo movido a posición cerrada");
}

// Función para enviar datos a ThingSpeak
void enviarThingSpeak(String uid) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String url = thingSpeakServer + "?api_key=" + apiKey + "&field1=" + uid;

    http.begin(url);
    int httpResponseCode = http.GET();

    if (httpResponseCode > 0) {
      Serial.print("Datos enviados a ThingSpeak. Código de respuesta: ");
      Serial.println(httpResponseCode);
    } else {
      Serial.print("Error al enviar datos. Código: ");
      Serial.println(httpResponseCode);
    }

    http.end();
  } else {
    Serial.println("WiFi no conectado.");
  }
}

// Configuración inicial
void setup() {
  // Iniciar comunicación serial
  Serial.begin(115200);
  Serial.println("Iniciando sistema...");

  // Conectar a WiFi
  conectarWiFi();

  // Inicializar SPI para RC522
  SPI.begin(18, 19, 23, SS_PIN);
  rfid.PCD_Init();
  Serial.println("Lector RFID RC522 inicializado.");

  // Configurar el servomotor
  servo.attach(SERVO_PIN, 500, 2400);
  servo.write(0); // Posición inicial

  // Configurar rutas para controlar el servomotor desde el navegador
  server.on("/moverServo", moverServo);
  server.on("/cerrarServo", cerrarServo);

  // Configurar ruta de error para otras solicitudes
  server.onNotFound([]() {
    server.send(404, "text/plain", "Ruta no encontrada");
  });

  // Iniciar servidor web
  server.begin();
  Serial.println("Servidor web iniciado.");
  Serial.print("IP del servidor: ");
  Serial.println(WiFi.localIP());
}

// Bucle principal
void loop() {
  // Comprobar si hay una nueva tarjeta presente
  if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
    String uid = "";
    for (byte i = 0; i < rfid.uid.size; i++) {
      uid += String(rfid.uid.uidByte[i], HEX);
    }
    uid.toUpperCase();
    Serial.print("UID detectado: ");
    Serial.println(uid);

    // Verificar si el UID es válido
    if (uid == uidValido) {
      Serial.println("UID válido detectado, activando servomotor...");
      enviarThingSpeak(uid);
      moverServo();
      delay(10000);
      cerrarServo();
      
    } else {
      Serial.println("UID no válido.");
      // Enviar UID a ThingSpeak
      enviarThingSpeak(uid);
    }

    

    rfid.PICC_HaltA(); // Detener lectura de la tarjeta
  }

  // Manejar solicitudes del servidor web
  server.handleClient();
  delay(500); // Pausa para evitar múltiples lecturas
}