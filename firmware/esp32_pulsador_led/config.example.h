// Copie este archivo como "config.h" en esta misma carpeta y llene sus datos.
// config.h NO se sube a GitHub (está en .gitignore): ahí van las claves.

#pragma once

// Red WiFi de 2.4 GHz. La ESP32 no se conecta a redes de 5 GHz ni a redes
// con página de inicio de sesión (portal cautivo).
#define WIFI_SSID     "nombre-de-la-red"
#define WIFI_PASSWORD "clave-de-la-red"

// Dirección del computador donde corre el backend (docker compose up).
// En Windows se averigua con "ipconfig" (Dirección IPv4); en Linux o macOS,
// con "ip addr" o "ifconfig". El puerto 8000 es el del backend.
#define SERVIDOR_URL  "http://192.168.1.50:8000"
