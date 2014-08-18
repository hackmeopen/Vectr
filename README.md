Vectr
=====

Vectr 3D Sensing Controller

Vectr is an open source 3D sensing controller and sequencer. The board files and firmware are here. Vectr is released under 
the Modified BSD License. Vectr uses a number of libraries from Microchip which, although they are free to use,
 are not fully open source. 
The BSD license allows me to release my contributions as free to use while retaining Microchip's copyrights. This arrangement
was worked out in discussions with the legal department of Microchip.


Vectr uses Microchip Technology's MGC3130 to sense a user's hand in free space above it's surface. It is able to determine the position of the user's hand within a  cubic volume which is 4inx4inx6in. 

Vectr has 36 LEDs which provide visual feedback about where it senses the user's hand.

It is designed to work with Eurorack modular synthesizers, but the underlying technology can be used for controlling just about anything which will receive an analog voltage input.

Vectr has lots of nifty features for recording sequences of hand movements and playing them back. The sequences can be manipulated as they are played with all sorts of different behaviors. These are the things that make Vectr really special.

So, the code's all there. Vectr uses a PIC32 microcontroller running FreeRTOS. 
There's a lot going on but I've tried to comparmentalize it the best I can. So get started, use it, tweak it, build it!

Cheers

Matt
