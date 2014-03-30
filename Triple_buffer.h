/* 
 * File:   Triple_buffer.h
 * Author: Alessandro Mariani
 *
 * Created on March 24, 2014, 11:16 AM
 */

#ifndef TRIPLE_BUFFER_H
#define	TRIPLE_BUFFER_H

#include "miosix.h"

using namespace miosix;

template<typename T, unsigned int size>
class Triple_buffer {
private:
    T buf[3][size];
    T* current, ready, blocked;

    /**
     * Swap the value of two pointers. Interrupts are disabled
     * but the function is not thread safe in case of a preemptive
     * scheduler.
     * 
     * @param a first pointer to swap
     * @param b second pointer to swap
     */
    void swap(T*& a, T*& b) {
        FastInterruptDisableLock dLock;
        a = a ^ b;
        b = a ^ b;
        a = a ^ b;
    }

public:

    Triple_buffer() {
        T* current = buf[0];
        T* ready = buf[1];
        T* blocked = buf[2];
    };
    
    ~Triple_buffer(){};

    /**
     * Change the current buffer with the ready one
     * to permit the double buffering.
     * 
     * @param buffer pointer to new writable buffer
     * @return size of the buffer
     */
    unsigned int get_writable(T *&buffer) {
        swap(current, ready);
        buffer = current;

        return size;
    }

    /**
     * @param buffer pointer to new readable buffer
     * @return size of the buffer
     */
    unsigned int get_readable(T *&buffer) {
        swap(blocked, ready);
        buffer = blocked;

        return size;
    }
};


#endif	/* TRIPLE_BUFFER_H */

