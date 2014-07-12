
#include <cstdio>
#include <complex>
/*
#include "miosix.h"
#include "miosix/kernel/buffer_queue.h"
#include "miosix/kernel/scheduler/scheduler.h"
#include "util/software_i2c.h"
*/
#include "FFT.cpp"

#define L 16 //lunghezza del segnale (campioni)

int main(){
    double d = 1.0/L; //passo di campionamento dummy
    complex<double> s[L]; //segnale sinusoidale di lunghezza L
    for(int i = 0 ; i < L; i++){
        s[i] = polar( 1.*( i % 2 ) * ( (i % 4) - 2 ),0.);
    }
    FFT(s, L, d);
    
    /*for(int i=0; i<L; i++){
        double asd = abs(s[i]);
        while(asd>=1){
            printf("|");
            asd /= 2;
        } 
        printf("\n");
    }*/
    
    for(int i = 0; i < L; i++){
        double asd = abs(s[i]);
        printf("%d : %d", i, asd );
        printf("\n");
    }
}