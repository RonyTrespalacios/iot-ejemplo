"""Backend del ejemplo IoT: un pulsador (sensor) y un LED (actuador).

La ESP32 es siempre la que inicia la comunicación:
  - POST /api/lecturas          -> envía el estado del pulsador
  - GET  /api/actuador          -> pregunta si el LED debe estar encendido
  - POST /api/actuador/alternar -> al presionar el pulsador, invierte el LED

La página web usa las mismas rutas:
  - GET  /api/lecturas  -> muestra las últimas lecturas
  - PUT  /api/actuador  -> cambia el estado deseado del LED

El backend es la única fuente de verdad del LED: tanto el pulsador como la
página cambian el estado aquí, y la ESP32 lo aplica al consultarlo. Así la
página siempre muestra lo mismo que el LED.

Los datos se guardan en memoria: al reiniciar el contenedor se pierden.
Guardarlos en una base de datos es una buena extensión del proyecto.
"""

from collections import deque
from datetime import datetime, timezone

from fastapi import FastAPI
from pydantic import BaseModel

app = FastAPI(title="IoT ejemplo: pulsador y LED")

# Cuántas lecturas se conservan. Las más viejas se descartan solas.
MAX_LECTURAS = 100


class Lectura(BaseModel):
    """Lo que envía la ESP32 en cada lectura."""

    sensor: str  # nombre del sensor, por ejemplo "pulsador"
    valor: float  # para el pulsador: 1 = presionado, 0 = suelto


class LecturaGuardada(Lectura):
    """Una lectura con la hora en que llegó al servidor."""

    fecha: datetime


class EstadoActuador(BaseModel):
    """Estado deseado del actuador: lo fija la página y lo lee la ESP32."""

    encendido: bool


lecturas: deque[LecturaGuardada] = deque(maxlen=MAX_LECTURAS)
actuador = EstadoActuador(encendido=False)


@app.post("/api/lecturas", status_code=201)
def recibir_lectura(lectura: Lectura) -> LecturaGuardada:
    guardada = LecturaGuardada(**lectura.model_dump(), fecha=datetime.now(timezone.utc))
    lecturas.append(guardada)
    return guardada


@app.get("/api/lecturas")
def listar_lecturas(limite: int = 20) -> list[LecturaGuardada]:
    """Devuelve las últimas lecturas, de la más reciente a la más vieja."""
    return list(reversed(lecturas))[:limite]


@app.get("/api/actuador")
def leer_actuador() -> EstadoActuador:
    return actuador


@app.put("/api/actuador")
def cambiar_actuador(nuevo: EstadoActuador) -> EstadoActuador:
    global actuador
    actuador = nuevo
    return actuador


@app.post("/api/actuador/alternar")
def alternar_actuador() -> EstadoActuador:
    """Invierte el estado del LED. La usa la ESP32 al presionar el pulsador."""
    global actuador
    actuador = EstadoActuador(encendido=not actuador.encendido)
    return actuador
