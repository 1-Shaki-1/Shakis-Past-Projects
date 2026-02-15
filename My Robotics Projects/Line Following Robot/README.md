			Line Following Robot

Overview:
This project implements a line-following robot using Arduino and a 3-sensor array. The robot:
* Follows a black line on a white surface using proportional (P) control.
* Supports calibration for black and white thresholds.
* Tracks laps and signals a win state after completing four laps.
* Includes LED feedback for different robot states (standby, moving, calibration, win).
Features:
- PID-style control: smooth adjustments based on sensor input.
-   Multiple robot states: START, IDLE, FORWARD, TURNING180, CALIBRATION, WIN.
-   LED indicators: glowing LED during standby, blinking during calibration, win signal.
-  Lap tracking: counts laps and signals completion.
-   Single-file Arduino code: compact, modular, and easy to run.
Tech Stack:
- Arduino Uno / compatible board
- Servo motors
- 3 opto sensors for line detection
- Arduino IDE (C++)
How it works:
1. START: robot initializes and moves to IDLE.
2. IDLE: motors stop, LED glows. Waits for start or calibration button press.
3. CALIBRATION: measures black and white thresholds for line sensors.
4. FORWARD: reads sensor values, calculates PID error, adjusts motors.
5. TURNING180: executes turn after lap completion.
6. WIN: signals completion with LED blinking after 4 laps.

Installation and usage:
1. Open line_following.ino in Arduino IDE.
2. Connect servos to pins 7 and 8, sensors to A0–A2, LED to pin 5, buttons to pins 4 and 6.
3. Upload code to Arduino board.
4. Press Calibrate to calibrate black/white thresholds.
5. Press Start to begin line-following operation.
Lessons Learned:
- Understanding thresholds is critical for reliable line following.
- PID-style control improves smoothness over simple on/off steering.
- Clear state management simplifies complex robot behavior.
- Timing and delay management prevents unexpected behavior during turns.
Future Improvements
- Add obstacle detection and avoidance.
- Implement full PID (P+I+D) for more precise control.
- Support multiple track types and intersections.
- Integrate with ROS or a simulator for testing without hardware.
- Add obstacle detection and avoidance.
- Implement full PID (P+I+D) for more precise control.
- Support multiple track types and intersections.
- Integrate with ROS or a simulator for testing without hardware.

