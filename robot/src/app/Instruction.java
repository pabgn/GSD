package app;

public class Instruction {
	public enum MOVEMENT {
	     FORWARD, BACKWARD, TURN, PICK, DROP
	}
	public enum DIRECTION {
	     RIGHT, LEFT
	}
	public MOVEMENT movement;
	public int duration;
	public DIRECTION direction;
	public Instruction(MOVEMENT movement, int duration){
		this.movement=movement;
		this.duration=duration;
	}

	public Instruction(MOVEMENT movement, int duration, DIRECTION direction){
		this.movement=movement;
		this.duration=duration;
		this.direction=direction;
	}
	
}
