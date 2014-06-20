/* 
 * File:   Microphone.h
 * Author: user
 *
 * Created on December 29, 2013, 4:54 PM
 */


#include "miosix.h"



#ifndef MICROPHONE_H
#define	MICROPHONE_H

class CICFilter{
    
};

class Microphone {
public:
    typedef enum {F8000HZ, F44100HZ} SampleFreq;
    static Microphone& instance();
    bool getBuffer(SampleFreq freq, unsigned short* buffer, unsigned short size);
    

private:
    Microphone();
    Microphone(const Microphone& orig);
    virtual ~Microphone();
    bool busy;
    mutable miosix::Mutex mutex;
    unsigned short PCMsize;
    unsigned short PCMindex;
    unsigned short* PCMbuffer;
    bool processPDM(const unsigned short *pdmbuffer, int size);
    unsigned short PDMFilter(const unsigned short* PDMBuffer, unsigned short index);
    
    
};

#endif	/* MICROPHONE_H */

