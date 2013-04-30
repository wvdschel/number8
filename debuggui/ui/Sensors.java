package ui;

public class Sensors {

	public static final int LEFT  = 0;
	public static final int RIGHT = 1;
	
	public static final int FRONT = 0;
	public static final int REAR  = 2;

	String[] groundSensorValues = new String[4];
	boolean[] groundSensorStates = new boolean[4];
	
	String[] distanceSensors = new String[2];

	String pushSensor;
	
	int[] motorSpeed = new int[2];
	String state = "";
	String stateTimer = "";
}
