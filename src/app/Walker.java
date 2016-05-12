package app;

import java.util.ArrayList;
import java.util.List;

import lejos.hardware.lcd.LCD;
import lejos.hardware.motor.EV3LargeRegulatedMotor;
import lejos.hardware.port.MotorPort;
import lejos.hardware.sensor.EV3ColorSensor;
import lejos.hardware.sensor.EV3UltrasonicSensor;
import lejos.robotics.RegulatedMotor;

public class Walker extends Thread{
	public RegulatedMotor m1;
	public RegulatedMotor m2;
	public RegulatedMotor gripper;
	public int counter;
	public EV3ColorSensor[] colorSensors = new EV3ColorSensor[2] ;
	public EV3UltrasonicSensor sonar;
	public int colorSensor = 1;
	public boolean in_white=true;
	public static int limitBlack=10;
	public ArrayList<Instruction> instructions;
	public float[] fetched = new float[1];
	public boolean pickingStop=false;
	public boolean goBack=false;
	public boolean dropStarted=false;
	public LineFollower lineFollower;
	public Core core;
	public Walker(RegulatedMotor m1, RegulatedMotor m2, EV3ColorSensor colorSensor1, EV3ColorSensor colorSensor2, EV3UltrasonicSensor sonar, RegulatedMotor gripper,  ArrayList<Instruction> instructions, LineFollower lineFollower, Core core){
		this.m1=m1;
		this.m2=m2;
		this.colorSensors[0] = colorSensor1;
		this.colorSensors[1] = colorSensor2;
		this.sonar=sonar;
		this.gripper=gripper;
		this.instructions=instructions;
		this.lineFollower=lineFollower;
		this.core=core;
		
	}
	public void run(){
		while(true){
			if(!this.instructions.isEmpty()){
				Instruction i = instructions.get(0);
			    LCD.drawString(counter+"", 0, 0);
			    if(i.movement==Instruction.MOVEMENT.DROP){
			    	if(!dropStarted){
			    	m1.resetTachoCount();
			    	m1.setSpeed(100);
			    	m2.setSpeed(100);
			    	m1.rotate(250, true);
			    	m2.rotate(250, true);
			    	dropStarted=true;
			    	}else{
			    		if(m1.getTachoCount()==250 && !goBack){
			    			m1.stop(true);
					    	m2.stop(true);
					    	gripper.rotate(600);
					    	m1.rotate(-340, true);
					    	m2.rotate(-340, true);
					    	goBack=true;
			    		}
			    		if(goBack && m1.getTachoCount()==0){
			    			m1.stop(true);
			    			m2.stop(true);
			    			nextInstruction();
			    		}
			    	}
			    	
			    	/*
			    	gripper.rotate(600);
			    	m1.rotate(-340, true);
			    	m2.rotate(-340);
			    	*/
			    	
			    }
			    if(i.movement==Instruction.MOVEMENT.PICK){
			    	float[] sample = new float[1];
			    	sonar.fetchSample(sample, 0);
			    	if(pickingStop){
			    		if(goBack){
					    	float[] sampleColor = new float[1];	
					    	colorSensors[1].fetchSample(sampleColor, 0);
					    	if(sampleColor[0]>0.10f){
					    		m1.setSpeed(100);
					    		m2.setSpeed(100);
					    		m1.backward();
					    		m2.backward();
					    	}else{
					    		m1.stop(true);
					    		m2.stop(true);
					    		goBack=false;
					    		pickingStop=false;
					    		nextInstruction();
					    		
					    	}
			    		}else{
			    			gripper.rotate(-630);
			    			goBack=true;
			    		}
			    	}
			    	if(!goBack && !pickingStop){
				    	if(sample[0]<0.04f && pickingStop==false){
				    		m1.stop(true);
				    		m2.stop();
				    		pickingStop=true;
				    	}else{
				    		m1.setSpeed(100);
				    		m2.setSpeed(100);
				    		m1.forward();
				    		m2.forward();
				    	
				    	}
			    	}
					
			    }
				if(i.movement==Instruction.MOVEMENT.FORWARD){
		    		core.activateLineFollower();
					controlDots();
					m1.forward();
					m2.forward();
					if(i.duration==counter){
						core.disableLineFollower();
						nextInstruction();
					}
				}
				if(i.movement==Instruction.MOVEMENT.TURN){
					m1.stop(true);
					m2.stop(true);
					if(i.direction==Instruction.DIRECTION.RIGHT){
						/*
						if(lineFollower.colorSensor==0){
							this.colorSensor=0;
							lineFollower.colorSensor=1;
						}
						*/
						if(i.duration==1){
							m2.rotate(360);
							nextInstruction();
						}else{
							m1.rotate(-360);
							nextInstruction();
						}

					}
					if(i.direction==Instruction.DIRECTION.LEFT){
						
						if(i.duration==1){
							m1.rotate(360);
							nextInstruction();						
						}else{
							m2.rotate(-360);
							nextInstruction();
						}

					}
				}
			}else{
				m1.stop(true);
				m2.stop();
			}
			
		}
	}
	public void nextInstruction(){
		instructions.remove(0);
		counter=0;
	}
	public void controlDots(){
		   colorSensors[colorSensor].fetchSample(fetched, 0);
		   float value = fetched[0]*100;
		   if(value<=limitBlack && in_white){
				counter++;
				in_white=false;
			}
			if(value>limitBlack && !in_white){
				in_white=true;
			}	   
	
	}

}
