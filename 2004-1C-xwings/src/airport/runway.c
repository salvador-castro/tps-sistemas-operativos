#include "runway.h"
#include "../common/logger.h"
#include "../common/protocol.h"
#include "../common/utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

extern int landing_time_ms;
extern int takeoff_time_ms;

RunwayManager *runway_manager_init(int num_runways, int *runway_types) {
  RunwayManager *mgr = malloc(sizeof(RunwayManager));
  mgr->num_runways = num_runways;
  mgr->runways = malloc(num_runways * sizeof(Runway));
  mgr->current_landing_idx = 0;
  mgr->current_takeoff_idx = 0;
  pthread_mutex_init(&mgr->allocation_mutex, NULL);

  for (int i = 0; i < num_runways; i++) {
    mgr->runways[i].runway_id = i;
    mgr->runways[i].type = runway_types[i];
    mgr->runways[i].queue_head = 0;
    mgr->runways[i].queue_tail = 0;
    mgr->runways[i].queue_size = 0;
    mgr->runways[i].in_use = 0;
    pthread_mutex_init(&mgr->runways[i].mutex, NULL);
    pthread_cond_init(&mgr->runways[i].cond, NULL);
  }

  return mgr;
}

int runway_enqueue(RunwayManager *mgr, int client_fd, const char *plane_id,
                   int is_landing, int is_emergency) {
  pthread_mutex_lock(&mgr->allocation_mutex);

  int selected_runway = -1;

  // Emergency planes get priority on any available landing runway
  if (is_emergency && is_landing) {
    for (int i = 0; i < mgr->num_runways; i++) {
      if (mgr->runways[i].type == RUNWAY_TYPE_LANDING ||
          mgr->runways[i].type == RUNWAY_TYPE_BOTH) {
        selected_runway = i;
        break;
      }
    }
  } else {
    // Strict alternation: find next runway of appropriate type
    if (is_landing) {
      int attempts = 0;
      while (attempts < mgr->num_runways) {
        int idx = mgr->current_landing_idx;
        if (mgr->runways[idx].type == RUNWAY_TYPE_LANDING ||
            mgr->runways[idx].type == RUNWAY_TYPE_BOTH) {
          selected_runway = idx;
          mgr->current_landing_idx = (idx + 1) % mgr->num_runways;
          break;
        }
        mgr->current_landing_idx = (idx + 1) % mgr->num_runways;
        attempts++;
      }
    } else {
      int attempts = 0;
      while (attempts < mgr->num_runways) {
        int idx = mgr->current_takeoff_idx;
        if (mgr->runways[idx].type == RUNWAY_TYPE_TAKEOFF ||
            mgr->runways[idx].type == RUNWAY_TYPE_BOTH) {
          selected_runway = idx;
          mgr->current_takeoff_idx = (idx + 1) % mgr->num_runways;
          break;
        }
        mgr->current_takeoff_idx = (idx + 1) % mgr->num_runways;
        attempts++;
      }
    }
  }

  pthread_mutex_unlock(&mgr->allocation_mutex);

  if (selected_runway == -1) {
    log_message(LOG_ERROR, "No suitable runway found for plane %s", plane_id);
    return -1;
  }

  // Add to runway queue
  Runway *rwy = &mgr->runways[selected_runway];
  pthread_mutex_lock(&rwy->mutex);

  if (rwy->queue_size >= MAX_QUEUE_SIZE) {
    pthread_mutex_unlock(&rwy->mutex);
    log_message(LOG_ERROR, "Runway %d queue full for plane %s", selected_runway,
                plane_id);
    return -1;
  }

  PlaneRequest *req = &rwy->queue[rwy->queue_tail];
  req->client_fd = client_fd;
  strncpy(req->plane_id, plane_id, sizeof(req->plane_id) - 1);
  req->is_emergency = is_emergency;
  req->is_landing = is_landing;

  rwy->queue_tail = (rwy->queue_tail + 1) % MAX_QUEUE_SIZE;
  rwy->queue_size++;

  log_message(LOG_EVENT, "Plane %s enqueued to runway %d (queue size: %d)",
              plane_id, selected_runway, rwy->queue_size);

  pthread_cond_signal(&rwy->cond);
  pthread_mutex_unlock(&rwy->mutex);

  return selected_runway;
}

void *runway_process_thread(void *arg) {
  Runway *rwy = (Runway *)arg;

  while (1) {
    pthread_mutex_lock(&rwy->mutex);

    // Wait for a plane in queue
    while (rwy->queue_size == 0) {
      pthread_cond_wait(&rwy->cond, &rwy->mutex);
    }

    // Pop plane from queue (FIFO)
    PlaneRequest req = rwy->queue[rwy->queue_head];
    rwy->queue_head = (rwy->queue_head + 1) % MAX_QUEUE_SIZE;
    rwy->queue_size--;
    rwy->in_use = 1;

    pthread_mutex_unlock(&rwy->mutex);

    // Grant runway access
    log_message(LOG_EVENT, "Runway %d granted to plane %s (%s)", rwy->runway_id,
                req.plane_id, req.is_landing ? "landing" : "takeoff");

    send_message(req.client_fd, MSG_PISTA_OTORGADA, NULL, 0);

    // Simulate runway operation time
    int operation_time = req.is_landing ? landing_time_ms : takeoff_time_ms;
    usleep(operation_time * 1000);

    log_message(LOG_EVENT, "Runway %d operation complete for plane %s",
                rwy->runway_id, req.plane_id);

    pthread_mutex_lock(&rwy->mutex);
    rwy->in_use = 0;
    pthread_mutex_unlock(&rwy->mutex);
  }

  return NULL;
}

void runway_manager_free(RunwayManager *mgr) {
  if (!mgr)
    return;

  for (int i = 0; i < mgr->num_runways; i++) {
    pthread_mutex_destroy(&mgr->runways[i].mutex);
    pthread_cond_destroy(&mgr->runways[i].cond);
  }

  pthread_mutex_destroy(&mgr->allocation_mutex);
  free(mgr->runways);
  free(mgr);
}
