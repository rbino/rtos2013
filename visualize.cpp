
#include <cstdio>
#include "miosix.h"
#include "Microphone.h"
#include "player.h"
#include <math.h>

using namespace std;
using namespace miosix;

typedef Gpio<GPIOD_BASE,12> led1;

/*
 * This example program records 40 ms from the microphone and transmits via serial
 * the PCM samples (in the format "index:value" ). Then send a "new" command and restart from the beginning.
 */

static const unsigned short size = 1764; // 40ms of sound
int main()
{
    led1::mode(Mode::OUTPUT);

    
    while(1)
    {
        unsigned short *buffer = (unsigned short*) malloc(sizeof(unsigned short)*size);
        if (buffer==NULL){
            led1::high();
        }

        if (Microphone::instance().getBuffer(Microphone::F44100HZ, buffer, size)){
            iprintf("new\n");
            for(int i = 0; i < size; i++){
                iprintf("%d:%d", i, buffer[i] );
                iprintf("\n");
            }
        }
        
        free(buffer);

    }
}
