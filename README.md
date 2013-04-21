number8
=======

Number8 is the second iteration of our [WELEK robot sumo competition](http://ieeesb.elis.ugent.be/nl/welek/robot/2013) entry.
It is a differential wheeled robot, built out of Lego Technics and powered by 5 Lego Technic motors and a PIC18.

It is equiped with 8 input sources:

1. 2x analog infrared distance sensors on the front
2. 4x analog color sensors on the bottom, one on each corner of the robot, to detect the edge of the arena
3. 1x digital pressure sensor on the back, to detect an attack from behind (this sensor is colloqually referred to as "the butt-sex-detector").
4. 1x rotary encoder to measure our forward/backward movement

Media
-----

[A video featuring our first build, number7](http://www.youtube.com/embed/BkIkCT7gmbQ). It is the robot with double batteries and green-grey scoop. It is also the only robot consistently winning throughout the video, and the only one that does not recieve human help ;)

Since then, we have made the robot faster (a gear ratio change, resulting in a 9 times faster robot, at the cost of a 9 times lower torque).

We also replaced the scoop, since it was mechanically flawed and it on multiple occasions resulted in our robot being lifted off the ground with all but its last wheels. It now has a bumper made of wheels constantly pushing the enemy away.

The progress wheel was also not present in the first version.

Operating instructions
----------------------

As per the contest rules, the robot operates autonomously during combat. The only operations a meatbag assistant should perform is to start the robot. This is done as follows:

1. Put the robot down inside the arena. Make sure all corner sensors are inside the arena, and not on or over the edge. Also make sure that nothing is in front of the robot, since that would mess up the initial sensor calibration in the next step.
2. Press any button on the Dwengo board to calibrate the sensors. Whatever is under the ground sensors will be considered as the color of the arena, and the distance sensor values will be considered "infinite distance" values.
3. Press the north or south buttons to start the robot in forward drive mode, or left or right to have it perform a flanking operation.
4. Watch it destroy the competition.

Code
----
We started from a modified version of the "sumolib" provided by the contest organization. We modified it to use our motor controller and to print to serial instead of the LCD.

Most of the logic is contained in aiLib.c. The robot operates as a state machine, switching between a search state, a destroy state, a flanking state and a survival state based on its sensor readings. Transitions can easily be found by searching for `SWITCH_STATE`.

The `sumolib` directory contains the modified version of the sumolib we use, while `quadenclib` contains code we plucked off the internet (TODO: add link) for using our rotary encoder.

Debugging
---------

There are two debugging methods available: serial prints and LEDs 4 trough 7 on the Dwengo board.

### Serial debugging
Serial debugging is enabled by setting `DEBUG` to `1` in `serialDebug.h`.

Since the motor controller is being operated over serial as well, we can't have serial output and motor operation at the same time. When serial debugging is enabled, the motor controller should be disconnected. Because of this reuse of the serial port, this debugging method can only be used for diagnosing sensor reading and state transition issues, but not for logging or debugging the robot in action.

### LED debugging
The four last LEDs of the Dwengo board indicate the state the robot is in:

	 LD7 LD6 LD5 LD4 LD3 LD2 LD1 LD0
	/-------------------------------\
	| o   o   o   o   o   o   o   o |
	\-------------------------------/
	  |   |   |   |       |   |_ Rotary encoder feedback
	  |   |   |   |       |_____ Rotary encoder feedback
	  |   |   |   |_____________ Not used
	  |   |   |_________________ STATE_SEEK
	  |   |_____________________ STATE_DESTROY
	  |_________________________ STATE_SURVIVE

If both LD5 and LD6 are lit, the robot is in STATE_FLANK.