#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int load_airport_config(const char *filename, AirportConfig *config) {
  FILE *f = fopen(filename, "r");
  if (!f) {
    return -1;
  }

  char line[256];
  config->runway_types = NULL;
  config->num_runways = 0;

  while (fgets(line, sizeof(line), f)) {
    if (line[0] == '#' || line[0] == '\n')
      continue;

    if (sscanf(line, "id=%4s", config->id) == 1)
      continue;
    if (sscanf(line, "port=%hu", &config->port) == 1)
      continue;
    if (sscanf(line, "num_runways=%d", &config->num_runways) == 1) {
      config->runway_types = malloc(config->num_runways * sizeof(int));
      continue;
    }
    if (sscanf(line, "terminal_quota=%d", &config->terminal_quota) == 1)
      continue;
    if (sscanf(line, "landing_time_ms=%d", &config->landing_time_ms) == 1)
      continue;
    if (sscanf(line, "takeoff_time_ms=%d", &config->takeoff_time_ms) == 1)
      continue;
    if (sscanf(line, "beacon_ip=%15s", config->beacon_ip) == 1)
      continue;
    if (sscanf(line, "beacon_port=%hu", &config->beacon_port) == 1)
      continue;
    if (sscanf(line, "status=%c", &config->status) == 1)
      continue;

    if (strncmp(line, "runway_types=", 13) == 0) {
      char *p = line + 13;
      for (int i = 0; i < config->num_runways; i++) {
        config->runway_types[i] = atoi(p);
        while (*p && *p != ',')
          p++;
        if (*p == ',')
          p++;
      }
    }
  }

  fclose(f);
  return 0;
}

int load_fuel_config(const char *filename, FuelConfig *config) {
  FILE *f = fopen(filename, "r");
  if (!f) {
    return -1;
  }

  char line[256];
  while (fgets(line, sizeof(line), f)) {
    if (line[0] == '#' || line[0] == '\n')
      continue;

    sscanf(line, "port=%hu", &config->port);
    sscanf(line, "num_tanks=%d", &config->num_tanks);
    sscanf(line, "quantum_ms=%d", &config->quantum_ms);
    sscanf(line, "gallons_per_quantum=%d", &config->gallons_per_quantum);
    if (sscanf(line, "scheduler=%9s", config->scheduler) == 1)
      continue;
  }

  fclose(f);
  return 0;
}

int load_plane_config(const char *filename, PlaneConfig *config) {
  FILE *f = fopen(filename, "r");
  if (!f) {
    return -1;
  }

  char line[256];
  config->num_airports = 0;

  while (fgets(line, sizeof(line), f)) {
    if (line[0] == '#' || line[0] == '\n')
      continue;

    if (sscanf(line, "id=%6s", config->id) == 1)
      continue;
    if (sscanf(line, "initial_fuel=%u", &config->initial_fuel) == 1)
      continue;
    if (sscanf(line, "trip_type=%c", &config->trip_type) == 1)
      continue;
    if (sscanf(line, "num_passengers=%d", &config->num_passengers) == 1)
      continue;
    if (sscanf(line, "first_airport_ip=%15s", config->first_airport_ip) == 1)
      continue;
    if (sscanf(line, "first_airport_port=%hu", &config->first_airport_port) ==
        1)
      continue;

    if (strncmp(line, "journey=", 8) == 0) {
      strncpy(config->journey, line + 8, sizeof(config->journey) - 1);
      config->journey[strcspn(config->journey, "\n")] = 0;

      // Parse journey into airport IDs (simplified - assumes numeric IDs)
      char *p = config->journey;
      while (*p && config->num_airports < 20) {
        config->airport_ids[config->num_airports++] = strtoul(p, &p, 0);
        while (*p == ',' || *p == ' ')
          p++;
      }
    }
  }

  fclose(f);
  return 0;
}

int load_beacon_config(const char *filename, BeaconConfig *config) {
  FILE *f = fopen(filename, "r");
  if (!f) {
    return -1;
  }

  char line[256];
  config->num_beacons = 0;
  config->num_airports = 0;

  while (fgets(line, sizeof(line), f)) {
    if (line[0] == '#' || line[0] == '\n')
      continue;

    if (sscanf(line, "id=%4s", config->id) == 1)
      continue;
    if (sscanf(line, "port=%hu", &config->port) == 1)
      continue;

    // Parse beacon connections
    if (strncmp(line, "beacon=", 7) == 0 && config->num_beacons < 10) {
      sscanf(line + 7, "%15[^:]:%hu", config->beacon_ips[config->num_beacons],
             &config->beacon_ports[config->num_beacons]);
      config->num_beacons++;
    }

    // Parse airport connections
    if (strncmp(line, "airport=", 8) == 0 && config->num_airports < 10) {
      sscanf(line + 8, "%u,%15[^:]:%hu",
             &config->airport_ids[config->num_airports],
             config->airport_ips[config->num_airports],
             &config->airport_ports[config->num_airports]);
      config->num_airports++;
    }
  }

  fclose(f);
  return 0;
}

void free_airport_config(AirportConfig *config) {
  if (config->runway_types) {
    free(config->runway_types);
    config->runway_types = NULL;
  }
}
