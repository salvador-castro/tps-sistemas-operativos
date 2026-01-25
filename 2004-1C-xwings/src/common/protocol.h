#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>

// Message Types for Plane <-> Airport
#define MSG_INGRESO_TERMINAL 0x0000
#define MSG_OK_INGRESO 0x0100
#define MSG_PEDIDO_PISTA 0x0010
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

// Standard Header (Airport/Plane/Fuel)
struct __attribute__((packed)) Header {
  uint32_t id_msg;
  uint16_t tipo;
  uint32_t largo_msg;
};

// Payloads
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

struct __attribute__((packed)) Payload_PedidoCombustible {
  uint32_t galones;
};

#endif // PROTOCOL_H
