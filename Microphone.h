/* 
 * File:   Microphone.h
 * Author: grp51
 *
 * Created on December 29, 2013, 4:54 PM
 * 
 * This class define a simple interface for recording audio with the embedded
 * microphone of the STM32F4 Discovery board. It outputs an array of PCM samples
 * obtained by transcoding the PDM signal of the microphone.
 * Note that the microphone on the board outputs PDM samples that are internally
 * converted to PCM by the Microphone class.
 * 
 */


#include "miosix.h"



#ifndef MICROPHONE_H
#define	MICROPHONE_H

class CICFilter{
    
};

/*
 * The Microphone class is the singleton that handles everthing: the device configuration
 * the recording and the transcoding of the PDM into PCM 
 */
class Microphone {
public:
    
    /* 
     * \return the instance of the Microphone class to be used for recording.
     */
    static Microphone& instance();
    
    /*
     * Fills the given buffer with PCM samples.
     * 
     * \param buffer the buffer to fill with 16-bit PCM samples
     * \param the number of PCM sample to record
     * \return true when the recording ends successfully, false otherwise
     */
    bool getBuffer(unsigned short* buffer, unsigned short size);
    

private:
    Microphone();
    Microphone(const Microphone& orig);
    virtual ~Microphone();
    unsigned short PCMsize;
    unsigned short PCMindex;
    unsigned short* PCMbuffer;
    bool processPDM(const unsigned short *pdmbuffer, int size);
    unsigned short PDMFilter(const unsigned short* PDMBuffer, unsigned short index);
    
    
};

#endif	/* MICROPHONE_H */

