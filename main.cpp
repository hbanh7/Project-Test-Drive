#include "mbed.h"
#include "PinDetect.h"


Serial pc(USBTX, USBRX);
DigitalOut myled(LED1);
PinDetect pb(p19, PullDown);
Timer t;

void flash(DigitalOut* led) {
    *led = 1;
    t.start();
    float weight = rand() / (float) RAND_MAX;
    wait(3 * weight);
    *led = 0;
}

int main() {

    pc.printf("System starting in ...");
    for(int i = 1; i <= 5; i++) {  
        pc.printf("%d...", i);
    }
    pc.printf("Go!");

    while(1) {
        flash(&myled);
        if (pb == 1) {
            float time = t.read();
            pc.printf("Reaction Time: %f ms", time);
        }
    }
}
