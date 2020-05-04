#include "esp_camera.h"
#include <WiFi.h>
#include <Servo.h>
// WARNING!!! Make sure that you have either selected ESP32 Wrover Module,
//            or another board which has PSRAM enabled

// Select camera model
//#define CAMERA_MODEL_WROVER_KIT
#define CAMERA_MODEL_ESP_EYE
//#define CAMERA_MODEL_M5STACK_PSRAM
//#define CAMERA_MODEL_M5STACK_WIDE
//#define CAMERA_MODEL_AI_THINKER

#include "camera_pins.h"

const char* ssid = "WIFI NAME";
const char* password = "WIFI PASSWORD";

Servo head1; // Horizontal Submotor Object
Servo head2; // Vertical Submotor Object
int headM1 = 5; // Horizontal Submotor pin
int headM2 = 6; // Vertical Submotor pin
int rips = 7; //rip sensor
int ultrasounds_triger = 9; //triger pin
int ultrasounds_echo = 10;  // echo pin
int redP = 1; //RGBLED red
int greenP = 2; //RGBLED greed
int blueP = 3; //RGBLED blue

void startCameraServer();

void setup() {
  //rip sensor
  pinMode(rips, INPUT);
  //submoter
  head1.attach(headM1);
  head2.attach(headM2);
  //ultrasounds
  pinMode(ultrasounds_triger, OUTPUT);
  pinMode(ultrasounds_echo, INPUT);
  digitalWrite(ultrasounds_triger, LOW);
  //led
  pinMode(redP, OUTPUT);
  pinMode(greenP, OUTPUT);
  pinMode(blueP, OUTPUT);

  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();

  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  //init with high specs to pre-allocate larger buffers
  if(psramFound()){
    config.frame_size = FRAMESIZE_UXGA;
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

#if defined(CAMERA_MODEL_ESP_EYE)
  pinMode(13, INPUT_PULLUP);
  pinMode(14, INPUT_PULLUP);
#endif
}

void loop() {
  int tmp = 0;
  if(check_rip()){
    RGBLED_ON(1,2,3);
    if(check_ultra()){
      head_go();
      camera_start();
      tmp = 1;
    }
    while(check_ultra()){
      delay(1000);
    }
    if(tmp == 1){
      head_back();
      tmp = 0;
    }
    RGBLED_OFF();
  }
}
//led on
void RGBLED_ON(int red, int green, int blue){
  analogWrite(redP, red);
  analogWrite(greenP, green);
  analogWrite(blueP, blue);
}
//led off
void RGBLED_OFF(){
  digitalWrite(redP, LOW);
  digitalWrite(greenP, LOW);
  digitalWrite(blueP, LOW);
}


//rip sensor Recognition
boolean check_rip(){
    int sensor = digitalRead(rips);
    if(sensor == HIGH){
      return true;
    }
    else return false;
}

//ultrasounds sensor
boolean check_ultra(){
  //measuring part
  digitalWrite(ultrasounds_triger, HIGH);
  delayMicroseconds(10);
  digitalWrite(ultrasounds_triger, LOW);
  //calculate part
  int width = pulseIn(ultrasounds_echo, HIGH);
  int distance = width/58; //cm
  //Human Position Estimate Point
  if(30 <= distance || distance <=100){
    return true;
  }
}

void bird_go(){
  //go
  digitalWrite(bird_move1, HIGH);
  digitalWrite(bird_move2, LOW);
  delay(3000);
  //stop
  digitalWrite(bird_move1, LOW);
}

void bird_back(){
  digitalWrite(bird_move1, LOW);
  digitalWrite(bird_move2, HIGH);
  delay(3000);
  //stop
  digitalWrite(bird_move2, LOW);
}


void camera_start(){
  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  sensor_t * s = esp_camera_sensor_get();
  //initial sensors are flipped vertically and colors are a bit saturated
  if (s->id.PID == OV3660_PID) {
    s->set_vflip(s, 1);//flip it back
    s->set_brightness(s, 1);//up the blightness just a bit
    s->set_saturation(s, -2);//lower the saturation
  }
  //drop down frame size for higher initial frame rate
  s->set_framesize(s, FRAMESIZE_QVGA);

#if defined(CAMERA_MODEL_M5STACK_WIDE)
  s->set_vflip(s, 1);
  s->set_hmirror(s, 1);
#endif

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");

  startCameraServer();

  Serial.print("Camera Ready! Use 'http://");
  Serial.print(WiFi.localIP());
  Serial.println("' to connect");
}
