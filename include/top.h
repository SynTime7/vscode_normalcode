#ifndef __TOP_H_
#define __TOP_H_

#include "MemManager.h"
#include "mongoose.h"
#ifndef MUTEX_WRAPPER_H
#define MUTEX_WRAPPER_H

#ifdef _WIN32
#include <windows.h>
typedef CRITICAL_SECTION mutex_t;

static inline void mutex_init(mutex_t *m) {
    InitializeCriticalSection(m);
}

static inline void mutex_lock(mutex_t *m) {
    EnterCriticalSection(m);
}

static inline void mutex_unlock(mutex_t *m) {
    LeaveCriticalSection(m);
}

static inline void mutex_destroy(mutex_t *m) {
    DeleteCriticalSection(m);
}

#elif defined(FREERTOS)
#include "FreeRTOS.h"
#include "semphr.h"
typedef SemaphoreHandle_t mutex_t;

static inline void mutex_init(mutex_t *m) {
    *m = xSemaphoreCreateMutex();
}

static inline void mutex_lock(mutex_t *m) {
    if (*m != NULL) {
        xSemaphoreTake(*m, portMAX_DELAY);
    }
}

static inline void mutex_unlock(mutex_t *m) {
    if (*m != NULL) {
        xSemaphoreGive(*m);
    }
}

static inline void mutex_destroy(mutex_t *m) {
    if (*m != NULL) {
        vSemaphoreDelete(*m);
        *m = NULL;
    }
}

#else
#error "Platform not supported"
#endif

#endif // MUTEX_WRAPPER_H

#endif