         /////////////////////////////////////////////  
        //  Iron Man Walkie-Talkie (Two-Way Radio) //
       //                w/ LoRa                  //
      //             ---------------             //
     //             (Arduino Nano)              //           
    //             by Kutluhan Aktar           // 
   //                                         //
  /////////////////////////////////////////////

//
// Via RYLR998 LoRa module and Arduino Nano, create a two-way radio (transceiver) w/ a 4x4 keypad to transmit and receive text messages.
//
// For more information:
// https://www.theamplituhedron.com/projects/Iron_Man_Walkie_Talkie_Two_Way_Radio_w_LoRa
//
//
// Connections
// Arduino Nano :  
//                                Nokia 5110 Screen
// D4  --------------------------- SCK (Clk)
// D7  --------------------------- MOSI (Din) 
// D8  --------------------------- DC 
// D9  --------------------------- RST
// D13 --------------------------- CS (CE)
//                                RYLR998 LoRa Module
// D2  --------------------------- TX
// D3  --------------------------- RX
//                                RGB LEB (RAGB)
// D5  --------------------------- R
// D6  --------------------------- G
// D10 --------------------------- B
//                                4x4 Keypad
// D11 --------------------------- R1
// D12 --------------------------- R2
// A0  --------------------------- R3
// A1  --------------------------- R4
// A2  --------------------------- C1
// A3  --------------------------- C2
// A4  --------------------------- C3
// A5  --------------------------- C4


// Include the required libraries.
#include <SoftwareSerial.h>
#include <Keypad.h>
#include <LCD5110_Graph.h>

SoftwareSerial LoRa(2, 3); // RX, TX

// Define the screen settings.
LCD5110 myGLCD(4,7,8,9,13);

extern uint8_t SmallFont[];
extern uint8_t MediumNumbers[];
// Define the graphics:
extern uint8_t iron_man[];

const byte ROWS = 4; // four rows
const byte COLS = 4; // four columns
// Define the symbols on the keypad buttons as letters (alphabet) and numbers:
char letters_1[ROWS][COLS] = {
  {'A','B','C','D'},
  {'E','F','G','H'},
  {'I','J','K','L'},
  {'#','-','*','+'}
};
char letters_2[ROWS][COLS] = {
  {'M','N','O','P'},
  {'Q','R','S','T'},
  {'U','V','W','X'},
  {'#','-','*','+'}
};
char numbers[ROWS][COLS] = {
  {'Y','Z','1','2'},
  {'3','4','5','6'},
  {'7','8','9','0'},
  {'#','-','*','+'}
};

// Connect to the row pinouts of the keypad:
byte rowPins[ROWS] = {11,12,A0,A1};
// Connect to the column pinouts of the keypad:
byte colPins[COLS] = {A2,A3,A4,A5};

// Initialize instances of different keypad classes:
Keypad k_letters_1 = Keypad( makeKeymap(letters_1), rowPins, colPins, ROWS, COLS); 
Keypad k_letters_2 = Keypad( makeKeymap(letters_2), rowPins, colPins, ROWS, COLS);
Keypad k_numbers = Keypad( makeKeymap(numbers), rowPins, colPins, ROWS, COLS);

// Define RGB pins:
#define redPin 5
#define greenPin 6
#define bluePin 10

// Define interface options:
volatile boolean msg_send, module_settings, last_msg, _sleep, activated;

// Define the data holders:
char key;
String commands = "", msg = "", MESSAGE = "", latest_message = "", received_msg = "";
int t = 150, selected = 0, keypad_number = 1, x = 0;

void setup(){
  Serial.begin(115200);
  while (!Serial) {
    ; // Wait for the serial port to connect. Needed for the Native USB only. 
  }
  
  // RGB:
  pinMode(redPin, OUTPUT);
  pinMode(greenPin, OUTPUT);
  pinMode(bluePin, OUTPUT);
  adjustColor(0,0,0);

  // Initiate the serial communication between the Arduino Nano and the RYLR998 LoRa module.
  LoRa.begin(115200);

    // Initiate the screen.
  myGLCD.InitLCD();
  myGLCD.setFont(SmallFont);
  
}

void loop(){
  read_keypad();
  change_menu_options();
  interface();
  get_serial_data();

  if(msg != ""){
    Serial.print(msg);
    if(msg.indexOf("RCV") > 0){
      myGLCD.clrScr();
      myGLCD.update();
      activated = true;
      // Elicit and format the received message:
      int delimiter_1, delimiter_2;
      delimiter_1 = msg.indexOf("%");
      delimiter_2 = msg.indexOf("%", delimiter_1 + 1);
      // Glean information as substrings.
      received_msg = msg.substring(delimiter_1 + 1, delimiter_2);
      adjustColor(0,255,0); delay(1000); adjustColor(0,0,0); delay(1000); adjustColor(0,255,0);
      while(activated == true){
        read_keypad();
        myGLCD.print("Received:", 0, 0);
        scrolling_text(received_msg, 16);
        myGLCD.update();
        // Exit.
        if(key && key == '-'){ activated = false; msg = ""; received_msg = ""; x = 0; myGLCD.clrScr(); myGLCD.update(); }  
      }
    }
    msg = "";
  }

  if(msg_send){
    do{
      myGLCD.invertText(true);
      myGLCD.print("A.Send Message", 0, 16);
      myGLCD.invertText(false);
      myGLCD.update();
      adjustColor(255, 0, 255);
      delay(100);
      if(key && key == '*'){
        myGLCD.clrScr();
        myGLCD.update();
        activated = true;
        while(activated == true){
          read_keypad();
          get_serial_data();
          myGLCD.print("Enter Message:", 0, 0);
          if(key){
            if(key == '*' && MESSAGE != ""){
              // Send the recently entered message to the given RYLR998 transceiver module.
              LoRa.print("AT+SEND=107,"+String(MESSAGE.length()+2)+",%"+MESSAGE+"%\r\n");
              delay(t);
              adjustColor(255, 255, 0); delay(1000); adjustColor(255, 0, 255);
            }else if(key == '+'){
              // Change keypad classes:
              keypad_number++;
              if(keypad_number > 3) keypad_number = 1;
            }else{  
              if(key != '#') MESSAGE += key;
              scrolling_text(MESSAGE, 16);
            } 
          }
          // Print the response from the RYLR998 module.
          if(msg != ""){ Serial.println(msg); msg = ""; }
          myGLCD.update();
          // Exit.
          if(key && key == '-'){ activated = false; MESSAGE = ""; x = 0; myGLCD.clrScr(); myGLCD.update(); }         
        }
      }
    }while(!msg_send);
  }

  if(module_settings){
    do{
      myGLCD.invertText(true);
      myGLCD.print("B.Settings", 0, 24);
      myGLCD.invertText(false);
      myGLCD.update();
      adjustColor(0, 0, 255);
      delay(100);
      if(key && key == '*'){
        myGLCD.clrScr();
        myGLCD.update();
        activated = true;
        while(activated == true){
          read_keypad();
          get_serial_data();
          myGLCD.print("Use the serial", 0, 0);
          myGLCD.print("monitor to", 0, 8);
          myGLCD.print("check and set", 0, 16);
          myGLCD.print("module", 0, 24);
          myGLCD.print("settings!", 0, 32);
          // Utilizing the serial monitor, check and set the RYLR998 module settings with AT commands. 
          if(commands == "check") check_AT_settings();
          if(commands == "set") set_AT_commands("108", "10");
          // Print the response from the RYLR998 module.
          if(msg != ""){ Serial.println(msg); msg = ""; }
          myGLCD.update();
          // Exit.
          if(key && key == '-'){ activated = false; commands = ""; msg = ""; myGLCD.clrScr(); myGLCD.update(); }         
        }
      }
    }while(!module_settings);
  }

  if(last_msg){
    do{
      myGLCD.invertText(true);
      myGLCD.print("C.Last Message", 0, 32);
      myGLCD.invertText(false);
      myGLCD.update();
      adjustColor(0, 255, 255);
      delay(100);
      if(key && key == '*'){
        myGLCD.clrScr();
        myGLCD.update();
        activated = true;
        while(activated == true){
          read_keypad();
          get_serial_data();
          myGLCD.print("Last Message:", 0, 0);
          // Obtain and display the latest transmitted message.
          if(key && key == '*') LoRa.print("AT+SEND?\r\n");
          if(msg != ""){
            // Print the response from the RYLR998 module.
            Serial.println(msg);
            latest_message = "";
            x = 0;
            if(msg.indexOf("%") > 0){
              int delimiter_1, delimiter_2;
              delimiter_1 = msg.indexOf("%");
              delimiter_2 = msg.indexOf("%", delimiter_1 + 1);
              // Glean information as substrings.
              latest_message = msg.substring(delimiter_1 + 1, delimiter_2);
            }else{
              latest_message = "Nothing Transmitted Yet!";
            }
            msg = "";
          }
          scrolling_text(latest_message, 16);
          myGLCD.update();
          // Exit.
          if(key && key == '-'){ activated = false; latest_message = ""; x = 0; myGLCD.clrScr(); myGLCD.update(); }         
        }
      }
    }while(!last_msg);
  }

  if(_sleep){
    do{
      myGLCD.invertText(true);
      myGLCD.print("D.Sleep", 0, 40);
      myGLCD.invertText(false);
      myGLCD.update();
      adjustColor(255, 0, 0);
      delay(100);
      if(key && key == '*'){
        myGLCD.clrScr();
        myGLCD.update();
        activated = true;
        while(activated == true){
          read_keypad();
          // Define and print monochrome images on the screen:
          myGLCD.drawBitmap(25,0,iron_man,36,50);
          myGLCD.update();
          // Exit.
          if(key && key == '-'){ activated = false; myGLCD.clrScr(); myGLCD.update(); }         
        }
      }
    }while(!_sleep);
  }

}

void interface(){
   // Define options.
   myGLCD.print("LoRa:", 0, 0);
   myGLCD.print("A.Send Message", 0, 16);
   myGLCD.print("B.Settings", 0, 24);
   myGLCD.print("C.Last Message", 0, 32);
   myGLCD.print("D.Sleep", 0, 40);
   myGLCD.update();
}

void change_menu_options(){
  // Increase or decrease the option number using the keypad buttons (+, -).
  if(key){
    if(key == '-') selected--; 
    if(key == '+') selected++;
  } 
  if(selected < 0) selected = 4;
  if(selected > 4) selected = 1;
  delay(100);

  // Depending on the selected option number, change the boolean status.
  switch(selected){
    case 1:
      msg_send = true; module_settings = false; last_msg = false; _sleep = false;
    break;
    case 2:     
      msg_send = false; module_settings = true; last_msg = false; _sleep = false;
    break;
    case 3:
      msg_send = false; module_settings = false; last_msg = true; _sleep = false;
    break;
    case 4:
      msg_send = false; module_settings = false; last_msg = false; _sleep = true;
    break;
  }
}

void read_keypad(){
  // Change keypad classes depending on the keypad number.
  if(keypad_number == 1) key = k_letters_1.getKey();
  if(keypad_number == 2) key = k_letters_2.getKey();
  if(keypad_number == 3) key = k_numbers.getKey();
}

void get_serial_data(){
  while(LoRa.available()){
    msg += (char)LoRa.read();
  }
  while(Serial.available()){
    commands += (char)Serial.read();
  }
}

void check_AT_settings(){
    commands = "";
    LoRa.print("AT\r\n");
    delay(t);
    LoRa.print("AT+ADDRESS?\r\n");
    delay(t);
    LoRa.print("AT+NETWORKID?\r\n");
    delay(t);
    LoRa.print("AT+BAND?\r\n");
    delay(t);
    LoRa.print("AT+PARAMETER?\r\n");
    delay(t);
}

void set_AT_commands(String address, String network_ID){
    commands = "";
    LoRa.print("AT\r\n");
    delay(t);
    LoRa.print("AT+ADDRESS=" + address + "\r\n");
    delay(t);
    LoRa.print("AT+NETWORKID="+network_ID+"\r\n");
    delay(t);
    LoRa.print("AT+BAND=470000000\r\n");
    delay(t);
    LoRa.print("AT+PARAMETER=10,8,1,12\r\n");
    delay(t);
}

void scrolling_text(String text, int y){
  int len = text.length();
  // Scroll the given text using the keypad button (#).
  if(key == '#') x--;
  if(x<=-(len*6)) x = -(len*6);
  // Print.
  myGLCD.print(text, x, y);
  delay(25);
}

void adjustColor(int r, int g, int b){
  analogWrite(redPin, (255-r));
  analogWrite(greenPin, (255-g));
  analogWrite(bluePin, (255-b));
}
