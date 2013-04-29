#define DATA_SIZE 			116
#define EYES_DATA 			(DATA_SIZE / 2)
#define ACCELERATE_TURNS	EYES_DATA
static int data[DATA_SIZE];

void testRotation()
{
	int speed, i;
	puts("Testing rotation speed.");
	for(speed = 1023; speed > 0; speed -= 128)
	{
		for(i = 0; i < 4; i++)
		{
			int left, right, j;
			if(i % 2 == 0)
			{
				left = -speed;
				right = speed;
			} else {
				right = -speed;
				left = speed;
			}

			MOTOR_LEFT(left);
			MOTOR_RIGHT(right);

			for(j = 0; j < EYES_DATA; j++)
			{
				readSensors();
				data[EYES_DATA+j] 	= distanceSensor(DIR_LEFT);
				data[j] 			= distanceSensor(DIR_RIGHT);
				delay_ms(SLEEP_TIME);
			}

			printString("Turning around at speed ");
			printInt(speed);
			puts("");

			printString("Left eye: ");
			left = 0;
			for(j = 0; j < EYES_DATA; j++)
			{
				left += data[EYES_DATA+j];
				printInt(data[EYES_DATA+j]);
				printString(" ");
			}
			printString("- avg: ");
			printInt(left/EYES_DATA);
			puts("");

			printString("Right eye: ");
			right = 0;
			for(j = 0; j < EYES_DATA; j++)
			{
				right += data[j];
				printInt(data[j]);
				printString(" ");
			}
			printString("- avg: ");
			printInt(right/EYES_DATA);
			puts("");
		}
	}

	puts("Done.");
	while(1)
		delay_ms(500);
}

void driveUntilWhite()
{
	int speed = 800;
	while(1)
	{
		int j = 0, i;
		printString("Changing speed to ");
		printInt(speed);
		puts("");

		MOTOR_LEFT(speed);
		MOTOR_RIGHT(speed);

		for(i = 0; i < 20; i++)
		{
			if(i % 5 == 0)
			{
				readSensors();
				printState();
			}
			delay_ms(20);
			if(pushSensor(DIR_BACK))
				break;
		}
		
		puts("Looking for white.");
		while(1)
		{
			readSensors();
			if(j % 25 == 0)
			{
				printState();
			}
			if(	groundSensor(DIR_FORWARD | DIR_RIGHT)  || groundSensor(DIR_BACK | DIR_RIGHT) ||
				groundSensor(DIR_FORWARD | DIR_LEFT) || groundSensor(DIR_BACK | DIR_LEFT))
				break;

			data[j%EYES_DATA+EYES_DATA] = sensor[0];
			data[j%EYES_DATA] = sensor[1];
			delay_ms(SLEEP_TIME);
			
			if(pushSensor(DIR_BACK))
				break;

			j++;
		}

		MOTOR_LEFT(0);
		MOTOR_RIGHT(0);

		if(j > EYES_DATA)
		{
			i = j - EYES_DATA;
		} else {
			i = 0;
		}
		
		printString("Last moments (speed ");
		printInt(speed);
		printString("):");
		for(; i < j; i++)
		{
			printString(" [");
			printInt(i);
			printString("] L=");
			printInt(data[i % EYES_DATA]);
			printString(" R=");
			printInt(data[EYES_DATA + i % EYES_DATA]);
		}

		speed = -speed;

		if(pushSensor(DIR_BACK))
			break;
	}

	MOTOR_LEFT(0);
	MOTOR_RIGHT(0);

	puts("Aaaand we're done.");
	while(1)
	{
		readSensors();
		printState();
		delay_ms(300);
	}
}

void oneEyedBrake()
{
	int count = 0;
	MOTOR_LEFT(1023);
	MOTOR_RIGHT(1023);
	while(count < 3)
	{
		if(initialEyeLeft - sensor[1] >= 20)
		{
			count++;
		} else {
			count = 0;
		}
		readSensors();
		if(pushSensor(DIR_BACK))
			break;
		delay_ms(SLEEP_TIME);
	}
	MOTOR_LEFT(0);
	MOTOR_RIGHT(0);

	puts("Aaaand we're done.");
	while(1)
	{
		readSensors();
		printState();
		delay_ms(300);
	}
}

void testBrakes()
{
	int speed, i;
	for(speed = 1023; speed > 0; speed -= 128)
	{
		for(i = 0; i < 4; i++)
		{
			long distance, distance_since_braking;
			int realspeed, j, brake_time;
			if(i % 2 == 0)
			{
				realspeed = speed;
			} else {
				realspeed = -speed;
			}

			printString("Speeding up to ");
			printInt(realspeed);
			printString(" for ");
			printInt(ACCELERATE_TURNS * SLEEP_TIME);
			puts("ms");

			distance = 0;

			MOTOR_LEFT(realspeed);
			MOTOR_RIGHT(realspeed);

			for(j = 0; j < ACCELERATE_TURNS; j++)
			{
				readSensors();
				data[j] = progressRaw;
				distance += progressRaw;
				delay_ms(SLEEP_TIME);
			}

			MOTOR_LEFT(-realspeed);
			MOTOR_RIGHT(-realspeed);

			brake_time = 0;
			distance_since_braking = 0;
			do
			{
				readSensors();
				data[ACCELERATE_TURNS+brake_time % ACCELERATE_TURNS] = progressRaw;
				distance_since_braking += progressRaw;
				delay_ms(SLEEP_TIME);
				brake_time += 1;
			} while(
				   // If this is a forward braking test, first backward motion makes us exit
				   (progressRaw >= 0 && realspeed > 0)
				   // If this is a backward braking test, first forward motion makes us exit
				|| (progressRaw <= 0 && realspeed < 0));

			MOTOR_LEFT(0);
			MOTOR_RIGHT(0);
			
			printString("Acceleration progress: ");
			for(j = 0; j < ACCELERATE_TURNS; j++)
			{
				printString("[");
				printInt(j);
				printString("]");
				printInt(data[j]);
			}
			puts("");
			printString("Distance travelled: ");
			printInt(distance);
			puts("");

			printString("Braking took ");
			printInt(brake_time * SLEEP_TIME);
			puts("ms.");

			if(brake_time > ACCELERATE_TURNS)
			{
				j = brake_time - ACCELERATE_TURNS;
			} else {
				j = 0;
			}
			
			printString("Last turns: ");
			for(; j < brake_time; j++)
			{
				printString(" [");
				printInt(j);
				printString("]");
				printInt(data[ACCELERATE_TURNS+j % ACCELERATE_TURNS]);
			}
			puts("");
			printString("Distance travelled: ");
			printInt(distance_since_braking);
			puts("");
			delay_ms(1000);
		}
	}

	puts("Done.");
	while(1)
		delay_ms(500);
}

void flankTheBox()
{
	int count = 0, stallCount = 0;
	MOTOR_LEFT(800);
	MOTOR_RIGHT(800);
	delay_ms(300);

	while(stallCount < 3)
	{
		readSensors();
		if(count > PROGRESS_HISTORY_SIZE && progress < 5)
		{
			printString("Progress: ");
			printInt(progress);
			puts("");
			stallCount++;
		} else
			stallCount = 0;
		delay_ms(SLEEP_TIME);
		count ++;
	}

	MOTOR_LEFT(-1023);
	MOTOR_RIGHT(-1023);

	delay_ms(1000);

	MOTOR_LEFT(0);

	delay_ms(1000);

	MOTOR_LEFT(1023);
	MOTOR_RIGHT(1023);

	delay_ms(1000);

	puts("Aaaand we're done.");
	while(1)
	{
		readSensors();
		printState();
		delay_ms(300);
	}
}

void drive100Clicks()
{
	int totalProgress = 0;
	MOTOR_LEFT(700);
	MOTOR_RIGHT(700);
	while(totalProgress < 100)
	{
		readSensors();
		totalProgress += progressRaw;
		printState();
		delay_ms(SLEEP_TIME);
	}
	MOTOR_LEFT(0);
	MOTOR_RIGHT(0);
	puts("Aaaand we're done.");
	while(1)
	{
		readSensors();
		printState();
		delay_ms(300);
	}
}
