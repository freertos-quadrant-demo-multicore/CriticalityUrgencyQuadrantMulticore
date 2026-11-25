/*
 * FreeRTOS Priority-Urgency Quadrant Model Demonstration
 * Multi-Core SMP Version
 *
 * Based on: "Multi-Core Extension of the Priority-Urgency Quadrant Model
 *            for Automotive Real-Time Systems"
 * Author: Azad Mohammed Shaik
 * Date: December 22, 2025
 *
 * This implementation demonstrates partitioned scheduling with quadrant-aware
 * core allocation strategies for FreeRTOS-SMP.
 */

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <math.h>
#include <sched.h>
#include <pthread.h>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "timers.h"

/* ============================================================================
 * MULTI-CORE CONFIGURATION
 * ============================================================================ */

// Number of cores to use (4 or 8)
#define NUM_CORES  4

// Allocation strategy selection
// 0 = Quadrant-Based Core Dedication (conservative, maximum safety isolation)
// 1 = PU-WFD (Priority-Urgency Aware Worst-Fit Decreasing, load balanced)
#define USE_PU_WFD_ALLOCATION  0

// CPU burn iterations for stress testing
#define CPU_BURN_ITERATIONS  30000  // Reduced for realistic workload

// Maximum tasks per core
#define MAX_TASKS_PER_CORE  10

/* ============================================================================
 * QUADRANT TYPE DEFINITIONS
 * ============================================================================ */

typedef enum {
    QUADRANT_NONE = 0,
    QUADRANT_1,  // High Criticality + High Urgency
    QUADRANT_2,  // High Criticality + Low Urgency
    QUADRANT_3,  // Low Criticality + High Urgency
    QUADRANT_4   // Low Criticality + Low Urgency
} QuadrantType_t;

typedef uint32_t CoreID_t;

/* ============================================================================
 * CORE INFORMATION STRUCTURE
 * ============================================================================ */

typedef struct {
    CoreID_t core_id;
    QuadrantType_t primary_quadrant;
    QuadrantType_t secondary_quadrant;
    float max_utilization;
    float current_utilization;
    uint32_t num_tasks;
    TaskHandle_t tasks[MAX_TASKS_PER_CORE];
    char task_names[MAX_TASKS_PER_CORE][32];
    uint32_t total_executions;
    uint32_t total_deadline_misses;
} CoreInfo_t;

/* ============================================================================
 * TASK INFORMATION STRUCTURE
 * ============================================================================ */

typedef struct {
    char taskName[32];
    QuadrantType_t quadrant;
    uint32_t period_ms;
    uint32_t wcet_ms;
    uint32_t deadline_ms;
    float utilization;
    UBaseType_t priority;
    CoreID_t assigned_core;

    // Statistics
    uint32_t executionCount;
    uint32_t missedDeadlines;
    uint32_t totalExecutionTime;
    TickType_t lastExecutionTime;

    // Inter-core communication metrics
    uint32_t inter_core_messages;
    uint32_t shared_data_accesses;
} TaskInfo_t;

/* ============================================================================
 * GLOBAL VARIABLES
 * ============================================================================ */

CoreInfo_t coreInfo[NUM_CORES];
TaskInfo_t taskInfo[24];  // Support up to 24 tasks (as per Table III in paper)
uint32_t numTasks = 0;

SemaphoreHandle_t statsMutex;
SemaphoreHandle_t allocationMutex;
volatile int keepRunning = 1;

/* ============================================================================
 * CORE ALLOCATION TABLE (TABLE II FROM PAPER)
 * ============================================================================ */

void initializeCoreConfiguration(void) {
    if (NUM_CORES == 4) {
        // 4-Core Configuration (Table II from paper)
        coreInfo[0].core_id = 0;
        coreInfo[0].primary_quadrant = QUADRANT_1;
        coreInfo[0].secondary_quadrant = QUADRANT_NONE;
        coreInfo[0].max_utilization = 0.85f;  // 15% headroom for Q1 urgency

        coreInfo[1].core_id = 1;
        coreInfo[1].primary_quadrant = QUADRANT_1;
        coreInfo[1].secondary_quadrant = QUADRANT_2;
        coreInfo[1].max_utilization = 0.90f;  // 10% headroom

        coreInfo[2].core_id = 2;
        coreInfo[2].primary_quadrant = QUADRANT_2;
        coreInfo[2].secondary_quadrant = QUADRANT_3;
        coreInfo[2].max_utilization = 0.95f;  // 5% headroom

        coreInfo[3].core_id = 3;
        coreInfo[3].primary_quadrant = QUADRANT_3;
        coreInfo[3].secondary_quadrant = QUADRANT_4;
        coreInfo[3].max_utilization = 1.00f;  // Background work, fully utilize
    } else if (NUM_CORES == 8) {
        // 8-Core Configuration (extended from paper)
        coreInfo[0].core_id = 0;
        coreInfo[0].primary_quadrant = QUADRANT_1;
        coreInfo[0].secondary_quadrant = QUADRANT_NONE;
        coreInfo[0].max_utilization = 0.85f;

        coreInfo[1].core_id = 1;
        coreInfo[1].primary_quadrant = QUADRANT_1;
        coreInfo[1].secondary_quadrant = QUADRANT_NONE;
        coreInfo[1].max_utilization = 0.85f;

        coreInfo[2].core_id = 2;
        coreInfo[2].primary_quadrant = QUADRANT_1;
        coreInfo[2].secondary_quadrant = QUADRANT_2;
        coreInfo[2].max_utilization = 0.90f;

        coreInfo[3].core_id = 3;
        coreInfo[3].primary_quadrant = QUADRANT_2;
        coreInfo[3].secondary_quadrant = QUADRANT_NONE;
        coreInfo[3].max_utilization = 0.90f;

        coreInfo[4].core_id = 4;
        coreInfo[4].primary_quadrant = QUADRANT_2;
        coreInfo[4].secondary_quadrant = QUADRANT_3;
        coreInfo[4].max_utilization = 0.95f;

        coreInfo[5].core_id = 5;
        coreInfo[5].primary_quadrant = QUADRANT_3;
        coreInfo[5].secondary_quadrant = QUADRANT_NONE;
        coreInfo[5].max_utilization = 0.95f;

        coreInfo[6].core_id = 6;
        coreInfo[6].primary_quadrant = QUADRANT_3;
        coreInfo[6].secondary_quadrant = QUADRANT_4;
        coreInfo[6].max_utilization = 1.00f;

        coreInfo[7].core_id = 7;
        coreInfo[7].primary_quadrant = QUADRANT_4;
        coreInfo[7].secondary_quadrant = QUADRANT_NONE;
        coreInfo[7].max_utilization = 1.00f;
    }

    // Initialize remaining fields
    for (int i = 0; i < NUM_CORES; i++) {
        coreInfo[i].current_utilization = 0.0f;
        coreInfo[i].num_tasks = 0;
        coreInfo[i].total_executions = 0;
        coreInfo[i].total_deadline_misses = 0;
    }
}

/* ============================================================================
 * PRIORITY ASSIGNMENT (PER QUADRANT)
 * ============================================================================ */

UBaseType_t assignPriorityByQuadrant(QuadrantType_t quadrant, uint32_t period_ms) {
    // Priority ranges from Table I in paper
    UBaseType_t base_priority = 0;

    switch (quadrant) {
        case QUADRANT_1:  // Q1: Priorities 8-9
            base_priority = (period_ms <= 20) ? 9 : 8;
            break;
        case QUADRANT_2:  // Q2: Priority 7
            base_priority = 7;
            break;
        case QUADRANT_3:  // Q3: Priorities 4-6
            if (period_ms <= 20) base_priority = 6;
            else if (period_ms <= 50) base_priority = 5;
            else base_priority = 4;
            break;
        case QUADRANT_4:  // Q4: Priorities 1-3
            if (period_ms <= 1000) base_priority = 3;
            else if (period_ms <= 5000) base_priority = 2;
            else base_priority = 1;
            break;
        default:
            base_priority = 1;
    }

    return base_priority;
}

/* ============================================================================
 * ALGORITHM 1: QUADRANT-BASED CORE DEDICATION
 * ============================================================================ */

int allocateTasksDedication(void) {
    printf("\n[ALLOCATION] Using Quadrant-Based Core Dedication Strategy\n");
    printf("============================================================\n");

    // Sort tasks by quadrant: Q1 → Q2 → Q3 → Q4
    for (uint32_t i = 0; i < numTasks - 1; i++) {
        for (uint32_t j = i + 1; j < numTasks; j++) {
            if (taskInfo[j].quadrant < taskInfo[i].quadrant) {
                TaskInfo_t temp = taskInfo[i];
                taskInfo[i] = taskInfo[j];
                taskInfo[j] = temp;
            }
        }
    }

    // Allocate each task
    for (uint32_t i = 0; i < numTasks; i++) {
        QuadrantType_t Q = taskInfo[i].quadrant;
        int assigned = 0;

        // Try to allocate to cores that accept this quadrant
        for (CoreID_t c = 0; c < NUM_CORES; c++) {
            if (Q == coreInfo[c].primary_quadrant ||
                Q == coreInfo[c].secondary_quadrant) {

                float new_util = coreInfo[c].current_utilization + taskInfo[i].utilization;

                if (new_util <= coreInfo[c].max_utilization) {
                    // Assign task to this core
                    taskInfo[i].assigned_core = c;
                    coreInfo[c].current_utilization = new_util;
                    coreInfo[c].num_tasks++;
                    strncpy(coreInfo[c].task_names[coreInfo[c].num_tasks - 1],
                            taskInfo[i].taskName, 31);

                    printf("  [ALLOC] %s (Q%d, U=%.3f) → Core %u (U=%.3f)\n",
                           taskInfo[i].taskName, taskInfo[i].quadrant,
                           taskInfo[i].utilization, c, coreInfo[c].current_utilization);

                    assigned = 1;
                    break;
                }
            }
        }

        if (!assigned) {
            printf("  [ERROR] Cannot allocate task %s! System infeasible.\n",
                   taskInfo[i].taskName);
            return -1;
        }
    }

    printf("\n[ALLOCATION] SUCCESS - All tasks allocated\n\n");
    return 0;
}

/* ============================================================================
 * ALGORITHM 2: PU-WFD (PRIORITY-URGENCY AWARE WORST-FIT DECREASING)
 * ============================================================================ */

int allocateTasksPU_WFD(void) {
    printf("\n[ALLOCATION] Using PU-WFD (Worst-Fit Decreasing) Strategy\n");
    printf("==========================================================\n");

    // Sort tasks by (quadrant, utilization) in decreasing order
    for (uint32_t i = 0; i < numTasks - 1; i++) {
        for (uint32_t j = i + 1; j < numTasks; j++) {
            if (taskInfo[j].quadrant < taskInfo[i].quadrant ||
                (taskInfo[j].quadrant == taskInfo[i].quadrant &&
                 taskInfo[j].utilization > taskInfo[i].utilization)) {
                TaskInfo_t temp = taskInfo[i];
                taskInfo[i] = taskInfo[j];
                taskInfo[j] = temp;
            }
        }
    }

    // Allocate each task using Worst-Fit
    for (uint32_t i = 0; i < numTasks; i++) {
        QuadrantType_t Q = taskInfo[i].quadrant;
        CoreID_t best_core = NUM_CORES;
        float max_remaining = -1.0f;

        // Find core with most remaining capacity (Worst-Fit)
        for (CoreID_t c = 0; c < NUM_CORES; c++) {
            if (Q == coreInfo[c].primary_quadrant ||
                Q == coreInfo[c].secondary_quadrant) {

                float new_util = coreInfo[c].current_utilization + taskInfo[i].utilization;

                if (new_util <= coreInfo[c].max_utilization) {
                    float remaining = coreInfo[c].max_utilization - new_util;

                    if (remaining > max_remaining) {
                        best_core = c;
                        max_remaining = remaining;
                    }
                }
            }
        }

        if (best_core < NUM_CORES) {
            // Assign task to best core
            taskInfo[i].assigned_core = best_core;
            coreInfo[best_core].current_utilization += taskInfo[i].utilization;
            coreInfo[best_core].num_tasks++;
            strncpy(coreInfo[best_core].task_names[coreInfo[best_core].num_tasks - 1],
                    taskInfo[i].taskName, 31);

            printf("  [ALLOC] %s (Q%d, U=%.3f) → Core %u (U=%.3f, Remaining=%.3f)\n",
                   taskInfo[i].taskName, taskInfo[i].quadrant,
                   taskInfo[i].utilization, best_core,
                   coreInfo[best_core].current_utilization, max_remaining);
        } else {
            printf("  [ERROR] Cannot allocate task %s! System infeasible.\n",
                   taskInfo[i].taskName);
            return -1;
        }
    }

    printf("\n[ALLOCATION] SUCCESS - All tasks allocated with load balancing\n\n");
    return 0;
}

/* ============================================================================
 * MULTI-CORE METRICS (SECTION IV.G FROM PAPER)
 * ============================================================================ */

// Core Affinity Score (CAS) - Equation (1)
float calculateCAS(uint32_t task_i, uint32_t task_j) {
    uint32_t shared = taskInfo[task_i].shared_data_accesses;
    uint32_t total_i = taskInfo[task_i].executionCount;
    uint32_t total_j = taskInfo[task_j].executionCount;

    if (total_i + total_j == 0) return 0.0f;

    return (float)shared / (float)(total_i + total_j);
}

// Inter-Core Dependency Factor (ICD) - Equation (2)
float calculateICD(CoreID_t core) {
    uint32_t total_messages = 0;
    uint32_t total_executions = 0;

    for (uint32_t i = 0; i < numTasks; i++) {
        if (taskInfo[i].assigned_core == core) {
            total_messages += taskInfo[i].inter_core_messages;
            total_executions += taskInfo[i].executionCount;
        }
    }

    if (total_executions == 0) return 0.0f;

    return (float)total_messages / (float)total_executions;
}

// Load Balance Index (LBI) - Equation (3)
float calculateLBI(void) {
    float Umax = 0.0f;
    float Umin = 1.0f;

    for (CoreID_t i = 0; i < NUM_CORES; i++) {
        if (coreInfo[i].current_utilization > Umax) {
            Umax = coreInfo[i].current_utilization;
        }
        if (coreInfo[i].current_utilization < Umin) {
            Umin = coreInfo[i].current_utilization;
        }
    }

    if (Umax == 0.0f) return 1.0f;

    return 1.0f - ((Umax - Umin) / Umax);
}

/* ============================================================================
 * UTILITY FUNCTIONS
 * ============================================================================ */

void burnCPU(uint32_t iterations) {
    volatile double result = 0.0;
    for (uint32_t i = 0; i < iterations; i++) {
        result += sqrt((double)i) * 0.001;
    }
}

void simulateWork(uint32_t durationMs) {
    burnCPU(CPU_BURN_ITERATIONS);
    vTaskDelay(pdMS_TO_TICKS(durationMs / 2));
}

void recordTaskExecution(uint32_t taskIndex, uint32_t execTime) {
    xSemaphoreTake(statsMutex, portMAX_DELAY);

    taskInfo[taskIndex].executionCount++;
    taskInfo[taskIndex].totalExecutionTime += execTime;
    taskInfo[taskIndex].lastExecutionTime = xTaskGetTickCount();

    CoreID_t core = taskInfo[taskIndex].assigned_core;
    coreInfo[core].total_executions++;

    xSemaphoreGive(statsMutex);
}

void recordMissedDeadline(uint32_t taskIndex) {
    xSemaphoreTake(statsMutex, portMAX_DELAY);

    taskInfo[taskIndex].missedDeadlines++;

    CoreID_t core = taskInfo[taskIndex].assigned_core;
    coreInfo[core].total_deadline_misses++;

    xSemaphoreGive(statsMutex);
}

/* ============================================================================
 * SIGNAL HANDLER
 * ============================================================================ */

void sigintHandler(int sig_num) {
    (void)sig_num;
    printf("\n\nReceived Ctrl+C. Shutting down gracefully...\n");
    keepRunning = 0;
}

/* ============================================================================
 * TASK TEMPLATE WITH CORE AFFINITY
 * ============================================================================ */

void genericTaskFunction(void *pvParameters) {
    uint32_t taskIndex = *((uint32_t *)pvParameters);
    TaskInfo_t *task = &taskInfo[taskIndex];

    // Set CPU affinity for this task (POSIX-specific for Linux demo)
    #ifdef __linux__
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(task->assigned_core, &cpuset);
    pthread_t current_thread = pthread_self();
    pthread_setaffinity_np(current_thread, sizeof(cpu_set_t), &cpuset);
    #endif

    printf("[INIT] %s started on Core %u - Priority: %lu, Quadrant: Q%d\n",
           task->taskName, task->assigned_core, task->priority, task->quadrant);

    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xPeriod = pdMS_TO_TICKS(task->period_ms);
    const TickType_t xDeadline = pdMS_TO_TICKS(task->deadline_ms);

    while (keepRunning) {
        TickType_t startTime = xTaskGetTickCount();

        // Simulate work
        simulateWork(task->wcet_ms);

        TickType_t executionTime = xTaskGetTickCount() - startTime;

        // Check deadline
        if (task->deadline_ms > 0 && executionTime > xDeadline) {
            recordMissedDeadline(taskIndex);
            printf("[WARNING] %s MISSED DEADLINE! Exec: %lu ms, Deadline: %u ms, Core: %u\n",
                   task->taskName, executionTime * portTICK_PERIOD_MS,
                   task->deadline_ms, task->assigned_core);
        }

        recordTaskExecution(taskIndex, task->wcet_ms);
        vTaskDelayUntil(&xLastWakeTime, xPeriod);
    }

    vTaskDelete(NULL);
}

/* ============================================================================
 * TASK REGISTRATION (TABLE III FROM PAPER)
 * ============================================================================ */

void registerTask(const char *name, QuadrantType_t quadrant,
                  uint32_t period_ms, uint32_t wcet_ms, uint32_t deadline_ms) {
    if (numTasks >= 24) {
        printf("ERROR: Maximum task limit reached!\n");
        return;
    }

    TaskInfo_t *task = &taskInfo[numTasks];
    strncpy(task->taskName, name, 31);
    task->quadrant = quadrant;
    task->period_ms = period_ms;
    task->wcet_ms = wcet_ms;
    task->deadline_ms = deadline_ms;
    task->utilization = (float)wcet_ms / (float)period_ms;
    task->priority = assignPriorityByQuadrant(quadrant, period_ms);
    task->assigned_core = 0;  // Will be set by allocation algorithm
    task->executionCount = 0;
    task->missedDeadlines = 0;
    task->totalExecutionTime = 0;
    task->lastExecutionTime = 0;
    task->inter_core_messages = 0;
    task->shared_data_accesses = 0;

    numTasks++;
}

void registerAutomotiveTasks(void) {
    printf("\n[SETUP] Registering Automotive IVI Benchmark Tasks\n");
    printf("====================================================\n\n");

    // Q1 Tasks (High Criticality + High Urgency)
    registerTask("CAN_Handler", QUADRANT_1, 10, 3, 5);
    registerTask("Brake_Monitor", QUADRANT_1, 20, 3, 10);
    registerTask("Sensor_Fusion", QUADRANT_1, 50, 8, 25);
    registerTask("Emergency_Shutdown", QUADRANT_1, 100, 5, 50);

    // Q2 Tasks (High Criticality + Low Urgency)
    registerTask("Diagnostics", QUADRANT_2, 500, 80, 200);
    registerTask("Watchdog_Refresh", QUADRANT_2, 100, 1, 50);
    registerTask("NVM_Logging", QUADRANT_2, 1000, 50, 500);
    registerTask("Health_Monitor", QUADRANT_2, 200, 10, 100);

    // Q3 Tasks (Low Criticality + High Urgency)
    registerTask("HMI_Render", QUADRANT_3, 17, 8, 17);  // 60 FPS
    registerTask("Touch_Handler", QUADRANT_3, 20, 2, 20);
    registerTask("Audio_Buffer", QUADRANT_3, 10, 1, 10);
    registerTask("Nav_Update", QUADRANT_3, 100, 15, 100);

    // Q4 Tasks (Low Criticality + Low Urgency)
    registerTask("Telemetry", QUADRANT_4, 5000, 100, 0);
    registerTask("Data_Logging", QUADRANT_4, 1000, 30, 0);
    registerTask("File_Cleanup", QUADRANT_4, 10000, 200, 0);
    registerTask("Statistics", QUADRANT_4, 2000, 50, 0);

    printf("  Total tasks registered: %u\n", numTasks);

    // Calculate total utilization
    float total_util = 0.0f;
    for (uint32_t i = 0; i < numTasks; i++) {
        total_util += taskInfo[i].utilization;
    }
    printf("  Total system utilization: %.3f\n", total_util);
    printf("  Average per core: %.3f\n\n", total_util / NUM_CORES);
}

/* ============================================================================
 * MONITORING TASK
 * ============================================================================ */

void Task_Monitor(void *pvParameters) {
    (void)pvParameters;
    const TickType_t xPeriod = pdMS_TO_TICKS(5000);
    TickType_t xLastWakeTime = xTaskGetTickCount();

    vTaskDelay(pdMS_TO_TICKS(3000));

    while (keepRunning) {
        printf("\n\n");
        printf("================================================================================\n");
        printf("MULTI-CORE SYSTEM STATISTICS (Time: %lu ms)\n",
               xTaskGetTickCount() * portTICK_PERIOD_MS);
        printf("================================================================================\n\n");

        xSemaphoreTake(statsMutex, portMAX_DELAY);

        // Per-Core Statistics
        printf("PER-CORE UTILIZATION AND STATISTICS:\n");
        printf("--------------------------------------------------------------------------------\n");
        printf("%-6s %-10s %-10s %-10s %-10s %-12s %-10s\n",
               "Core", "Primary", "Util", "Tasks", "Executions", "Misses", "ICD");
        printf("--------------------------------------------------------------------------------\n");

        for (CoreID_t i = 0; i < NUM_CORES; i++) {
            const char *primary_str = "";
            switch (coreInfo[i].primary_quadrant) {
                case QUADRANT_1: primary_str = "Q1"; break;
                case QUADRANT_2: primary_str = "Q2"; break;
                case QUADRANT_3: primary_str = "Q3"; break;
                case QUADRANT_4: primary_str = "Q4"; break;
                default: primary_str = "--"; break;
            }

            float icd = calculateICD(i);

            printf("%-6u %-10s %-10.3f %-10u %-12u %-10u %-10.3f\n",
                   i, primary_str, coreInfo[i].current_utilization,
                   coreInfo[i].num_tasks, coreInfo[i].total_executions,
                   coreInfo[i].total_deadline_misses, icd);
        }

        printf("\n");

        // Task Statistics
        printf("PER-TASK STATISTICS:\n");
        printf("--------------------------------------------------------------------------------\n");
        printf("%-25s %-5s %-6s %-10s %-10s %-10s\n",
               "Task Name", "Quad", "Core", "Exec", "Missed", "Total(ms)");
        printf("--------------------------------------------------------------------------------\n");

        uint32_t totalExecutions = 0;
        uint32_t totalMissed = 0;
        uint32_t q1_missed = 0, q2_missed = 0, q3_missed = 0, q4_missed = 0;

        for (uint32_t i = 0; i < numTasks; i++) {
            printf("%-25s Q%-4d %-6u %-10u %-10u %-10u\n",
                   taskInfo[i].taskName,
                   taskInfo[i].quadrant,
                   taskInfo[i].assigned_core,
                   taskInfo[i].executionCount,
                   taskInfo[i].missedDeadlines,
                   taskInfo[i].totalExecutionTime);

            totalExecutions += taskInfo[i].executionCount;
            totalMissed += taskInfo[i].missedDeadlines;

            switch (taskInfo[i].quadrant) {
                case QUADRANT_1: q1_missed += taskInfo[i].missedDeadlines; break;
                case QUADRANT_2: q2_missed += taskInfo[i].missedDeadlines; break;
                case QUADRANT_3: q3_missed += taskInfo[i].missedDeadlines; break;
                case QUADRANT_4: q4_missed += taskInfo[i].missedDeadlines; break;
                default: break;
            }
        }

        // Multi-Core Metrics
        float lbi = calculateLBI();

        printf("\n");
        printf("MULTI-CORE METRICS:\n");
        printf("--------------------------------------------------------------------------------\n");
        printf("Total Executions:        %u\n", totalExecutions);
        printf("Total Deadline Misses:   %u %s\n", totalMissed,
               totalMissed > 0 ? "⚠️ ALERT!" : "✓");
        printf("  Q1 (Critical) Misses:  %u %s\n", q1_missed,
               q1_missed > 0 ? "⚠️ CRITICAL!" : "✓");
        printf("  Q2 Misses:             %u\n", q2_missed);
        printf("  Q3 Misses:             %u\n", q3_missed);
        printf("  Q4 Misses:             %u\n", q4_missed);
        printf("\n");
        printf("Load Balance Index (LBI): %.3f %s\n", lbi,
               lbi > 0.90f ? "✓ Excellent" : lbi > 0.75f ? "Good" : "⚠️ Poor");
        printf("================================================================================\n");

        xSemaphoreGive(statsMutex);

        vTaskDelayUntil(&xLastWakeTime, xPeriod);
    }

    vTaskDelete(NULL);
}

/* ============================================================================
 * MAIN
 * ============================================================================ */

int main(void) {
    signal(SIGINT, sigintHandler);

    printf("\n\n");
    printf("================================================================================\n");
    printf("FreeRTOS Multi-Core Priority-Urgency Quadrant Model Demonstration\n");
    printf("================================================================================\n");
    printf("Paper: 'Multi-Core Extension of the Priority-Urgency Quadrant Model\n");
    printf("        for Automotive Real-Time Systems'\n");
    printf("Author: Azad Mohammed Shaik\n");
    printf("================================================================================\n");
    printf("\n");
    printf("Configuration:\n");
    printf("  Number of Cores:          %d\n", NUM_CORES);
    printf("  Allocation Strategy:      %s\n",
           USE_PU_WFD_ALLOCATION ? "PU-WFD (Load-Balanced)" : "Quadrant-Based Dedication");
    printf("  Scheduling:               Partitioned (No Migration)\n");
    printf("\n");

    // Initialize mutexes
    statsMutex = xSemaphoreCreateMutex();
    allocationMutex = xSemaphoreCreateMutex();

    if (statsMutex == NULL || allocationMutex == NULL) {
        printf("ERROR: Failed to create mutexes!\n");
        return -1;
    }

    // Initialize core configuration
    initializeCoreConfiguration();

    printf("[SETUP] Core Configuration:\n");
    printf("--------------------------------------------------------------------------------\n");
    printf("%-6s %-12s %-12s %-12s\n", "Core", "Primary", "Secondary", "Max Util");
    printf("--------------------------------------------------------------------------------\n");
    for (CoreID_t i = 0; i < NUM_CORES; i++) {
        const char *pri = coreInfo[i].primary_quadrant == QUADRANT_1 ? "Q1" :
                          coreInfo[i].primary_quadrant == QUADRANT_2 ? "Q2" :
                          coreInfo[i].primary_quadrant == QUADRANT_3 ? "Q3" :
                          coreInfo[i].primary_quadrant == QUADRANT_4 ? "Q4" : "--";
        const char *sec = coreInfo[i].secondary_quadrant == QUADRANT_1 ? "Q1" :
                          coreInfo[i].secondary_quadrant == QUADRANT_2 ? "Q2" :
                          coreInfo[i].secondary_quadrant == QUADRANT_3 ? "Q3" :
                          coreInfo[i].secondary_quadrant == QUADRANT_4 ? "Q4" : "--";
        printf("%-6u %-12s %-12s %-12.2f\n", i, pri, sec, coreInfo[i].max_utilization);
    }
    printf("\n");

    // Register all automotive tasks
    registerAutomotiveTasks();

    // Allocate tasks to cores
    int allocation_result;
    if (USE_PU_WFD_ALLOCATION) {
        allocation_result = allocateTasksPU_WFD();
    } else {
        allocation_result = allocateTasksDedication();
    }

    if (allocation_result != 0) {
        printf("ERROR: Task allocation failed!\n");
        return -1;
    }

    // Create tasks with core affinity
    printf("[SETUP] Creating tasks with core affinity...\n\n");

    for (uint32_t i = 0; i < numTasks; i++) {
        uint32_t *taskIndexPtr = pvPortMalloc(sizeof(uint32_t));
        *taskIndexPtr = i;

        xTaskCreate(genericTaskFunction,
                    taskInfo[i].taskName,
                    2000,  // Stack size
                    (void *)taskIndexPtr,
                    taskInfo[i].priority,
                    &coreInfo[taskInfo[i].assigned_core].tasks[coreInfo[taskInfo[i].assigned_core].num_tasks - 1]);
    }

    // Create monitoring task
    xTaskCreate(Task_Monitor, "Monitor", 4000, NULL, tskIDLE_PRIORITY + 2, NULL);

    printf("Starting FreeRTOS Multi-Core Scheduler...\n");
    printf("Press Ctrl+C to stop\n\n");

    vTaskStartScheduler();

    printf("ERROR: Scheduler failed to start!\n");
    return -1;
}

/* ============================================================================
 * FreeRTOS HOOK FUNCTIONS
 * ============================================================================ */

void vApplicationIdleHook(void) {
    usleep(1000);
}

void vAssertCalled(const char * const pcFileName, unsigned long ulLine) {
    printf("ASSERT FAILED: %s:%lu\n", pcFileName, ulLine);
}

void vApplicationMallocFailedHook(void) {
    printf("ERROR: Malloc failed!\n");
    exit(-1);
}

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName) {
    (void)xTask;
    printf("ERROR: Stack overflow in task: %s\n", pcTaskName);
    exit(-1);
}
