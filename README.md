# TicTacToe-CC3200-AWS
Two-player Tic-Tac-Toe on the TI CC3200 LaunchPad with OLED display, IR remote input, shake-to-reset via accelerometer, and AWS IoT SNS notifications over Wi-Fi.

🎮 Two-Player Tic-Tac-Toe on CC3200 LaunchPad

A standalone two-player Tic-Tac-Toe game implemented on the Texas Instruments
CC3200 LaunchPad microcontroller. Players use an IR remote to place marks on a
3x3 grid displayed on a 128x128 OLED screen. Game results are transmitted over
Wi-Fi to AWS IoT Core, triggering an Amazon SNS mobile notification.

---

🛠️ Technologies

- TI CC3200 LaunchPad – Target microcontroller platform
- C – Firmware implementation
- SSD1351 OLED (128x128) – Game display via SPI
- IR Receiver – Player input via remote control
- BMA222 Accelerometer – Shake-to-reset via I2C
- AWS IoT Core – Cloud game result reporting via HTTP POST
- Amazon SNS – SMS notification on game completion
- Wi-Fi (CC3200 NWP) – Wireless connectivity
- Serial Flash – Tetherless boot storage

---

✨ Features

- Two-Player Gameplay: Players alternate placing X and O on a 3x3 grid
  using IR remote keys 1-9
- OLED Display: Full game board rendered on a 128x128 SSD1351 display
  over SPI with real-time mark updates and result screen
- Win/Draw Detection: Automatic evaluation of all win conditions and
  draw state after every move
- Shake to Reset: BMA222 accelerometer detects a shake gesture above
  a threshold and triggers an interrupt-driven board reset
- IR Reset: Dedicated IR remote button to reset the board at any time
- AWS IoT Integration: On game end the CC3200 sends an HTTP POST
  request with the result to AWS IoT Core
- SMS Notification: AWS IoT Rule triggers Amazon SNS to send an SMS
  to a registered phone number with the game result
- Tetherless Operation: System boots fully from serial flash with no
  USB or host connection required

---

🏗️ Architecture

The firmware is organized as a finite state machine with six states:

| State          | Description                                              |
|----------------|----------------------------------------------------------|
| INIT           | Initialize SPI, I2C, UART, GPIO and connect to Wi-Fi    |
| WAIT_FOR_INPUT | Poll IR receiver for player move (keys 1-9)              |
| UPDATE_GAME    | Validate move, update game array, render mark on OLED   |
| CHECK_STATUS   | Evaluate board for win or draw condition                 |
| AWS_NOTIFY     | Send HTTP POST with game result to AWS IoT Core          |
| RESET          | Clear board and return to INIT (shake or IR triggered)   |

Hardware Interfaces:
- SPI – OLED display (SSD1351)
- I2C – Accelerometer (BMA222)
- GPIO – IR receiver input
- UART – Debug output
- Wi-Fi NWP – AWS IoT HTTP POST

---

☁️ Cloud Architecture

Game Result → CC3200 HTTP POST → AWS IoT Core → IoT Rule → Amazon SNS → SMS

The CC3200 connects to AWS IoT Core over Wi-Fi and publishes the game
result as a JSON payload. An IoT Rule monitors the topic and forwards
the message to Amazon SNS, which sends an SMS notification to a
registered phone number.

---

🚀 How to Run

Requirements:
- TI CC3200 LaunchPad
- SSD1351 128x128 OLED display
- IR receiver and remote control
- BMA222 accelerometer (onboard)
- AWS account with IoT Core and SNS configured
- Code Composer Studio (CCS)

Steps:
1. Clone the repository and open the project in Code Composer Studio
2. Configure your Wi-Fi credentials and AWS IoT endpoint in the config file
3. Flash the firmware to the CC3200 via USB
4. Power the board — it boots from serial flash and connects to Wi-Fi
5. Use IR remote keys 1-9 to place marks on the grid
6. Shake the board or press the IR reset button to restart the game

---

💡 What I Learned

- Embedded FSM Design: Structuring firmware as a clean state machine
  across multiple hardware peripherals
- SPI Display Driving: Rendering graphics on the SSD1351 OLED by
  writing directly to the display controller over SPI
- I2C Peripheral Communication: Reading accelerometer data from the
  BMA222 and implementing interrupt-driven gesture detection
- IR Signal Decoding: Parsing IR remote signals from a GPIO receiver
  and mapping them to game inputs
- AWS IoT Integration: Connecting an embedded device to AWS IoT Core
  over Wi-Fi and triggering cloud actions via IoT Rules and SNS
- Tetherless Embedded Systems: Configuring serial flash boot for fully
  standalone operation without a host computer

---

👥 Contributors

Shayan Khosh Keifi — Firmware implementation, FSM design, OLED display
driver, IR remote input handling, accelerometer shake detection, AWS IoT
integration, SNS notification setup, testing and debugging

Awaab Mirghani — Hardware integration, peripheral wiring, testing and
verification, project documentation

Sebastian Bustamante — Hardware integration, peripheral wiring, testing
and verification, project documentation
