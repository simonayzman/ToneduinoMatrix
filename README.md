<h1>ToneduinoMatrix</h1>

Arduino based adaptation of the popular Tone Matrix.

Here is the source of inspiration: http://tonematrix.audiotool.com/

<h3>Description</h3>

I have made an Arduino-based version of the Tone Matrix and it has served as my inspiration from the very beginning of this project. The original Tone Matrix can be found at tonematrix.audiotool.com. Conceptually, the Tone Matrix is a grid of squares. Usually, the matrix itself is square, but it doesn’t necessarily have to be. All the squares in any single row represent the same note. For example, all the squares in the first row can represent a C (2093 Hz) note. Moreover, each individual square in a column represents a unique note. For example, if a column has four squares, they might (from top to bottom) represent a C (2093 Hz) followed by an A (1760 Hz) followed by a G (1568 Hz) followed by an F (1394 Hz). As in the example, the notes do not necessarily have to be tonally sequential. With this configuration of rows and columns, the purpose of the tone matrix is to continuously loop through a sequence of notes that the user “activates.” Any square in the matrix can be activated. Starting at the first column, the looping mechanism plays the active note(s) in that column for a set amount of time, moves on to the column in the immediate right, and repeats the process. If the program reaches the last column, it simply loops back to the leftmost column. It is therefore easiest to think of the columns as representing time; columns that are more left will play notes earlier than the columns that are more right. This is essentially the way the tone matrix works. 

A few adjustments and improvements were made with my Arduino adaptation. (1) The user can pause and play the program. (2) The user can automatically clear the screen of active notes. (3) The user can adjust the volume. (4) The user can adjust the speed at which the program runs. (5) The Toneduino Matrix can only support playing a single note per column. (6) A physical 32x32 matrix will be used, but it will function as a 16x16 matrix (each “square” is 2x2). The reasons for adjustments (1), (2), and (3) are simply that they provide extra conveniences to the user that were not present in the original Tone Matrix. Adjustment (4) adds a new dimension by supporting different rhythms, allowing for a larger variety of sequences that can be either fast or slow. Adjustment (5), on the other hand, detracts from the original. With the current Tone library, the Arduino cannot handle more than two notes simultaneously, even if there are more than two timers available (such as on a Mega). For purposes of simplicity, this adaptation chooses not to take the option of even playing two notes per column and instead stays with one. Adjustment (6) was decided for aesthetic reasons and for user visibility, but the general availability of parts on the market was also a factor.

Note that there is one last major design difference—this project resembles the function of a laptop. The actual touch activation of a square is separate from where that active square appears; a touch screen registers a touch (where the keyboard would be) and the LED Matrix reflects this change (where the screen would be). If the touch screen were the same size as the LED Matrix, this would be less of an issue; it could simply serve as an overlay on top of it, thereby saving space. However, in this version, this was not the design choice made.

<h3>Parts List</h3>

<b>Arduino MEGA 2560 R3</b><br>
The Arduino that will do the calculation and synchonrization.<br>
http://www.amazon.com/gp/product/B006H0DWZW/ref=oh_details_o06_s00_i00?ie=UTF8&psc=1

<b>32x32 RGB LED Matrix Panel</b><br>
The visible screen that shows the currently activated squares.<br>
https://www.adafruit.com/product/1484

<b>Resistive Touchscreen Overlay</b><br>
Touchscreen that the user will use to activate and deactivate squares in the matrix.
https://www.adafruit.com/product/1676

<b>Resistive Touch Screen Controller - STMPE610</b><br>
Controller that interfaces between the Arduino and the touchscreen<br>
https://www.adafruit.com/product/1571


<b>12VDC 20A Two Position ON/OFF Toggle Switch</b><br>
Turns the entire program on and off.<br>
http://www.amazon.com/gp/product/B009IS86ZG/ref=oh_details_o03_s00_i01?ie=UTF8&psc=1

<b>5V 3A Switching AC Adapter Power Supply</b><br>
Used to simultaneously power the Arduino and the LED Matrix.<br>
http://www.ebay.com/itm/300637535405?ssPageName=STRK:MEWNX:IT&_trksid=p3984.m1497.l2649

<b>Power Adaptor DC Splitter 1 Female to 2 Male Cable</b><br>
Splits the power from the adapter; one end goes to the Arduino and the other to the LED Matrix.<br>
http://www.ebay.com/itm/261463291739?ssPageName=STRK:MEWNX:IT&_trksid=p3984.m1497.l2649

<b>2.1mm DC barrel jack</b><br>
Required for the connection between the adapter and the LED Matrix.<br>
https://www.adafruit.com/products/610

<b>30mm Arcade Button (x2)</b><br>
One button is used for pausing/playing and the other is used to clear the screen.<br>
https://www.adafruit.com/products/471<br>

<b>8 Ohm .5 W Speakers (x2) with Stereo Headphone Plug</b><br>
Source was a previous music player that came with a headphone plug attached. Plays the notes as per the direction of the Arduino.<br>

<b>3.5mm Stereo Headphone Jack</b><br>
Connects the speaker to the arduino.<br>
https://www.adafruit.com/products/1699


<b>10K Ohm Rotary Linear Potentiometers (x2)</b><br>
One is used for volume control and the other is for speed controller.<br>
http://www.amazon.com/gp/product/B00AH8DN4A/ref=oh_details_o03_s01_i00?ie=UTF8&psc=1

<b>Potentiometer Knob Caps (x 2)</b><br>
Used for aesthetic reasons.<br>
http://www.amazon.com/gp/product/B008DFA6YE/ref=oh_details_o03_s02_i00?ie=UTF8&psc=1

<b>10K Ω resistor (x4)</b><br>

<h2>Libraries Used</h2>

<b>Adafruit_STMPE610.h</b><br>
Adafruit native library used to interface the STMPE610 Touch Screen Controller.<br>
https://github.com/adafruit/Adafruit_STMPE610

<b>RGBmatrixPanel.h</b><br>
Native library used to operate LED Matrices. Supports the 16x32 and 32x32 designs.<br>
https://github.com/adafruit/RGB-matrix-Panel

<b>Adafruit_GFX.h</b><br>
Adafruit native graphics library necessary for the RGBmatrixPanel library to function.<br>
https://github.com/adafruit/Adafruit-GFX-Library

<h3>Assembly Instructions</h3>

<h5>I. Power Supply</h5>

For this project, the Arduino and the LED Matrix need to be powered separately, but they can still receive power from a single source. Because the Arduino operates at 5V and draws up to 500 mA and the LED Matrix operates at 5V draws up to 2000 mA, a single 5V 3A Switching AC Adapter Power Supply will be used to receive power from an outlet. Start by separating the bifurcated rubber wire of the adapter. Identify the ground wire and the power wire. Cut open only the power wire and leave the ground wire intact. Take out the exposed wire and solder both ends to a toggle switch. This switch will allow the user to turn the project on and off. Next, connect the adapter to a splitter. For more stability, use strong tape on this connection so that the adapter and the splitter won’t disconnect in the future; by itself, it is rather weak. One end of the splitter will power the Arduino. You may connect it now; this part is done. The other end will power the LED Matrix, but some more connections must first be made. Notice that the custom power lines provided with the LED Matrix have two metal forks—one for ground and one for power. Solder a wire to each fork and then solder those wires to the respective ground and power connections of the DC barrel jack. Finally, plug the second split end into the DC barrel jack and plug the LED Matrix power line into the matrix itself.

<h5>II. Buttons</h5>
Start by soldering a connection from 5V and ground to the power columns of a protoboard. The first things to be soldered will be the buttons. Take a 10K Ω resistor and solder into from 5V to any row. Solder two wires into this row. One will be for the pause/play button and the other will be for the reset button. Solder each of these to one end of a buttons. Solder some more wires to the other end of the buttons and solder them into two separate rows of the protoboard. Then solder a 10K Ω resistor from each of these rows to ground. Then solder another wire into each of these rows and plug the other end into the Arduino. The pause/button wire is pin 2 and the reset button is pin 3. 

<h5>III. Potentiometers</h5>
Start with the volume potentiometer. Solder the ground connection of the Stereo Headphone Jack to ground and solder the power connection to the rightmost solder point of a 10K Ω resistor. Then solder a wire to the middle point of the potentiometer. Then plug this wire into pin 5 of the Arduino. Now, we will do the speed potentiometer. Solder wires into all three points of the potentiometer. The solder the left wire to power, solder the right wire to ground, and plug the middle wire into pin A15.

<h5>IV. Touch Screen & Touch Screen Controller (STMPE610)</h5>
Start by soldering the touch screen microcontroller into an appropriately sized protoboard. Solder a wire into one of the power columns of the protoboard. This will be the outbound ground connection. Solder a 10K Ω resistor from the row labeled Mode to the row labeled GND. Next, solder a wire from GND to the power column ground connection. Solder four wires into the protoboard for the rows labeled SCL, SDO, Vin, and 3.3V; plug each of these into their respective pins on the Arduino, noting that SCL is pin 21 and SDO is pin 20. The wiring for the controller is done. Move on to the touch screen itself. Start by removing the plastic of the overlay from both the top and the bottom. For the convenience of the user, it may be prudent to prefabricate a 16x16 grid of some sort that matches the dimensions of the touch screen. This could be printed out on paper and simply taped on. Finally, push the thin plastic copper connection of the touchscreen into the appropriate plug in the controller.

<h5>V. LED Matrix</h5>
You must now make a connection between the LED Matrix and the Arduino. You can follow the complicated intstructions on https://learn.adafruit.com/32x16-32x32-rgb-led-matrix/wiring-the-32x32-matrix. Essentially, though, you must do the following. Respectively connect the pins labeled R0, G0, B0, R1, G1, and B1 to pins 24-29 on the Arduino. Connect A, B, C, and D to A0-A3. Connect OE to 9, CLK to 11, and LAT/STNDBY to 10. Lastly, connect the remaining ground pins to a ground connection.

