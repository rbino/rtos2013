
#include <cstdio>
#include "miosix.h"
#include "Microphone.h"
#include "player.h"
#include <math.h>
#include <tr1/functional>

using namespace std;
using namespace std::tr1;
using namespace miosix;

/*
 * This example program records half second (22050 samples) from the microphone and
 * plays such sound through the board's DAC.
 */
int main()
{

    Player& player = Player::instance();
    Microphone& mic = Microphone::instance(); 
    
    static const unsigned short size = 20050;
    mic.init(bind(&Player::play,&player,placeholders::_1,placeholders::_2),size);
    player.init();
    mic.start();
    
    while (1){
    };
    
}
