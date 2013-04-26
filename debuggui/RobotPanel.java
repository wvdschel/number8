import java.awt.Color;
import java.awt.FontMetrics;
import java.awt.Graphics;

import javax.swing.JPanel;

public class RobotPanel extends JPanel {

	public String[] sensors;
	
	public RobotPanel(){}
	
	public void paintComponent(Graphics g){
		
		int x = 0;
		int y = 0;
		int w = 250;
		int h = 250;
		int dh = 30;
		int space = 5;
		
		int dx = (getWidth() - w)/2;
		int dy = (getHeight() - h)/2;
		g.translate(dx, dy);
		
		g.drawRect(x, y, w, h - dh);
		
		if(sensors != null){
			
			FontMetrics fontMetrics = g.getFontMetrics();
			
			// long distance sensors
			g.drawString(sensors[1], x + w / 4, y + h / 5);
			g.drawString(sensors[0], x + 3 * w / 4 - fontMetrics.stringWidth(sensors[0]), y + h / 5);
			
			// ground sensors
			g.drawString(sensors[4], x + space, y + fontMetrics.getHeight() + space);
			g.drawString(sensors[2], x - space + w - fontMetrics.stringWidth(sensors[2]), y + fontMetrics.getHeight() + space);
			g.drawString(sensors[3], x + space, y + h - dh - fontMetrics.getDescent() - space);
			g.drawString(sensors[5], x - space + w - fontMetrics.stringWidth(sensors[5]), y + h - dh - fontMetrics.getDescent() - space);
			
			// push sensor
			g.setColor(sensors[6].equals("0") ? Color.RED : Color.GREEN);
			g.fillRect(x, y + h - dh, w, dh);
			
			g.setColor(Color.BLACK);
			g.drawRect(x, y + h - dh, w, dh);
		}
		
		g.translate(-dx, -dy);
	}
}
