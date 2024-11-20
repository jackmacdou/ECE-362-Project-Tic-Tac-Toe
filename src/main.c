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

#define SIZE 3

void nano_wait(int);
void autotest();

char board [3][3];
char currentplayer = 'X';

/**
 * @brief Read GPIO port B pin values
 *
 * @param pin_num   : Pin number in GPIO B to be read
 * @return int32_t  : 1: the pin is high; 0: the pin is low
 */
int32_t readpin(int32_t pin_num) {
  int16_t idr = GPIOB->IDR;
  idr &= 1 << pin_num;
  if (idr == 0){
    return 0;
  }
  else {
    return 1;
  }
}

void setn(int32_t pin_num, int32_t val) {
  if (val == 0){
    GPIOB->BRR |= 1<<(pin_num);
  }
  else{
    GPIOB->BSRR |= 1<<(pin_num);
  }
}

int success;
/*int buttons(Row, Column) {
  //setn(8, readpin(0));
  //setn(9, readpin(4));
  //check if position is empty
  success = playermove(Row, Column);
  if (success == 1){
    //reset timer
    //place X or O graphic
    //Alternate player.
  }
  else{
    //don't reset timer
    //player remains the same
    //print invalid move message.
  }
  //reset timer
}*/

char initboard() {
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 3; j++) {
      board[i][j] = ' ';
    }
  }
  return board;
}

int checkwin(char board[SIZE][SIZE]) {
  //check rows and columns
  for (int i = 0; i < 3; i++) {
    if (board[i][0] == currentplayer && board[i][1] == currentplayer && board[i][2] == currentplayer){
      return 1; //row win
    }
    if (board[0][i] == currentplayer && board[1][i] == currentplayer && board[2][i] == currentplayer){
      return 1; //column win
    }
  }
  //check diagonals
  if (board[0][0] == currentplayer && board[1][1] == currentplayer && board[2][2] == currentplayer){
    return 1;
  }
  if (board[0][2] == currentplayer && board[1][1] == currentplayer && board[2][0] == currentplayer){
    return 1;
  }
  return 0;
}

int checkdraw(char board[SIZE][SIZE]) {
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            if (board[i][j] == ' ') {
                return 0;  // Board is not full
            }
        }
    }
    return 1;  // Board is full
}

/*int checkdraw() {
  for (int i = 0; i < 3; i++){
    for (int j = 0; j < 3; j++){
      if (board[i][j] == ' '){
        return 0;//no draw
      }
    }
  }
  return 1; //draw
}*/

int playermove(char board[SIZE][SIZE], int row, int col, char player) {
  if (board[row][col] != ' '){
    return 0;//invalid move
  }
  board[row][col] = player;
  return 1;//valid move
}

/*void switchplayer(){
  if (currentplayer == 'X') {
    currentplayer = 'O';
  }
  else {
    currentplayer = 'X';
  }
}*/

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

//SPI DISPLAY FUNCTIONS

void init_spi1() {
    RCC->AHBENR |= RCC_AHBENR_GPIOAEN;
    RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;
    GPIOA->MODER &= ~0xC000CC00;
    GPIOA->MODER |= 0x80008800;
    GPIOA->AFR[1] &= ~(0b1111<<28);
    GPIOA->AFR[0] &= ~(0b1111<<20);
    GPIOA->AFR[0] &= ~(0b1111<<28);
    SPI1->CR1 &= ~SPI_CR1_SPE;
    SPI1->CR2 = 0b1001<<8;
    //SPI1->CR2 &= 0b0000<<8;
    
    //RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;
    SPI1->CR2 |= SPI_CR2_SSOE;
    SPI1->CR2 |= SPI_CR2_NSSP;
    SPI1->CR2 |= SPI_CR2_TXDMAEN;
    SPI1->CR1 |= 0b111<<3;
    SPI1->CR1 |= SPI_CR1_MSTR;
    SPI1->CR1 |= SPI_CR1_SPE;
}

void spi_cmd(unsigned int data) {
    //RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;
    while ((SPI1->SR&SPI_SR_TXE) == 0){

    }
    SPI1->DR = data;
}

void spi1_init_oled() {
    nano_wait(1000000);
    spi_cmd(0x38);
    nano_wait(600000);
    spi_cmd(0x08);
    nano_wait(600000);
    spi_cmd(0x01);
    nano_wait(2000000);
    spi_cmd(0x06);
    nano_wait(600000);
    spi_cmd(0x02);
    nano_wait(600000);
    spi_cmd(0x0c);
}

void spi1_display1(const char *string) {
    //move cursor to home position
    spi_cmd (0x02);
    //for each character in the string
    while (*string != '\0'){
        spi_data(*string);
        string++;
    }
}
void spi1_display2(const char *string) {
    //move the cursor to the second row (0xc0)
    spi_cmd (0xc0);
    //for each character in the string
    while (*string != '\0'){
        spi_data(*string);
        string++;
    }
}
void spi_data(unsigned int data) {
    spi_cmd(0x200 | data);
}

//============================================================================
// enable_ports()
//============================================================================
void enable_ports(void) {
    RCC->AHBENR |= RCC_AHBENR_GPIOBEN;
    RCC->AHBENR |= RCC_AHBENR_GPIOCEN;
    GPIOB->MODER &= ~0x3FFFFF;
    GPIOB->MODER |= 0x155554;
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

void printtime(int time) {
  if (time == 10){
    spi1_display2("10");
  }
  else if (time == 9){
    spi1_display2("9 ");
  }
  else if (time == 8){
    spi1_display2("8");
  }
  else if (time == 7){
    spi1_display2("7");
  }
  else if (time == 6){
    spi1_display2("6");
  }
  else if (time == 5){
    spi1_display2("5");
  }
  else if (time == 4){
    spi1_display2("4");
  }
  else if(time == 3){
    spi1_display2("3");
  }
  else if(time == 2){
    spi1_display2("2");
  }
  else if(time == 1){
    spi1_display2("1");
  }
  else{
    spi1_display2("0");
  }
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
    #define SPI_OLED
    #if defined(SPI_OLED)

    //spi1_display1("Hello again,"); //REMOVE LATER, THIS IS JUST FOR SPI DISPLAY TESTING
    //spi1_display2(username); //THIS ONE NEEDS TO BE REMOVED, TOO.
    #endif

    // Comment this for-loop before you demo part 1!
    // Uncomment this loop to see if "ECE 362" is displayed on LEDs.
    //for (;;) {
        // asm("wfi");
     //}
    //End of for loop

    // Demonstrate part 1
  //#define SCROLL_DISPLAY
    init_tim7();
    setup_adc();
    init_tim2();
    init_spi1();
    spi1_init_oled();
uint32_t uord1 = 2048;
uint32_t lorr1 = 2048;
uint32_t uord2 = 2048;
uint32_t lorr2 = 2048;
int ud1;
int lr1;
int ud2;
int lr2;
uint32_t uord1prev = 0;
uint32_t lorr1prev = 0;
uint32_t uord2prev = 0;
#define UDLR
#ifdef UDLR
  char board [SIZE][SIZE];
  int row, col;
  int turn = 0;
  char currentplayer;
  int time = 10;
  int tencount = 0;
  int timemilli = 0;
  int tx = 0;
  int to = 0;
  //initboard(board);
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 3; j++) {
      board[i][j] = ' ';
    }
  }
  //return board;
  //volume = 1.3;
  //ADC1->CHSELR |= ADC_CHSELR_CHSEL1;
  for(;;){
    if (turn == 2){
      turn = 0;
    }
    currentplayer = (turn % 2 == 0) ? 'X' : 'O';
    //inputs
    if (currentplayer == 'X'){
      if (tx == 0){
        spi1_display1("Player X Turn");
        tx = 1;
      }
          if (time == 0){
      spi1_display1("Player O Wins!");
      return 0;
      }
      if (tencount == 0){
      printtime(10);
      tencount = 1;
      }
      ADC1->CHSELR |= ADC_CHSELR_CHSEL1;
      ADC1->CHSELR |= ADC_CHSELR_CHSEL2;
      //ADC1->CHSELR &= ~ADC_CHSELR_CHSEL3;
      //ADC1->CHSELR &= ~ADC_CHSELR_CHSEL6;
      nano_wait(100000000);
      timemilli++;//increase millisecond counter
      while ((ADC1->ISR & ADC_ISR_ADRDY) == 0); 
      while (ADC1->DR == 0);
      uord1 = (ADC1->DR);
      if (2.95*uord1/4096 >= 1.9){
        print1("U");
        ud1 = 0;
      }
      else if (2.95*uord1/4096 <= 0.8){
        print1("D");
        ud1 = 2;
      }
      else{
        print1("N");
        ud1 = 1;
      }
      ADC1->CHSELR &= ~ADC_CHSELR_CHSEL1;
      nano_wait(100000000);
      timemilli++;//increase millisecond counter
      if (timemilli == 10){
        time = time - 1;
        printtime(time);
        timemilli = 0;
      }
      while ((ADC1->ISR & ADC_ISR_ADRDY) == 0);
      while (ADC1->DR == 0);
      lorr1 = (ADC1->DR);
      if (2.95*lorr1/4096 >= 1.9){
        print2("R");
        lr1 = 2;
      }
      else if (2.95*lorr1/4096 <= 0.8){
        print2("L");
        lr1 = 0;
      }
      else{
        print2("N");
        lr1 = 1;
      }
      if (readpin(0) == 1){//if the button is pushed
        if (playermove(board, ud1, lr1, currentplayer)){//check if move is valid
          //print X on board graphic
          turn++;//update turn
          while (readpin(0) == 1);//wait for button to be released
          tx = 0;
          time = 10;
          tencount = 0;
          timemilli = 0;//reset timer
          if (checkwin(board)){//check if X won
            spi1_display1("Player X Wins!");
            return 0;
          }
          if (checkdraw(board)){//check if board full
            spi1_display1("Draw!          ");
            return 0;
          }
        }
        else{//invalid move
          spi1_display1("Invalid move!");
        }
      }
    }
    else{
      if (time == 0){
        spi1_display1("Player X Wins!");
        return 0;
      }
      if (tencount == 0){
        printtime(10);
        tencount = 1;
      }
      if (to == 0){
        spi1_display1("Player O Turn");
        to = 1;
      }
      ADC1->CHSELR &= ~ADC_CHSELR_CHSEL1;
      ADC1->CHSELR &= ~ADC_CHSELR_CHSEL2;
      ADC1->CHSELR |= ADC_CHSELR_CHSEL3;
      ADC1->CHSELR |= ADC_CHSELR_CHSEL6;
      nano_wait(100000000);
      timemilli++;//increase millisecond counter
      while ((ADC1->ISR & ADC_ISR_ADRDY) == 0);
      //ADC1->DR &= 0b0;
      while (ADC1->DR == 0);
      //lorr1 = lorr1prev;
      uord2 = (ADC1->DR);
      if (2.95*uord2/4096 >= 1.9){
        print3("U");
        ud2 = 0;
        //volume = volume - 0;
      }
      else if (2.95*uord2/4096 <= 0.8){
        print3("D");
        ud2 = 2;
      }
      else{
        print3("N");
        ud2 = 1;
      }
      //lorr1prev = lorr1;
      //lorr1 = 0;
      ADC1->CHSELR &= ~ADC_CHSELR_CHSEL3;
      nano_wait(100000000);
      timemilli++;//increase millisecond counter
      if (timemilli == 10){
        time = time - 1;
        printtime(time);
        timemilli = 0;
      }
      while ((ADC1->ISR & ADC_ISR_ADRDY) == 0);
      //ADC1->DR &= 0b0;
      while (ADC1->DR == 0);
      //lorr1 = lorr1prev;
      lorr2 = (ADC1->DR);
      if (2.95*lorr2/4096 >= 1.9){
        print4("R");
        lr2 = 2;
        //volume = volume - 0;
      }
      else if (2.95*lorr2/4096 <= 0.8){
        print4("L");
        lr2 = 0;
      }
      else{
        print4("N");
        lr2 = 1;
      }
      //lorr1prev = lorr1;
      //lorr1 = 0;
      if (readpin(0) == 1){//if the button is pushed
        if (playermove(board, ud2, lr2, currentplayer)){//check if move is valid
          //print X on board graphic
          turn++;//update turn
          while (readpin(0) == 1);//wait for button to be released
          to = 0;
          time = 10;
          tencount = 0;
          timemilli = 0;//reset timer
          if (checkwin(board)){//check if O won
            spi1_display1("Player O Wins!");
            return 0;
          }
          if (checkdraw(board)){//check if board full
            spi1_display1("Draw!");
            return 0;
          }
        }
        else{//invalid move
          spi1_display1("Invalid move!");
        }
      }
    }
    
  }
#endif
}
/*
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
*/