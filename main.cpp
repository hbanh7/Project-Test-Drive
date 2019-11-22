#include "mbed.h"
#include "rtos.h"
#include "PinDetect.h"


RawSerial pc(USBTX, USBRX);
DigitalOut myled(p21);
DigitalOut myled2(p22);
PinDetect pb(p26, PullDown);
PinDetect pb2(p27, PullDown);
Timer t1;
Timer t2;
const int TEN_SEC_DEL = 5000;  // Change back to 10 seconds (changed for testing purposes)
const int window_size = 5;

volatile float rxn_time = 0;
volatile int pb1_asserted = 0;
volatile int pb2_asserted = 0;
volatile int pb1_chosen = 0;
volatile int pb2_chosen = 0;
volatile int total_count = 0;
volatile int incorrect_count = 0;
volatile int interval = 0;
volatile bool timeout = false;
volatile float baseline_avg = 0;
volatile float current_avg = 0;
volatile float readings[window_size]; // Change back to 10 later
volatile int step = 0;
volatile bool calc_baseline = false;

void clear_timers() {
    t1.stop();
    t1.reset();
    t2.stop();
    t2.reset();
}

void flash(void const *args) {
    while(1){
        
        if(t1.read()>0 || t2.read()>0){
            clear_timers();
            incorrect_count++;  
            timeout = true; 
        }
        
        int choose = rand()%2;
        if (choose == 0) {
            pb1_chosen = 1;
            myled = 1;
            t1.start();
            Thread::wait(1000);
            myled = 0;
        } else {
            pb2_chosen = 1;
            myled2 = 1;
            t2.start();
            Thread::wait(1000);
            myled2 = 0;
        }
        
        float weight = rand() / (float) RAND_MAX;
        Thread::wait( (int) (TEN_SEC_DEL * weight));
        
        total_count++;
        interval++;
    }
}

void button_ready(void) {
    pb1_asserted = 1;
    rxn_time = t1.read();
    readings[step] = rxn_time;
    step++;
    if (step == window_size) {
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
    if (step == window_size) {
        calc_baseline = true;
        step = 0;
    }
    t2.stop();
    t2.reset();
}
    
int main() {

    pc.printf("System starting in ...");
    for(int i = 1; i <= 5; i++) {  
        pc.printf("%d...", i);
        wait(1);
    }
    pc.printf("Go!\n\r");
    pb.attach_deasserted(&button_ready);
    pb.setSampleFrequency();
    pb2.attach_deasserted(&button_ready2);
    pb2.setSampleFrequency();

    Thread thread1(flash);

    float accuracy;

    while(1) {
        // Change back to 10 later
        if (calc_baseline && baseline_avg == 0) {
            float sum = 0;
            for (int j = 0; j < window_size; j++)
                sum += readings[j];
                
            baseline_avg = sum / window_size;
            pc.printf("Training Complete -- Baseline Average Established\n\r");
        } else {
            float sum = 0;
            for (int j = 0; j < window_size; j++) 
                sum += readings[j];   
            current_avg = sum / window_size;
        }
        
        if (pb1_chosen && pb1_asserted == 1 && rxn_time != 0) {
            pb1_asserted = 0;
            pc.printf("Reaction Time: %f s  Baseline Reaction Time: %f s Average Reaction Time: %f s \n\r", rxn_time, baseline_avg, current_avg);
            pb1_chosen = 0;
            clear_timers();
        } else if (pb2_chosen && pb2_asserted == 1 && rxn_time != 0) {
            pb2_asserted = 0;
             pc.printf("Reaction Time: %f s  Baseline Reaction Time: %f s Average Reaction Time: %f s \n\r", rxn_time, baseline_avg, current_avg);
            pb2_chosen = 0;
            clear_timers();
        } else if(pb1_asserted || pb2_asserted) {
            pc.printf("Wrong Button Press\n\r");
            pb1_asserted = 0;
            pb2_asserted = 0;
            
            clear_timers();
            
            incorrect_count++;
        } else if (timeout) {
            pc.printf("Button timeout\n\r");
            timeout = false;
        } 
        
        
        if((interval%5==0)&&(interval!=0)){
               accuracy = ((float) total_count - incorrect_count)/total_count;
               accuracy = accuracy*100;
               pc.printf("TotalCount: %d; IncorrCount: %d\n\r", total_count, incorrect_count);
               pc.printf("Current Accuracy: %.2f%%\n\r", accuracy);
               interval = 0;
        }  
        
        Thread::wait(100);
    }
}