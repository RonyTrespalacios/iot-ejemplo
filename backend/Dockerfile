FROM python:3.12-slim

WORKDIR /servicio

COPY requirements.txt .
RUN pip install --no-cache-dir -r requirements.txt

COPY app ./app

# 0.0.0.0 hace que el backend acepte conexiones desde otros equipos de la
# red (la ESP32, un teléfono), no solo desde este computador.
CMD ["uvicorn", "app.main:app", "--host", "0.0.0.0", "--port", "8000"]
