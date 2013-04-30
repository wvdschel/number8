package ui;

import static ui.Sensors.FRONT;
import static ui.Sensors.LEFT;
import static ui.Sensors.REAR;
import static ui.Sensors.RIGHT;

import java.io.BufferedReader;
import java.io.InputStreamReader;

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
												
						String[] data = line.split(",");
						
						if(data.length == 21){
							sensors.state = data[0];		// 0:  [A-Z]{3} state
							sensors.stateTimer = data[1];	// 1:  stateTimer

							sensors.motorSpeed[LEFT ] = Integer.parseInt(data[2]); // 2:  LSpeed
							sensors.motorSpeed[RIGHT] = Integer.parseInt(data[3]); // 3:  RSpeed

							/* Ground sensors */
							sensors.groundSensorStates[RIGHT | FRONT] = data[4].equals("1"); // 4:  FRGround
							sensors.groundSensorStates[LEFT  | REAR ] = data[5].equals("1"); // 5:  RRGround
							sensors.groundSensorStates[LEFT  | FRONT] = data[6].equals("1"); // 6:  FLGround
							sensors.groundSensorStates[RIGHT | REAR ] = data[7].equals("1"); // 7:  RLGround
							
							/* Push sensors */
							sensors.pushSensor = data[8]; // 8:  RPressure

							/* Long distance sensors */
							// 9:  LDistance
							// 10: LDistance
							sensors.distanceSensors[LEFT ] = data[12]; // 11: RDistanceRaw
							sensors.distanceSensors[RIGHT] = data[11]; // 12: LDistanceRaw

							/* Ground sensors */
							sensors.groundSensorValues[RIGHT | FRONT] = data[13]; // 13: FRGroundRaw
							sensors.groundSensorValues[LEFT  | REAR ] = data[14]; // 14: RLGroundRaw
							sensors.groundSensorValues[LEFT  | FRONT] = data[15]; // 15: FLGroundRaw
							sensors.groundSensorValues[RIGHT | REAR ] = data[16]; // 16: RRGroundRaw

							/* Miscellaneous */
							// 17: ProgressSum
							// 18: ProgressLast
							// 19: stateProgress
							
							try{ Thread.sleep(100); } catch(InterruptedException e){}
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
