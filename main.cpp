#include "mbed.h"
#include "rtos.h"
#include "PinDetect.h"


RawSerial pc(USBTX, USBRX);
DigitalOut myled(LED1);
DigitalOut myled2(LED2);
PinDetect pb(p26, PullDown);
PinDetect pb2(p27, PullDown);
Timer t1;
Timer t2;
const int TEN_SEC_DEL = 10000;
volatile float rxn_time = 0;
volatile int input = 0;
volatile int input2 = 0;
volatile int pb1_asserted = 0;
volatile int pb2_asserted = 0;

void flash(void const *args) {
    while(1){
        int choose = rand()%2;
        if (choose == 0) {
            pb1_asserted = 1;
            myled = 1;
            t1.start();
            Thread::wait(1000);
            myled = 0;
        } else {
            pb2_asserted = 1;
            myled2 = 1;
            t2.start();
            Thread::wait(1000);
            myled2 = 0;
        }

        float weight = rand() / (float) RAND_MAX;
        Thread::wait( (int) (TEN_SEC_DEL * weight));
    }
}

void button_ready(void) {
    input = 1;
    rxn_time = t1.read();
    t1.stop();
    t1.reset();
}

void button_ready2(void) {
    input2 = 1;
    rxn_time = t2.read();
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

    while(1) {
        if (pb1_asserted && input == 1) {
            input = 0;
            pc.printf("Reaction Time: %f s\n\r", rxn_time);
            pb1_asserted = 0;
        } else if (pb2_asserted && input2 == 1) {
            input2 = 0;
            pc.printf("Reaction Time: %f s\n\r", rxn_time);
            pb2_asserted = 0;
        }

        Thread::wait(100);
    }
}