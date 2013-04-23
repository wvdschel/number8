#define DATA_SIZE 			116
#define EYES_DATA 			(DATA_SIZE / 2)
#define ACCELERATE_TURNS	EYES_DATA
static int data[DATA_SIZE];

void testRotation()
{
	int speed, i;
	puts("Testing rotation speed.");
	for(speed = 1024; speed > 0; speed -= 128)
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

			for(j = 0; j < EYES_DATA; i++)
			{
				readSensors();
				data[j+EYES_DATA] = distanceSensor(DIR_LEFT);
				data[j] = distanceSensor(DIR_RIGHT);
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

void testBrakes()
{
	int speed, i;
	for(speed = 1024; speed > 0; speed -= 128)
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
			} while(progressRaw >= 0);

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
				printString("[");
				printInt(j);
				printString("]");
				printInt(data[ACCELERATE_TURNS+j % ACCELERATE_TURNS]);
			}
			puts("");
			printString("Distance travelled: ");
			printInt(distance_since_braking);
			puts("");
		}
	}

	puts("Done.");
	while(1)
		delay_ms(500);
}
