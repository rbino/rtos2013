/* 
 * File:   Microphone.h
 * Author: user
 *
 * Created on December 29, 2013, 4:54 PM
 */


#include "miosix.h"

#ifndef MICROPHONE_H
#define	MICROPHONE_H

class Microphone {
public:
    static Microphone& instance();
    void start();
    void stop();
    bool isRecording() const;
    unsigned int getBuffer(const unsigned short *&buffer);

private:
    Microphone();
    Microphone(const Microphone& orig);
    virtual ~Microphone();
    bool recording;
    
    
};

#endif	/* MICROPHONE_H */

