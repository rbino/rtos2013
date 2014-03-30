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

using namespace std;
using namespace miosix;

typedef Gpio<GPIOB_BASE,10> clk;
typedef Gpio<GPIOC_BASE,3> dout;

static const int bufferSize=2048; //Buffer RAM is 4*bufferSize bytes
static Thread *waiting;
static BufferQueue<unsigned short,bufferSize> *bq;
static bool enobuf=true;

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
                     DMA_SxCR_MSIZE_0 | //Read  16bit at a time from RAM
					 DMA_SxCR_PSIZE_0 | //Write 16bit at a time to SPI
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
        bq->IRQbufferEmptied();
	waiting->IRQwakeup();
	if(waiting->IRQgetPriority()>Thread::IRQgetCurrentThread()->IRQgetPriority())
		Scheduler::IRQfindNextThread();
}

Microphone& Microphone::instance()
{
	static Microphone singleton;
	return singleton;
}

Microphone::Microphone() {
    recording = false;
}

Microphone::Microphone(const Microphone& orig) {
}

void Microphone::start(){
    recording = true;
    bq=new BufferQueue<unsigned short,bufferSize>();

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
        
        
        //Enable audio PLL (settings for 44100Hz audio) TODO: trovare divisore (Questo diviso 16?)
        // I2S PLL Clock Frequency: 135.5 Mhz
        //RCC->PLLI2SCFGR=(2<<28) | (271<<6);
        
        //I2S PLL Clock Frequency: 262 Mhz
        RCC->PLLI2SCFGR=(2<<28) | (262<<6);
        
        RCC->CR |= RCC_CR_PLLI2SON;
    }
    //Wait for PLL to lock
    while((RCC->CR & RCC_CR_PLLI2SRDY)==0) ;
    
    SPI2->CR2 = SPI_CR2_RXDMAEN;  
    
    // Fs = I2SxCLK / [(16*2)*((2*I2SDIV)+ODD)*8)] when the channel frame is 16-bit wide (/2 if Mono)
    SPI2->I2SPR=  SPI_I2SPR_MCKOE | 2;

    //Configure SPI
    SPI2->I2SCFGR = SPI_I2SCFGR_I2SMOD | SPI_I2SCFGR_I2SCFG_0 | SPI_I2SCFGR_I2SCFG_1 | SPI_I2SCFGR_I2SE;
    
    // RX buffer not empty interrupt enable

    
    NVIC_SetPriority(DMA1_Stream3_IRQn,2);//High priority for DMA
    NVIC_EnableIRQ(DMA1_Stream3_IRQn);
    
    waiting = Thread::getCurrentThread();
    
    dmaRefill();  
    
}

}

Microphone::~Microphone() {
}

bool Microphone::isRecording() const{
    return recording;
}
