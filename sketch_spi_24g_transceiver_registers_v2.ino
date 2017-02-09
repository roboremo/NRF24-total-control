// Access the registers of a SPI 2.4G transceiver using RoboRemo app.
// www.roboremo.com

// This is a very useful tool (together with a SDR)
// for understanding how a new transceiver module works


// HARDWARE SETUP:

// Arduino UNO is connected with USB OTG cable to Android tablet


// Transceiver connection:

// LCX24G  Arduino  Wire
//  pin      pin    color

//  IRQ       2     gray
//  MIS      12     orange
//  MOS      11     violet
//  SCK      13     yellow
//  CSN      10     green
//  CE        8     blue
//  VDD    3.3V     white
//  GND     GND     black


// NRF24L01 Arduino  Wire
//  pin      pin     color

//  IRQ      NC      ---
//  MIS       4      orange
//  MOS       5      gray
//  SCK       6      yellow
//  CSN       7      brown
//  CE        9      violet
//  VDD    3.3V      red
//  GND     GND      blue



//                       IRQ MIS MOS SCK CSN CE 3.3V GND
// RFM73 wire colors:    org yel grn blu vio gry wht blk


// pin variables:
int irq, miso, mosi, sck, csn, ce;


void usePins(uint8_t *s) {
  // s is a null-terminated char sequence
  // containing the pin numbers
  // example: "2 12 11 13 10 8"


  /*for(int i=0; s[i]!=0; i++) {
    Serial.print((char)(s[i]));
  }
  Serial.println("");*/
  
  uint8_t i = 0;
  uint8_t pinIndex = 0;

  uint8_t pin = 0;
  uint8_t c;

  while(1) {
    c = s[i++];
    
    if(c==0 || c==' ') { // current pin parsing complete
      switch(pinIndex) {
        case 0: irq = pin; break;
        case 1: miso = pin; break;
        case 2: mosi = pin; break;
        case 3: sck = pin; break;
        case 4: csn = pin; break;
        case 5: ce = pin; break;
        default:
          Serial.print("err. pinIndex out of bounds ");
          Serial.println(pinIndex);
      }
      pinIndex++;
      pin = 0; // reset parsed value to 0
    } else if(c>='0' && c<='9') { // if it is a digit
      pin = pin*10 + (c-'0'); // append digit to parsed value
      //Serial.print("pin = ");
      //Serial.println(pin);
    } else {
      Serial.print("err. expected digit, found ");
      Serial.println((char)c);
    }
    if(c==0) break;
  }

  if(pinIndex==6) { // 6 pins were set OK
    Serial.println("OK, using pins");
    Serial.println("IRQ MISO MOSI SCK CSN CE");
    Serial.print(irq); Serial.print(" ");
    Serial.print(miso); Serial.print(" ");
    Serial.print(mosi); Serial.print(" ");
    Serial.print(sck); Serial.print(" ");
    Serial.print(csn); Serial.print(" ");
    Serial.print(ce); Serial.println("");
  } else {
    Serial.print("err. expected 6 pins, found ");
    Serial.print(pinIndex);
  }
}


void spiInit() { // assuming pins already set with usePins()

  pinMode(ce, OUTPUT);
  digitalWrite(ce, LOW);
  
  pinMode(csn, OUTPUT);
  digitalWrite(csn, HIGH);
  
  pinMode(sck, OUTPUT);
  digitalWrite(sck, LOW);
    
  pinMode(miso, INPUT);
  
  pinMode(mosi, OUTPUT);
  digitalWrite(mosi, LOW);
    
  pinMode(irq, INPUT);
}


uint8_t spiTransfer(uint8_t data) { // software SPI
  uint8_t resData = 0;

  // reverse bit order:
  uint8_t invData = 0;
  for(int i=0; i<8; i++) {
    invData = invData << 1;
    invData += (data%2);
    data = data >> 1;
  }
  data = invData;

  // software SPI:
          
  for(int i=0; i<8; i++) { 
    if(data%2==1) {
      digitalWrite(mosi, HIGH);      
    } else {
      digitalWrite(mosi, LOW);
    }
    
    data = data >> 1;
              
    delayMicroseconds(1);

    digitalWrite(sck, HIGH);

    resData = resData << 1;
    resData = resData + digitalRead(miso);

    delayMicroseconds(1);

    digitalWrite(sck, LOW);
    
    delayMicroseconds(1);       
  }
          
  return resData;
}


// values to be OR-ed with the register address for read, write:
// these are same for all transceiver modules
#define regReadCmd 0x00
#define regWriteCmd 0x20


void readRegister(uint8_t reg, uint8_t *dest, uint8_t len) {
  digitalWrite(csn, LOW);
  spiTransfer(reg | regReadCmd);
  for(int i=0; i<len; i++) {
    dest[i] = spiTransfer(0x00);
  }
  digitalWrite(csn, HIGH);
}


void writeCommand(uint8_t cmd, uint8_t *data, uint8_t len) {
  digitalWrite(csn, LOW);
  spiTransfer(cmd);
  for(int i=0; i<len; i++) {
    spiTransfer(data[i]);
  }
  digitalWrite(csn, HIGH);
}


void writeRegister(uint8_t reg, uint8_t *data, uint8_t len) {
  writeCommand(reg | regWriteCmd, data, len);
}


void setBit(uint8_t reg, uint8_t b) {
  uint8_t val;
  readRegister(reg, &val, 1);
  val = val | (1 << b);
  writeRegister(reg, &val, 1);
}


void clearBit(uint8_t reg, uint8_t b) {
  uint8_t val;
  readRegister(reg, &val, 1);
  val = val & (~(1 << b));
  writeRegister(reg, &val, 1);
}

uint8_t readBit(uint8_t reg, uint8_t b) {
  uint8_t val;
  readRegister(reg, &val, 1);
  return (val >> b) % 2;
}

uint8_t rfm73SelectBank(uint8_t newBank) {
  uint8_t bank;
  readRegister(0x07, &bank, 1);
  bank = (bank & 0x80) >> 7;
  // now bank is the old bank
  bool sw = ((bank==0) && (newBank==1)) || ((bank==1) && (newBank==0));
  if(sw) {
    uint8_t a = 0x53;
    writeCommand(0x50, &a, 1); // command to switch bank
    // now check if it has switched:
    readRegister(0x07, &bank, 1);
    bank = (bank & 0x80) >> 7; // now is new bank
  }
  return bank;
}




String cmd; // stores the command received from RoboRemo


void setup() {
  Serial.begin(115200); 
  cmd = "";
}



/*
boolean cmdStartsWith(const char *st) { // checks if cmd starts with st
  for(int i=0; ; i++) {
    if(st[i]==0) return true;
    if(cmd[i]==0) return false;
    if(cmd[i]!=st[i]) return false;
  }
  return false;
}
*/


int hexCharToInt(char c) {
  if(c>='a') return (c-'a')+10;
  if(c>='A') return (c-'A')+10;
  return c-'0';
}

int hexStringToInt(String st) {
  int res = 0;
  int len = st.length();
  for(int i=0; i<len; i++) {
    char c = st.charAt(i);
    res = res * 16 + hexCharToInt(c);
  }
  return res;
}

String charToHexString(char c) {
  int val = (int)(c&0xFF);
  int a = (val/16)+'0';
  int b = (val%16)+'0';
  if(a>'9') a = ((a-'0')-10)+'A';
  if(b>'9') b = ((b-'0')-10)+'A';
  return (String)"" + (char)a + (char)b;
}

String toHexString(uint8_t* buf, int len) { // converts char array to hex string (ex: "jkl" -> "6A6B6C")
   
  String result = "";
  for(int i=0; i<len; i++) {
    result += charToHexString(buf[i]);
  }
  return result;
}





void exe(String st) { // executes the command
  
  // the st does not contain the ending '\n' or '\r'


  if( st.startsWith("ping") ) {
    Serial.println("hi :)");
    return;
  }

  if( st.startsWith("use pins ") ) {
    uint8_t charArrayLen = st.length()-9;
    uint8_t charArray[charArrayLen+1]; // null-terminated   
    st.substring(9).toCharArray((char*)charArray, charArrayLen+1);
    // +1 because it also adds the null-termination
    //charArray[charArrayLen] = 0; // already made by toCharArray()
    usePins(charArray);
  }

  if( st.startsWith("spi init") ) {
    spiInit();
    Serial.println("SPI init OK");
  }

  /*if( st.startsWith("sel ") ) {
    int n = st.charAt(4) - '0';
    selectTransceiver(n);
    Serial.print("selected transceiver ");
    Serial.print(n);
    if(n==1) {
      Serial.println(" (LCX24G)");
    } else if(n==2) {
      Serial.println(" (RFM73)");
    }
  }*/

  // read 02 10 -> read 2 bytes from address 0x10

  if( st.startsWith("read ") ) {
    int len = hexStringToInt(st.substring(5,7));
    int reg = hexStringToInt(st.substring(8,10));

    uint8_t buf[len];
    readRegister(reg, buf, len);

    String res = "reg.0x";
    res += st.substring(8,10);
    res += "[";
    int maxBit = len*8-1;
    if(maxBit<10) res += " ";
    res += maxBit;
    res += ":";
    res += "0] = ";

    res += toHexString(buf, len);
    res += "h";

    /*for(int i=0; i<len; i++) {
      res += " ";
      res += buf[i];
    }*/
    
    Serial.println(res);
    
    //Serial.println(bytesToRead);
    return;
  }

  // write 50 53   -> in reg.0x53 write 0x50 (1 Byte)
  // write 11 AABBCC -> write 3 Bytes

  if( st.startsWith("wr") ) { // "write " / "wrcmd "
    int reg = hexStringToInt(st.substring(6,8));
    int len = (st.length()-8)/2;

    uint8_t buf[len];
    for(int i=0; i<len; i++) {
      buf[i] = hexStringToInt(st.substring(9+2*i,11+2*i));
    }

    String res = "";

    if( st.startsWith("write ") ) {
      writeRegister(reg, buf, len);
      res = "reg.0x";
    }

    if( st.startsWith("wrcmd ") ) {
      writeCommand(reg, buf, len);
      res = "cmd.0x";
    }

    res += st.substring(6,8);
    res += "[";
    int maxBit = len*8-1;
    if(maxBit<10) res += " ";
    res += maxBit;
    res += ":";
    res += "0] <- ";

    res += toHexString(buf, len);
    res += "h";

    Serial.println(res);
    return;
  }


  if( st.startsWith("ce low") ) {
    digitalWrite(ce, LOW);
    Serial.println("CE set LOW");
  }

  if( st.startsWith("ce high") ) {
    digitalWrite(ce, HIGH);
    Serial.println("CE set HIGH");
  }

  if( st.startsWith("sb ") ) {  // sb 00 5 => set bit 5 in reg. 0x00
    
    int reg = hexStringToInt(st.substring(3,5));
    int b = st.charAt(6)-'0';
    setBit(reg, b);
    String res = "[0x";
    res += st.substring(3,5);
    res += "].bit";
    res += b;
    res += "<- 1";
    Serial.println(res);
  }

  if( st.startsWith("cb ") ) {  // cb 00 5 => clear bit 5 in reg. 0x00
    
    int reg = hexStringToInt(st.substring(3,5));
    int b = st.charAt(6)-'0';
    clearBit(reg, b);
    String res = "[0x";
    res += st.substring(3,5);
    res += "].bit";
    res += b;
    res += "<- 0";
    Serial.println(res);
  }

  if( st.startsWith("rb ") ) {  // rb 00 5 => read bit 5 from reg. 0x00
    
    int reg = hexStringToInt(st.substring(3,5));
    int b = st.charAt(6)-'0';
    int val = readBit(reg, b);
    String res = "[0x";
    res += st.substring(3,5);
    res += "].bit";
    res += b;
    res += "== ";
    res += val;
    Serial.println(res);
  }

  if( st.startsWith("bank 1") ) {
    int b = rfm73SelectBank(1);
    String res = "selected bank ";
    res += b;
    Serial.println(res);
  }

  if( st.startsWith("bank 0") ) {
    int b = rfm73SelectBank(0);
    String res = "selected bank ";
    res += b;
    Serial.println(res);
  }
  

  /*if( st.startsWith("fromhex ") ) {
    debug("result: " + fromHexString(st.substring(8)));
    return;
  }

  if( st.startsWith("tohex ") ) {
    debug("result: " + toHexString(st.substring(6)));
    return;
  }*/

  
}



void loop() {
  
  while(Serial.available()) {
    char c = (char)Serial.read(); // read char from RoboRemo app

    if(c=='\n' || c=='\r') { // if it is command ending
      exe(cmd);  // execute the command
      cmd = ""; // reset the cmd String
    } else {      
      cmd += c;
    }
  }
  
}

