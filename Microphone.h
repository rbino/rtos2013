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

private:
    Microphone();
    Microphone(const Microphone& orig);
    virtual ~Microphone();
    mutable miosix::Mutex mutex;
};

#endif	/* MICROPHONE_H */

