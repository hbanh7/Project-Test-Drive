#include "mbed.h"
#include "rtos.h"
#include "PinDetect.h"

// Hardware devices
RawSerial pc(USBTX, USBRX);
DigitalOut myled(p21);
DigitalOut myled2(p22);
PinDetect pb(p26, PullDown);
PinDetect pb2(p27, PullDown);
PwmOut speaker(p25);

// Constants and Timers
Timer t1;
Timer t2;
const int FIFTEEN_SEC_DEL = 15000;
const int MIN_DEL = 3000;
const int WINDOW_SIZE = 5;
const float INCORRECT_PENALTY = 3.00;
const float WEIGHT = 1.25; 


// Pushbutton flags and inputs
volatile float rxn_time = 0;
volatile int pb1_asserted = 0;
volatile int pb2_asserted = 0;
volatile int pb1_chosen = 0;
volatile int pb2_chosen = 0;

// Reading windows, counters, and accuracy calculations
volatile int total_count = 0;
volatile int incorrect_count = 0;
volatile int interval = 0;
volatile bool timeout = false;
volatile float baseline_avg = 0;
volatile float current_avg = 0;
volatile float readings[WINDOW_SIZE]; 
volatile int step = 0;
volatile bool calc_baseline = false;

void clear_timers() {
    t1.stop();
    t1.reset();
    t2.stop();
    t2.reset();
}

// Main LED control thread
void flash(void const *args) {
    float timeout_time = 0;
    while(1){
        
        if(t1.read()>0 || t2.read()>0){
            clear_timers();
            incorrect_count++;  
            timeout = true; 
            
            // Add in the timeout reading into the current average 
            if (baseline_avg != 0) {
                // Convert to seconds 
                readings[step] = timeout_time / 1000.0;
                step++;
                if (step == WINDOW_SIZE) { 
                    step = 0;
                }
            }
        }
        
        int choose = rand()%2;
        if (choose == 0) {
            pb1_chosen = 1;
            myled = 1;
            t1.start();
            Thread::wait(2000);
            myled = 0;
        } else {
            pb2_chosen = 1;
            myled2 = 1;
            t2.start();
            Thread::wait(2000);
            myled2 = 0;
        }
        
        float weight = rand() / (float) RAND_MAX;
        timeout_time = (int) (FIFTEEN_SEC_DEL * weight);
        Thread::wait(MIN_DEL + timeout_time);
        
        total_count++;
        interval++;
    }
}

// Pushbutton interrupts to read in a reaction time reading
void button_ready(void) {
    pb1_asserted = 1;
    rxn_time = t1.read();
    readings[step] = rxn_time;
    step++;
    if (step == WINDOW_SIZE) {
        calc_baseline = true;
        step = 0;
    }
    t1.stop();
    t1.reset();
}

void button_ready2(void) {
    pb2_asserted = 1;
    rxn_time = t2.read();
    readings[step] = rxn_time;
    step++;
    if (step == WINDOW_SIZE) {
        calc_baseline = true;
        step = 0;
    }
    t2.stop();
    t2.reset();
}

// Alarm and sound control thread
void sound(void const* args) {
    bool playOnce = true;
    while(1) {
        if (baseline_avg != 0 && playOnce) {
            speaker.period(1.0/1000.0);
            speaker = 0.5;
            Thread::wait(500);
            speaker = 0;
            playOnce = false;    
        }   
        
        if (baseline_avg != 0 && current_avg >= WEIGHT * baseline_avg) {
            speaker.period(1.0/100.0);
            speaker = 0.5;
            Thread::wait(250);
            speaker = 0;
            Thread::wait(250);
            speaker.period(1.0/200.0);
            speaker = 0.5;
            Thread::wait(250);
            speaker = 0;
            Thread::wait(250);
        }
        
        Thread::wait(100);
    }
}
    
int main() {

    // Messages to alert the user that the system is starting
    pc.printf("System starting in ...");
    for(int i = 1; i <= 5; i++) {  
        pc.printf("%d...", i);
        wait(1);
    }
    pc.printf("Go!\n\r");

    // Pushbutton setup
    pb.attach_deasserted(&button_ready);
    pb.setSampleFrequency();
    pb2.attach_deasserted(&button_ready2);
    pb2.setSampleFrequency();

    // Start led and sound control threads
    Thread thread1(flash);
    Thread thread2(sound);

    float accuracy;
    pc.printf("Starting Training Phase\n\r");

    // Continuously monitor the user's reaction 10x a second
    while(1) {
        if (calc_baseline && baseline_avg == 0) {
            float sum = 0;
            for (int j = 0; j < WINDOW_SIZE; j++)
                sum += readings[j];
                
            baseline_avg = sum / WINDOW_SIZE;
            pc.printf("Training Complete -- Baseline Average Established\n\r");
        } else {
            float sum = 0;
            for (int j = 0; j < WINDOW_SIZE; j++) 
                sum += readings[j];   
            current_avg = sum / WINDOW_SIZE;
        }
        
        if (pb1_chosen && pb1_asserted == 1 && rxn_time != 0) {
            pb1_asserted = 0;
            pc.printf("Reaction Time: %fs  Baseline Reaction Time: %fs Average Reaction Time: %fs \n\r", rxn_time, baseline_avg, current_avg);
            pb1_chosen = 0;
            clear_timers();
        } else if (pb2_chosen && pb2_asserted == 1 && rxn_time != 0) {
            pb2_asserted = 0;
             pc.printf("Reaction Time: %fs  Baseline Reaction Time: %fs Average Reaction Time: %fs \n\r", rxn_time, baseline_avg, current_avg);
            pb2_chosen = 0;
            clear_timers();
        } else if(pb1_asserted || pb2_asserted) {
            pc.printf("Wrong Button Press\n\r");
            pb1_asserted = 0;
            pb2_asserted = 0;
            
            // Penalize for the wrong button press into the current average
            if (baseline_avg != 0) {
                readings[step] = INCORRECT_PENALTY;
                step++;
                if (step == WINDOW_SIZE) { 
                    step = 0;
                }
            }
            
            clear_timers();
            incorrect_count++;
        } else if (timeout) {
            pc.printf("Button timeout\n\r");
            timeout = false;
        } 
        
        
        if((interval%5==0)&&(interval!=0)){
               accuracy = ((float) total_count - incorrect_count)/total_count;
               accuracy = accuracy*100;
               pc.printf("--- TotalCount: %d IncorrectCount: %d Current Accuracy: %.2f%% ---\n\r", total_count, incorrect_count, accuracy);
               pc.printf("\n\r");
               interval = 0;
        }  
                
        Thread::wait(100);
    }
}