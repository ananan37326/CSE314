#include<bits/stdc++.h>
#include<pthread.h>
#include<semaphore.h>
#include<unistd.h>
#include"passenger.h"

#define MAX_PASSENGERS 10
#define TOTAL_TIME 60

using namespace std;

// output file stream
FILE* out_file;

// M - number of kiosk  N - number of belts    P - passengers per belt
// w - self check-in time   x - security check time
// y - boarding time    z - walking on VIP channel time
int M,N,P,w,x,y,z;

// Global variables
int global_time = 0; // ttime counting variable
int vip_count = 0; // VIP passenger counter
int lost_count = 0; // passenger counter who lost boarding pass

// Passenger and thread arrays
Passenger** passengers = new Passenger*[MAX_PASSENGERS];
pthread_t thread_passengers[MAX_PASSENGERS];

// Mutex and semaphores
pthread_mutex_t* kiosk_mutex; // passenger in a kiosk
pthread_mutex_t print_mutex; // only one access to printing to console
pthread_mutex_t boarding_mutex; // checking during boarding
pthread_mutex_t special_kiosk_mutex; // special kiosk
pthread_mutex_t vip_mutex; // vip mutex
pthread_mutex_t channel_mutex; // vip channel mutex
sem_t* belt_sem; // for availanility in each belt

// timer function
void *timeCount(void *arg)
{
    clock_t start = clock();
    while (true)
    {
        clock_t end = clock();
        global_time = (round)(end - start) / CLOCKS_PER_SEC;
    }
}

// reading the file and setting the parameters
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


// initializing the mutex and semaphores
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

// function for self check-in
void selfCheckin(Passenger* passenger){
    int kiosk_number = passenger->getKioskNumber();

    pthread_mutex_lock(&kiosk_mutex[kiosk_number]);

    pthread_mutex_lock(&print_mutex);
    //string comment = passenger->getName() + " has started self-check in at kiosk " + to_string(kiosk_number) + " at time " + to_string(global_time);
    printf("%s has started self-check in at kiosk %d at time %d\n",passenger->getName().c_str(),kiosk_number,global_time);
    fprintf(out_file,"%s has started self-check in at kiosk %d at time %d\n",passenger->getName().c_str(),kiosk_number,global_time);
    pthread_mutex_unlock(&print_mutex);

    sleep(w);

    pthread_mutex_lock(&print_mutex);
    printf("%s has finished check-in at time %d\n",passenger->getName().c_str(),global_time);
    fprintf(out_file,"%s has finished check-in at time %d\n",passenger->getName().c_str(),global_time);
    pthread_mutex_unlock(&print_mutex);

    pthread_mutex_unlock(&kiosk_mutex[kiosk_number]);
}

// function for security check
void securityCheck(Passenger* passenger){
    int belt_number = passenger->getBeltNumber();

    pthread_mutex_lock(&print_mutex);
    printf("%s has started waiting for security check in in belt %d from time %d\n",passenger->getName().c_str(),belt_number, global_time);
    fprintf(out_file,"%s has started waiting for security check in in belt %d from time %d\n",passenger->getName().c_str(),belt_number, global_time);
    pthread_mutex_unlock(&print_mutex);

    sem_wait(&belt_sem[belt_number]);

    pthread_mutex_lock(&print_mutex);
    printf("%s has started the security check at time %d\n",passenger->getName().c_str(), global_time);
    fprintf(out_file,"%s has started the security check at time %d\n",passenger->getName().c_str(), global_time);
    pthread_mutex_unlock(&print_mutex);

    sleep(x);

    pthread_mutex_lock(&print_mutex);
    printf("%s has crossed the security check at time %d\n",passenger->getName().c_str(), global_time);
    fprintf(out_file,"%s has crossed the security check at time %d\n",passenger->getName().c_str(), global_time);
    pthread_mutex_unlock(&print_mutex);

    sem_post(&belt_sem[belt_number]);
}

// function to start boarding
void startBoarding(Passenger* passenger){
    pthread_mutex_lock(&boarding_mutex);

    pthread_mutex_lock(&print_mutex);
    printf("%s has started boarding the plane at time %d\n",passenger->getName().c_str(), global_time);
    fprintf(out_file,"%s has started boarding the plane at time %d\n",passenger->getName().c_str(), global_time);
    pthread_mutex_unlock(&print_mutex);

    int hasLostBoardingPass = rand()%10;
    if(hasLostBoardingPass<2){
        passenger->setBoardingPass(false);
    }
    else{
        passenger->setBoardingPass(true);
    }

    if(!passenger->hasBoardingPass()){
        pthread_mutex_unlock(&boarding_mutex);
        return;
    }

    sleep(y);

    pthread_mutex_lock(&print_mutex);
    printf("%s has boarded the plane at time %d\n",passenger->getName().c_str(), global_time);
    fprintf(out_file,"%s has boarded the plane at time %d\n",passenger->getName().c_str(), global_time);
    pthread_mutex_unlock(&print_mutex);

    passenger->setBoarded(true);

    pthread_mutex_unlock(&boarding_mutex);

}

// function to go to the special kiosk
void goToSpecialKiosk(Passenger* passenger){
    pthread_mutex_lock(&special_kiosk_mutex);

    pthread_mutex_lock(&print_mutex);
    printf("%s has started self-check in at special kiosk at time %d\n",passenger->getName().c_str(), global_time);
    fprintf(out_file,"%s has started self-check in at special kiosk at time %d\n",passenger->getName().c_str(), global_time);
    pthread_mutex_unlock(&print_mutex);

    sleep(w);

    pthread_mutex_lock(&print_mutex);
    printf("%s has finished check-in at special kiosk at time %d\n",passenger->getName().c_str(),global_time);
    fprintf(out_file,"%s has finished check-in at special kiosk at time %d\n",passenger->getName().c_str(),global_time);
    pthread_mutex_unlock(&print_mutex);

    pthread_mutex_unlock(&special_kiosk_mutex);
}

// function to go to the vip channel to boarding
void goToVIPChannelLefttoRight(Passenger* passenger){
    //printf("VIP forwards\n");
    vip_count++;
    if(vip_count == 1){
        pthread_mutex_lock(&vip_mutex);
        pthread_mutex_lock(&channel_mutex);
    }

    sleep(z);

    pthread_mutex_lock(&print_mutex);
    printf("%s has crossed the VIP channel to the boarding gate at time %d\n",passenger->getName().c_str(), global_time);
    fprintf(out_file,"%s has crossed the VIP channel to the boarding gate at time %d\n",passenger->getName().c_str(), global_time);
    pthread_mutex_unlock(&print_mutex);

    vip_count--;
    if(vip_count == 0){
        pthread_mutex_unlock(&channel_mutex);
        pthread_mutex_unlock(&vip_mutex);
    }

}

// function to go to the vip channel to special kiosk
void goToVIPChannelRighttoLeft(Passenger* passenger){
    //printf("VIP Backwards\n");
    pthread_mutex_lock(&vip_mutex);
    pthread_mutex_unlock(&vip_mutex);
    lost_count++;
    if(lost_count == 1){
        pthread_mutex_lock(&channel_mutex);
    }

    sleep(z);

    pthread_mutex_lock(&print_mutex);
    printf("%s has used the VIP channel to go to the special kiosk at time %d\n",passenger->getName().c_str(), global_time);
    fprintf(out_file,"%s has used the VIP channel to go to the special kiosk at time %d\n",passenger->getName().c_str(), global_time);
    pthread_mutex_unlock(&print_mutex);

    lost_count--;
    if(lost_count == 0){
        pthread_mutex_unlock(&channel_mutex);
    }

}

// starting function for each passenger
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

    //startBoarding(curr_passenger);
}


// initializing and starting the threads
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
        
        Passenger* new_passenger = new Passenger(i+1);
        sleep(new_arrival_time);

        new_passenger->setArrivalTime(arrival_time);
        int setVIPStatus = rand()%2;
        if(setVIPStatus == 1){
            new_passenger->setVIP(true);
            new_passenger->addName(" (VIP)");
            //printf("%s\n",new_passenger->getName().c_str());
        }
        else{
            new_passenger->setVIP(false);
        }
        //new_passenger.setVIP(setVIPStatus);
        new_passenger->setKioskNumber(rand()%M);
        new_passenger->setBeltNumber(rand()%N);

        passengers[i] = new_passenger;

        pthread_mutex_lock(&print_mutex);
        //printf("%s has arrived ar the airport at time %d \n",this_passenger.getName().c_str(),this_passenger.getArrivaltime());
        printf("%s has arrived at the airport at time %d \n",new_passenger->getName().c_str(),global_time);
        fprintf(out_file,"%s has arrived at the airport at time %d \n",new_passenger->getName().c_str(),global_time);
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
    //printf("%d %d %d\n",M,N,P);
    //printf("%d %d %d %d\n",w,x,y,z);

    out_file = fopen("output.txt","w");
	fclose(out_file);
	out_file = fopen("output.txt","a");

    pthread_create(&timer_thread,NULL, timeCount, NULL);

    initMutexSemaphores();
    createPassengerThreads();

    fclose(out_file);
    
    return 0;
}