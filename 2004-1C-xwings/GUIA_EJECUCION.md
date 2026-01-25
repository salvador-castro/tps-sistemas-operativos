# 📚 Guía de Ejecución - X-Wings (Sistemas Operativos)

## 🎯 ¿Qué es este proyecto?

Este es un trabajo práctico de **Sistemas Operativos** del 1er Cuatrimestre 2004 que simula un **sistema de control aéreo distribuido** usando sockets y comunicación entre procesos.

### Componentes del Sistema:

1. **Airport (Aeropuerto)** - Puerto 8090
   - Torre de control que gestiona pistas de aterrizaje/despegue
   - Acepta conexiones de aviones
   - Asigna pistas y gestiona terminales

2. **Plane (Avión)** - Cliente
   - Se conecta al aeropuerto
   - Solicita servicios (pista, ruta, combustible)
   - Se conecta a la estación de combustible

3. **Fuel (Estación de Combustible)** - Puerto 8091
   - Gestiona cisternas de combustible
   - Atiende pedidos de recarga de aviones

---

## 🚀 Cómo Compilar

```bash
# Desde el directorio raíz del proyecto
cd /Users/salvacastro/Desktop/sistemas-operativos/2004-1C-xwings

# Limpiar y compilar todo
make clean && make
```

Esto generará tres ejecutables:
- `src/airport/airport`
- `src/plane/plane`
- `src/fuel/fuel`

---

## ✅ Cómo Ejecutar y Verificar

### Opción 1: Ejecución Manual (3 Terminales)

**Terminal 1 - Aeropuerto:**
```bash
cd src/airport
./airport
```

Deberías ver:
```
Airport Process Starting...
Header size: 10 bytes
Airport listening on port 8090
```

**Terminal 2 - Estación de Combustible:**
```bash
cd src/fuel
./fuel
```

Deberías ver:
```
Fuel Process Starting...
Fuel Station listening on port 8091
```

**Terminal 3 - Avión (Cliente):**
```bash
cd src/plane
./plane
```

El avión se conectará primero al aeropuerto y luego a la estación de combustible.

### Opción 2: Ejecución con Script (Recomendado)

Puedes ejecutar todo en una sola terminal usando comandos en background:

```bash
# Desde el directorio raíz
cd /Users/salvacastro/Desktop/sistemas-operativos/2004-1C-xwings

# 1. Iniciar aeropuerto en background
src/airport/airport &
AIRPORT_PID=$!

# 2. Iniciar estación de combustible en background
src/fuel/fuel &
FUEL_PID=$!

# 3. Esperar un segundo para que los servidores estén listos
sleep 1

# 4. Ejecutar el avión (cliente)
src/plane/plane

# 5. Limpiar procesos en background
kill $AIRPORT_PID $FUEL_PID 2>/dev/null
```

---

## 📋 Qué Debe Pasar (Verificación)

### 1️⃣ **Handshake Aeropuerto-Avión**

**Aeropuerto envía:**
```
AER: AeroPuerto Listo. Id: A123 Protocolo v1.0\r\n
```

**Avión responde:**
```
AVN: Avion id: P99999 iniciando comunicacion\r\n
```

**Aeropuerto confirma:**
```
AER: Comunicacion aceptada\r\n
```

### 2️⃣ **Handshake Avión-Estación de Combustible**

**Estación envía:**
```
COM: Estacion de recarga Lista. Protocolo v1.0\r\n
```

**Avión solicita:**
```
AVN: Avion id: P99999 solicita combustible\r\n
```

**Estación confirma:**
```
COM: Esperando pedido\r\n
```

---

## ✨ Salida Esperada

Si todo funciona correctamente, deberías ver algo como esto:

**En el terminal del Avión:**
```
Plane Process Starting...
Connected to Airport at 127.0.0.1:8090
Received: AER: AeroPuerto Listo. Id: A123 Protocolo v1.0
Sent: AVN: Avion id: P99999 iniciando comunicacion
Received: AER: Comunicacion aceptada
Handshake with Airport completed successfully.
Connection with Airport remains open (protocol requirement)

--- Connecting to Fuel Station ---
Connected to Fuel Station at 127.0.0.1:8091
Received: COM: Estacion de recarga Lista. Protocolo v1.0
Sent: AVN: Avion id: P99999 solicita combustible
Received: COM: Esperando pedido
Handshake with Fuel Station completed successfully.

=== All handshakes completed ===
```

---

## 🔍 Estado Actual de la Implementación

### ✅ Implementado:
- ✅ Comunicación básica Avión-Aeropuerto (handshake)
- ✅ Comunicación básica Avión-Estación de Combustible (handshake)
- ✅ Estructura de protocolo (headers y payloads)
- ✅ Funciones de sockets (bind, listen, connect, send, receive)

### ⚠️ Pendiente (según enunciado completo):
- ⚠️ Mensajes de protocolo completo (ingreso terminal, pedido pista, etc.)
- ⚠️ Gestión de pistas con alternancia estricta
- ⚠️ Sistema de planificación de cisternas (RR, FIFO, SRT)
- ⚠️ Comunicación con Beacons para rutas
- ⚠️ Sistema de emergencias
- ⚠️ Archivos de configuración
- ⚠️ Sistema de logging

---

## 📖 Documentación del Protocolo

Consulta los archivos en `enunciados/`:
- `enunciado.txt` - Especificación completa del protocolo
- `aclaraciones.txt` - Aclaraciones sobre pistas, hangares y entregas
- `enunciado.pdf` - Documento PDF original

---

## 🐛 Troubleshooting

### Error: "Address already in use"
Si obtienes este error, significa que el puerto ya está en uso. Puedes:
```bash
# Ver qué proceso está usando el puerto
lsof -i :8090
lsof -i :8091

# Matar el proceso (reemplaza PID con el número que aparece)
kill -9 PID
```

### Error: "Connection refused"
Asegúrate de que el servidor (airport o fuel) esté ejecutándose **antes** de ejecutar el cliente (plane).

### Los procesos no terminan
Usa `Ctrl+C` o encuentra y mata los procesos:
```bash
ps aux | grep airport
ps aux | grep fuel
kill -9 PID
```

---

## 📝 Notas Importantes

1. **Orden de ejecución**: Siempre ejecuta los servidores (airport y fuel) **antes** que el cliente (plane)
2. **Puertos**: El aeropuerto usa puerto 8090, la estación de combustible usa 8091
3. **Localhost**: Todo se ejecuta en 127.0.0.1 (localhost) para pruebas
4. **Protocolo**: Los mensajes siguen el formato especificado en el enunciado con `\r\n` como terminación

---

## 🎓 Próximos Pasos

Para completar el proyecto según el enunciado, deberías implementar:

1. **Mensajes del protocolo completo** (ingreso terminal, pedido pista, etc.)
2. **Gestión de pistas** con múltiples colas y alternancia
3. **Sistema de combustible** con planificación RR/FIFO/SRT
4. **Archivos de configuración** para parámetros
5. **Sistema de logging** de eventos
6. **Beacons** para enrutamiento entre aeropuertos
