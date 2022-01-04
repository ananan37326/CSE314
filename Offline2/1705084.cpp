#include <iostream>
#include <bits/stdc++.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <time.h>
#include <string>

using namespace std;

#define PASSENGER_CAPACITY 5

int M = 3;  //No Of Kiosk
int N = 2;  //No Of Belts For Security Check
int P = 5; //No Of Passengers Can Be Served By Each Belt
int w = 8;  //Self Check-In Time at a Kiosk
int x = 10; //Security Check Time
int y = 12; //Boarding Time at the Gate
int z = 14; //Walking On VIP Channnel Time

int current_time = 0;
int global_time = 0;

//semaphores and pthread_mutex
pthread_mutex_t *kiosk_mutex;
pthread_mutex_t print_mutex;
pthread_mutex_t boarding_gate_mutex;
sem_t *security_belts_sem;

struct Passenger
{
    int id;
    int belt_no;
    int kiosk_no;
    bool has_boarding_pass;
    bool is_vip;
    bool has_boarded;
    string name = "";
};

void *global_time_count(void *arg)
{
    // while (true)
    // {
    //     sleep(1);
    //     current_time = current_time + 1;
    // }
    const clock_t begin_time = clock();
    while (true)
    {
        current_time = round(clock() - begin_time) / CLOCKS_PER_SEC;
    }
}

void initializeMutexAndSemaphore()
{

    int return_value;

    //initialize kiosk_mutex;
    kiosk_mutex = new pthread_mutex_t[M];
    for (int i = 0; i < M; i++)
    {
        pthread_mutex_t temp_kiosk_mutex;
        return_value = pthread_mutex_init(&kiosk_mutex[i], NULL);
        if (return_value != 0)
        {
            cout << "Kiosk " << i << " initialization failed" << endl;
        }
    }

    //initialize print mutex
    return_value = pthread_mutex_init(&print_mutex, NULL);
    if (return_value != 0)
    {
        cout << "Print mutex initialization failed" << endl;
    }

    //initialize security belts counting semaphore
    security_belts_sem = new sem_t[N];
    for (int i = 0; i < N; i++)
    {
        return_value = sem_init(&security_belts_sem[i], 0, P);
        if (return_value != 0)
        {
            cout << "Security Belts " << i << " initialization failed" << endl;
        }
    }

    //initialize boarding gate mutex
    return_value = pthread_mutex_init(&boarding_gate_mutex, NULL);
    if (return_value != 0)
    {
        cout << "Boarding Gate mutex initialization failed" << endl;
    }
}

void self_check_in_at_kiosk(Passenger *passenger)
{
    int kiosk_no = passenger->kiosk_no;
    pthread_mutex_lock(&kiosk_mutex[kiosk_no]);

    int self_check_start_time = current_time;
    pthread_mutex_lock(&print_mutex);
    cout << passenger->name << " has started  self check-in at kiosk " << passenger->kiosk_no << " at time " << self_check_start_time << endl;
    pthread_mutex_unlock(&print_mutex);

    sleep(w);

    //**

    pthread_mutex_unlock(&kiosk_mutex[kiosk_no]);

    //before or after the portion ** has to decide later

    passenger->has_boarding_pass = true;
    int self_check_end_time = current_time;
    pthread_mutex_lock(&print_mutex);
    cout << passenger->name << " has finished self check-in at kiosk " << passenger->kiosk_no << " at time " << self_check_end_time << endl;
    pthread_mutex_unlock(&print_mutex);
}

void security_check_normal_passenger(Passenger *passenger)
{
    int belt_no = passenger->belt_no;

    sem_wait(&security_belts_sem[belt_no]);

    int security_check_start_time = current_time;
    pthread_mutex_lock(&print_mutex);
    cout << passenger->name << " has started  the security check in belt " << passenger->belt_no << " at time " << security_check_start_time << endl;
    pthread_mutex_unlock(&print_mutex);

    sleep(x);

    //**

    sem_post(&security_belts_sem[belt_no]);

    //before or after the portion ** has to decide later

    int security_check_end_time = current_time;
    pthread_mutex_lock(&print_mutex);
    cout << passenger->name << " has crossed  the security check in belt " << passenger->belt_no << " at time " << security_check_end_time << endl;
    pthread_mutex_unlock(&print_mutex);
}

void pass_boarding_gate(Passenger *passenger)
{

    pthread_mutex_lock(&boarding_gate_mutex);

    int boarding_gate_pass_start_time = current_time;
    pthread_mutex_lock(&print_mutex);
    cout << passenger->name << " has started  boarding the plane at time " << boarding_gate_pass_start_time << endl;
    pthread_mutex_unlock(&print_mutex);

    sleep(y);

    passenger->has_boarded = passenger->has_boarding_pass;

    //**

    pthread_mutex_unlock(&boarding_gate_mutex);

    //before or after the portion ** has to decide later
    int boarding_gate_pass_end_time = current_time;
    if (passenger->has_boarding_pass)
    {
        pthread_mutex_lock(&print_mutex);
        cout << passenger->name << " has boarded  the plane at time " << boarding_gate_pass_end_time << endl;
        pthread_mutex_unlock(&print_mutex);
        // return true;
    }
    else
    {
        pthread_mutex_lock(&print_mutex);
        cout << passenger->name << " has failed   to board the plane this attempt at time " << boarding_gate_pass_end_time << endl;
        pthread_mutex_unlock(&print_mutex);
        // return false;
    }
}

void *visit_airport(void *passenger)
{
    srand(time(NULL));
    Passenger *arrived_passenger = (Passenger *)passenger;

    self_check_in_at_kiosk(arrived_passenger);
S
    if (!arrived_passenger->is_vip)
    {
        int security_check_waiting_start_time = current_time;
        pthread_mutex_lock(&print_mutex);
        cout << arrived_passenger->name << " has started  waiting for security check in belt " << arrived_passenger->belt_no << " at time " << security_check_waiting_start_time << endl;
        pthread_mutex_unlock(&print_mutex);

        security_check_normal_passenger(arrived_passenger);
    }

    while (!arrived_passenger->has_boarded)
    {
        int boarding_gate_wait_start_time = current_time;
        pthread_mutex_lock(&print_mutex);
        cout << arrived_passenger->name << " has started  waiting to be boarded at time " << boarding_gate_wait_start_time << endl;
        pthread_mutex_unlock(&print_mutex);

        int boarding_pass = rand() % 2;
        if (boarding_pass)
        {
            arrived_passenger->has_boarding_pass = true;
        }
        else
        {
            //losing the boarding pass
            arrived_passenger->has_boarding_pass = false;
        }
        pass_boarding_gate(arrived_passenger);
        if(arrived_passenger->has_boarded) break;
    }
}

int main()
{

    pthread_t timer_thread;

    int return_value = pthread_create(&timer_thread, NULL, global_time_count, NULL);
    if (return_value != 0)
    {
        cout << "Global Timer count thread creation failed" << endl;
        exit(0);
    }

    initializeMutexAndSemaphore();

    srand(time(NULL));

    pthread_t *passenger_threads;
    passenger_threads = new pthread_t[PASSENGER_CAPACITY + 1];
    Passenger **passenger_list = new Passenger *[PASSENGER_CAPACITY + 1];

    //creating thread for every passenger arrived
    for (int i = 1; i <= PASSENGER_CAPACITY; i++)
    {
        //current_time = current_time + 2;
        sleep(2);

        //defining initial value for each passenger
        Passenger *passenger = new Passenger;
        passenger->id = i;
        int type = rand() % 2;
        if (type == 0)
        {
            passenger->is_vip = true;
            string name = "Passenger ";
            passenger->name = name.append(to_string(passenger->id)).append(" (VIP) ");
        }
        else
        {
            string name = "Passenger ";
            passenger->is_vip = false;
            passenger->name = name.append(to_string(passenger->id)).append("       ");
        }
        passenger->kiosk_no = rand() % M;
        passenger->belt_no = rand() % N;
        passenger->has_boarding_pass = false;
        passenger->has_boarded = false;

        passenger_list[i] = passenger;

        return_value = pthread_create(&passenger_threads[i], NULL, visit_airport, (void *)passenger);
        int arrival_time = current_time;

        pthread_mutex_lock(&print_mutex);
        cout << passenger->name << " has arrived  at airport at time " << arrival_time << endl;
        pthread_mutex_unlock(&print_mutex);

        if (return_value != 0)
        {
            pthread_mutex_lock(&print_mutex);
            cout << passenger->name << " thread creation failed" << endl;
            pthread_mutex_lock(&print_mutex);
        }
    }

    //joining each passenger thread
    for (int i = 1; i <= PASSENGER_CAPACITY; i++)
    {
        Passenger *passenger = passenger_list[i];
        return_value = pthread_join(passenger_threads[i], NULL);
        if (return_value != 0)
        {
            pthread_mutex_lock(&print_mutex);
            cout << passenger->name << " thread join failed" << endl;
            pthread_mutex_unlock(&print_mutex);
        }
    }
    return 0;
}