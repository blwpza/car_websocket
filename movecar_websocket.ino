#include <WiFi.h>
#include <WebSocketsServer.h>
#include "MotorControl.h"-

// สร้าง object สำหรับควบคุมมอเตอร์
MotorControl motorControl;

const char* ssid = "RobotWiFi";
const char* password = "robot1234";

IPAddress local_IP(172, 168, 10, 4);  // Set your static IP
IPAddress gateway(172, 168, 10, 1);     // Set your gateway
IPAddress subnet(255, 255, 255, 248);    // Subnet mask

// สร้าง WebSocket server บนพอร์ต 8080
WebSocketsServer webSocket = WebSocketsServer(8888);

// ตัวแปรสำหรับเก็บความเร็วของมอเตอร์
int currentSpeed = 100;  // เริ่มต้นที่ความเร็ว 100



void setup() {
  Serial.begin(115200);

  if (!WiFi.config(local_IP, gateway, subnet)) {
    Serial.println("STA Failed to configure");
  }

  // เชื่อมต่อกับ Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  // เริ่ม WebSocket server
  webSocket.begin();
  webSocket.onEvent(onWebSocketEvent);

  Serial.println(WiFi.localIP());

  // สร้าง instance ของ MotorControl
  motorControl = MotorControl();

  // หยุดมอเตอร์ในตอนเริ่มต้น
  stopMotors();
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    stopMotors();
    Serial.println("Stop");
    delay(1000);
  } else {
    webSocket.loop();
  }
}

// ฟังก์ชันหยุดมอเตอร์
void stopMotors() {
  motorControl.stop();
}

// Handle incoming WebSocket events
void onWebSocketEvent(uint8_t client_num, WStype_t type, uint8_t* payload, size_t length) {
  switch (type) {
    case WStype_DISCONNECTED:
      // Serial.printf("Client %u disconnected\n", client_num);
      stopMotors();  // หยุดมอเตอร์เมื่อ client ตัดการเชื่อมต่อ
      break;

    case WStype_CONNECTED:
      {
        stopMotors();
        IPAddress ip = webSocket.remoteIP(client_num);
        String speedStr = String(currentSpeed);

        webSocket.sendTXT(client_num, speedStr);
        break;
      }

    case WStype_BIN:
      {
        if (length > 0) {
          if (length == 1) {
            switch (payload[0]) {
              case 0x04:
                currentSpeed = min(currentSpeed + 100, 255);
                break;
              case 0x05:
                currentSpeed = max(currentSpeed - 100, 100);
                break;
                // case 0x06:
                //   break;
                // case 0x07:
                //   break;
                // case 0x08:
                //   break;
                // case 0x09:
                //   break;
                // case 0x10:
                //   break;
                // case 0x11:
                //   break;
                // case 0x12:
                //   break;
                // case 0x13:
                //   break;
                // case 0x14:
                //   break;
                // case 0x1:
                //   break;
              default:
                stopMotors();
                break;
            }
          } else if (length == 5) {
            // Handle joystick data
            uint8_t direction = payload[0];
            uint16_t joystickX = (payload[1] << 8) | payload[2];
            uint16_t joystickY = (payload[3] << 8) | payload[4];
            // Serial.printf("Joystick X: %d, Y: %d, Direction: %d\n", joystickX, joystickY, direction);

            // Existing motor control logic
            if (direction == 0x02) {
              if (joystickY >= 8) {
                motorControl.moveForward(currentSpeed);
                delay(50);
                // Serial.println("Motor moving forward");
              } else if (joystickY < 3) {
                motorControl.moveBackward(currentSpeed);
                delay(50);
                // Serial.println("Motor moving backward");
              } else {
                stopMotors();
                delay(50);
              }
            } else if (direction == 0x01) {
              if (joystickX >= 8) {
                motorControl.turnLeft(currentSpeed);
                delay(50);
                // Serial.println("Motor turning left");
              } else if (joystickX < 3) {
                motorControl.turnRight(currentSpeed);
                delay(50);
                // Serial.println("Motor turning right");
              } else {
                stopMotors();
                delay(50);
              }
            }
          }
        } else {
          stopMotors();  // Stop motors if no data
        }
      }
      break;
  }
}
