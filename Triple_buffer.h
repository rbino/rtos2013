/* 
 * File:   Triple_buffer.h
 * Author: Alessandro Mariani
 *
 * Created on March 24, 2014, 11:16 AM
 */

#ifndef TRIPLE_BUFFER_H
#define	TRIPLE_BUFFER_H

template<typename T, unsigned int size>
class Triple_buffer {
private:
    T buf[3][size];
    T* current = buf[0][0];
    T* ready = buf[1][0];
    T* blocked = buf[2][0];

    void swap(T*& a, T*& b) {
        a = a ^ b;
        b = a ^ b;
        a = a ^ b;
    }

public:
    Triple_buffer();
    ~Triple_buffer();

    void get_writable(T *&buffer) {
        swap(current, ready);
        buffer = current;
    }

    void get_readable(T *&buffer) {
        swap (blocked, ready);
        buffer = blocked;
    }
};


#endif	/* TRIPLE_BUFFER_H */

