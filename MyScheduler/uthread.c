#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/time.h>
#include <ucontext.h>
#include "uthread.h"
#include "list_head.h"
#include "types.h"
/* You can include other header file, if you want. */

/*******************************************************************
 * struct tcb
 *
 * DESCRIPTION
 *    tcb is a thread control block.
 *    This structure contains some information to maintain thread.
 *
 ******************************************************************/
struct tcb {
    struct list_head list;
    ucontext_t *context;
    enum uthread_state state;
    int tid;

    int lifetime; // This for preemptive scheduling
    int priority; // This for priority scheduling
};

/***************************************************************************************
 * LIST_HEAD(tcbs);
 *
 * DESCRIPTION
 *    Initialize list head of thread control block.
 *
 **************************************************************************************/
LIST_HEAD(tcbs);

int n_tcbs = 0;
enum uthread_sched_policy cur_policy;
int arr[100] = {};
int finish = 0;

int lock = 0;

struct ucontext_t *t_context;


/***************************************************************************************
 * next_tcb()
 *
 * DESCRIPTION
 *
 *    Select a tcb with current scheduling policy
 *
 **************************************************************************************/
void next_tcb() {
    sigset_t set;
    int s;
    sigaddset(&set, SIGALRM);

    if(lock) sigprocmask(SIG_BLOCK, &set, NULL);
    else sigprocmask(SIG_UNBLOCK, &set, NULL);
    
    /* TODO: You have to implement this function. */
    struct tcb *now_tcb = NULL;
    struct list_head *tcb_pointer = NULL;
    struct tcb *next_tcb = NULL;

    list_for_each(tcb_pointer, &tcbs) {
        now_tcb = list_entry(tcb_pointer, struct tcb, list);

        if(now_tcb->state == RUNNING) {
            if (cur_policy == FIFO) {
                next_tcb = fifo_scheduling(now_tcb);
                if(next_tcb->tid == now_tcb->tid) return;
            }
            else if (cur_policy == RR)
                next_tcb = rr_scheduling(now_tcb);
            else if (cur_policy == PRIO)
                next_tcb = prio_scheduling(now_tcb);
            else if (cur_policy == SJF){
                next_tcb = sjf_scheduling(now_tcb);
                if(next_tcb->tid == now_tcb->tid) return;
            }

            if (now_tcb->tid == -1 && next_tcb->tid == -1) {
                return;
            }

            while(sigwait(&set, &s) != 0){
                swapcontext(now_tcb->context, next_tcb->context);
            }
            fprintf(stderr, "SWAP %d -> %d\n", now_tcb->tid, next_tcb->tid);
            return;
        }
    }
    return;
}

/***************************************************************************************
 * struct tcb *fifo_scheduling(struct tcb *next)
 *
 * DESCRIPTION
 *
 *    This function returns a tcb pointer using First-In-First-Out policy
 *
 **************************************************************************************/

struct tcb *fifo_scheduling(struct tcb *now) {

    /* TODO: You have to implement this function. */

    if(now->tid == MAIN_THREAD_TID){
        struct tcb *next_tcb = NULL;
        struct list_head *tcb_pointer = NULL;

        list_for_each(tcb_pointer, &tcbs) {
            next_tcb = list_entry(tcb_pointer, struct tcb, list);
            if(next_tcb->state == READY && next_tcb->tid != MAIN_THREAD_TID) {
                now->state = READY;
                next_tcb->lifetime -= 1;
                next_tcb->state = RUNNING;
                return next_tcb;
            }
        }
    }

    //same thread
    if(now->lifetime > 0){
        now->lifetime -= 1;
        return now;
    }

    //one thread -> other thread
    if(now->lifetime == 0){
        n_tcbs -= 1;
        //one thread -> another thread
        if(n_tcbs > 1){
            struct tcb *next_tcb = NULL;
            struct list_head *tcb_pointer = NULL;

            list_for_each(tcb_pointer, &tcbs) {
                next_tcb = list_entry(tcb_pointer, struct tcb, list);
                if(next_tcb->state == READY && next_tcb->tid != MAIN_THREAD_TID){
                    now->state = TERMINATED;
                    next_tcb->lifetime -= 1;
                    next_tcb->state = RUNNING;
                    arr[now->tid] = 1;
                    return next_tcb;
                }
            }
        }

        //one thread -> main thread
        else {
            struct tcb *next_tcb = NULL;
            struct list_head *tcb_pointer = NULL;

            list_for_each(tcb_pointer, &tcbs) {
                next_tcb = list_entry(tcb_pointer, struct tcb, list);
                if(next_tcb->tid == MAIN_THREAD_TID){
                    now->state = TERMINATED;
                    next_tcb->state = RUNNING;
                    arr[now->tid] = 1;
                    finish = 1;
                    return next_tcb;
                }
            }
        }
    }
}

/***************************************************************************************
 * struct tcb *rr_scheduling(struct tcb *next)
 *
 * DESCRIPTION
 *
 *    This function returns a tcb pointer using round robin policy
 *
 **************************************************************************************/
struct tcb *rr_scheduling(struct tcb *now) {
    
    /* TODO: You have to implement this function. */

    if(now->tid == MAIN_THREAD_TID){
        finish = 0;
    }

    if(now->lifetime == 0){
        now->state = TERMINATED;
        n_tcbs -= 1;
        arr[now->tid] = 1;
    }
    else{
        now->state = READY;
    }

    while(1){
        struct tcb *next_tcb = list_next_entry(now, list);
        if (next_tcb->lifetime > 0 && next_tcb->state == READY){
            next_tcb->lifetime -= 1;
            next_tcb->state = RUNNING;

            if(next_tcb->tid == MAIN_THREAD_TID){
                finish = 1;
            }
            return next_tcb;
        }
        now = next_tcb;
    }
}

/***************************************************************************************
 * struct tcb *prio_scheduling(struct tcb *next)
 *
 * DESCRIPTION
 *
 *    This function returns a tcb pointer using priority policy
 *
 **************************************************************************************/
struct tcb *prio_scheduling(struct tcb *now) {

    /* TODO: You have to implement this function. */

    if(now->tid == MAIN_THREAD_TID){
        int max_priority = -1;
        struct tcb *max_priority_tcb = NULL;

        struct tcb *next_tcb = NULL;
        struct list_head *tcb_pointer = NULL;

        list_for_each(tcb_pointer, &tcbs) {
            next_tcb = list_entry(tcb_pointer, struct tcb, list);
            if((next_tcb->state == READY) && (max_priority < next_tcb->priority)){
                max_priority = next_tcb->priority;
                max_priority_tcb = next_tcb;
            }
        }
        now->state = READY;
        max_priority_tcb->lifetime -= 1;
        max_priority_tcb->state = RUNNING;

        return max_priority_tcb;
    }

    if(now->lifetime > 0){
        now->lifetime -= 1;
        return now;
    }

    if(now->lifetime == 0){
        n_tcbs -= 1;
        if(n_tcbs > 1){
            int max_priority = -1;
            struct tcb *max_priority_tcb = NULL;


            struct tcb *next_tcb = NULL;
            struct list_head *tcb_pointer = NULL;

            list_for_each(tcb_pointer, &tcbs) {
                next_tcb = list_entry(tcb_pointer, struct tcb, list);
                if(next_tcb->state == READY && next_tcb->tid != MAIN_THREAD_TID && max_priority < next_tcb->priority){
                    max_priority = next_tcb->priority;
                    max_priority_tcb = next_tcb;
                }
            }
            
            now->state = TERMINATED;
            max_priority_tcb->lifetime -= 1;
            max_priority_tcb->state = RUNNING;
            arr[now->tid] = 1;
            return max_priority_tcb;
        }
        else{
            struct tcb *next_tcb = NULL;
            struct list_head *tcb_pointer = NULL;

            list_for_each(tcb_pointer, &tcbs) {
                next_tcb = list_entry(tcb_pointer, struct tcb, list);
                if(next_tcb->tid == MAIN_THREAD_TID){
                    now->state = TERMINATED;
                    next_tcb->state = RUNNING;
                    arr[now->tid] = 1;
                    finish = 1;
                    return next_tcb;
                }
            }
        }
    }
}

/***************************************************************************************
 * struct tcb *sjf_scheduling(struct tcb *next)
 *
 * DESCRIPTION
 *
 *    This function returns a tcb pointer using shortest job first policy
 *
 **************************************************************************************/
struct tcb *sjf_scheduling(struct tcb *now) {

    /* TODO: You have to implement this function. */
    if(now->tid == MAIN_THREAD_TID){
        int min_lifetime = MAIN_THREAD_LIFETIME;
        struct tcb *min_lifetime_tcb = NULL;

        struct tcb *next_tcb = NULL;
        struct list_head *tcb_pointer = NULL;

        list_for_each(tcb_pointer, &tcbs) {
            next_tcb = list_entry(tcb_pointer, struct tcb, list);
            if((next_tcb->state == READY) && (min_lifetime > next_tcb->lifetime)){
                min_lifetime = next_tcb->lifetime;
                min_lifetime_tcb = next_tcb;
            }
        }
        now->state = READY;
        min_lifetime_tcb->lifetime -= 1;
        min_lifetime_tcb->state = RUNNING;

        return min_lifetime_tcb;
    }

    if(now->lifetime > 0){
        now->lifetime -= 1;
        return now;
    }

    if(now->lifetime == 0){
        n_tcbs -= 1;
        if(n_tcbs > 1){
            int min_lifetime = MAIN_THREAD_LIFETIME;
            struct tcb *min_lifetime_tcb = NULL;

            struct tcb *next_tcb = NULL;
            struct list_head *tcb_pointer = NULL;

            list_for_each(tcb_pointer, &tcbs) {
                next_tcb = list_entry(tcb_pointer, struct tcb, list);
                if(next_tcb->state == READY && next_tcb->tid != MAIN_THREAD_TID && min_lifetime > next_tcb->lifetime){
                    min_lifetime = next_tcb->lifetime;
                    min_lifetime_tcb = next_tcb;
                }
            }
            
            now->state = TERMINATED;
            min_lifetime_tcb->lifetime -= 1;
            min_lifetime_tcb->state = RUNNING;
            arr[now->tid] = 1;
            return min_lifetime_tcb;
        }
        else{
            struct tcb *next_tcb = NULL;
            struct list_head *tcb_pointer = NULL;

            list_for_each(tcb_pointer, &tcbs) {
                next_tcb = list_entry(tcb_pointer, struct tcb, list);
                if(next_tcb->tid == MAIN_THREAD_TID){
                    now->state = TERMINATED;
                    next_tcb->state = RUNNING;
                    arr[now->tid] = 1;
                    finish = 1;
                    return next_tcb;
                }
            }
        }

    }
}

/***************************************************************************************
 * uthread_init(enum uthread_sched_policy policy)
 *
 * DESCRIPTION

 *    Initialize main tread control block, and do something other to schedule tcbs
 *
 **************************************************************************************/
void uthread_init(enum uthread_sched_policy policy) {

    /* TODO: You have to implement this function. */

    // Set current policy
    cur_policy = policy;

    // Initialize main thread control block
    struct tcb *main_tcb = (struct tcb *)malloc(sizeof(struct tcb));
    main_tcb->tid = MAIN_THREAD_TID;
    main_tcb->lifetime = MAIN_THREAD_LIFETIME;
    main_tcb->priority = MAIN_THREAD_PRIORITY;
    main_tcb->state = RUNNING;

    main_tcb->context = (struct ucontext_t *)malloc(sizeof(struct ucontext_t));
    getcontext(main_tcb->context);

    // Add main thread control block into TCBs list
    list_add_tail(&(main_tcb->list), &tcbs);
    n_tcbs++;


    /* DO NOT MODIFY THESE TWO LINES */
    __create_run_timer();
    __initialize_exit_context();
}


/***************************************************************************************
 * uthread_create(void* stub(void *), void* args)
 *
 * DESCRIPTION
 *
 *    Create user level thread. This function returns a tid.
 *
 **************************************************************************************/
int uthread_create(void* stub(void *), void* args) {

    /* TODO: You have to implement this function. */

    //Initialize new thread control block
    struct tcb *new_tcb = (struct tcb *)malloc(sizeof(struct tcb));
    new_tcb->tid = ((int *)args)[0];
    new_tcb->lifetime = ((int *)args)[1];
    new_tcb->priority = ((int *)args)[2];
    new_tcb->state = READY;

    new_tcb->context = (struct ucontext_t *)malloc(sizeof(struct ucontext_t));
    getcontext(new_tcb->context);
    (new_tcb->context)->uc_stack.ss_sp = malloc(MAX_STACK_SIZE);
    (new_tcb->context)->uc_stack.ss_size = MAX_STACK_SIZE;

    //linking to mainthread
    struct tcb *find_tcb = NULL;
    struct list_head *tcb_pointer = NULL;

    list_for_each(tcb_pointer, &tcbs){
        find_tcb = list_entry(tcb_pointer, struct tcb, list);
        if(find_tcb->tid == -1){
            (new_tcb->context)->uc_link = (find_tcb->context);
        }
    }

    list_add_tail(&(new_tcb->list), &tcbs);
    n_tcbs++;

    makecontext(new_tcb->context, (void*)stub, 0);

    return new_tcb->tid;
}

/***************************************************************************************
 * uthread_join(int tid)
 *
 * DESCRIPTION
 *
 *    Wait until thread context block is terminated.
 *
 **************************************************************************************/


void uthread_join(int tid) {

    /* TODO: You have to implement this function. */

    if(cur_policy == FIFO || cur_policy == SJF){
        if(n_tcbs == 1){
            fprintf(stderr, "JOIN %d\n", tid);
        }
        else{
            while(arr[tid] == 0 || finish == 0){
            }
            finish = 0;
            fprintf(stderr, "JOIN %d\n", tid);
        }
    }
    else{
            while(arr[tid] == 0 || finish == 0){
                
            }
            fprintf(stderr, "JOIN %d\n", tid);
    }
}

/***************************************************************************************
 * __exit()
 *
 * DESCRIPTION
 *
 *    When your thread is terminated, the thread have to modify its state in tcb block.
 *
 **************************************************************************************/
void __exit() {

    /* TODO: You have to implement this function. */

    //So you have to create the context that will be resumed after the terminated thread.
    

}

/***************************************************************************************
 * __initialize_exit_context()
 *
 * DESCRIPTION
 *
 *    This function initializes exit context that your thread context will be linked.
 *
 **************************************************************************************/
void __initialize_exit_context() {

    /* TODO: You have to implement this function. */
}

/***************************************************************************************
 *
 * DO NOT MODIFY UNDER THIS LINE!!!
 *
 **************************************************************************************/

static struct itimerval time_quantum;
static struct sigaction ticker;

void __scheduler() {
    if(n_tcbs > 1){
        next_tcb();
    }
}

void __create_run_timer() {

    time_quantum.it_interval.tv_sec = 0;
    time_quantum.it_interval.tv_usec = SCHEDULER_FREQ;
    time_quantum.it_value = time_quantum.it_interval;
    ticker.sa_handler = __scheduler;

    sigemptyset(&ticker.sa_mask);

    sigaction(SIGALRM, &ticker, NULL);

    ticker.sa_flags = 0;

    setitimer(ITIMER_REAL, &time_quantum, (struct itimerval*) NULL);
}

void __free_all_tcbs() {
    struct tcb *temp;

    list_for_each_entry(temp, &tcbs, list) {
        if (temp != NULL && temp->tid != -1) {
            list_del(&temp->list);
            free(temp->context);
            free(temp);
            n_tcbs--;
            temp = list_first_entry(&tcbs, struct tcb, list);
        }
    }
    temp = NULL;
    free(t_context);
}
