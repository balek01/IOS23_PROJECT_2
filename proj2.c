/**
 *
 * @file proj2.c
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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>
#include <time.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>
#include <semaphore.h>
#include "proj2.h"

sem_t *mutex_post_status;
sem_t *mutex_output;
sem_t *mutex_queue_update;
Service service1;
Service service2;
Service service3;
FILE *file;
bool *post_open;
int *action_id;
int seed;

Arg ParseArgs(int argc, char *const argv[])
{
    Arg args;
    char *str;

    // Check argument count
    if (argc != ARGCOUNT)
        exit_error("Incorrect argument count.", 1);

    // Check if argument is digit
    for (int i = 1; i < argc; i++)
    {
        str = argv[i];
        for (int i = 0; str[i] != '\0'; i++)
        {
            if (isdigit(str[i]) == 0)
            {
                exit_error("Some of provided arguments are not positive numbers.", 1);
            }
        }
    }
    args.NZ = atoi(argv[1]);
    args.NU = atoi(argv[2]);
    args.TZ = atoi(argv[3]);
    args.TU = atoi(argv[4]);
    args.F = atoi(argv[5]);

    if (!((0 <= args.TZ && args.TZ <= 10000) && (0 <= args.F && args.F <= 10000)))
        exit_error("TZ or F values are not in range(0 <= arg <= 10000).", 1);

    if (args.NU < 1)
        exit_error("NU must be bigger then 0", 1);

    if (!(0 <= args.TU && args.TU <= 100))
        exit_error("TU value are not in range (0 <= TU <= 100).", 1);

    return args;
}

void exit_error(char *msg, int errcode)
{
    fprintf(stderr, "ERROR: %s Exit code: %d\n", msg, errcode);
    exit(errcode);
}

void customer_wait_to_be_called(int id, int service)
{
    if (service == 1)
    {
        wait_sem(&(service1.queue));
        output(Z_CALLED_BY_WORKER, id, service);
    }
    else if (service == 2)
    {
        //  wait_sem(&(service2.queue));
        output(Z_CALLED_BY_WORKER, id, service);
    }
    else if (service == 3)
    {
        // wait_sem(&(service3.queue));
        output(Z_CALLED_BY_WORKER, id, service);
    }

    usleep_random_in_range(0, 10);
    customer_go_home(id);
}

void customer_queue_up(int id)
{
    // TODO fix ti 1-3 debug
    int rand_service = random_int(1, 1);
    output(Z_ENTERING_OFFICE, id, rand_service);
    wait_sem(&mutex_queue_update);
    if (rand_service == 1)
        (*(service1.count))++;

    if (rand_service == 2)
        (*(service2.count))++;

    if (rand_service == 3)
        (*(service3.count))++;

    post_sem(&mutex_queue_update);
    customer_wait_to_be_called(id, rand_service);
}

void customer(Arg args, int id)
{
    // U started
    output(Z_STARTED, id, NONE);
    // U sleep
    usleep_random_in_range(0, args.TZ);
    // if post is closed go home
    wait_sem(&mutex_post_status);
    if (!post_open)
    {
        post_sem(&mutex_post_status);
        customer_go_home(id);
    }
    post_sem(&mutex_post_status);
    // else continue
    customer_queue_up(id);

    exit(0);
}

bool check_any_customer(int service)
{
    bool result = true;
    wait_sem(&mutex_queue_update);
    if (service == ANY)
    {
        if (*(service1.count) == 0 && *(service2.count) == 0 && *(service3.count) == 0)
        {
            result = false;
        }
    }
    else if (service == 1 && *(service1.count) == 0)
    {
        result = false;
    }
    else if (service == 2 && *(service2.count) == 0)
    {
        result = false;
    }
    else if (service == 3 && *(service3.count) == 0)
    {
        result = false;
    }

    post_sem(&mutex_queue_update);

    return result;
}
void postman(Arg args, int id)
{
    // U started
    output(U_STARTED, id, NONE);
    // TODO deleto u sleep debug
    usleep(6666);
    if (check_any_customer(ANY))
    {
        output(DEBUG, id, ANY);
        int rand_service;
        bool finding_queue = check_any_customer(ANY);
        if (finding_queue)
        {

            while (finding_queue)
            {

                rand_service = random_int(1, 3);
                // TODO delete debug
                output(DEBUG, id, rand_service);
                finding_queue = !check_any_customer(rand_service);
            }
        }

        wait_sem(&mutex_queue_update);
        (*(service2.count))--;
        post_sem(&mutex_queue_update);
        output(U_SERVING_SERVICE, id, rand_service);
        post_sem(&(service1.queue));
    }
    else
    {
        // TODO deleto debug
        output(DEBUG, id, NONE);
        // TODO: no customer left
    }
    // U sleep
    usleep_random_in_range(0, args.TZ);
    output(U_GOING_HOME, id, NONE);
    exit(0);
}

void change_post_status()
{
    wait_sem(&mutex_post_status);
    *post_open = !*post_open;
    post_sem(&mutex_post_status);
}

void output(int action_type, int id, int service)
{
    wait_sem(&mutex_output);
    *action_id += 1;
    switch (action_type)
    {
    case Z_STARTED:
        fprintf(file, "%d: Z %d: started\n", *action_id, id);
        break;
    case Z_GOING_HOME:
        fprintf(file, "%d: Z %d: going home\n", *action_id, id);
        break;
    case Z_ENTERING_OFFICE:
        fprintf(file, "%d: Z %d: entering office for a service %d\n", *action_id, id, service);
        break;
    case Z_CALLED_BY_WORKER:
        fprintf(file, "%d: Z %d: called by office worker\n", *action_id, id);
        break;
        // postman
    case U_STARTED:
        fprintf(file, "%d: U %d: started\n", *action_id, id);
        break;
    case U_SERVING_SERVICE:
        fprintf(file, "%d: U %d: serving a service of type %d\n", *action_id, id, service);
        break;
    case U_SERVICE_FINISHED:
        fprintf(file, "%d: U %d: service finished\n", *action_id, id);
        break;
    case U_BREAK:
        fprintf(file, "%d: U %d: taking break\n", *action_id, id);
        break;
    case U_BREAK_FINISHED:
        fprintf(file, "%d: U %d: break finished\n", *action_id, id);
        break;
    case U_GOING_HOME:
        fprintf(file, "%d: U %d: going home\n", *action_id, id);
        break;
    case DEBUG:
        fprintf(file, "%d: ID %d: Debug %d\n", *action_id, id, service);
        break;
    default:
        exit_error("Internal error: Unknown output action_type.", 1);
    }

    post_sem(&mutex_output);
}

void customer_go_home(int id)
{
    output(Z_GOING_HOME, id, NONE);
    exit(0);
}

void init_semaphores()
{
    init_sem(&mutex_post_status, 1);
    init_sem(&mutex_output, 1);
    init_sem(&mutex_queue_update, 1);
    init_sem(&service1.queue, 0);
    init_sem(&service2.queue, 0);
    init_sem(&service3.queue, 0);

    // TODO: genericity
    post_open = mmap(NULL, sizeof(bool), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
    if (post_open == MAP_FAILED)
        exit_error("Mmap failed.", 1);

    action_id = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
    if (action_id == MAP_FAILED)
        exit_error("Mmap failed.", 1);

    service1.count = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
    if (service1.count == MAP_FAILED)
        exit_error("Mmap failed.", 1);
    service2.count = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
    if (service2.count == MAP_FAILED)
        exit_error("Mmap failed.", 1);

    service3.count = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
    if (service3.count == MAP_FAILED)
        exit_error("Mmap failed.", 1);

    *(service1.count) = 0;
    *(service2.count) = 0;
    *(service3.count) = 0;
    *post_open = true;
    *action_id = 0;
}

void wait_sem(sem_t **sem)
{
    if (sem_wait(*sem) == -1)
        exit_error("Wait sem failed.", 1);
}

void post_sem(sem_t **sem)
{
    if (sem_post(*sem) == -1)
        exit_error("Post sem failed.", 1);
}

void destroy_sem(sem_t **sem)
{
    if (sem_destroy(*sem) == -1)
        exit_error("Destroy sem failed.", 1);

    if (munmap((*sem), sizeof(sem_t)) == -1)
        exit_error("Munmap sem failed.", 1);
}

void cleanup_semaphores()
{
    destroy_sem(&mutex_post_status);
    destroy_sem(&mutex_queue_update);
    destroy_sem(&mutex_output);
    destroy_sem(&service1.queue);
    destroy_sem(&service2.queue);
    destroy_sem(&service3.queue);
    if (munmap(post_open, sizeof(bool)) == -1)
        exit_error("Munmap sem failed.", 1);

    if (munmap(action_id, sizeof(int)) == -1)
        exit_error("Munmap sem failed.", 1);

    if (munmap(service1.count, sizeof(int)) == -1)
        exit_error("Munmap sem failed.", 1);

    if (munmap(service2.count, sizeof(int)) == -1)
        exit_error("Munmap sem failed.", 1);

    if (munmap(service3.count, sizeof(int)) == -1)
        exit_error("Munmap sem failed.", 1);
}

void init_sem(sem_t **sem, int value)
{
    *sem = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
    if (*sem == MAP_FAILED)
        exit_error("Mmap failed.", 1);

    if (sem_init(*sem, 1, value) == -1)
        exit_error("Init sem failed.", 1);
}

int random_int(int lower, int upper)
{
    seed += getpid() * 3696;

    srand(seed);
    // generate random integer in the range <lower, upper>
    return (int)rand() % (upper - lower + 1) + lower;
}
void usleep_random_in_range(int lower, int upper)
{
    int rand_n = random_int(lower, upper);
    // usleep
    usleep(rand_n);
}

void clear_and_open_output_file()
{
    // Clear the output file.

    file = fopen("proj2.out", "w");
    if (file == NULL)
    {
        exit_error("Fopen failed.", 1);
    }

    if (fclose(file) == EOF)
    {
        exit_error("fclose close.", 1);
    }

    file = fopen("proj2.out", "a");
    if (file == NULL)
    {
        exit_error("Fopen 2 failed.", 1);
    }
}
int main(int argc, char *const argv[])
{
    Arg args = ParseArgs(argc, argv);
    init_semaphores();
    clear_and_open_output_file();
    setbuf(stdout, NULL);
    setbuf(stderr, NULL);
    setbuf(file, NULL);

    for (int i = 1; i < args.NZ + 1; i++)
    {
        pid_t customer_id = fork();

        if (customer_id < 0)
            exit_error("Fork failed.", 1);

        if (customer_id == 0)
            customer(args, i);
    }

    for (int i = 1; i < args.NU + 1; i++)
    {
        pid_t postman_id = fork();
        if (postman_id < 0)
            exit_error("Fork postman failed.", 1);

        if (postman_id == 0)
            postman(args, i);
    }

    usleep_random_in_range((int)args.F / 2, args.F);
    change_post_status(true);
    // wait for all procceses to finish
    while (wait(NULL) > 0)
        ;
    if (fclose(file) == EOF)
        exit_error("Final fclose close.", 1);

    cleanup_semaphores();
    return 0;
}
