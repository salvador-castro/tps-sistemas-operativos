#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <pthread.h>
#include <stdint.h>

#define MAX_FUEL_QUEUE 100

typedef enum { FUEL_SCHED_FIFO, FUEL_SCHED_RR, FUEL_SCHED_SRT } SchedulerType;

typedef struct {
  int client_fd;
  char plane_id[7];
  uint32_t gallons_needed;
  uint32_t gallons_loaded;
  int remaining_quantum;
} FuelRequest;

typedef struct {
  int tank_id;
  FuelRequest queue[MAX_FUEL_QUEUE];
  int queue_head;
  int queue_tail;
  int queue_size;
  int in_use;
  pthread_mutex_t mutex;
  pthread_cond_t cond;
} FuelTank;

typedef struct {
  FuelTank *tanks;
  int num_tanks;
  SchedulerType scheduler;
  int quantum_ms;
  int gallons_per_quantum;
  pthread_mutex_t scheduler_mutex;
} FuelManager;

// Thread argument for tank threads
typedef struct {
  FuelTank *tank;
  FuelManager *manager;
} TankThreadArg;

// Initialize fuel manager
FuelManager *fuel_manager_init(int num_tanks, SchedulerType scheduler,
                               int quantum_ms, int gallons_per_quantum);

// Enqueue fuel request
int fuel_enqueue(FuelManager *mgr, int client_fd, const char *plane_id,
                 uint32_t gallons);

// Process fuel tank (blocking - used by tank thread)
void *fuel_tank_thread(void *arg);

// Change scheduler at runtime
void fuel_change_scheduler(FuelManager *mgr, SchedulerType new_scheduler);

// Free fuel manager
void fuel_manager_free(FuelManager *mgr);

#endif // SCHEDULER_H
