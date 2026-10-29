// =============================================================================
//  IoT ejemplo: pulsador (sensor) y LED (actuador) con ESP32
// =============================================================================
//  La ESP32 es siempre la que inicia la comunicación con el backend:
//    - Cada INTERVALO_LECTURA_MS envía el estado del pulsador:
//        POST /api/lecturas            {"sensor": "pulsador", "valor": 1}
//    - Cada INTERVALO_ACTUADOR_MS pregunta cómo debe estar el LED:
//        GET  /api/actuador            ->  {"encendido": true}
//    - Cada vez que se presiona el pulsador, le pide al backend invertir el LED:
//        POST /api/actuador/alternar   ->  {"encendido": false}
//
//  El backend es la única fuente de verdad del LED. El pulsador no enciende
//  el LED directamente: le avisa al backend, y el LED cambia con la respuesta.
//  Así la página web y el LED siempre coinciden, cambie quien cambie el estado.
//
//  Conexiones:
//    - Pulsador entre GPIO 4 y GND (se usa la resistencia pull-up interna,
//      así que presionado = LOW).
//    - LED en GPIO 2. En muchas placas ESP32 DevKit es el LED azul integrado;
//      si se usa uno externo, va con su resistencia (220 a 330 ohm) a GND.
//
//  Bibliotecas:
//    - WiFi y HTTPClient: vienen con el paquete de placas "esp32" de
//      Espressif (Gestor de tarjetas del IDE de Arduino).
//    - ArduinoJson (de Benoit Blanchon), versión 7: se instala desde el
//      Gestor de bibliotecas.
//
//  Antes de compilar: copiar config.example.h como config.h y llenarlo.
// =============================================================================

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

#include "config.h"

const int PIN_PULSADOR = 4;
const int PIN_LED = 2;

const unsigned long INTERVALO_LECTURA_MS = 1000;
const unsigned long INTERVALO_ACTUADOR_MS = 300;
// Tiempo que la entrada debe quedarse quieta para aceptar un cambio. Los
// contactos de un pulsador rebotan unos milisegundos al presionarlo.
const unsigned long ANTIRREBOTE_MS = 50;
const int TIMEOUT_HTTP_MS = 3000;

unsigned long ultimaLectura = 0;
unsigned long ultimaConsultaActuador = 0;

// Estado del antirrebote
int lecturaAnterior = HIGH;            // última lectura cruda del pin
int estadoPulsador = HIGH;             // estado ya filtrado (HIGH = suelto)
unsigned long ultimoCambioPulsador = 0;

// -----------------------------------------------------------------------------
//  WiFi
// -----------------------------------------------------------------------------

void conectarWiFi() {
  Serial.print("Conectando a la red ");
  Serial.print(WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.print("Conectada. IP de la ESP32: ");
  Serial.println(WiFi.localIP());
}

// -----------------------------------------------------------------------------
//  Actuador: aplicar la respuesta del backend
// -----------------------------------------------------------------------------

// Lee un JSON como {"encendido": true} y deja el LED en ese estado.
void aplicarEstadoLed(const String &respuesta) {
  JsonDocument documento;
  DeserializationError error = deserializeJson(documento, respuesta);
  if (error) {
    Serial.print("Respuesta JSON inválida: ");
    Serial.println(error.c_str());
    return;
  }

  bool encendido = documento["encendido"];
  digitalWrite(PIN_LED, encendido ? HIGH : LOW);
}

// -----------------------------------------------------------------------------
//  Sensor: enviar una lectura al backend
// -----------------------------------------------------------------------------

void enviarLectura() {
  int presionado = (estadoPulsador == LOW) ? 1 : 0;

  // Armar el JSON: {"sensor": "pulsador", "valor": 0 o 1}
  JsonDocument documento;
  documento["sensor"] = "pulsador";
  documento["valor"] = presionado;
  String cuerpo;
  serializeJson(documento, cuerpo);

  HTTPClient http;
  http.begin(String(SERVIDOR_URL) + "/api/lecturas");
  http.setTimeout(TIMEOUT_HTTP_MS);
  http.addHeader("Content-Type", "application/json");
  int codigo = http.POST(cuerpo);
  http.end();

  if (codigo != 201) {
    // Un código negativo es un error de conexión (backend apagado, IP
    // equivocada, firewall); uno positivo es la respuesta HTTP del backend.
    Serial.print("Error al enviar la lectura: ");
    Serial.println(codigo);
  }
}

// -----------------------------------------------------------------------------
//  Actuador: preguntar al backend cómo debe estar el LED
// -----------------------------------------------------------------------------

void consultarActuador() {
  HTTPClient http;
  http.begin(String(SERVIDOR_URL) + "/api/actuador");
  http.setTimeout(TIMEOUT_HTTP_MS);
  int codigo = http.GET();

  if (codigo == 200) {
    aplicarEstadoLed(http.getString());
  } else {
    Serial.print("Error al consultar el actuador: ");
    Serial.println(codigo);
  }

  http.end();
}

// -----------------------------------------------------------------------------
//  Pulsador: pedirle al backend que invierta el LED
// -----------------------------------------------------------------------------

void alternarActuador() {
  HTTPClient http;
  http.begin(String(SERVIDOR_URL) + "/api/actuador/alternar");
  http.setTimeout(TIMEOUT_HTTP_MS);
  int codigo = http.POST("");

  if (codigo == 200) {
    // El backend responde con el estado nuevo: se aplica de una vez, sin
    // esperar a la próxima consulta.
    aplicarEstadoLed(http.getString());
  } else {
    Serial.print("Error al alternar el actuador: ");
    Serial.println(codigo);
  }

  http.end();
}

// Revisa el pulsador con antirrebote. Devuelve true solo en el instante en
// que se presiona (flanco de suelto a presionado), no mientras se mantiene.
bool pulsadorRecienPresionado() {
  int lectura = digitalRead(PIN_PULSADOR);
  unsigned long ahora = millis();

  if (lectura != lecturaAnterior) {
    // La entrada se movió: se reinicia la cuenta del antirrebote.
    ultimoCambioPulsador = ahora;
    lecturaAnterior = lectura;
  }

  if (ahora - ultimoCambioPulsador >= ANTIRREBOTE_MS && lectura != estadoPulsador) {
    // La entrada lleva ANTIRREBOTE_MS quieta en un valor nuevo: cambio real.
    estadoPulsador = lectura;
    return estadoPulsador == LOW;
  }

  return false;
}

// -----------------------------------------------------------------------------
//  Programa principal
// -----------------------------------------------------------------------------

void setup() {
  Serial.begin(115200);
  pinMode(PIN_PULSADOR, INPUT_PULLUP);
  pinMode(PIN_LED, OUTPUT);
  digitalWrite(PIN_LED, LOW);

  conectarWiFi();
}

void loop() {
  // Si se cayó el WiFi, se reconecta antes de seguir.
  if (WiFi.status() != WL_CONNECTED) {
    conectarWiFi();
  }

  if (pulsadorRecienPresionado()) {
    alternarActuador();
  }

  // millis() en lugar de delay(): así cada tarea corre a su ritmo sin
  // bloquear a las demás (incluida la lectura del pulsador).
  unsigned long ahora = millis();

  if (ahora - ultimaLectura >= INTERVALO_LECTURA_MS) {
    ultimaLectura = ahora;
    enviarLectura();
  }

  if (ahora - ultimaConsultaActuador >= INTERVALO_ACTUADOR_MS) {
    ultimaConsultaActuador = ahora;
    consultarActuador();
  }
}
