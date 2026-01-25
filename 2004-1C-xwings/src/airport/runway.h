#ifndef RUNWAY_H
#define RUNWAY_H

#include <pthread.h>
#include <stdint.h>

#define RUNWAY_TYPE_LANDING 0
#define RUNWAY_TYPE_TAKEOFF 1
#define RUNWAY_TYPE_BOTH 2

#define MAX_QUEUE_SIZE 100

typedef struct {
  int client_fd;
  char plane_id[7];
  int is_emergency;
  int is_landing;
} PlaneRequest;

typedef struct {
  int runway_id;
  int type;
  PlaneRequest queue[MAX_QUEUE_SIZE];
  int queue_head;
  int queue_tail;
  int queue_size;
  int in_use;
  pthread_mutex_t mutex;
  pthread_cond_t cond;
} Runway;

typedef struct {
  Runway *runways;
  int num_runways;
  int current_landing_idx;
  int current_takeoff_idx;
  pthread_mutex_t allocation_mutex;
} RunwayManager;

RunwayManager *runway_manager_init(int num_runways, int *runway_types);
int runway_enqueue(RunwayManager *mgr, int client_fd, const char *plane_id,
                   int is_landing, int is_emergency);
void *runway_process_thread(void *arg);
void runway_manager_free(RunwayManager *mgr);

#endif
