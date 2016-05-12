package app;

import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.IOException;


import lejos.hardware.Bluetooth;
import lejos.hardware.lcd.LCD;
import lejos.remote.nxt.BTConnector;
import lejos.remote.nxt.NXTCommConnector;
import lejos.remote.nxt.NXTConnection;
import lejos.remote.nxt.RemoteNXT;

public class BluetoothManager extends Thread{
	public Bluetooth bt;
	public BTConnector btc;
	public NXTConnection connection;
	public Walker walker;
	public BluetoothManager(Walker walker){
		this.walker = walker;
	}
	public void Parse(String str){
		//Style: INSTRUCTIONS:FORWARD,2;TURN,1,LEFT
		String[] set = str.split(":");
		String key = set[0];
		String content = set[1];
		if(key.equals("INSTRUCTIONS")){
			String[] ins = content.split(";");
			for(String i:ins){
				String[] r = i.split(",");
				Instruction instruction;
				try{
				if(r.length==2){
					instruction = new Instruction(
							Instruction.MOVEMENT.valueOf(r[0]), 
							Integer.parseInt(r[1])
							);
				}else{
				 	instruction = new Instruction(
						Instruction.MOVEMENT.valueOf(r[0]), 
						Integer.parseInt(r[1]),
						Instruction.DIRECTION.valueOf(r[2]));
				}
				walker.instructions.add(instruction);

				}catch(Exception e){}
			}
		}
	}
	public void run(){
		
		NXTCommConnector cm = bt.getNXTCommConnector();
		connection = cm.waitForConnection(100, 0);
	    DataInputStream dis = connection.openDataInputStream();
	    DataOutputStream dou = connection.openDataOutputStream();

	    while(true){
	        try {
	        	if(dis.available()>0){
	        	byte[] b = new byte[200];
				dis.read(b);
				String n = new String(b);
				Parse(n);
				dou.write(b);
				dou.flush();
			    //LCD.drawString("Data: "+n, 0, 0);
	        	}
			} catch (IOException e) {
				System.out.println("ERROR");
				//e.printStackTrace();
			}

	    }
		
	}
}
