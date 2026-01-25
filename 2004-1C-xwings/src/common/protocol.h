#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>

// Message Types for Plane <-> Airport
#define MSG_INGRESO_TERMINAL 0x0000
#define MSG_OK_INGRESO 0x0100
#define MSG_PEDIDO_PISTA 0x0010
#define MSG_PEDIDO_PISTA_EMERGENCIA 0x0012
#define MSG_PISTA_OTORGADA 0x0110
#define MSG_PISTA_CERRADA 0x0111
#define MSG_PEDIDO_COMBUSTIBLE 0x0020
#define MSG_DATOS_COMBUSTIBLE 0x0120
#define MSG_PEDIDO_RUTA 0x0030
#define MSG_RUTA 0x0130
#define MSG_AEROPUERTO_DESC 0x0131
#define MSG_INGRESO_ESPACIO 0x0040
#define MSG_ABANDONO_ESPACIO 0x0041
#define MSG_OK 0x0142

// Message Types for Plane <-> Fuel Station
#define MSG_FUEL_REQUEST 0x0050
#define MSG_FUEL_FINISHED 0x0251

// Message Types for Airport <-> Beacon
#define MSG_BEACON_PEDIDO_RUTA 0x0160
#define MSG_BEACON_RUTA 0x0360
#define MSG_BEACON_AEROPUERTO_DESC 0x0361
#define MSG_BEACON_CAMBIO_STATUS 0x0170
#define MSG_BEACON_OK_STATUS 0x0370

// Message Types for Beacon <-> Beacon
#define MSG_PING 0x80
#define MSG_PONG 0x81
#define MSG_DISCOVER 0x82
#define MSG_FOUND 0x84

// Trip types
#define VIAJE_IDA 'I'
#define VIAJE_VUELTA 'V'
#define VIAJE_ROUND 'R'

// Airport status
#define STATUS_ABIERTO 'A'
#define STATUS_CERRADO 'C'
#define STATUS_DEMORAS 'D'

// Standard Header (Airport/Plane/Fuel) - 10 bytes
struct __attribute__((packed)) Header {
  uint32_t id_msg;
  uint16_t tipo;
  uint32_t largo_msg;
};

// Beacon Header - 24 bytes
struct __attribute__((packed)) BeaconHeader {
  uint8_t id_msg[16];
  uint8_t tipo;
  uint8_t ttl;
  uint8_t hops;
  uint8_t largo_msg[5];
};

// Payloads for Airport <-> Plane
struct __attribute__((packed)) Payload_PistaCerrada {
  uint32_t id_aeropuerto_alternativo;
};

struct __attribute__((packed)) Payload_DatosCombustible {
  uint32_t ip;
  uint16_t port;
};

struct __attribute__((packed)) Payload_PedidoRuta {
  uint32_t id_aeropuerto;
};

struct __attribute__((packed)) ElementoRuta {
  uint32_t id_aeropuerto;
  uint32_t ip;
  uint16_t port;
};

struct __attribute__((packed)) Payload_IngresoEspacio {
  uint16_t num_aterrizajes;
  uint16_t num_aeropuertos;
  uint32_t combustible;
  uint8_t tipo_viaje;
  uint8_t sentido;
  uint8_t items_viaje;
  uint8_t indice_viaje;
  // Followed by: uint32_t viaje[items_viaje]
  // Then: uint8_t cantidad_pasajeros
  // Then: uint16_t cantidad_rutas
  // Then: uint8_t indice_ruta
  // Then: ElementoRuta rutas[cantidad_rutas]
};

// Payloads for Fuel Station
struct __attribute__((packed)) Payload_PedidoCombustible {
  uint32_t galones;
};

// Payloads for Beacon
struct __attribute__((packed)) Payload_CambioStatus {
  uint8_t status;
};

struct __attribute__((packed)) Payload_Pong {
  uint32_t id_beacon;
  uint32_t ip;
  uint16_t port;
};

struct __attribute__((packed)) Payload_Discover {
  uint32_t id_aeropuerto;
};

struct __attribute__((packed)) ElementoFound {
  uint32_t id_beacon;
  uint32_t id_aeropuerto;
  uint32_t ip;
  uint16_t port;
};

// Constants
#define MAX_AIRPORTS 100
#define MAX_ROUTES 100
#define MAX_PASSENGERS 300
#define PLANE_ID_LEN 6
#define AIRPORT_ID_LEN 4
#define BEACON_ID_LEN 4

#endif // PROTOCOL_H
