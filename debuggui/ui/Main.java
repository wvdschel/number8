package ui;

import static ui.Sensors.FRONT;
import static ui.Sensors.LEFT;
import static ui.Sensors.REAR;
import static ui.Sensors.RIGHT;

import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.util.Arrays;

import javax.swing.JFrame;

public class Main {

	public static void main(String[] args) throws Exception {
		
		final Sensors sensors = new Sensors();
		final RobotPanel robotPanel = new RobotPanel(sensors);
		
		final JFrame window = new JFrame("Robot");
		window.setContentPane(robotPanel);
		window.pack();
		window.setSize(400, 400);
		window.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
		window.setVisible(true);
		
		final BufferedReader reader = new BufferedReader(new InputStreamReader(System.in));
		
		Runnable runnable = new Runnable(){

			public void run(){

				try{
					String line = reader.readLine();
					
					while(line != null){
						
						boolean update = true;

						if(line.startsWith("Raw sensor readings: ")){
							
							String[] rawSensors = line.split(":")[1].trim().split(" ");
							sensors.distanceSensors[LEFT ] = rawSensors[1];
							sensors.distanceSensors[RIGHT] = rawSensors[0];
							
							sensors.groundSensorValues[LEFT  | FRONT] = rawSensors[4];
							sensors.groundSensorValues[RIGHT | FRONT] = rawSensors[2];
							sensors.groundSensorValues[LEFT  | REAR ] = rawSensors[3];
							sensors.groundSensorValues[RIGHT | REAR ] = rawSensors[5];
						} 
						else if(line.startsWith("Current push sensors being pressed: ")) {
						
							sensors.pushSensor = line.split(":")[1].trim();
						}
						else if(line.startsWith("Current ground sensors seeing white: ")){
							
							Arrays.fill(sensors.groundSensorStates, false);
							
							String[] groundSensors = line.split(": ")[1].trim().replace(") (", "),(").split(",");
							
							for(int i = 0; i < groundSensors.length; i++){
								
								String groundSensor = groundSensors[i];
								groundSensor = groundSensor.substring(1, groundSensor.length() - 1);
								String[] parts = groundSensor.split(" ");
								
								int lr = parts[1].equals("LEFT")  ? LEFT  : RIGHT;
								int fr = parts[0].equals("FRONT") ? FRONT : REAR;
								
								sensors.groundSensorStates[lr | fr] = true;
							}
						}
						else{
						
							update = false;
						}

						if(update){

							window.repaint();
						}
						
						line = reader.readLine();
					}
				}
				catch(Exception e){ throw new RuntimeException(e); }
			}
		};
		new Thread(runnable).start();
	}
}
