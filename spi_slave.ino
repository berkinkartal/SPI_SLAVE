#include <SPI.h>

#define CS 10
#define MOSI 11
#define MISO 12
#define CLK 13
#define LED 9
#define eeprom_SS 5 //ss

//opcodes
#define WREN  6
#define WRDI  4
#define RDSR  5
#define WRSR  1
#define READ  3
#define WRITE 2

byte eeprom_output_data;
byte eeprom_input_data=0;
byte clr = 0;
byte temp;
int address = 0;
//data buffer
char buffer [128];


char buf [8];
char buf1[8];
volatile byte pos;
volatile boolean process_it;

  /* SPCR byte setup (as per SPI specifications): SPIE|SPE|DORD|MSTR|CPOL|CPHA|SPR1|SPR2
   *  bit8 SPIE= 1 enable SPI interrupt
   *  bit7 SPE= 1 enable SPI
   *  bit6 DORD= 1 LSB first - 0 MSB first
   *  bit5 MSTR= 1 set as Master - 0 set as SLAVE
   *  bit 4 CPOL= 1 clock idle HIGH - idle LOW
   *  bit3 CHPA= 1 sample data on FALLING - 0 on RISING
   *  bit2+1 SPR1+SPR2= 00 speed fastest - 11 slowest

   * Now different way to setup and start SPI . I choosed the direct register write within the main loop()
   * SPCR = (1 << SPE); // Enable SPI as slave.
   * SPI.beginTransaction(SPISettings(16000000,MSBFIRST,SPI_MODE0));
  */
  
void setup (void)
{
  pinMode(LED, OUTPUT);
  pinMode(MOSI, INPUT);
  pinMode(MISO, OUTPUT); // have to send on master in, *slave out*
  pinMode(CS, INPUT);
  pinMode(eeprom_SS,OUTPUT);
  digitalWrite(eeprom_SS,HIGH); //disable device
  Serial.begin (9600);   // debugging
  digitalWrite(LED, LOW);
  // turn on SPI in slave mode
//  SPCR |= bit (SPE);
  SPCR = 0b11000001;
//  SPSR |= 0b00000001;
  SPSR |= bit(SPI2X);
  // get ready for an interrupt 
  pos = 0;   // buffer empty
  process_it = false;

  // now turn on interrupts
  SPI.attachInterrupt();

}  // end of setup

void disable_spi(){
  SPCR = 0b00000000;
  SPDR = clr; 
  delay(10);
}

void start_spi_slave(){
  SPCR = 0b11000001;
//  SPDR = clr; 
  delay(10);
}

void init_eeprom(){
  // SPCR = 01010000
  //interrupt disabled,spi enabled,msb 1st,master,clk low when idle,
  //sample on leading edge of clk,system clock/4 rate (fastest)
  SPCR = 0b01010011;
//  SPCR = (1<<SPE)|(1<<MSTR)|(1<<SPR1)|(1<<SPR2);
  SPSR |= bit(SPI2X);
//  clr=SPSR;
  SPDR = clr;
  delay(10);
}

char spi_transfer(volatile char data){
  
//  SPDR = data;                    // Start the transmission
//  while (!(SPSR & (1<<SPIF)))     // Wait the end of the transmission
//  { };
  byte a = SPI.transfer (data);
  delay(10);
//  return SPDR;  // return the received byte
  return a;

}

byte read_eeprom(int EEPROM_address){
  //READ EEPROM
  int data;
  digitalWrite(eeprom_SS, LOW); //release chip, signal end transfer
  spi_transfer(READ); //transmit read opcode
  spi_transfer((char)(EEPROM_address>>8));   //send MSByte address first
  spi_transfer((char)(EEPROM_address));      //send LSByte address
  data = spi_transfer(0xFF); //get data byte
  digitalWrite(eeprom_SS, HIGH); //release chip, signal end transfer
  return data;
}

// SPI interrupt routine
ISR (SPI_STC_vect)
{
  digitalWrite(LED, HIGH);
  byte c = SPDR;  // grab byte from SPI Data Register
  c = c & 0xFF;
  // add to buffer if room
 // if (pos < (sizeof (buf) - 1)){
    if (pos < 4){
    buf [pos++] = c;
  }
  // example: newline means time to process buffer
  if (c == '\n'){
//    else{
//    SPDR = clr;
    process_it = true;
    }
}  // end of interrupt routine SPI_STC_vect

// main loop - wait for flag set in interrupt routine
void loop (void)
{
  digitalWrite(LED, LOW);
  delay(100);
  //      disable_spi();
  temp = SPDR;
  init_eeprom();
  eeprom_output_data = read_eeprom(address);
  Serial.print(eeprom_output_data, HEX);
  Serial.print('\n');
  SPDR = temp;
  start_spi_slave();
  if (process_it)
    {
    buf [pos] = 0; 
//    Serial.print("data="); 
    for (int i = 0; i < pos ; i++){
      if (buf1[i] != buf[i]){
      Serial.print(buf[i], HEX);

      buf1[i] = buf[i];
      }
    }
    
    
    pos = 0;
    process_it = false;
    }  // end of flag set
    
}  // end of loop
