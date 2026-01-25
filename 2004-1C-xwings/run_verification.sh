#!/bin/bash

# Script de verificación del proyecto X-Wings
# Este script ejecuta los tres componentes del sistema en orden

echo "=== Inicializando Sistema X-Wings ==="
echo ""

# Limpiar procesos anteriores si existen
killall airport fuel 2>/dev/null
sleep 1

# 1. Iniciar aeropuerto
echo "1️⃣  Iniciando Aeropuerto (puerto 8090)..."
./src/airport/airport &
AIRPORT_PID=$!
echo "   ✓ Aeropuerto PID: $AIRPORT_PID"
sleep 1

# 2. Iniciar estación de combustible
echo "2️⃣  Iniciando Estación de Combustible (puerto 8091)..."
./src/fuel/fuel &
FUEL_PID=$!
echo "   ✓ Estación PID: $FUEL_PID"
sleep 1

# 3. Ejecutar avión (cliente)
echo "3️⃣  Ejecutando Avión (cliente)..."
echo ""
echo "--- Output del Avión ---"
./src/plane/plane
PLANE_EXIT=$?

echo ""
echo "--- Fin Output del Avión ---"
echo ""

# 4. Limpiar procesos en background
echo "🧹 Limpiando procesos en background..."
kill $AIRPORT_PID $FUEL_PID 2>/dev/null
wait $AIRPORT_PID $FUEL_PID 2>/dev/null

echo ""
if [ $PLANE_EXIT -eq 0 ]; then
    echo "✅ VERIFICACIÓN EXITOSA - Todos los handshakes completados"
else
    echo "❌ ERROR - El avión terminó con código de error: $PLANE_EXIT"
fi

echo ""
echo "=== Fin de Verificación ==="
