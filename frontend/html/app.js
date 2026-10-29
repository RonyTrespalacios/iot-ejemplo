// La página consulta al backend cada segundo (polling), igual que la ESP32.
// Las rutas son relativas (/api/...): nginx las reenvía al backend.

const INTERVALO_MS = 1000;
// Si la última lectura tiene más de estos segundos, la ESP32 se da por
// desconectada.
const SEGUNDOS_SIN_DATOS = 5;

const elConexion = document.getElementById("conexion");
const elPulsador = document.getElementById("pulsador");
const elUltimaLectura = document.getElementById("ultima-lectura");
const elLed = document.getElementById("led");
const botonLed = document.getElementById("boton-led");
const tablaLecturas = document.getElementById("tabla-lecturas");

let ledEncendido = false;

async function actualizarLecturas() {
  const respuesta = await fetch("/api/lecturas?limite=10");
  const lecturas = await respuesta.json();

  tablaLecturas.innerHTML = "";
  for (const lectura of lecturas) {
    const fila = document.createElement("tr");
    const hora = new Date(lectura.fecha).toLocaleTimeString();
    // textContent (y no innerHTML) para que un texto raro enviado al
    // backend se muestre tal cual y no se interprete como HTML.
    for (const texto of [hora, lectura.sensor, lectura.valor]) {
      const celda = document.createElement("td");
      celda.textContent = texto;
      fila.appendChild(celda);
    }
    tablaLecturas.appendChild(fila);
  }

  if (lecturas.length === 0) return;

  const ultima = lecturas[0];
  const presionado = ultima.valor === 1;
  elPulsador.textContent = presionado ? "Presionado" : "Suelto";
  elPulsador.classList.toggle("activo", presionado);

  const segundos = (Date.now() - new Date(ultima.fecha).getTime()) / 1000;
  elUltimaLectura.textContent = `Última lectura hace ${Math.round(segundos)} s`;
  const conectada = segundos < SEGUNDOS_SIN_DATOS;
  elConexion.textContent = conectada ? "ESP32 conectada" : "ESP32 sin datos recientes";
  elConexion.classList.toggle("ok", conectada);
}

async function actualizarLed() {
  const respuesta = await fetch("/api/actuador");
  const estado = await respuesta.json();
  mostrarLed(estado.encendido);
}

function mostrarLed(encendido) {
  ledEncendido = encendido;
  elLed.textContent = encendido ? "Encendido" : "Apagado";
  elLed.classList.toggle("activo", encendido);
  botonLed.textContent = encendido ? "Apagar" : "Encender";
}

botonLed.addEventListener("click", async () => {
  const respuesta = await fetch("/api/actuador", {
    method: "PUT",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify({ encendido: !ledEncendido }),
  });
  const estado = await respuesta.json();
  mostrarLed(estado.encendido);
});

async function ciclo() {
  try {
    await Promise.all([actualizarLecturas(), actualizarLed()]);
  } catch (error) {
    elConexion.textContent = "No hay conexión con el backend";
    elConexion.classList.remove("ok");
  }
}

ciclo();
setInterval(ciclo, INTERVALO_MS);
