# railcomTX

Experimentation board, to generate a railcom signal.  I cannot be sure my decoders are doing what I expect.
This board will look for the railcom cutout and assert data

During a cutout, there is no power to the unit.  it runs off the charge stored in C2 (47uF).
it is trying to drive the line current loop through 2 transistors (2v drop) off 5v and through 22R.
The timeconstant on this is RC or 1uS.  But the RC cutout is 488uS long.  It takes roughly 5 x RC to full charge/discharge a cap.   So max 5uS.  
so it appears the unit will kill its own power source long before the cutout is complete.
even if we assume that the data transmission is 50/50 mark/space, we'd only eek it out to 10uS.

And there is also a 1uF cap on the 12v side into the regulator, but this will not sustain the circuit for long.  further more its also being discharged by the ESD diodes in the PIC
(via dual 100k) as these clamp the rail at 5v.  RC here is 50uS and that's before we consider the current draw of the regulator and rest of circuit.

First test is to apply a DCC signal with cutout and see what voltage droop results prior to even firing the Q1-4 transistor arrangement.













