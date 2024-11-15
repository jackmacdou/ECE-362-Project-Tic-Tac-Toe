/**
  ******************************************************************************
  * @file    main.c
  * @author  Weili An, Niraj Menon
  * @date    Jan 24 2024
  * @brief   ECE 362 Lab 4 Student template
  ******************************************************************************
*/

/**
******************************************************************************/

// Fill out your username, otherwise your completion code will have the 
// wrong username!
const char* username = "jwmacdou";

/******************************************************************************
*/ 

#include "stm32f0xx.h"
#include <math.h>   // for M_PI
#include <stdint.h>
#include <stdio.h>

void nano_wait(int);
void autotest();

//=============================================================================
// Part 1: 7-segment display update with DMA
//=============================================================================

// 16-bits per digit.
// The most significant 8 bits are the digit number.
// The least significant 8 bits are the segments to illuminate.
uint16_t msg[8] = { 0x0000,0x0100,0x0200,0x0300,0x0400,0x0500,0x0600,0x0700 };
extern const char font[];
// Print an 8-character string on the 8 digits
void print(const char str[]);
// Print a floating-point value.
void printfloat(float f);


//============================================================================
// enable_ports()
//============================================================================
void enable_ports(void) {
    RCC->AHBENR |= RCC_AHBENR_GPIOBEN;
    RCC->AHBENR |= RCC_AHBENR_GPIOCEN;
    GPIOB->MODER &= ~0x3FFFFF;
    GPIOB->MODER |= 0x155555;
    GPIOC->OTYPER |= 0b1111<<4;
    GPIOC->MODER &= ~0xFFFF;
    GPIOC->MODER |= 0b01<<14;
    GPIOC->MODER |= 0b01<<12;
    GPIOC->MODER |= 0b01<<10;
    GPIOC->MODER |= 0b01<<8;
    GPIOC->PUPDR |= 0b01;
    GPIOC->PUPDR |= 0b01<<2;
    GPIOC->PUPDR |= 0b01<<4;
    GPIOC->PUPDR |= 0b01<<6;
}

//============================================================================
// setup_dma() + enable_dma()
//============================================================================
void setup_dma(void) {
    RCC->AHBENR |= RCC_AHBENR_DMAEN;
    DMA1_Channel1->CCR &= ~0b1;
    DMA1_Channel5->CMAR = (uint32_t) msg;
    DMA1_Channel5->CPAR = (uint32_t) &(GPIOB->ODR);
    DMA1_Channel5->CNDTR = 8;
    DMA1_Channel5->CCR |= 0b1<<4;
    DMA1_Channel5->CCR |= 0b1<<7;
    DMA1_Channel5->CCR |= 0b01<<10;
    DMA1_Channel5->CCR |= 0b01<<8;
    DMA1_Channel5->CCR |= 0b1<<5;
}

void enable_dma(void) {
    DMA1_Channel5->CCR |= 0b1;
}

//============================================================================
// init_tim15()
//============================================================================
void init_tim15(void) {
   RCC->APB2ENR |= RCC_APB2ENR_TIM15EN;
   TIM15->PSC = 4800-1;
   TIM15->ARR = 10-1;
   TIM15->DIER |= TIM_DIER_UDE;
   NVIC->ISER[0] |= (1 << 18);
   TIM15->CR1 |= TIM_CR1_CEN;
}

//=============================================================================
// Part 2: Debounced keypad scanning.
//=============================================================================

uint8_t col; // the column being scanned

void drive_column(int);   // energize one of the column outputs
int  read_rows();         // read the four row inputs
void update_history(int col, int rows); // record the buttons of the driven column
char get_key_event(void); // wait for a button event (press or release)
char get_keypress(void);  // wait for only a button press event.
float getfloat(void);     // read a floating-point number from keypad
void show_keys(void);     // demonstrate get_key_event()

//============================================================================
// The Timer 7 ISR
//============================================================================
// Write the Timer 7 ISR here.  Be sure to give it the right name.

void TIM7_IRQHandler() {
  TIM7->SR &= ~TIM_SR_UIF;
  int rows = read_rows();
  update_history(col, rows);
  col = (col + 1) & 3;
  drive_column(col);
}

//============================================================================
// init_tim7()
//============================================================================
void init_tim7(void) {
  RCC->APB1ENR |= RCC_APB1ENR_TIM7EN;
  TIM7->PSC = 4800-1;
  TIM7->ARR = 10-1;
  TIM7->DIER |= TIM_DIER_UIE;
  NVIC->ISER[0] |= (1 << 18);
  TIM7->CR1 |= TIM_CR1_CEN;
}

//=============================================================================
// Part 3: Analog-to-digital conversion for a volume level.
//=============================================================================
uint32_t volume = 2048;

//============================================================================
// setup_adc()
//============================================================================
//int i = 0;
void setup_adc(void) {
  RCC->AHBENR |= RCC_AHBENR_GPIOAEN;
  //configure adc_in1 analog mode
  GPIOA->MODER |= 0b11<<2;
  GPIOA->MODER |= 0b11<<4;
  GPIOA->MODER |= 0b11<<6;
  GPIOA->MODER |= 0b11<<10;
  RCC->APB2ENR |= RCC_APB2ENR_ADCEN;
  //RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;
  RCC->CR2 |= RCC_CR2_HSI14ON;
  while ((RCC->CR2 & RCC_CR2_HSI14RDY) == 0);
  ADC1->CR |= ADC_CR_ADEN;
  while ((ADC1->ISR & ADC_ISR_ADRDY) == 0){

  }
  /*ADC1->CHSELR |= ADC_CHSELR_CHSEL1;
  ADC1->CHSELR |= ADC_CHSELR_CHSEL2;
  ADC1->CHSELR |= ADC_CHSELR_CHSEL3;
  ADC1->CHSELR |= ADC_CHSELR_CHSEL6;*/
  while ((ADC1->ISR & ADC_ISR_ADRDY) == 0){

  } 
  
}

//============================================================================
// Varables for boxcar averaging.
//============================================================================
#define BCSIZE 32
int bcsum = 0;
int boxcar[BCSIZE];
int bcn = 0;

//============================================================================
// Timer 2 ISR
//============================================================================
// Write the Timer 2 ISR here.  Be sure to give it the right name.
int i = 0;
void TIM2_IRQHandler() {
  TIM2->SR &= ~TIM_SR_UIF;
  ADC1->CR |= ADC_CR_ADSTART;
  /*while ((ADC1->ISR & ADC_ISR_EOC) == 0){

  } 
  bcsum -= boxcar[bcn];
  bcsum += boxcar[bcn] = ADC1->DR;
  bcn += 1;
  if (bcn >= BCSIZE){
    bcn = 0;
  }*/
  //volume = bcsum / BCSIZE;
  /*while ((ADC1->ISR & ADC_ISR_EOC)==0){
    if (i = 0){
      ADC1->CHSELR |= ADC_CHSELR_CHSEL1;
      volume = (ADC1->DR);
      while ((ADC1->ISR & ADC_ISR_ADRDY) == 0){

      } 
    }
    else if (i = 1){
      ADC1->CHSELR |= ADC_CHSELR_CHSEL2;
      volume = (ADC1->DR);
      while ((ADC1->ISR & ADC_ISR_ADRDY) == 0){

      } 
    }
    i++;
    if (i == 2){
      i = 0;
    }*/
    volume = (ADC1->DR);
    //uord1 = (ADC1->DR);
  //lorr1 =
    //ADC1->CR &= ~ADC_CR_ADSTART; 
    //ADC1->CR &= ~ADC_CR_ADEN;
    //ADC1->CR |= ADC_CR_ADEN;
    //while ((ADC1->ISR & ADC_ISR_ADRDY) == 0);
    //ADC1->CHSELR |= ADC_CHSELR_CHSEL2;
    //ADC1->CHSELR &= ~ADC_CHSELR_CHSEL2;
    //while ((ADC1->ISR & ADC_ISR_ADRDY) == 0){
    //} 
    //ADC1->CR |= ADC_CR_ADSTART;
    //ADC1->CR |= ADC_CR_ADSTART;
    //ADC1->CHSELR |= ADC_CHSELR_CHSEL3;
    //ADC1->CHSELR |= ADC_CHSELR_CHSEL6; 
  //}
  //ADC1->CHSELR |= ADC_CHSELR_CHSEL2;
  //ADC1->CHSELR |= ADC_CHSELR_CHSEL1;
  //ADC1->CHSELR &= ~ADC_CHSELR_CHSEL1;

}

//============================================================================
// init_tim2()
//============================================================================
void init_tim2(void) {
  RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;
  TIM2->PSC = 4800-1;
  TIM2->ARR = 1000-1;
  TIM2->DIER |= TIM_DIER_UIE;
  NVIC->ISER[0] |= (1 << 15);
  TIM2->CR1 |= TIM_CR1_CEN;
}


//===========================================================================
// Part 4: Create an analog sine wave of a specified frequency
//===========================================================================
void dialer(void);

// Parameters for the wavetable size and expected synthesis rate.
#define N 1000
#define RATE 20000
short int wavetable[N];
int step0 = 0;
int offset0 = 0;
int step1 = 0;
int offset1 = 0;

//===========================================================================
// init_wavetable()
// Write the pattern for a complete cycle of a sine wave into the
// wavetable[] array.
//===========================================================================
void init_wavetable(void) {
    for(int i=0; i < N; i++)
        wavetable[i] = 32767 * sin(2 * M_PI * i / N);
}

//============================================================================
// set_freq()
//============================================================================
void set_freq(int chan, float f) {
    if (chan == 0) {
        if (f == 0.0) {
            step0 = 0;
            offset0 = 0;
        } else
            step0 = (f * N / RATE) * (1<<16);
    }
    if (chan == 1) {
        if (f == 0.0) {
            step1 = 0;
            offset1 = 0;
        } else
            step1 = (f * N / RATE) * (1<<16);
    }
}

//============================================================================
// setup_dac()
//============================================================================
void setup_dac(void) {
  RCC->AHBENR |= RCC_AHBENR_GPIOAEN;
  GPIOA->MODER |= 0b11<<8;
  RCC->APB1ENR |= RCC_APB1ENR_DACEN;
  DAC->CR &= ~DAC_CR_TSEL1;
  DAC->CR |= DAC_CR_TEN1;
  DAC->CR |= DAC_CR_EN1;
}

//============================================================================
// Timer 6 ISR
//============================================================================
// Write the Timer 6 ISR here.  Be sure to give it the right name.
void TIM6_DAC_IRQHandler() {
  TIM6->SR &= ~TIM_SR_UIF;
  offset0 = offset0 + step0;
  offset1 = offset1 + step1;
  if (offset0 >= (N<<16)){
    offset0 = offset0 - (N<<16);
  }
  if (offset1 >= (N<<16)){
    offset1 = offset1 - (N<<16);
  }
  int samp = wavetable[offset0>>16] + wavetable[offset1>>16];
  samp = samp * volume;
  samp = samp >> 17;
  samp = samp + 2048;
  DAC->DHR12R1 = samp;
  //copy samp to DAC->DHR12R1
}
//============================================================================
// init_tim6()
//============================================================================
void init_tim6(void) {
  //(PSC+1)(ARR+1) = clock_f/goal_f
  RCC->APB1ENR |= RCC_APB1ENR_TIM6EN;
  TIM6->PSC = 480-1;
  TIM6->ARR = (48000000/(TIM6->PSC+1))/RATE - 1;
  TIM6->DIER |= TIM_DIER_UIE;
  NVIC->ISER[0] |= (1 << 17);
  TIM6->CR1 |= TIM_CR1_CEN;
  TIM6->CR2 |= TIM_CR2_MMS_1;
}

//============================================================================
// All the things you need to test your subroutines.
//============================================================================
int main(void) {
    // printf("here\n");
    internal_clock();
    // Initialize the display to something interesting to get started.
    msg[0] |= font['E'];
    msg[1] |= font['C'];
    msg[2] |= font['E'];
    msg[3] |= font[' '];
    msg[4] |= font['3'];
    msg[5] |= font['6'];
    msg[6] |= font['2'];
    msg[7] |= font[' '];
    // printf("here\n");
    // Uncomment when you are ready to produce a confirmation code.
    //autotest();

    enable_ports();
    setup_dma();
    enable_dma();
    init_tim15();

    // Comment this for-loop before you demo part 1!
    // Uncomment this loop to see if "ECE 362" is displayed on LEDs.
    //for (;;) {
        // asm("wfi");
     //}
    //End of for loop

    // Demonstrate part 1
  //#define SCROLL_DISPLAY
#ifdef SCROLL_DISPLAY
    for(;;)
        for(int i=0; i<8; i++) {
            print(&"Hello...Hello..."[i]);
            nano_wait(250000000);
        }
#endif

    init_tim7();

    // Demonstrate part 2
//#define SHOW_KEY_EVENTS
#ifdef SHOW_KEY_EVENTS
    show_keys();
#endif

    setup_adc();
    init_tim2();

    // Demonstrate part 3
//#define SHOW_VOLTAGE
#ifdef SHOW_VOLTAGE
    for(;;) {
        printfloat((2.95 * volume / 4096)/10000);
    }
#endif
uint32_t uord1 = 2048;
uint32_t lorr1 = 2048;
uint32_t uord2 = 2048;
uint32_t lorr2 = 2048;
uint32_t uord1prev = 0;
uint32_t lorr1prev = 0;
uint32_t uord2prev = 0;
#define UDLR
#ifdef UDLR
  //volume = 1.3;
  //ADC1->CHSELR |= ADC_CHSELR_CHSEL1;
  for(;;){
    //ADC1->CR &= ~ADC_CR_ADDIS;
    //ADC1->CR &= ~ADC_CR_ADSTART;
    //ADC1->CR |= ADC_CR_ADSTART;
    ADC1->CHSELR |= ADC_CHSELR_CHSEL1;
    ADC1->CHSELR |= ADC_CHSELR_CHSEL2;
    ADC1->CHSELR |= ADC_CHSELR_CHSEL3;
    ADC1->CHSELR |= ADC_CHSELR_CHSEL6;
    nano_wait(100000000);
    while ((ADC1->ISR & ADC_ISR_ADRDY) == 0); 
    //ADC1->DR &= 0b0;
    while (ADC1->DR == 0);
    //uord1 = uord1prev;
    uord1 = (ADC1->DR);
    if (2.95*uord1/4096 >= 1.9){
      print1("U");
      //volume = volume - 0;
    }
    else if (2.95*uord1/4096 <= 0.8){
      print1("D");
    }
    else{
      print1("N");
    }
    //uord1prev = uord1;
    //uord1 = 0;
    ADC1->CHSELR &= ~ADC_CHSELR_CHSEL1;
    //ADC1->CR |= ADC_CR_ADDIS;
    //ADC1->CR &= ~ADC_CR_ADSTART;
    //ADC1->CR |= ADC_CR_ADSTART;
    nano_wait(100000000);
    //ADC1->CR &= ~ADC_CR_ADDIS;
    while ((ADC1->ISR & ADC_ISR_ADRDY) == 0);
    //ADC1->DR &= 0b0;
    while (ADC1->DR == 0);
    //lorr1 = lorr1prev;
    lorr1 = (ADC1->DR);
    if (2.95*lorr1/4096 >= 1.9){
      print2("R");
      //volume = volume - 0;
    }
    else if (2.95*lorr1/4096 <= 0.8){
      print2("L");
    }
    else{
      print2("N");
    }
    //lorr1prev = lorr1;
    //lorr1 = 0;
    ADC1->CHSELR &= ~ADC_CHSELR_CHSEL2;
    nano_wait(100000000);
    //ADC1->CR &= ~ADC_CR_ADDIS;
    while ((ADC1->ISR & ADC_ISR_ADRDY) == 0);
    //ADC1->DR &= 0b0;
    while (ADC1->DR == 0);
    //lorr1 = lorr1prev;
    uord2 = (ADC1->DR);
    if (2.95*uord2/4096 >= 1.9){
      print3("R");
      //volume = volume - 0;
    }
    else if (2.95*uord2/4096 <= 0.8){
      print3("L");
    }
    else{
      print3("N");
    }
    //lorr1prev = lorr1;
    //lorr1 = 0;
    ADC1->CHSELR &= ~ADC_CHSELR_CHSEL3;
    nano_wait(100000000);
    //ADC1->CR &= ~ADC_CR_ADDIS;
    while ((ADC1->ISR & ADC_ISR_ADRDY) == 0);
    //ADC1->DR &= 0b0;
    while (ADC1->DR == 0);
    //lorr1 = lorr1prev;
    lorr2 = (ADC1->DR);
    if (2.95*lorr2/4096 >= 1.9){
      print4("R");
      //volume = volume - 0;
    }
    else if (2.95*lorr2/4096 <= 0.8){
      print4("L");
    }
    else{
      print4("N");
    }
    //lorr1prev = lorr1;
    //lorr1 = 0;
    
    
  }
#endif

    init_wavetable();
    setup_dac();
    init_tim6();

// #define ONE_TONE
#ifdef ONE_TONE
    for(;;) {
        float f = getfloat();
        set_freq(0,f);
    }
#endif

    // demonstrate part 4
// #define MIX_TONES
#ifdef MIX_TONES
    for(;;) {
        char key = get_keypress();
        if (key == 'A')
            set_freq(0,getfloat());
        if (key == 'B')
            set_freq(1,getfloat());
    }
#endif

    // Have fun.
    //dialer();
}
