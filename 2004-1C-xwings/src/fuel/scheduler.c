#include "scheduler.h"
#include "../common/logger.h"
#include "../common/protocol.h"
#include "../common/utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

FuelManager *fuel_manager_init(int num_tanks, SchedulerType scheduler,
                               int quantum_ms, int gallons_per_quantum) {
  FuelManager *mgr = malloc(sizeof(FuelManager));
  mgr->num_tanks = num_tanks;
  mgr->scheduler = scheduler;
  mgr->quantum_ms = quantum_ms;
  mgr->gallons_per_quantum = gallons_per_quantum;
  mgr->tanks = malloc(num_tanks * sizeof(FuelTank));
  pthread_mutex_init(&mgr->scheduler_mutex, NULL);

  for (int i = 0; i < num_tanks; i++) {
    mgr->tanks[i].tank_id = i;
    mgr->tanks[i].queue_head = 0;
    mgr->tanks[i].queue_tail = 0;
    mgr->tanks[i].queue_size = 0;
    mgr->tanks[i].in_use = 0;
    pthread_mutex_init(&mgr->tanks[i].mutex, NULL);
    pthread_cond_init(&mgr->tanks[i].cond, NULL);
  }

  return mgr;
}

int fuel_enqueue(FuelManager *mgr, int client_fd, const char *plane_id,
                 uint32_t gallons) {
  // Find tank with smallest queue (load balancing)
  int selected_tank = 0;
  int min_queue = mgr->tanks[0].queue_size;

  for (int i = 1; i < mgr->num_tanks; i++) {
    if (mgr->tanks[i].queue_size < min_queue) {
      min_queue = mgr->tanks[i].queue_size;
      selected_tank = i;
    }
  }

  FuelTank *tank = &mgr->tanks[selected_tank];
  pthread_mutex_lock(&tank->mutex);

  if (tank->queue_size >= MAX_FUEL_QUEUE) {
    pthread_mutex_unlock(&tank->mutex);
    log_message(LOG_ERROR, "Fuel tank %d queue full for plane %s",
                selected_tank, plane_id);
    return -1;
  }

  FuelRequest *req = &tank->queue[tank->queue_tail];
  req->client_fd = client_fd;
  strncpy(req->plane_id, plane_id, sizeof(req->plane_id) - 1);
  req->gallons_needed = gallons;
  req->gallons_loaded = 0;
  req->remaining_quantum = mgr->quantum_ms;

  tank->queue_tail = (tank->queue_tail + 1) % MAX_FUEL_QUEUE;
  tank->queue_size++;

  log_message(LOG_EVENT,
              "Plane %s enqueued to fuel tank %d for %u gallons (queue: %d)",
              plane_id, selected_tank, gallons, tank->queue_size);

  pthread_cond_signal(&tank->cond);
  pthread_mutex_unlock(&tank->mutex);

  return selected_tank;
}

// FIFO scheduler: serve one plane completely, then next
static void process_fifo(FuelTank *tank, FuelManager *mgr) {
  FuelRequest *req = &tank->queue[tank->queue_head];

  log_message(LOG_EVENT, "Tank %d: FIFO serving plane %s (%u/%u gallons)",
              tank->tank_id, req->plane_id, req->gallons_loaded,
              req->gallons_needed);

  while (req->gallons_loaded < req->gallons_needed) {
    usleep(mgr->quantum_ms * 1000);
    req->gallons_loaded += mgr->gallons_per_quantum;
    if (req->gallons_loaded > req->gallons_needed) {
      req->gallons_loaded = req->gallons_needed;
    }
    log_message(LOG_INFO, "Tank %d: Plane %s loaded %u/%u gallons",
                tank->tank_id, req->plane_id, req->gallons_loaded,
                req->gallons_needed);
  }

  // Notify completion
  send_message(req->client_fd, MSG_FUEL_FINISHED, NULL, 0);
  log_message(LOG_EVENT, "Tank %d: Plane %s fuel loading complete",
              tank->tank_id, req->plane_id);

  // Remove from queue
  tank->queue_head = (tank->queue_head + 1) % MAX_FUEL_QUEUE;
  tank->queue_size--;
}

// RR scheduler: give each plane one quantum, round-robin
static void process_rr(FuelTank *tank, FuelManager *mgr) {
  if (tank->queue_size == 0)
    return;

  // Process current plane for one quantum
  FuelRequest *req = &tank->queue[tank->queue_head];

  usleep(mgr->quantum_ms * 1000);
  req->gallons_loaded += mgr->gallons_per_quantum;
  if (req->gallons_loaded > req->gallons_needed) {
    req->gallons_loaded = req->gallons_needed;
  }

  log_message(LOG_INFO, "Tank %d: RR quantum for plane %s (%u/%u gallons)",
              tank->tank_id, req->plane_id, req->gallons_loaded,
              req->gallons_needed);

  // If complete, remove and notify
  if (req->gallons_loaded >= req->gallons_needed) {
    send_message(req->client_fd, MSG_FUEL_FINISHED, NULL, 0);
    log_message(LOG_EVENT, "Tank %d: Plane %s fuel loading complete (RR)",
                tank->tank_id, req->plane_id);

    tank->queue_head = (tank->queue_head + 1) % MAX_FUEL_QUEUE;
    tank->queue_size--;
  } else {
    // Move to back of queue
    FuelRequest temp = *req;
    tank->queue_head = (tank->queue_head + 1) % MAX_FUEL_QUEUE;
    tank->queue[tank->queue_tail] = temp;
    tank->queue_tail = (tank->queue_tail + 1) % MAX_FUEL_QUEUE;
  }
}

// SRT scheduler: serve request with shortest remaining time
static void process_srt(FuelTank *tank, FuelManager *mgr) {
  if (tank->queue_size == 0)
    return;

  // Find request with minimum remaining gallons
  int min_idx = tank->queue_head;
  uint32_t min_remaining =
      tank->queue[min_idx].gallons_needed - tank->queue[min_idx].gallons_loaded;

  int idx = (tank->queue_head + 1) % MAX_FUEL_QUEUE;
  int count = 1;
  while (count < tank->queue_size) {
    uint32_t remaining =
        tank->queue[idx].gallons_needed - tank->queue[idx].gallons_loaded;
    if (remaining < min_remaining) {
      min_remaining = remaining;
      min_idx = idx;
    }
    idx = (idx + 1) % MAX_FUEL_QUEUE;
    count++;
  }

  // Process shortest request for one quantum
  FuelRequest *req = &tank->queue[min_idx];

  usleep(mgr->quantum_ms * 1000);
  req->gallons_loaded += mgr->gallons_per_quantum;
  if (req->gallons_loaded > req->gallons_needed) {
    req->gallons_loaded = req->gallons_needed;
  }

  log_message(LOG_INFO, "Tank %d: SRT quantum for plane %s (%u/%u gallons)",
              tank->tank_id, req->plane_id, req->gallons_loaded,
              req->gallons_needed);

  // If complete, remove and notify
  if (req->gallons_loaded >= req->gallons_needed) {
    send_message(req->client_fd, MSG_FUEL_FINISHED, NULL, 0);
    log_message(LOG_EVENT, "Tank %d: Plane %s fuel loading complete (SRT)",
                tank->tank_id, req->plane_id);

    // Remove from queue (shift elements if needed)
    if (min_idx == tank->queue_head) {
      tank->queue_head = (tank->queue_head + 1) % MAX_FUEL_QUEUE;
    } else {
      // Compact queue (simplified)
      int i = min_idx;
      while (i != tank->queue_tail) {
        int next = (i + 1) % MAX_FUEL_QUEUE;
        tank->queue[i] = tank->queue[next];
        i = next;
      }
      tank->queue_tail =
          (tank->queue_tail - 1 + MAX_FUEL_QUEUE) % MAX_FUEL_QUEUE;
    }
    tank->queue_size--;
  }
}

void *fuel_tank_thread(void *arg) {
  TankThreadArg *targ = (TankThreadArg *)arg;
  FuelTank *tank = targ->tank;
  FuelManager *mgr = targ->manager;
  free(targ);

  while (1) {
    pthread_mutex_lock(&tank->mutex);

    while (tank->queue_size == 0) {
      pthread_cond_wait(&tank->cond, &tank->mutex);
    }

    tank->in_use = 1;
    pthread_mutex_unlock(&tank->mutex);

    // Get current scheduler
    pthread_mutex_lock(&mgr->scheduler_mutex);
    SchedulerType sched = mgr->scheduler;
    pthread_mutex_unlock(&mgr->scheduler_mutex);

    // Process based on scheduler
    pthread_mutex_lock(&tank->mutex);
    switch (sched) {
    case FUEL_SCHED_FIFO:
      process_fifo(tank, mgr);
      break;
    case FUEL_SCHED_RR:
      process_rr(tank, mgr);
      break;
    case FUEL_SCHED_SRT:
      process_srt(tank, mgr);
      break;
    }

    tank->in_use = (tank->queue_size > 0);
    pthread_mutex_unlock(&tank->mutex);
  }

  return NULL;
}

void fuel_change_scheduler(FuelManager *mgr, SchedulerType new_scheduler) {
  pthread_mutex_lock(&mgr->scheduler_mutex);
  mgr->scheduler = new_scheduler;
  const char *sched_names[] = {"FIFO", "RR", "SRT"};
  log_message(LOG_EVENT, "Scheduler changed to %s", sched_names[new_scheduler]);
  pthread_mutex_unlock(&mgr->scheduler_mutex);
}

void fuel_manager_free(FuelManager *mgr) {
  if (!mgr)
    return;

  for (int i = 0; i < mgr->num_tanks; i++) {
    pthread_mutex_destroy(&mgr->tanks[i].mutex);
    pthread_cond_destroy(&mgr->tanks[i].cond);
  }

  pthread_mutex_destroy(&mgr->scheduler_mutex);
  free(mgr->tanks);
  free(mgr);
}
