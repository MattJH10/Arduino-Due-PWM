uint32_t pwmPeriod = 40000000; // in hundredths of usecs (1e-8 secs) - this is 0.4 s
double pwmFreq = 1/(pwmPeriod*pow(10, -8));
double rcDouble = VARIANT_MCK / 2 / pwmFreq; // Works out the required rc to get the desired frequency with the chosen clock prescaling (here /2 - timer_clock1)
uint32_t rc = static_cast<unsigned int>(rcDouble + 0.5); // Add 0.5 to round correctly

double pwmDuty[12] = {}; // Array storage for pwm duty cycles 0-100%
double pwmDutyMin = 3; 

void TimerStart(Tc *tc, uint32_t channel, IRQn_Type irq, double dutyCycleA, double dutyCycleB) 
{
   pmc_set_writeprotect(false);
   pmc_enable_periph_clk(irq); 
   TC_Configure(tc, channel, TC_CMR_WAVE | TC_CMR_WAVSEL_UP_RC | TC_CMR_TCCLKS_TIMER_CLOCK1);
   uint32_t ra = static_cast<unsigned int>(rcDouble*dutyCycleA*0.01 + 0.5);
   uint32_t rb = static_cast<unsigned int>(rcDouble*dutyCycleB*0.01 + 0.5);
   TC_SetRA(tc, channel, ra); // Changes Duty Cycle
   TC_SetRB(tc, channel, rb);
   TC_SetRC(tc, channel, rc);
   TC_Start(tc, channel);
   tc->TC_CHANNEL[channel].TC_IER=  TC_IER_CPCS | TC_IER_CPAS | TC_IER_CPBS;
   tc->TC_CHANNEL[channel].TC_IDR=~(TC_IER_CPCS | TC_IER_CPAS | TC_IER_CPBS);
   NVIC_EnableIRQ(irq);
}

char inData[8]; // Input array for duty cycle data from serial
char inChar; // Temporary variable for serial read
int i = 0; // Command array index
int m;
int n;
float store = 0;

void setup() {

 Serial.begin(250000);
  while (!Serial) {
      ;
  }

  // output pins for valve PWM
  pinMode(2, OUTPUT); // TIOA0
  pinMode(3, OUTPUT); // TIOA7
  pinMode(4, OUTPUT); // TIOB6
  pinMode(5, OUTPUT); // TIOA6
  pinMode(10, OUTPUT); // TIOB7
  pinMode(11, OUTPUT); // TIOA8
  pinMode(12, OUTPUT); // TIOB8
  pinMode(13, OUTPUT); // TIOB0

  TimerStart(TC0, 0, TC0_IRQn, pwmDuty[0], pwmDuty[11]); // D2, D13  PWM duties assigned in increasing order of Due pin number
  TimerStart(TC2, 0, TC6_IRQn, pwmDuty[3], pwmDuty[2]); // D5, D4
  TimerStart(TC2, 1, TC7_IRQn, pwmDuty[1], pwmDuty[8]); //D3, D10
  TimerStart(TC2, 2, TC8_IRQn, pwmDuty[9], pwmDuty[10]); //D11,D12

  for(int n = 2; n < 6; n++)  // May not be necessary?
  {
    int ulPin = n;  
    PIO_Configure(g_APinDescription[ulPin].pPort,
    g_APinDescription[ulPin].ulPinType,
    g_APinDescription[ulPin].ulPin,
    g_APinDescription[ulPin].ulPinConfiguration); 
  }

  for(int n = 10; n < 14; n++)
  {
    int ulPin = n;  
    PIO_Configure(g_APinDescription[ulPin].pPort,
    g_APinDescription[ulPin].ulPinType,
    g_APinDescription[ulPin].ulPin,
    g_APinDescription[ulPin].ulPinConfiguration); 
  }

}

void changeDuty(int channel) 
{ 
  switch (channel) {
    case 0:
      TC_SetRA(TC0, 0, static_cast<unsigned int>(rcDouble*pwmDuty[0]*0.01 + 0.5));  // May have to reconfigure and restart counters to change RA/RB values?
      break;
    case 1:
      TC_SetRA(TC2, 1, static_cast<unsigned int>(rcDouble*pwmDuty[1]*0.01 + 0.5));
      break;
    case 2:
      TC_SetRB(TC2, 0, static_cast<unsigned int>(rcDouble*pwmDuty[2]*0.01 + 0.5));
      break;
    case 3:
      TC_SetRA(TC2, 0, static_cast<unsigned int>(rcDouble*pwmDuty[3]*0.01 + 0.5));
      break;
    case 8:
      TC_SetRB(TC2, 1, static_cast<unsigned int>(rcDouble*pwmDuty[8]*0.01 + 0.5));
      break;
    case 9:
      TC_SetRA(TC2, 2, static_cast<unsigned int>(rcDouble*pwmDuty[9]*0.01 + 0.5));
      break;
    case 10:
      TC_SetRB(TC2, 2, static_cast<unsigned int>(rcDouble*pwmDuty[10]*0.01 + 0.5)); 
      break;
    case 11:
      TC_SetRB(TC0, 0, static_cast<unsigned int>(rcDouble*pwmDuty[11]*0.01 + 0.5)); 
      break;
  }
}  
void loop() {
  if(Serial.available() > 0)
  {
    while(Serial.available() > 0)
    {
      inChar = Serial.read(); // Read a character
  
      if(inChar == ':')
      {
        break;
      }
  
      else
      {
        inData[i] = inChar; // Store it
        i = i + 1; // Increment where to write next
        inData[i] = '\0'; // Null terminate the string
      }
      delayMicroseconds(80);
    }
    i = 0;
  
    sscanf(inData, "%d %f", &m, &store); // Need to have the "&" for sscanf to work properly
    pwmDuty[m] = store;
    if(pwmDuty[m] < pwmDutyMin)
      {
        if(pwmDuty[m] < pwmDutyMin/2)
        {
          pwmDuty[m] = 0;
        }
        else 
        {
          pwmDuty[m] = pwmDutyMin;
        }
      }
        changeDuty(m);
  }
}
