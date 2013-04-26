import java.io.BufferedReader;
import java.io.FileReader;
import java.io.InputStreamReader;

import javax.swing.JFrame;

public class Main {

	public static void main(String[] args) throws Exception {
		
		final RobotPanel robotPanel = new RobotPanel();
		
		final JFrame window = new JFrame("Robot");
		window.setContentPane(robotPanel);
		window.pack();
		window.setSize(400, 400);
		window.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
		window.setVisible(true);
		
		final BufferedReader reader = new BufferedReader(new InputStreamReader(System.in));
		
		Runnable runnable = new Runnable(){

			public void run(){

				String[] sensors = new String[7];

				try{
					String line = reader.readLine();
					
					while(line != null){

						if(line.startsWith("Raw sensor readings: ")){
							
							String[] rawSensors = line.split(":")[1].trim().split(" ");
							System.arrayCopy(rawSensors, 0, sensors, 0, 6);
						} 
						else if(line.startsWith("Current push sensors being pressed: ")) {
						
							
						}

						robotPanel.sensors = sensors;
						window.repaint();
						
						line = reader.readLine();
					}
				}
				catch(Exception e){ throw new RuntimeException(e); }
			}
		};
		new Thread(runnable).start();
	}
}
