#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>

// Configuration structures
typedef struct {
  char id[5];
  uint16_t port;
  int num_runways;
  int *runway_types; // 0=landing, 1=takeoff, 2=both
  int terminal_quota;
  int landing_time_ms;
  int takeoff_time_ms;
  char beacon_ip[16];
  uint16_t beacon_port;
  char status;
} AirportConfig;

typedef struct {
  uint16_t port;
  int num_tanks;
  int quantum_ms;
  int gallons_per_quantum;
  char scheduler[10]; // "RR", "FIFO", "SRT"
} FuelConfig;

typedef struct {
  char id[7];
  char journey[100]; // Comma-separated airport IDs
  int num_airports;
  uint32_t airport_ids[20];
  uint32_t initial_fuel;
  char trip_type; // 'I', 'V', 'R'
  int num_passengers;
  char first_airport_ip[16];
  uint16_t first_airport_port;
} PlaneConfig;

typedef struct {
  char id[5];
  uint16_t port;
  int num_beacons;
  char beacon_ips[10][16];
  uint16_t beacon_ports[10];
  int num_airports;
  uint32_t airport_ids[10];
  char airport_ips[10][16];
  uint16_t airport_ports[10];
} BeaconConfig;

// Configuration parsers
int load_airport_config(const char *filename, AirportConfig *config);
int load_fuel_config(const char *filename, FuelConfig *config);
int load_plane_config(const char *filename, PlaneConfig *config);
int load_beacon_config(const char *filename, BeaconConfig *config);

void free_airport_config(AirportConfig *config);

#endif // CONFIG_H
