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
	
	public String[] sensors;
	
	public RobotPanel(){}
	
	public void paintComponent(Graphics g){
		
		int dx = (getWidth() - w)/2;
		int dy = (getHeight() - h)/2;
		g.translate(dx, dy);
		
		if(sensors != null){
			
			FontMetrics fontMetrics = g.getFontMetrics();
			
			// long distance sensors
			g.drawString(sensors[1], x + w / 4, y + h / 5);
			g.drawString(sensors[0], x + 3 * w / 4 - fontMetrics.stringWidth(sensors[0]), y + h / 5);

			// ground sensors
			drawGroundSensor(g, sensors[4], x, y);
			drawGroundSensor(g, sensors[2], x + w - size, y);
			drawGroundSensor(g, sensors[3], x, y + h - size - dh);
			drawGroundSensor(g, sensors[5], x + w - size, y + h - size - dh);
			
			// push sensor
			g.setColor(sensors[6].equals("0") ? Color.RED : Color.GREEN);
			g.fillRect(x, y + h - dh, w, dh);
			
			g.setColor(Color.BLACK);
			g.drawRect(x, y + h - dh, w, dh);
		}

		g.drawRect(x, y, w, h - dh);
		
		g.translate(-dx, -dy);
	}
	
	private void drawGroundSensor(Graphics g, String sensorValue, int x, int y){

		FontMetrics fontMetrics = g.getFontMetrics();
		int value = Integer.parseInt(sensorValue);
		
		g.setColor(value > 100 ? Color.BLACK : Color.WHITE);
		g.fillRect(x, y, size, size);
		
		g.setColor(Color.BLACK);
		g.drawRect(x, y, size, size);
		
		int dx = (size - fontMetrics.stringWidth(sensorValue))/2;
		int dy = (size - fontMetrics.getHeight())/2 + fontMetrics.getHeight();
		
		g.setColor(value > 100 ? Color.WHITE : Color.BLACK);
		g.drawString(sensorValue, x + dx, y + dy);
	}
}
