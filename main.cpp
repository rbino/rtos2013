
#include <cstdio>
#include "miosix.h"
#include "Microphone.h"
#include "player.h"
#include <math.h>

using namespace std;
using namespace miosix;

typedef Gpio<GPIOD_BASE,12> led1;

/*
 * This example program records 1 second from the microphone (44100 samples) and
 * plays such sound through the board's DAC.
 */
int main()
{
    led1::mode(Mode::OUTPUT);

    
    while(1)
    {
        static const unsigned short size = 44100;
        unsigned short *buffer = (unsigned short*) malloc(sizeof(unsigned short)*size);
        
        if (buffer==NULL){
            led1::high();
        }

        if (Microphone::instance().getBuffer(Microphone::F44100HZ, buffer, size)){
  
                PCMSound sound(buffer,size);
                Player::instance().play(sound);                      

        }
        
        free(buffer);

    }
}
