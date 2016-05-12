package app;

import java.util.ArrayList;

import lejos.hardware.Button;
import lejos.hardware.motor.EV3LargeRegulatedMotor;
import lejos.hardware.motor.EV3MediumRegulatedMotor;
import lejos.hardware.port.MotorPort;
import lejos.hardware.port.SensorPort;
import lejos.hardware.sensor.EV3ColorSensor;
import lejos.hardware.sensor.EV3UltrasonicSensor;
import lejos.robotics.RegulatedMotor;

public class Core extends Thread {
	public static LineFollower lineFollower;
	public static Walker walker;
	public static BluetoothManager bluetooth;
	public ArrayList<Instruction> instructions=new ArrayList<>();
	
	public void run(){
		//Motor instances //
				RegulatedMotor m1 = new EV3LargeRegulatedMotor(MotorPort.A);
				RegulatedMotor m2 = new EV3LargeRegulatedMotor(MotorPort.B);
				RegulatedMotor m3 = new EV3MediumRegulatedMotor(MotorPort.C);
				m3.rotate(-360*5);
				m3.rotate(360*4);
				//Sensor instances
				EV3ColorSensor colorSensor1 = new EV3ColorSensor(SensorPort.S1);
				colorSensor1.setCurrentMode("Red");
				
				EV3ColorSensor colorSensor2 = new EV3ColorSensor(SensorPort.S4);
				colorSensor2.setCurrentMode("Red");
				
				EV3UltrasonicSensor sonar = new EV3UltrasonicSensor(SensorPort.S3);
				
				//
				//bluetooth.test();
				//Algorithm instances
				lineFollower = new LineFollower(m1, m2, colorSensor1, colorSensor2, instructions);
				walker = new Walker(m1, m2, colorSensor1, colorSensor2, sonar, m3, instructions, lineFollower, this);
				//
				//Bluetooth manager
				bluetooth = new BluetoothManager(walker);
				bluetooth.start();
				//
				
				Instruction i1 = new Instruction(Instruction.MOVEMENT.FORWARD, 1);
				Instruction i2 = new Instruction(Instruction.MOVEMENT.TURN, 1, Instruction.DIRECTION.RIGHT);
				Instruction i3 = new Instruction(Instruction.MOVEMENT.PICK, 1);
				Instruction i4 = new Instruction(Instruction.MOVEMENT.TURN, 2, Instruction.DIRECTION.LEFT);
				Instruction i5 = new Instruction(Instruction.MOVEMENT.FORWARD, 2);
				Instruction i6 = new Instruction(Instruction.MOVEMENT.TURN, 1, Instruction.DIRECTION.RIGHT);
				Instruction i7 = new Instruction(Instruction.MOVEMENT.FORWARD, 1);
				Instruction i8 = new Instruction(Instruction.MOVEMENT.TURN, 1, Instruction.DIRECTION.RIGHT);
				Instruction i9 = new Instruction(Instruction.MOVEMENT.FORWARD, 1);
				Instruction i10 = new Instruction(Instruction.MOVEMENT.TURN, 1, Instruction.DIRECTION.RIGHT);
				Instruction i11 = new Instruction(Instruction.MOVEMENT.DROP, 1);
				
				walker.instructions.add(i1);
				walker.instructions.add(i2);
				walker.instructions.add(i3);
				walker.instructions.add(i4);
				walker.instructions.add(i5);
				walker.instructions.add(i6);
				
				walker.instructions.add(i7);
				walker.instructions.add(i8);
				walker.instructions.add(i9);
				walker.instructions.add(i10);
				walker.instructions.add(i11);

			
				
				walker.start();
				lineFollower.start();
				activateLineFollower();
				while(!Button.ESCAPE.isDown()){
					if(Button.ENTER.isDown()){
						if(!lineFollower.active){
							activateLineFollower();
							
						}
						
					}
					if(Button.DOWN.isDown()){
						lineFollower.active=false;
					}
					//Empty
				}
				
			}
			public void disableLineFollower(){
				lineFollower.active=false;
			}
			public static void activateLineFollower(){
				synchronized (lineFollower.lock) {
				    lineFollower.active=true;
				    lineFollower.lock.notifyAll();
				}
			}
	
}
