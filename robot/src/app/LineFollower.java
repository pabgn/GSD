package app;

import java.util.ArrayList;

import lejos.hardware.lcd.LCD;
import lejos.hardware.port.SensorPort;
import lejos.hardware.sensor.EV3ColorSensor;
import lejos.robotics.RegulatedMotor;

public class  LineFollower extends Thread{
	public boolean active=false;
	public final Object lock = new Object();

	public RegulatedMotor m1;
	public RegulatedMotor m2;	
	public EV3ColorSensor[]colorSensors = new EV3ColorSensor[2]; 
	public int colorSensor = 0;
	public ArrayList<Instruction> instructions;
	public LineFollower(RegulatedMotor m1, RegulatedMotor m2, EV3ColorSensor colorSensor1, EV3ColorSensor colorSensor2,  ArrayList<Instruction> instructions){
		this.m1=m1;
		this.m2=m2;
		this.colorSensors[0] = colorSensor1;
		this.colorSensors[1] = colorSensor2;
		this.instructions=instructions;
	}
	public void run(){
	
		int objetivo = 28;
		float kp = 1.2f;
		float ki = 0.020f;
		float kd = 10;
		int v1=0,v2=0;
		float error=0, error_acum=0,error_anterior=0;
		int v=200, w=0;
		float[] fetched = new float[1];

		while(true){
			synchronized (lock) {
			    while (!active) {
			        try {
						lock.wait();
						error=0;
						error_acum=0;
						error_anterior=0;
					} catch (InterruptedException e) {
					}
			    }
			}
		     colorSensors[colorSensor].fetchSample(fetched, 0);

		     
			 error = fetched[0]*100 - objetivo;
		     w = (int) ((error * kp) + (ki * error_acum) + (kd * (error - error_anterior)));
		     if(colorSensor==0){
			     v1 = v + w;
			     v2 = v - w;
		     }else{
			     v1 = v - w;
			     v2 = v + w;		    	 
		     }
		     if(v2>v+50 || v1>v+50){
		    	 error_acum=0;
		    	 v1=200;
		    	 v2=200;
		     }
		     m1.setSpeed(Math.abs(v1));
		     m2.setSpeed(Math.abs(v2));
		     error_acum = error_acum + error;
		    
		     error_anterior =  error;
		 
		}
	}
}
