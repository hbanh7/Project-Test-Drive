#include "mbed.h"
#include "rtos.h"
#include "PinDetect.h"


RawSerial pc(USBTX, USBRX);
DigitalOut myled(LED1);
PinDetect pb(p26, PullDown);
Timer t;
const int TEN_SEC_DEL = 10000;
volatile float rxn_time = 0;
volatile int input = 0;

void flash(void const *args) {
    while(1){
        myled = 1;
        t.start();
        float weight = rand() / (float) RAND_MAX;
        Thread::wait(1000);
        myled = 0;
        Thread::wait( (int) (TEN_SEC_DEL * weight));
    }
}

void button_ready(void) {
    input = 1;
    rxn_time = t.read();
    t.stop();
    t.reset();
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


    Thread thread1(flash);

    while(1) {
        if (input == 1) {
            input = 0;
            pc.printf("Reaction Time: %f s\n\r", rxn_time);
        }
        Thread::wait(100);
    }
}