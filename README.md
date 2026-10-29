# IoT ejemplo: pulsador y LED con ESP32

Proyecto de ejemplo del curso **Máquinas Digitales**. Una ESP32 lee un
**pulsador** y controla un **LED**. El LED se enciende o apaga de dos formas:
con el pulsador o con un botón en una página web, que además muestra el
estado del pulsador. Todo corre en la red local.

Este repositorio es el **punto de partida** del proyecto de curso: cada grupo
lo copia a su propio repositorio, cambia el hardware y adapta el software a su
aplicación.

> El pulsador y el LED de este ejemplo **no cuentan** para el mínimo del
> proyecto. Pueden conservarlos, pero deben sumar al menos un sensor y un
> actuador distintos.

## Cómo funciona

```
 ESP32 ──── HTTP / JSON ────▶ backend (FastAPI, puerto 8000)
                                  ▲
 Navegador ─▶ frontend (nginx, puerto 8080) ─── /api/ ───┘
```

- La **ESP32** siempre inicia la comunicación. Cada segundo envía el estado del
  pulsador y, cada 300 ms, pregunta cómo debe estar el LED.
- El **backend** guarda las lecturas y el estado deseado del LED. Es el único
  punto de encuentro: la ESP32 y la página nunca se hablan directamente.
- El backend es la **única fuente de verdad** del LED. El pulsador no enciende
  el LED por su cuenta: al presionarlo, la ESP32 le pide al backend que lo
  invierta y aplica la respuesta. La página hace lo mismo con su botón. Así el
  LED y la página siempre coinciden, cambie quien cambie el estado.

### El recorrido de una orden

| Quién da la orden | Qué pasa |
|---|---|
| **Pulsador** | La ESP32 detecta la pulsación (con antirrebote) → `POST /api/actuador/alternar` → el backend invierte el estado y responde → la ESP32 cambia el LED de inmediato. La página se entera en su siguiente consulta |
| **Página** | Botón → `PUT /api/actuador` → el backend guarda el estado → la ESP32 lo recibe en su siguiente consulta (máximo 300 ms) y cambia el LED |
- El **frontend** sirve la página. Las peticiones a `/api/` las reenvía al
  backend, así que la página y la API salen de la misma dirección.

Los tres deben estar conectados a la **misma red**.

## Estructura

```
iot-ejemplo/
├── backend/                  API en Python con FastAPI
│   ├── app/main.py
│   ├── requirements.txt
│   └── Dockerfile
├── frontend/                 página web servida con nginx
│   ├── html/                 index.html, app.js, styles.css
│   ├── nginx.conf
│   └── Dockerfile
├── firmware/
│   └── esp32_pulsador_led/   programa de la ESP32 (IDE de Arduino)
└── docker-compose.yml        levanta backend y frontend
```

## Requisitos

- [Git](https://git-scm.com/) y [Docker Desktop](https://www.docker.com/products/docker-desktop/)
  (o Docker Engine con Compose).
- [IDE de Arduino](https://www.arduino.cc/en/software) 2.x.
- Una placa ESP32, un pulsador y un LED (o el LED integrado de la placa).
- Una red WiFi de **2.4 GHz**.

## 1. Levantar el servidor

```bash
git clone https://github.com/RonyTrespalacios/iot-ejemplo.git
cd iot-ejemplo
docker compose up --build
```

- Página: <http://localhost:8080>
- Documentación interactiva de la API: <http://localhost:8000/docs>

Para detenerlo: `Ctrl+C`, o `docker compose down` si se levantó con `-d`.

### Averiguar la IP del computador

La ESP32 necesita la IP del computador en la red local.

- **Windows:** `ipconfig` → "Dirección IPv4" del adaptador WiFi.
- **Linux / macOS:** `ip addr` o `ifconfig`.

Suele tener la forma `192.168.x.x`. Puede cambiar de un día a otro o al cambiar
de red: si la ESP32 deja de conectarse, revísela primero.

## 2. Programar la ESP32

1. En el IDE de Arduino, abra **Archivo › Preferencias** y agregue en
   "URLs adicionales de gestor de tarjetas":
   `https://espressif.github.io/arduino-esp32/package_esp32_index.json`
2. En **Herramientas › Placa › Gestor de tarjetas**, instale **esp32** de
   Espressif Systems. Trae las bibliotecas `WiFi` y `HTTPClient`.
3. En **Herramientas › Gestionar bibliotecas**, instale **ArduinoJson** de
   Benoit Blanchon (versión 7).
4. Abra `firmware/esp32_pulsador_led/esp32_pulsador_led.ino`.
5. En esa misma carpeta, copie `config.example.h` como **`config.h`** y llene
   el nombre y la clave del WiFi y la IP del computador.
   `config.h` no se sube a GitHub.
6. Elija la placa (por ejemplo **ESP32 Dev Module**) y el puerto, y suba el
   programa.
7. Abra el **Monitor serie** a 115200 baudios: debe mostrar la IP de la ESP32
   y ningún error.

### Conexiones

| Elemento | Pin de la ESP32 | Nota |
|---|---|---|
| Pulsador | GPIO 4 y GND | Usa la resistencia pull-up interna: presionado = `LOW` |
| LED | GPIO 2 | En muchas placas DevKit es el LED integrado. Si es externo, con resistencia de 220 a 330 Ω a GND |

## 3. Probar

Abra `http://<IP-del-computador>:8080` desde el **teléfono** conectado a la
misma red. Si el LED responde tanto al pulsador como al botón de la página, y
la página muestra el estado correcto en ambos casos, el sistema está
desplegado.

## La API

| Quién la usa | Método y ruta | Cuerpo / respuesta |
|---|---|---|
| ESP32 | `POST /api/lecturas` | `{"sensor": "pulsador", "valor": 1}` |
| ESP32 | `GET /api/actuador` | `{"encendido": true}` |
| ESP32 | `POST /api/actuador/alternar` | sin cuerpo; responde el estado nuevo, `{"encendido": false}` |
| Página | `GET /api/lecturas?limite=10` | lista de lecturas, la más reciente primero |
| Página | `PUT /api/actuador` | `{"encendido": false}` |

Se puede probar sin la ESP32, desde una terminal:

```bash
curl -X POST http://localhost:8000/api/lecturas -H "Content-Type: application/json" -d '{"sensor": "pulsador", "valor": 1}'
curl http://localhost:8000/api/actuador
curl -X POST http://localhost:8000/api/actuador/alternar
```

Las lecturas se guardan **en memoria**: se pierden al reiniciar el contenedor.

## Problemas comunes

| Síntoma | Causa probable |
|---|---|
| La ESP32 se queda en "Conectando a la red..." | Red de 5 GHz, clave equivocada o red con página de inicio de sesión |
| Error `-1` en el Monitor serie | IP equivocada, backend apagado o firewall bloqueando el puerto 8000 |
| La página abre en el computador pero no en el teléfono | Firewall del computador, o red que aísla a los equipos entre sí |
| Funcionaba ayer y hoy no | Cambió la IP del computador |

- **Firewall de Windows:** al primer `docker compose up`, permita el acceso en
  redes privadas. Si no apareció el aviso, cree una regla de entrada para los
  puertos 8000 y 8080.
- **Redes institucionales:** muchas aíslan a los equipos o piden iniciar
  sesión. Un teléfono como **punto de acceso** de 2.4 GHz resuelve casi todo.

## Cómo adaptarlo a su proyecto

1. **Firmware:** cambie la lectura del pulsador por la de su sensor y el
   `digitalWrite` del LED por el control de su actuador.
2. **Backend:** ajuste los modelos `Lectura` y `EstadoActuador` de
   `backend/app/main.py` a sus datos.
3. **Frontend:** adapte `frontend/html/` para mostrar su sensor y controlar su
   actuador.

Ideas para ir más allá: guardar las lecturas en una base de datos, graficar
el histórico, un modo automático por umbral, o usar WebSocket o MQTT en lugar
de consultar periódicamente.

### ¿Por qué HTTP y no MQTT?

Consultar el estado cada 300 ms basta para que el LED responda sin demora
perceptible, y HTTP es más fácil de entender y de depurar: cada petición se
puede repetir con `curl` o ver en `/docs`. MQTT o WebSocket valen la pena
cuando la orden debe llegar al instante, cuando hay muchos dispositivos o
cuando la ESP32 funciona con batería y no puede consultar tan seguido.
