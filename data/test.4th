\ blippy:
0 constant LOW
1 constant HIGH 
2 constant OUTPUT
1 constant INPUT
5 constant INPUT_PULLUP 
\ Our green led:
22 constant G-LED                            \ Usage:
: output: create dup , output pinm does> @ ; \ g-led output: led
: on high digw ;                             \ led on
: off low digw ;                             \ led off

\ Blinking:
: 1s 1000 delay-ms ;
g-led output: led
: blik led on 1s led off 1s ;
blik blik blik