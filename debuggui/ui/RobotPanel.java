package ui;

import static ui.Sensors.FRONT;
import static ui.Sensors.LEFT;
import static ui.Sensors.REAR;
import static ui.Sensors.RIGHT;

import java.awt.Color;
import java.awt.FontMetrics;
import java.awt.Graphics;

import javax.swing.JPanel;

public class RobotPanel extends JPanel {

	private static int x = 0;
	private static int y = 0;
	private static int w = 250;
	private static int h = 250;
	private static int dh = 30;
	private static int size = 50;
	
	private Sensors sensors;
	
	public RobotPanel(Sensors sensors){
		
		this.sensors = sensors;
	}
	
	public void paintComponent(Graphics g){
		
		int dx = (getWidth() - w)/2;
		int dy = (getHeight() - h)/2;
		g.translate(dx, dy);
		
		FontMetrics fontMetrics = g.getFontMetrics();
		
		// long distance sensors
		g.drawString(sensors.distanceSensors[LEFT], x + w / 4, y + h / 5);
		g.drawString(sensors.distanceSensors[RIGHT], 
				x + 3 * w / 4 - fontMetrics.stringWidth(sensors.distanceSensors[RIGHT]), y + h / 5);

		// ground sensors
		drawGroundSensor(g, sensors.groundSensorValues[LEFT  | FRONT], x, y, 
				sensors.groundSensorStates[LEFT  | FRONT]);
		drawGroundSensor(g, sensors.groundSensorValues[RIGHT | FRONT], x + w - size, y,
				sensors.groundSensorStates[RIGHT | FRONT]);
		drawGroundSensor(g, sensors.groundSensorValues[LEFT  | REAR ], x, y + h - size - dh,
				sensors.groundSensorStates[LEFT  | REAR ]);
		drawGroundSensor(g, sensors.groundSensorValues[RIGHT | REAR ], x + w - size, y + h - size - dh,
				sensors.groundSensorStates[RIGHT | REAR ]);
		
		// push sensor
		g.setColor(sensors.pushSensor.equals("0") ? Color.RED : Color.GREEN);
		g.fillRect(x, y + h - dh, w, dh);
		
		g.setColor(Color.BLACK);
		g.drawRect(x, y + h - dh, w, dh);
		
		drawSpeedometer(g, x, y);

		g.drawRect(x, y, w, h - dh);
		g.translate(-dx, -dy);
	}
	
	private void drawGroundSensor(Graphics g, String sensorValue, int x, int y, boolean state){

		FontMetrics fontMetrics = g.getFontMetrics();
		
		g.setColor(state ? Color.WHITE : Color.BLACK);
		g.fillRect(x, y, size, size);
		
		g.setColor(Color.BLACK);
		g.drawRect(x, y, size, size);
		
		int dx = (size - fontMetrics.stringWidth(sensorValue))/2;
		int dy = (size - fontMetrics.getHeight())/2 + fontMetrics.getHeight();
		
		g.setColor(state ? Color.BLACK : Color.WHITE);
		g.drawString(sensorValue, x + dx, y + dy);
	}
	
	private void drawSpeedometer(Graphics g, int x, int y){
		
		//g.setColor(Color.YELLOW);
		//g.drawOval(x, y, size, size);
		//g.drawArc(x, y, size, size, startAngle, arcAngle)
	}
}
