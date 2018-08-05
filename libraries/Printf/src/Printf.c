#include <Arduino.h>
// Note: an alternative might be to add printf to the Print library
// see https://www.stm32duino.com/viewtopic.php?t=1014#p12014

// serial_putchar - char output function for printf
static int serial_putchar (char c, FILE *stream) {
   if( c == '\n' )
      Serial.write('\r');
   Serial.write(c) ;
   return 0 ;
}

static FILE fd1 = { 0 };   // FILE struct
static int uart_putchar(char c, FILE *stream);


void setup() {
   // For printf: fills in the UART file descriptor with pointer to putchar func.
   fdev_setup_stream(&uartout, uart_putchar, NULL, _FDEV_SETUP_WRITE);
   stdout = &uartout;

   Serial.begin(9600);
   printf("Hello, World!\n");
}


void loop( void ){
   printf("Elapsed time: %ld\n", millis());
}


