#include<bits/stdc++.h>
#include<pthread.h>
#include<semaphore.h>
#include<unistd.h>
#include"passenger.h"

#define MAX_PASSENGERS 5
#define TOTAL_TIME 45
#define LEFT_TO_RIGHT 1
#define RIGHT_TO_LEFT -1

using namespace std;

// M - number of kiosk  N - number of belts    P - passengers per belt
// w - self check-in time   x - security check time
// y - boarding time    z - walking on VIP channel time
int M,N,P,w,x,y,z;

// time counting variable
int global_time = 0;
int current_time = 0;
int vip_count = 0;
int lost_count = 0;

// Passenger and thread arrays
Passenger passengers[MAX_PASSENGERS];
pthread_t thread_passengers[MAX_PASSENGERS];

// Mutex and semaphores
pthread_mutex_t* kiosk_mutex; // passenger in a kiosk
pthread_mutex_t print_mutex; // only one access to printing to console
pthread_mutex_t boarding_mutex; // checking during boarding
pthread_mutex_t special_kiosk_mutex; // special kiosk
pthread_mutex_t vip_mutex;
pthread_mutex_t channel_mutex;
sem_t special_kiosk_sem; // for the passengers without boarding pass
sem_t* belt_sem; // for availanility in each belt

void *timeCount(void *arg)
{
    // while (true)
    // {
    //     sleep(1);
    //     global_time++;
    // }
    clock_t start = clock();
    while (true)
    {
        clock_t end = clock();
        global_time = (round)(end - start) / CLOCKS_PER_SEC;
    }
}

void readFile(){
    ifstream in_file;
    in_file.open("in.txt");
    if(!in_file.is_open()){
        printf("Cannot open specified file\n");
        exit(-1);
    }

    in_file >> M >> N >> P;
    in_file >> w >> x >> y >> z;
    in_file.close();
}

void initMutexSemaphores(){
    pthread_mutex_init(&print_mutex, NULL);

    kiosk_mutex = new pthread_mutex_t[M];
    for (int i = 0; i < M; i++)
    {
        /* code */
        pthread_mutex_init(&kiosk_mutex[i],NULL);
    }
    
    belt_sem = new sem_t[N];
    for (int i = 0; i < N; i++)
    {
        /* code */
        sem_init(&belt_sem[i],0,P);
    }

    pthread_mutex_init(&boarding_mutex, NULL);
    pthread_mutex_init(&special_kiosk_mutex,NULL);
    pthread_mutex_init(&vip_mutex,NULL);
    pthread_mutex_init(&channel_mutex,NULL);
    
}

void setPassengersInfo(){
    
    double lambda = TOTAL_TIME / (1.0 * MAX_PASSENGERS);
    int arrival_count = global_time;
    default_random_engine random_engine;
    poisson_distribution<int> poisson(lambda);

    for (int i = 0; i < MAX_PASSENGERS; i++)
    {
        /* code */
        int new_arrival = poisson(random_engine);
        arrival_count = arrival_count + new_arrival;
        
        Passenger new_passenger(i+1);
        //sleep(new_arrival);

        new_passenger.setArrivalTime(arrival_count);
        int setVIPStatus = rand()%2;
        if(setVIPStatus == 1){
            new_passenger.setVIP(true);
            new_passenger.addName(" (VIP)");
            printf("%s\n",new_passenger.getName().c_str());
        }
        else{
            new_passenger.setVIP(false);
        }
        //new_passenger.setVIP(setVIPStatus);
        new_passenger.setKioskNumber(rand()%M);
        new_passenger.setBeltNumber(rand()%N);

        passengers[i] = new_passenger;


    }
}

void selfCheckin(Passenger* passenger){
    int kiosk_number = passenger->getKioskNumber();

    pthread_mutex_lock(&kiosk_mutex[kiosk_number]);

    pthread_mutex_lock(&print_mutex);
    printf("%s has started self-check in at kiosk %d at time %d\n",passenger->getName().c_str(),kiosk_number,global_time);
    pthread_mutex_unlock(&print_mutex);

    sleep(w);

    pthread_mutex_lock(&print_mutex);
    printf("%s has finished check-in at time %d\n",passenger->getName().c_str(),global_time);
    pthread_mutex_unlock(&print_mutex);

    pthread_mutex_unlock(&kiosk_mutex[kiosk_number]);
}

void securityCheck(Passenger* passenger){
    int belt_number = passenger->getBeltNumber();

    pthread_mutex_lock(&print_mutex);
    printf("%s has started waiting for security check in in belt %d from time %d\n",passenger->getName().c_str(),belt_number, global_time);
    pthread_mutex_unlock(&print_mutex);

    sem_wait(&belt_sem[belt_number]);

    pthread_mutex_lock(&print_mutex);
    printf("%s has started the security check at time %d\n",passenger->getName().c_str(), global_time);
    pthread_mutex_unlock(&print_mutex);

    sleep(x);

    pthread_mutex_lock(&print_mutex);
    printf("%s has crossed the security check at time %d\n",passenger->getName().c_str(), global_time);
    pthread_mutex_unlock(&print_mutex);

    sem_post(&belt_sem[belt_number]);
}

void startBoarding(Passenger* passenger){
    pthread_mutex_lock(&boarding_mutex);

    pthread_mutex_lock(&print_mutex);
    printf("%s has started boarding the plane at time %d\n",passenger->getName().c_str(), global_time);
    pthread_mutex_unlock(&print_mutex);

    int hasLostBoardingPass = rand()%2;
    passenger->setBoardingPass(!hasLostBoardingPass);
    if(!passenger->hasBoardingPass()){
        return;
    }

    sleep(y);

    pthread_mutex_lock(&print_mutex);
    printf("%s has boarded the plane at time %d\n",passenger->getName().c_str(), global_time);
    pthread_mutex_unlock(&print_mutex);

    pthread_mutex_unlock(&boarding_mutex);

}

void goToSpecialKiosk(Passenger* passenger){
    pthread_mutex_lock(&special_kiosk_mutex);

    pthread_mutex_lock(&print_mutex);
    printf("%s has started self-check in at special kiosk at time %d\n",passenger->getName().c_str(), global_time);
    pthread_mutex_unlock(&print_mutex);

    sleep(w);

    pthread_mutex_lock(&print_mutex);
    printf("%s has finished check-in at time %d\n",passenger->getName().c_str(),global_time);
    pthread_mutex_unlock(&print_mutex);

    pthread_mutex_unlock(&special_kiosk_mutex);
}


void goToVIPChannelLefttoRight(Passenger* passenger){
    //printf("VIP forwards\n");
    vip_count++;
    if(vip_count == 1){
        pthread_mutex_lock(&vip_mutex);
        pthread_mutex_lock(&channel_mutex);
    }

    pthread_mutex_lock(&print_mutex);
    printf("%s has crossed the VIP channel to the boarding gate at time %d\n",passenger->getName().c_str(), global_time);
    pthread_mutex_unlock(&print_mutex);

    vip_count--;
    if(vip_count == 0){
        pthread_mutex_unlock(&channel_mutex);
        pthread_mutex_unlock(&vip_mutex);
    }

}

void goToVIPChannelRighttoLeft(Passenger* passenger){
    //printf("VIP Backwards\n");
    pthread_mutex_lock(&vip_mutex);
    pthread_mutex_unlock(&vip_mutex);
    lost_count++;
    if(lost_count == 1){
        pthread_mutex_lock(&channel_mutex);
    }

    pthread_mutex_lock(&print_mutex);
    printf("%s has used the VIP channel to go to the special kiosk at time %d\n",passenger->getName().c_str(), global_time);
    pthread_mutex_unlock(&print_mutex);

    lost_count--;
    if(lost_count == 0){
        pthread_mutex_unlock(&channel_mutex);
    }

}

void* arrivalAtAirport(void* passenger){
    Passenger* curr_passenger = (Passenger*) passenger;

    selfCheckin(curr_passenger);

    if(curr_passenger->isVIP()){
        goToVIPChannelLefttoRight(curr_passenger);
    }
    else{
        securityCheck(curr_passenger);
    }

    while(!curr_passenger->hasBoarded()){
        startBoarding(curr_passenger);
        if(!curr_passenger->hasBoardingPass()){
            goToVIPChannelRighttoLeft(curr_passenger);
            goToSpecialKiosk(curr_passenger);
            goToVIPChannelLefttoRight(curr_passenger);
        }
    }

    startBoarding(curr_passenger);
}

void startProcess(){
    for (int i = 0; i < MAX_PASSENGERS; i++)
    {
        /* code */
        Passenger this_passenger = passengers[i];

        pthread_mutex_lock(&print_mutex);
        //printf("%s has arrived ar the airport at time %d \n",this_passenger.getName().c_str(),this_passenger.getArrivaltime());
        printf("%s has arrived ar the airport at time %d \n",this_passenger.getName().c_str(),global_time);
        pthread_mutex_unlock(&print_mutex);

        pthread_create(&thread_passengers[i],NULL, arrivalAtAirport, (void*) &this_passenger);

        if(i != MAX_PASSENGERS-1){
            sleep(passengers[i+1].getArrivaltime() - passengers[i].getArrivaltime());
        }

        

        
    }

    for (int i = 0; i < MAX_PASSENGERS; i++)
    {
        /* code */
        pthread_join(thread_passengers[i],NULL);
    }
    
    
}

void createPassengerThreads(){
    double lambda = TOTAL_TIME / (1.0 * MAX_PASSENGERS);
    int arrival_time = global_time;
    default_random_engine random_engine;
    poisson_distribution<int> poisson(lambda);

    for (int i = 0; i < MAX_PASSENGERS; i++)
    {
        /* code */
        int new_arrival_time = poisson(random_engine);
        arrival_time = arrival_time + new_arrival_time;
        
        Passenger new_passenger(i+1);
        sleep(new_arrival_time);

        new_passenger.setArrivalTime(arrival_time);
        int setVIPStatus = rand()%2;
        if(setVIPStatus == 1){
            new_passenger.setVIP(true);
            new_passenger.addName(" (VIP)");
            printf("%s\n",new_passenger.getName().c_str());
        }
        else{
            new_passenger.setVIP(false);
        }
        //new_passenger.setVIP(setVIPStatus);
        new_passenger.setKioskNumber(rand()%M);
        new_passenger.setBeltNumber(rand()%N);

        passengers[i] = new_passenger;

        pthread_mutex_lock(&print_mutex);
        //printf("%s has arrived ar the airport at time %d \n",this_passenger.getName().c_str(),this_passenger.getArrivaltime());
        printf("%s has arrived ar the airport at time %d \n",new_passenger.getName().c_str(),global_time);
        pthread_mutex_unlock(&print_mutex);

        pthread_create(&thread_passengers[i],NULL, arrivalAtAirport, (void*) new_passenger);

    }

    for (int i = 0; i < MAX_PASSENGERS; i++)
    {
        /* code */
        pthread_join(thread_passengers[i],NULL);
    }
}

int main(){
    srand(time(NULL));

    pthread_t timer_thread;

    readFile();
    printf("%d %d %d\n",M,N,P);
    printf("%d %d %d %d\n",w,x,y,z);
    pthread_create(&timer_thread,NULL, timeCount, NULL);

    initMutexSemaphores();
    //setPassengersInfo();
    //global_time = passengers[0].getArrivaltime();
    //startProcess();

    createPassengerThreads();


    return 0;
}