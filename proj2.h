/**
 *
 * @file proj2.h
 *
 * @brief Project: IO23 - Post Office
 *
 * @author xbalek02 Miroslav Bálek
 *
 * Note: This project soultion is inspired by book written by Allen B. Downey:
 * SRC:
 * - https://greenteapress.com/semaphores/LittleBookOfSemaphores.pdf
 *
 *
 *   Last modified: Apr 25, 2023
 */

#ifndef PROJ2_H
#define PROJ2_H

#define ARGCOUNT 6

#define ANY -1
#define NONE 0
#define Z_STARTED 1
#define Z_GOING_HOME 2
#define Z_ENTERING_OFFICE 3
#define Z_CALLED_BY_WORKER 4

#define U_STARTED 5
#define U_SERVING_SERVICE 6
#define U_SERVICE_FINISHED 7
#define U_BREAK 8
#define U_BREAK_FINISHED 9
#define U_GOING_HOME 10
#define DEBUG 11

typedef struct
{
    int NZ;
    int NU;
    int TZ;
    int TU;
    int F;
} Arg;

typedef struct
{
    int *count;
    sem_t *queue;
} Service;

Arg ParseArgs(int argc, char *const argv[]);

void exit_error(char *msg, int errcode);

void customer(Arg args, int id);

void postman(Arg args, int id);

void change_post_status();
void customer_go_home(int id);
void wait_sem(sem_t **sem);
void post_sem(sem_t **sem);
void init_sem(sem_t **sem, int value);
void init_semaphores();
void destroy_sem(sem_t **sem);
void cleanup_semaphores();

void clear_and_open_output_file();
void output(int action_type, int id, int service);
void usleep_random_in_range(int lower, int upper);
int random_int(int lower, int upper);
#endif