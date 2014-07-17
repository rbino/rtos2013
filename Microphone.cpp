/* 
 * File:   Microphone.cpp
 * Author: user
 * 
 * Created on December 29, 2013, 4:54 PM
 */

#include <cstdio>
#include "miosix.h"
#include "miosix/kernel/buffer_queue.h"
#include "miosix/kernel/scheduler/scheduler.h"
#include "util/software_i2c.h"
#include "Microphone.h"
#include <tr1/functional>

using namespace std;
using namespace miosix;

typedef Gpio<GPIOB_BASE,10> clk;
typedef Gpio<GPIOC_BASE,3> dout;

static const int bufferSize=512; //Buffer RAM is 4*bufferSize bytes
static const int bufNum = 2;
static Thread *waiting;
static BufferQueue<unsigned short,bufferSize,bufNum> *bq;
static bool enobuf=true;
static const char filterOrder = 4;
static const short oversample = 16;
static unsigned short intReg[filterOrder] = {0x8000,0x8000,0x8000,0x8000};
static unsigned short combReg[filterOrder] = {0x8000, 0x8000,0x8000,0x8000};
static signed char pdmLUT[] = {-1, 1};

/**
 * Configure the DMA to do another transfer
 */
static void IRQdmaRefill()
{
    unsigned short *buffer;
    
	if(bq->IRQgetWritableBuffer(buffer)==false)
	{
		enobuf=true;
		return;
	}
    DMA1_Stream3->CR=0;
	DMA1_Stream3->PAR=reinterpret_cast<unsigned int>(&SPI2->DR);
	DMA1_Stream3->M0AR=reinterpret_cast<unsigned int>(buffer);
	DMA1_Stream3->NDTR=bufferSize;
	DMA1_Stream3->CR=DMA_SxCR_PL_1    | //High priority DMA stream
                     DMA_SxCR_MSIZE_0 | //Write 16bit at a time to RAM
					 DMA_SxCR_PSIZE_0 | //Read 16bit at a time from SPI
				     DMA_SxCR_MINC    | //Increment RAM pointer
			         DMA_SxCR_TCIE    | //Interrupt on completion
			  	     DMA_SxCR_EN;       //Start the DMA
}




static void dmaRefill()
{
	FastInterruptDisableLock dLock;
	IRQdmaRefill();
}

/**
 * DMA end of transfer interrupt
 */
void __attribute__((naked)) DMA1_Stream3_IRQHandler()
{
    saveContext();
    asm volatile("bl _Z17I2SdmaHandlerImplv");
    restoreContext();
}

/**
 * DMA end of transfer interrupt actual implementation
 */
void __attribute__((used)) I2SdmaHandlerImpl()
{
    
	DMA1->LIFCR=DMA_LIFCR_CTCIF3  |
                DMA_LIFCR_CTEIF3  |
                DMA_LIFCR_CDMEIF3 |
                DMA_LIFCR_CFEIF3;
	bq->IRQbufferFilled(bufferSize);
	IRQdmaRefill();
	waiting->IRQwakeup();
	if(waiting->IRQgetPriority()>Thread::IRQgetCurrentThread()->IRQgetPriority())
		Scheduler::IRQfindNextThread();
}

/**
 * This function allows to atomically check if a variable equals a specific
 * value and if not, put the thread in wait state until said condition is
 * satisfied. To wake the thread, another thread or an interrupt routine
 * should first set the variable to the desired value, and then call
 * wakeup() (or IRQwakeup()) on the sleeping thread.
 * \param T type of the variable to test
 * \param variable the variable to test
 * \param value the value that will cause this function to return.
 */
template<typename T>
static void atomicTestAndWaitUntil(volatile T& variable, T value)
{
	FastInterruptDisableLock dLock;
	while(variable!=value)
	{
		Thread::IRQgetCurrentThread()->IRQwait();
		{
			FastInterruptEnableLock eLock(dLock);
			Thread::yield();
		}
	}
}

/**
 * Helper function that waits until a buffer is available for reading
 * \return a readable buffer from bq
 */
static const unsigned short *getReadableBuffer()
{
	FastInterruptDisableLock dLock;
	const unsigned short *result;
        unsigned int size;
	while(bq->IRQgetReadableBuffer(result, size)==false)
	{
		waiting->IRQwait();
		{
			FastInterruptEnableLock eLock(dLock);
			Thread::yield();
		}
	}
	return result;
}

static void bufferEmptied()
{
	FastInterruptDisableLock dLock;
	bq->IRQbufferEmptied();
}


Microphone& Microphone::instance()
{
	static Microphone singleton;
	return singleton;
}

Microphone::Microphone() {

}

bool Microphone::processPDM(const unsigned short *pdmbuffer, int size) {
    int remaining = PCMsize - PCMindex;
    int length = std::min(remaining, size/2); 
    // convert couples of 16 bit pdm samples in one 16-bit PCM sample
    for (int i=0; i < length; i+=2){
        PCMbuffer[PCMindex++] = PDMFilter(pdmbuffer, i);
    }
    if (PCMindex < PCMsize) //if produced PCM sample are not enough 
        return false; 
    
    return true;    
}


Microphone::Microphone(const Microphone& orig) {
}

bool Microphone::getBuffer(unsigned short* buffer, unsigned short size){

    PCMsize = size;
    PCMindex = 0;
    PCMbuffer = buffer;
    
    bq=new BufferQueue<unsigned short,bufferSize,bufNum>();
    
    {
        FastInterruptDisableLock dLock;
        //Enable DMA1 and SPI2/I2S2 and GPIOB and GPIOC
        RCC->AHB1ENR |= RCC_AHB1ENR_DMA1EN;
        RCC->APB1ENR |= RCC_APB1ENR_SPI2EN;   
        RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN | RCC_AHB1ENR_GPIOCEN;
        
        //Configure GPIOs
        clk::mode(Mode::ALTERNATE);
        clk::alternateFunction(5);
        clk::speed(Speed::_50MHz);
        dout::mode(Mode::ALTERNATE);
        dout::alternateFunction(5);
        dout::speed(Speed::_50MHz);
        
        
        // I2S PLL Clock Frequency: 135.5 Mhz
        RCC->PLLI2SCFGR=(2<<28) | (271<<6);
        //RCC->PLLI2SCFGR=(4<<28) | (429<<6);
        
        RCC->CR |= RCC_CR_PLLI2SON;
    }
    //Wait for PLL to lock
    while((RCC->CR & RCC_CR_PLLI2SRDY)==0) ;
    
    // RX buffer not empty interrupt enable
    SPI2->CR2 = SPI_CR2_RXDMAEN;  
    
    /* The divider is half as 44100Hz 16 bit stereo because
     * the oversampling factor is 32 and PDM is 1 bit,
     * so 44100Hz * 16bit == (44100Hz * 32 * 1bit) / 2
     */
    SPI2->I2SPR=  SPI_I2SPR_MCKOE | 3;

    //Configure SPI
    SPI2->I2SCFGR = SPI_I2SCFGR_I2SMOD | SPI_I2SCFGR_I2SCFG_0 | SPI_I2SCFGR_I2SCFG_1 | SPI_I2SCFGR_I2SE;

    
    NVIC_SetPriority(DMA1_Stream3_IRQn,2);//High priority for DMA
    NVIC_EnableIRQ(DMA1_Stream3_IRQn); 
    
    waiting = Thread::getCurrentThread();
    for (;;){
        if(enobuf){
            enobuf = false;
            dmaRefill();
        }
        if(processPDM(getReadableBuffer(),bufferSize) == true){
            break;
        }
        bufferEmptied();  
        
    }
    
    atomicTestAndWaitUntil(enobuf,true);
unsigned short Microphone::PDMFilter(const unsigned short* PDMBuffer, unsigned int index) {
    
    short combInput, combRes;
    
    // perform integration on the first word of the PDM chunk to be filtered
    for (short i=0; i < 16; i++){
        intReg[0] += pdmLUT[(PDMBuffer[index] >> (15-i)) & 1];
        for (short j=1; j < filterOrder; j++){
            intReg[j] += intReg[j-1];
        }
    }
        
    combInput = intReg[filterOrder-1];// the last cell of intReg contains the integrated signal
    
    //apply the comb filter:
    for (short i=0; i < filterOrder; i++){
        combRes = combInput - combReg[i];
        combReg[i] = combInput;
        combInput = combRes;
    }
    
    return combRes;
    
}
    NVIC_DisableIRQ(DMA1_Stream3_IRQn);
    SPI2->I2SCFGR=0;
    {
	FastInterruptDisableLock dLock;
        RCC->CR &= ~RCC_CR_PLLI2SON;
    }
    delete bq;
    
    enobuf = true;
    
    return true;
}


Microphone::~Microphone() {
}


