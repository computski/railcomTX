# railcomTX

Experimentation board, to generate a railcom signal.  I cannot be sure my decoders are doing what I expect.
This board will look for the railcom cutout and assert data

During a cutout, there is no power to the unit.  it runs off the charge stored in C2 (47uF).
it is trying to drive the line current loop through 2 transistors (2v drop) off 5v and through 22R.
The timeconstant on this is RC or 1uS.  But the RC cutout is 488uS long.  It takes roughly 5 x RC to full charge/discharge a cap.   So max 5uS.  
so it appears the unit will kill its own power source long before the cutout is complete.
even if we assume that the data transmission is 50/50 mark/space, we'd only eek it out to 12uS.













