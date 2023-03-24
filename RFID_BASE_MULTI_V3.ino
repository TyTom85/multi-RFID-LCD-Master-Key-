/*
* Arduino RFID /I2C Display Project. 
* * Key Features: 
* *** 1) Allows RFID keys to be programmed using a master RFID Tag 
* *** 2) Stores the tag UID in EEPROM to support continued use between power cycles 
* *** 3) Matching I2C Displays for users to receive feedback from scans 
* *** 4) Expandable code, just copy/paste the functions and declarations NOTE: You will need to update variable names after copy/paste
*                
* by Tyler Dahl, Escape Room San Antonio, LLC , www.escaperoomsa.com
* 
* Resources:
*   Store Arrays to EEPROM: https://roboticsbackend.com/arduino-store-array-into-eeprom/#:~:text=1278%2034%20%2D9999-,Store%20long%20array%20into%20Arduino%20EEPROM,4%20bytes%20instead%20of%202.&text=As%20you%20can%20see%2C%20the,arrays%20is%20exactly%20the%20same.
*   Multi RFID:  https://community.element14.com/products/arduino/f/forum/28599/using-multiple-mfrc522-rfid-readers-on-one-arduino
*   Multi LCD displays: https://www.youtube.com/watch?v=Jj2KOFw12Kc
* 
*  * Typical pin layout used:
 * -----------------------------------------------------------------------------------------
 *             MFRC522      Arduino       Arduino   Arduino    Arduino          Arduino
 *             Reader/PCD   Uno/101       Mega      Nano v3    Leonardo/Micro   Pro Micro
 * Signal      Pin          Pin           Pin       Pin        Pin              Pin
 * -----------------------------------------------------------------------------------------
 * RST/Reset   RST          9             5         D9         RESET/ICSP-5     RST
 * SPI SS      SDA(SS)      10            53        D10        10               10
 * SPI MOSI    MOSI         11 / ICSP-4   51        D11        ICSP-4           16
 * SPI MISO    MISO         12 / ICSP-1   50        D12        ICSP-1           14
 * SPI SCK     SCK          13 / ICSP-3   52        D13        ICSP-3           15
 *
 * More pin layouts for other boards can be found here: https://github.com/miguelbalboa/rfid#pin-layout
*/

#include <SPI.h>
#include <MFRC522.h>
#include <LiquidCrystal_I2C.h>       
#include <EEPROM.h>
#define R1RST_PIN   9                    // Default RST PIN IS 9; Change to your configuration as needed
#define R2RST_PIN   3                    // Default RST PIN IS 9; Change to your configuration as needed
#define R1SS_PIN    10                   // Default SS PIN IS 10; Change to your configuration as needed
#define R2SS_PIN    8                   // Default SS PIN IS 10; Change to your configuration as needed

int globalMasterTag = 316368899; // !!!!!!! IMPORTANT !!!!!!!!!! MUST CHANGE this string variable to your specific master tag. This is hard coded in until later features are developed to update this master using the RFID Reader

//////////////// DECLARE READER ONE (R1) VARIABLES ////////////////////
bool rfid1= false;                          
const int R1_ARRAY_SIZE = 3;
const int R1_STARTING_EEPROM_ADDRESS = 17;
byte R1readCard[4];
int R1masterTag = globalMasterTag;          
int R1openSpot = 0;
int R1wantedPos;
int R1tags[R1_ARRAY_SIZE];                         // keyTag variable will be used to store the correct tag UID into EEPROM for retrieval after powercycle 
int R1tagID;
boolean R1successRead = false;
boolean R1correctTag = false;

//////////////// DECLARE READER ONE (R2) VARIABLES ////////////////////
bool rfid2 = false;
const int R2_ARRAY_SIZE = 3;
const int R2_STARTING_EEPROM_ADDRESS = 34;
byte R2readCard[4];
int R2masterTag = globalMasterTag;          
int R2openSpot = 0;
int R2wantedPos;
int R2tags[R2_ARRAY_SIZE];                         // keyTag variable will be used to store the correct tag UID into EEPROM for retrieval after powercycle 
int R2tagID;
boolean R2successRead = false;
boolean R2correctTag = false;

/////////////// CREATE READER ONE (R1) INSTANCES ///////////////////////
MFRC522 R1(R1SS_PIN, R1RST_PIN);
LiquidCrystal_I2C R1lcd(0x27, 16, 2);      // Default Address: 0 x 27 for PCF8574T chip; 0 x 3F for PCF8574AT chip; REMINDER: Adjust contrast screw. Defalut is set to 0 so it appears completely blank.

/////////////// CREATE READER TWO (R2) INSTANCES ///////////////////////
MFRC522 R2(R2SS_PIN, R2RST_PIN);
LiquidCrystal_I2C R2lcd(0x26, 16, 2);      // Default Address: 0 x 27 for PCF8574T chip; Adjust using MULTI LCD tutorial in notes above.  REMINDER: Adjust contrast screw. Defalut is set to 0 so it appears completely blank.

void setup() /////////////////////////// INITIALIZE AND DISPLAY DEFAULT LCD MESSAGE /////////////////////////////
{
///// RESET EEPROM HELPER ROUTINE. COMMENT IN/OUT AS NEEDED ///
//   for (int i = 0 ; i < EEPROM.length() ; i++) {           //
//      EEPROM.write(i, 0);  }                               //      
//      delay(100);                                          //                
///////////////////////////////////////////////////////////////
  
  /////////// INITIALIZE BOARD //////////////////
  Serial.begin(9600);
  SPI.begin();        // SPI bus
  
  //////////// INTIALIZE READER ONE (R1)  ////////////
  R1.PCD_Init(); 
  R1lcd.init();
  R1lcd.backlight();
  delay(100); 
  R1successRead = false;
  R1printNormalModeMessage();             // Calls function to display default LCD Message. Update language in the function to customize to your needs
  readIntArrayFromEEPROM(R1_STARTING_EEPROM_ADDRESS, R1tags, R1_ARRAY_SIZE);
  delay(100);
  Serial.println("R1 TAGS IN ARRAY: ");
  for ( uint8_t i = 0; i < R1_ARRAY_SIZE; i++)
      {                              // The MIFARE PICCs that we use have 4 byte UID
        Serial.println (R1tags[i]);
      }  
  //////////// INTIALIZE READER TWO (R2)  ////////////  
  R2.PCD_Init(); 
  R2lcd.init();
  R2lcd.backlight();
  delay(100); 
  R2successRead = false;
  R2printNormalModeMessage();             // Calls function to display default LCD Message. Update language in the function to customize to your needs
  readIntArrayFromEEPROM(R2_STARTING_EEPROM_ADDRESS, R2tags, R2_ARRAY_SIZE);
  delay(100);
  Serial.println("R2 TAGS IN ARRAY: ");
  for ( uint8_t i = 0; i < R2_ARRAY_SIZE; i++)
      {                              // The MIFARE PICCs that we use have 4 byte UID
        Serial.println (R2tags[i]);
      }    
}
void loop() ///////////////////////// RECOGNIZE, READ, AND USE TAG UID TO DO STUFF (SEE COMMENTS IN CODE) /////////////////////////////
{ 
    readIntArrayFromEEPROM(R1_STARTING_EEPROM_ADDRESS, R1tags, R1_ARRAY_SIZE);          // Recall R1 authorized tabs from EEPROM Memory
    readIntArrayFromEEPROM(R2_STARTING_EEPROM_ADDRESS, R2tags, R2_ARRAY_SIZE);          // Recall R1 authorized tabs from EEPROM Memory
//////////////////////////// HELPER ROUTINE TO BE CALLED LATER WHEN CONVERTING TAGS FROM BYTE TO INTEGER ////////////  
   union ByteArrayToInteger 
      {
        byte array[4];
        uint32_t integer;
      };
//////////////////////////// IF R2 TAG PRESENT, STORE THE TAG INTO A BYTE ARRAY, CALL FUNCTIONS TO DO STUFF /////////////////////////////
    if ((  R1.PICC_IsNewCardPresent()) && (  R1.PICC_ReadCardSerial()) ) 
      {                     //If a new PICC placed to RFID reader continue
        rfid1 = true;
        for ( uint8_t i = 0; i < 4; i++) 
          {                              // The MIFARE PICCs that we use have 4 byte UID
            R1readCard[i] = R1.uid.uidByte[i];                          // Adds the 4 bytes of data from tag into a byte array
          }                    
//////////////////////////// HELPER ROUTINE TO CONVERT BYTE ARRAY (CURRENT TAG) TO AN INTEGER (CURRENT TAG) ///////////////////
        ByteArrayToInteger converter1; //Create a converter
        for ( uint8_t i = 0; i < 4; i++)
          {  
            converter1.array[i] = R1readCard[i] ;
          }
           Serial.print("CONVERTER OUTPUT 1: ");
           Serial.println(converter1.integer); //Read the 32bit integer value.      
        R1tagID = converter1.integer; 
      }
      R1.PICC_HaltA();                                // Stop reading   
    if (R1tagID == R1masterTag)
      {
         updateKeys();
         rfid1 = false;
      }     
    if (rfid1)
      {
        R1_execute();
      }
//////////////////////////// IF R2 TAG PRESENT, STORE THE TAG INTO A BYTE ARRAY, CALL FUNCTIONS TO DO STUFF /////////////////////////////
    if ((  R2.PICC_IsNewCardPresent()) && (  R2.PICC_ReadCardSerial()) ) 
      {                     //If a new PICC placed to RFID reader continue
        rfid2 = true;
        for ( uint8_t i = 0; i < 4; i++) 
          {                              // The MIFARE PICCs that we use have 4 byte UID
            R2readCard[i] = R2.uid.uidByte[i];                          // Adds the 4 bytes of data from tag into a byte array
          } 
//////////////////////////// HELPER ROUTINE TO CONVERT BYTE ARRAY (CURRENT TAG) TO AN INTEGER (CURRENT TAG) ///////////////////
    ByteArrayToInteger converter2; //Create a converter
      for ( uint8_t i = 0; i < 4; i++)
        {  
          converter2.array[i] = R2readCard[i] ;
        }
   Serial.print("CONVERTER OUTPUT 2: ");
   Serial.println(converter2.integer); //Read the 32bit integer value.      
      R2tagID = converter2.integer; 
      }                                                    
    R2.PICC_HaltA();                                // Stop reading 
    if (R2tagID == R2masterTag)
      {
         updateKeys();
         rfid2 = false;
      }  
    if (rfid2)  
      {
      R2_execute();      
      }  
}
void R1_execute() /////////////////// R1 FUNCTION TO EVALUATE CURRENT TAG AS AUTHORIZED OR NOT, THEN DO STUFF OR DON'T ////////////
{
     Serial.print("R1 MASTER TAG: ");
     Serial.println(R1masterTag);                                                      
     R1correctTag = false;                                             // set correctTag variable to false for later use                      
     Serial.print("CURRENT R1 TAG (POST CONVERTER): ");
     Serial.println(R1tagID);
     Serial.println("R1 TAGS AUTHORIZED: ");
        for ( uint8_t i = 0; i < R1_ARRAY_SIZE; i++) 
        {                              // The MIFARE PICCs that we use have 4 byte UID
          Serial.println (R1tags[i]);
        }      
   ///////////////////////////// IF SCANNED TAG IS STORED IN MEMORY (AUTHORIZED) THEN DO STUFF /////////////////////////
    for (int i = 0; i < R1_ARRAY_SIZE; i++)
    {  
        if (R1tagID == R1tags[i])
        { 
          R1lcd.clear();
          R1lcd.setCursor(0, 0);
          R1lcd.print("Access Granted!");
          R1printNormalModeMessage();
          R1correctTag = true;    
        }
     }
    ///////////////////////////// IF SCANNED TAG IS NOT STORED IN MEMORY (UNAUTHORIZED) THEN DO STUFF /////////////////////////:  
    if (R1correctTag == false)
    {     
        R1lcd.clear();
        R1lcd.setCursor(0, 0);
        R1lcd.print(" Access Denied!");
        R1printNormalModeMessage();
    } 
    rfid1 = false;
}
void R2_execute() /////////////////// R2 FUNCTION TO EVALUATE CURRENT TAG AS AUTHORIZED OR NOT, THEN DO STUFF OR DON'T ////////////
{
     Serial.print("R2 MASTER TAG: ");
     Serial.println(R2masterTag);
     R2correctTag = false;                                             // set correctTag variable to false for later use      
     Serial.print("CURRENT R2 TAG (POST CONVERTER): ");
     Serial.println(R2tagID);         
     Serial.println("R2 TAGS AUTHORIZED:: ");
        for ( uint8_t i = 0; i < R2_ARRAY_SIZE; i++) 
        {                              // The MIFARE PICCs that we use have 4 byte UID
          Serial.println (R2tags[i]);
        }     
  ///////////////////////////// IF SCANNED TAG IS STORED IN MEMORY (AUTHORIZED) THEN DO STUFF /////////////////////////
    for (int i = 0; i < R2_ARRAY_SIZE; i++) {  
        if (R2tagID == R2tags[i]){ 
          R2lcd.clear();
          R2lcd.setCursor(0, 0);
          R2lcd.print("Access Granted!");
          R2printNormalModeMessage();
          R2correctTag = true;    
        }
      }
  ///////////////////////////// IF SCANNED TAG IS NOT STORED IN MEMORY (UNAUTHORIZED) THEN DO STUFF /////////////////////////:  
    if (R2correctTag == false){     
        R2lcd.clear();
        R2lcd.setCursor(0, 0);
        R2lcd.print(" Access Denied!");
        R2printNormalModeMessage();
      }
      rfid2 = false;
}
void updateKeys() /////////////////// SUB LOOP IF MASTER TAG IS SCANNED, THIS LOOKS FOR THE NEXT TAG TO BE READ  /////////////////////////////
{      
    if (R1tagID == R1masterTag)
    {
      R1lcd.clear();
      R1lcd.print("Program mode:");
      R1lcd.setCursor(0, 1);
      R1lcd.print("Add/Remove Tag");
      while (!R1successRead)
      {                                         // wait for next tag to be read by                             
        R1successRead = R1getID();                               
        if ( R1successRead == true)
        {                                  
   //////////////////////////// IF NEXT READ TAG IS ALREADY STORED IN EEPROM, CLEAR IT OUT FROM INT ARRAY //////////////////////////////////////
          for (int i = 0; i < R1_ARRAY_SIZE; i++){            
            if (R1tagID == R1tags[i])
            {                                      
              R1tags[i] = 0;    
              R1lcd.clear();
              R1lcd.setCursor(0, 0);
              R1lcd.print("  Tag Removed!");
              R1printNormalModeMessage();
              writeIntArrayIntoEEPROM(R1_STARTING_EEPROM_ADDRESS, R1tags, R1_ARRAY_SIZE);
              return;
            }     
          }
   /////////////////////////////// IF NEXT READ TAG IS NOT ALREADY STORED, THEN STORE IT AS THE TAG TO BE USED FOR GRANTING ACCESS /////////////               
          for (int i=0; i<R1_ARRAY_SIZE; i++) 
          {
             if (R1tags[i] == R1openSpot) 
             {
               R1wantedPos = i;
               break;
             }             
          }
          R1tags[R1wantedPos] = R1tagID;
          R1lcd.clear();
          R1lcd.setCursor(0, 0);
          R1lcd.print("   Tag Added!");
          R1printNormalModeMessage();
          writeIntArrayIntoEEPROM(R1_STARTING_EEPROM_ADDRESS, R1tags, R1_ARRAY_SIZE);
          return;
        }
      }
    }  
    R1successRead = false;                                        // Reset variable for next loop
   /////////////////////////////////// IF MASTER TAG 2 SUB LOOP, THIS LOOKS FOR THE NEXT TAG TO BE READ  /////////////////////////////      
    if (R2tagID == R2masterTag){
      R2lcd.clear();
      R2lcd.print("Program mode:");
      R2lcd.setCursor(0, 1);
      R2lcd.print("Add/Remove Tag");
      while (!R2successRead){                                         // wait for next tag to be read by                             
        R2successRead = R2getID();                               
        if ( R2successRead == true){                                  
   //////////////////////////// IF NEXT READ TAG IS ALREADY STORED IN EEPROM, CLEAR IT OUT FROM INT ARRAY //////////////////////////////////////
          for (int i = 0; i < R2_ARRAY_SIZE; i++)
          {            
            if (R2tagID == R2tags[i])
            {                                      
              R2tags[i] = 0;    
              R2lcd.clear();
              R2lcd.setCursor(0, 0);
              R2lcd.print("  Tag Removed!");
              R2printNormalModeMessage();
              writeIntArrayIntoEEPROM(R2_STARTING_EEPROM_ADDRESS, R2tags, R2_ARRAY_SIZE);
              return;
            }     
          }
   /////////////////////////////// IF NEXT READ TAG IS NOT ALREADY STORED, THEN STORE IT AS THE TAG TO BE USED FOR GRANTING ACCESS /////////////               
          for (int i=0; i<R2_ARRAY_SIZE; i++) 
          {
             if (R2tags[i] == R2openSpot) 
             {
               R2wantedPos = i;
               break;
             }             
          }
          R2tags[R2wantedPos] = R2tagID;
          R2lcd.clear();
          R2lcd.setCursor(0, 0);
          R2lcd.print("   Tag Added!");
          R2printNormalModeMessage();
          writeIntArrayIntoEEPROM(R2_STARTING_EEPROM_ADDRESS, R2tags, R2_ARRAY_SIZE);
          return;
        }
      }
    }    
    R2successRead = false;                                        // Reset variable for next loop
}
uint8_t R1getID() /////////////////////////// RECOGNIZE, READ, CONVERT, AND STORE RFID UID FOR USE BY 'UPDATE KEYS' FUNCTION  /////////////////////////////
{
  // Getting ready for Reading PICCs
  if (( ! R1.PICC_IsNewCardPresent()) || ( ! R1.PICC_ReadCardSerial())    )
    {              //If a new PICC placed to RFID reader continue
      return 0;
    }

  R1tagID = "";
  for ( uint8_t i = 0; i < 4; i++) {                     // The MIFARE PICCs that we use have 4 byte UID
      R1readCard[i] = R1.uid.uidByte[i];                 // Read tag and store each byte (decimal) into an array of 4 positions
    }    
  R1.PICC_HaltA();                                  // Stop reading
  //////////////////////////// HELPER ROUTINE TO CONVERT TAG ARRAY FROM BYTE TO INTEGER ///////////////////
    union ByteArrayToInteger {
      byte array[4];
      uint32_t integer;
      };
    ByteArrayToInteger converter; //Create a converter
      for ( uint8_t i = 0; i < 4; i++) {  
          converter.array[i] = R1readCard[i] ;
        }
    R1tagID = converter.integer;
  return 1;                                              // If UID is captured, return 1 (i.e. tell the main loop that there is a tag ready for analysis by the main code. 
}
uint8_t R2getID() /////////////////////////// RECOGNIZE, READ, CONVERT, AND STORE RFID UID FOR USE BY 'UPDATE KEYS' FUNCTION /////////////////////////////
{
  // Getting ready for Reading PICCs
  if (( ! R2.PICC_IsNewCardPresent()) ||  ( ! R2.PICC_ReadCardSerial()))   
    {              //If a new PICC placed to RFID reader continue
      return 0;
    }

  R2tagID = "";
  for ( uint8_t i = 0; i < 4; i++) {                     // The MIFARE PICCs that we use have 4 byte UID
      R2readCard[i] = R2.uid.uidByte[i];                 // Read tag and store each byte (decimal) into an array of 4 positions
    }    
  R2.PICC_HaltA();                                  // Stop reading
  //////////////////////////// HELPER ROUTINE TO CONVERT TAG ARRAY FROM BYTE TO INTEGER ///////////////////
    union ByteArrayToInteger {
      byte array[4];
      uint32_t integer;
      };
    ByteArrayToInteger converter; //Create a converter
      for ( uint8_t i = 0; i < 4; i++) {  
          converter.array[i] = R2readCard[i] ;
        }
    R2tagID = converter.integer;
  return 1;                                              // If UID is captured, return 1 (i.e. tell the main loop that there is a tag ready for analysis by the main code. 
}
void R1printNormalModeMessage() ///////////////////// DEFAULT LCD SCREEN BETWEN TAG READS  //////////////////////////////
{
  delay(1500);
  R1lcd.clear();
  R1lcd.print("Access Panel #1");
  R1lcd.setCursor(0, 1);
  R1lcd.print(" Scan Your Tag!");
}
void R2printNormalModeMessage() ///////////////////// DEFAULT LCD SCREEN BETWEN TAG READS  //////////////////////////////
{
  delay(1500);
  R2lcd.clear();
  R2lcd.print("Access Panel #2");
  R2lcd.setCursor(0, 1);
  R2lcd.print(" Scan Your Tag!");
}
void writeIntArrayIntoEEPROM(int address, int numbers[], int arraySize)  ///////////// WRITE INT TAG ARRAY TO EEPROM ///////////
{  
  int addressIndex = address;
  for (int i = 0; i < arraySize; i++) 
  {
    EEPROM.write(addressIndex, numbers[i] >> 8);
    EEPROM.write(addressIndex + 1, numbers[i] & 0xFF);
    addressIndex += 2;
  }
}
void readIntArrayFromEEPROM(int address, int numbers[], int arraySize)///////////// READ INT TAG ARRAY FROM EEPROM ///////////
{
  int addressIndex = address;
  for (int i = 0; i < arraySize; i++)
  {
    numbers[i] = (EEPROM.read(addressIndex) << 8) + EEPROM.read(addressIndex + 1);
    addressIndex += 2;
  }
}
