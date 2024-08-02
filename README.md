# railcomTX

Experimentation board, to generate a railcom signal.  I cannot be sure my decoders are doing what I expect.
This board will look for the railcom cutout and assert data

During a cutout, there is no power to the unit.  it runs off the charge stored in C2 (47uF).
it is trying to drive the line current loop through 2 transistors (2v drop) off 5v and through 22R.
The timeconstant on this is RC or 1mS.  The RC cutout is 488uS long.  It takes roughly 5 x RC to full charge/discharge a cap.   So max 5mS.  
Also the data signal is approx 50% mark/space so the current draw is less than the theoretical max of always-active.
i.e. there should be sufficient power reserve in the Cap to power the data and MCU, provided we are not drawing power via external LEDs, motors etc.


And there is also a 1uF cap on the 12v side into the regulator, but this will not sustain the circuit for long.  further more its also being discharged by the ESD diodes in the PIC
(via dual 100k) as these clamp the rail at 5v.  RC here is 50mS and that's before we consider the current draw of the regulator and rest of circuit.

First test is to apply a DCC signal with cutout and see what voltage droop results prior to even firing the Q1-4 transistor arrangement.

Data is sent in two channels.  12 bits (2 bytes) in Ch1 and 36 bits 6 (bytes) in Ch2, where a byte is actually 6 bits.   Datagrams are never split over multiple RC responses.

Decoders only respond when they see a packet addressed to them.  The Idle packet is a special case where the address is equivalent to 255.
Decoders are not allowed to send feedback to other addresses or to service mode packages.

Ch1 is used to echo the mobile decoder address for quick localisation.  e.g. transmit to loco 7003 and you expect to see 7003 come back in Ch1.
Its not clear from the spec whether all decoders are to respond, or just the addressed decoder.  I think just the addressed one, else you'd have clashes.
But this means railcom cannot dectect what locos are on the track unless it polled all possible addresses.  I think this is why railcomPlus was invented, but this is proprietary.

I am interested in polling a loco and in setting and reading its CVs.














