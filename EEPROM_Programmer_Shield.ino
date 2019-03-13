/*
 * This sketch allows you to program control instructions for Ben Eater's 8-bit Computer
 * using the Arduino shields developed by apexNSW.  
 * 
 * The code was developed by Ben Eater, with slight modification by apexNSW. If it doesn't work, 
 * don't blame Ben!
 * 
 * 
 */


#define SHIFT_DATA 4
#define SHIFT_CLK 3
#define SHIFT_LATCH 2
#define WRITE_ENABLE 13
#define EEPROM_D0 5
#define EEPROM_D1 6
#define EEPROM_D2 7
#define EEPROM_D3 8
#define EEPROM_D4 9
#define EEPROM_D5 10
#define EEPROM_D6 11
#define EEPROM_D7 12

#define HLT 0b1000000000000000
#define MI  0b0100000000000000
#define RI  0b0010000000000000
#define RO  0b0001000000000000
#define IO  0b0000100000000000
#define II  0b0000010000000000
#define AI  0b0000001000000000
#define AO  0b0000000100000000
#define EO  0b0000000010000000
#define SU  0b0000000001000000
#define BI  0b0000000000100000
#define OI  0b0000000000010000
#define CE  0b0000000000001000
#define CO  0b0000000000000100
#define J   0b0000000000000010
#define FI  0b0000000000000001

#define FLAGS_Z0C0 0
#define FLAGS_Z0C1 1
#define FLAGS_Z1C0 2
#define FLAGS_Z1C1 3

#define JC 0b0111
#define JZ 0b1000

const PROGMEM uint16_t UCODE_TEMPLATE[16][8] = {
      { MI|CO,  RO|II|CE,  RO|AI,  RI|EO|SU|FI,   0,   0,    0,    0 },              // 0000 - CTR
      { MI|CO,  RO|II|CE,  IO|MI,  RO|AI, 0,           0,    0,    0 },              // 0001 - LDA
      { MI|CO,  RO|II|CE,  IO|MI,  RO|BI, EO|AI|FI,    0,    0,    0 },              // 0010 - ADD
      { MI|CO,  RO|II|CE,  IO|MI,  RO|BI, EO|AI|SU|FI, 0,    0,    0 },              // 0011 - SUB
      { MI|CO,  RO|II|CE,  IO|MI,  AO|RI, 0,           0,    0,    0 },              // 0100 - STA
      { MI|CO,  RO|II|CE,  IO|AI,  0,     0,           0,    0,    0 },              // 0101 - LDI (Load Immediate - Load the value 0-15 directly into the A register ie. 01011111 - LDI 15)
      { MI|CO,  RO|II|CE,  IO|J,   0,     0,           0,    0,    0 },              // 0110 - JMP
      { MI|CO,  RO|II|CE,  0,      0,     0,           0,    0,    0 },              // 0111 - JC
      { MI|CO,  RO|II|CE,  0,      0,     0,           0,    0,    0 },              // 1000 - JZ
      { MI|CO,  RO|II|CE,  0,      0,     0,           0,    0,    0 },              // 1001 - NOP 
      { MI|CO,  RO|II|CE,  IO|BI,  0,     0,           0,    0,    0 },              // 1010 - LDB (Load Immediate B register)
      { MI|CO,  RO|II|CE,  RO|AI,  RI|EO, MI|IO,       RO|OI,0,    0 },              // 1011 - OTN (Out Next - display the value and increment the pointer)
      { MI|CO,  RO|II|CE,  0,      0,     0,           0,    0,    0 },              // 1100 - NOP
      { MI|CO,  RO|II|CE,  0,      0,     0,           0,    0,    0 },              // 1101 - NOP
      { MI|CO,  RO|II|CE,  AO|OI,  0,     0,           0,    0,    0 },              // 1110 - OUT
      { MI|CO,  RO|II|CE,  HLT,    0,     0,           0,    0,    0 },              // 1111 - HLT
  };

uint16_t ucode[4][16][8];

int DATAMODE = 0;//1 for read (0 for write)


void setup() {
  Serial.begin(9600);
  digitalWrite(SHIFT_DATA,LOW);
  digitalWrite(SHIFT_CLK,LOW);
  digitalWrite(SHIFT_LATCH,LOW);
  pinMode(SHIFT_DATA, OUTPUT);
  pinMode(SHIFT_CLK, OUTPUT);
  pinMode(SHIFT_LATCH, OUTPUT);
  digitalWrite(WRITE_ENABLE,HIGH);
  pinMode(WRITE_ENABLE,OUTPUT);
  dataPinsOff();
  pinsWrite(true);
  //initUCode();
  //progALL();
  //delay(500);
  readEEPROM(0);

}

void loop() {
  

}
void progALL() {
  Serial.println("Programming EEPROM");
  for(int address = 0; address < 1024; address += 1) {
      int flags        = (address & 0b1100000000) >> 8;
      int byte_sel     = (address & 0b0010000000) >> 7;
      int instruction  = (address & 0b0001111000) >> 3;
      int stp          = (address & 0b0000000111);  

      if (byte_sel) {
          writeData(ucode[flags][instruction][stp], address);    
      } else {
          writeData(ucode[flags][instruction][stp] >> 8, address);
      }
      if (address % 64 == 0) {
        Serial.print(".");
      }
      
  }
  Serial.println("Done");
}
void initUCode() {
  //ZF = 0, CF = 0
  memcpy_P(ucode[FLAGS_Z0C0],UCODE_TEMPLATE, sizeof(UCODE_TEMPLATE));
  //ZF = 0, CF = 1
  memcpy_P(ucode[FLAGS_Z0C1],UCODE_TEMPLATE, sizeof(UCODE_TEMPLATE));
  ucode[FLAGS_Z0C1][JC][2] = IO|J;
  //ZF = 1, CF = 0
  memcpy_P(ucode[FLAGS_Z1C0],UCODE_TEMPLATE, sizeof(UCODE_TEMPLATE));
  ucode[FLAGS_Z1C0][JZ][2] = IO|J;
  //ZF = 1, CF = 1
  memcpy_P(ucode[FLAGS_Z1C1],UCODE_TEMPLATE, sizeof(UCODE_TEMPLATE));
  ucode[FLAGS_Z1C1][JC][2] = IO|J;
  ucode[FLAGS_Z1C1][JZ][2] = IO|J;
}


void eraseEEPROM(bool val) {
  byte data;
  if (val) {
    data = 0xff;
  } else {
    data = 0x00;
  }
  for(int i = 0; i<2048; i++) {
    writeData(data,i);
  }
  Serial.println("EEPROM Erased.");
  
}

void set_register(word address, bool outputEnable) {
  byte left = highByte(address);
  byte right = lowByte(address);
  //Serial.println(left,BIN);
  //Serial.println(right,BIN);
  if (outputEnable) {
    bitClear(left,7);
  } else {
    bitSet(left,7);
  }
  //Serial.println(left,BIN);
  //Serial.println(right,BIN);
  shiftOut(SHIFT_DATA,SHIFT_CLK,LSBFIRST,right);
  shiftOut(SHIFT_DATA,SHIFT_CLK,LSBFIRST,left);
  latch();
}

void readEEPROM(int startaddress) {
  int endaddress = startaddress+255;
  for(int base = startaddress; base <= endaddress; base += 16) {
    byte data[16];
    for(int offset = 0; offset <=15; offset +=1) {
      data[offset] = readData(base+offset);
    }
    char buf[80];
    sprintf(buf,"%03x: %02x %02x %02x %02x %02x %02x %02x %02x  %02x %02x %02x %02x %02x %02x %02x %02x",
      base, data[0], data[1], data[2], data[3], data[4], data[5],
      data[6], data[7], data[8], data[9], data[10], data[11], data[12],
      data[13], data[14], data[15]
    );
    Serial.println(buf);
  }
}

byte readData(word address) {
  pinsRead();
  set_register(address,true);
  byte data = 0;
  int ct = 0;
  for(int pin=EEPROM_D0;pin<=EEPROM_D7;pin++) {
    bitWrite(data, ct, digitalRead(pin));
    ct++;
  }
  return data;
}
void writeData(byte data, word address) {
  pinsWrite();
  set_register(address,false);
  int ct = 0;
  for(int pin=EEPROM_D0;pin<=EEPROM_D7;pin++) {
     digitalWrite(pin,bitRead(data,ct));
     ct++;
  }
  writePulse();
}
void writePulse() {
  digitalWrite(WRITE_ENABLE,LOW);
  delayMicroseconds(1);
  digitalWrite(WRITE_ENABLE,HIGH);
  delay(10);
  dataPinsOff();
}
void dataPinsOff() {
  if (DATAMODE == 0) {
    for(int pin=EEPROM_D0;pin<=EEPROM_D7;pin++) {
      digitalWrite(pin,LOW);
    }
  }
}
void pinsRead(bool init) {
  int proceed = 0;
  if (init) {
    proceed = 1;
  } else {
    if (DATAMODE == 0) {
      proceed = 1;
    }
  }
  if (proceed) {
    for(int pin=EEPROM_D0;pin<=EEPROM_D7;pin++) {
      pinMode(pin,INPUT);
    }
    DATAMODE = 1;
  }
}
void pinsWrite(bool init) {
  int proceed = 0;
  if (init) {
    proceed = 1;
  } else {
    if (DATAMODE == 1) {
      proceed = 1;
    }
  }
  if (proceed) {
    for(int pin=EEPROM_D0;pin<=EEPROM_D7;pin++) {
      pinMode(pin,OUTPUT);
    }
    DATAMODE = 0;
  }
}
void pinsWrite() {
  pinsWrite(false);
}
void pinsRead() {
  pinsRead(false);
}
void latch() {
  digitalWrite(SHIFT_LATCH,LOW);
  digitalWrite(SHIFT_LATCH,HIGH);
  digitalWrite(SHIFT_LATCH,LOW);
}
