 /*
 * firmware/controller application.
 *
 */

#include "dsp56f807.h"
#include "options.h"
#include "asc.h"
#include "ti1.h"
#include "pid.h"
#include "calibration.h"
#include "trajectory.h"
#include "can_interface.h"
#include "serial_interface.h"
#include "ad.h"
#include "can1.h" 
#include "leds_interface.h"
#include "currents_interface.h"
#include "flash_interface.h"
#include "pwm_interface.h"
#include "faults_interface.h"
#include "temp_sens_interface.h"
#include "encoders_interface.h"
#include "filters.h"
#include "decoupling.h"

//#include "encoders_interface.h"
//#include "abs_analog_interface.h"
#include "abs_ssi_interface.h"

#include "phase_hall_sens.h"
#include "brushless_comm.h"

#include "pwm_a.h"
#include "pwm_b.h"


byte	_board_ID = 16;	
char    _additional_info [32];
UInt8    mainLoopOVF=0;
word    _build_number = 42;
int     _countBoardStatus =0;
Int16   _flash_version=0; 
UInt8   BUS_OFF=false;


#ifdef TEMPERATURE_SENSOR
	//temperature sensor variables
	Int16    TempSens[]=	 {0,0};
	UInt8    overtemp[]={0,0};
	UInt8    errortemp[]={0,0};

#endif 

//**********************
// externs
//**********************
extern sDutyControlBL DutyCycle[2];
extern sDutyControlBL DutyCycleReq[2];

//********************
// Local prototypes 
//********************

void check_range        (byte, Int16, Int32 *);
void check_range_torque (byte, Int16, Int32 *);


#ifndef VERSION
#	error "No valid version specified"
#endif


#ifdef ENCODER_SHIFT_11BITS
	#define Filter_Bit(x) ((x>>1)<<1)
#else
	#define Filter_Bit(x) (x)
#endif

/***********************************************************************
	This is the main controller loop.
	sequences of operations:
		- reads from CAN bus or serial port 1.
		- reads encoders (or ADC).
		- computes the control value (a PID in this version).
		- checks limits and other errors.
		- sets PWM
		- does extra functions (e.g. communicate with neighboring cards).
 ***********************************************************************/
void main(void)
{

	Int32 PWMoutput [JN];
	word temporary;
	byte i=0;
	byte k=0;
	UInt16 *value=0;
	Int32 t1val=0;
	Int32 PID_R= 2;
	Int32 kpp=1;
	Int16 current_turn=0;
	Int16 print_number=0;
	Int16 real_pos=0;
	byte first_step=0;
	
	//velocity and acceleration estimation
	byte j=0;

	byte headPos=0; //current pos/vel
	byte tailPos=0; //tail is (head+1)%winSize
	byte headVel=0; //current pos/vel
	byte tailVel=0; //tail is (head+1)%winSize
	byte winSizePos=35;
	byte winSizeVel=55;
	//Int32 windSizePos=35;
	//Int32 windSizeVel=50;
	Int32 positionWindow[35][JN]; //max window size: 254
	Int32 velocityWindow[55][JN]; //max window size: 254
  	Int16 _safeband[JN];	//it is a value for reducing the JOINT limit of 2*_safeband [tick encoder]
#ifdef TEMPERATURE_SENSOR
	byte   TempSensCount1 = 0;
	UInt32 TempSensCount2 = 0;
	byte   temp_sens_status=0;
	overtemp[0]=0;
	overtemp[1]=0;
	errortemp[0]=0;
	errortemp[1]=0;

#endif
	
	/* gets the address of flash memory from the linker */
	_flash_addr = get_flash_addr();
		
	/* enable interrupts */
	setReg(SYS_CNTL, 0);
	
	// IPL channels from 0 to 6 enabled
	// external interrupts IRQA and IRQB disabled
	setRegBits(IPR, 0xFE00); 

	// enable FAULT
	__ENIGROUP (61, 3);
	__ENIGROUP (60, 3);
		
	// enable SCI
	__ENIGROUP (52, 4);
	__ENIGROUP (53, 4);
	__ENIGROUP (50, 4);
	__ENIGROUP (51, 4);
	
    // enable data flash
	__ENIGROUP (13, 4);
	
	// enable CAN	
	__ENIGROUP (14, 6);
	__ENIGROUP (15, 6);
	__ENIGROUP (16, 6);
	__ENIGROUP (17, 6);
	
	// enable ADCA/ADCB
	__ENIGROUP (55, 4);
	__ENIGROUP (54, 4);
	
	// enable timers
	// TIMER_A
	__ENIGROUP (45, 7); //HallX0
	__ENIGROUP (44, 7); //HallY0
	__ENIGROUP (43, 7); //HallZ0
	__ENIGROUP (42, 7); //TI1 1ms delay main loop
	// TIMER_B

	__ENIGROUP (41, 7); //HallX1
	__ENIGROUP (40, 7); //HallY1
	__ENIGROUP (39, 7); //HallZ1
	__ENIGROUP (38, 7);

	// TIMER_C
	__ENIGROUP (37, 1); 
	__ENIGROUP (36, 1);
	__ENIGROUP (35, 1);
	__ENIGROUP (34, 1);
	// TIMER_D
	__ENIGROUP (33, 7); //1ms delay duty cycle
	__ENIGROUP (32, 1);
	__ENIGROUP (31, 1);
	__ENIGROUP (30, 1);

	__EI();
	
	flash_interface_init  (JN);			
	readFromFlash (_flash_addr);  
	if (_version==_flash_version)
	{
		
	}
	else
	{
		writeToFlash(_flash_addr);
	}
	__DI();
   
	init_leds  			  ();
#if VERSION != 0x0155 	
	Init_Brushless_Comm	  (JN);
#else 
    Init_Brushless_Comm	  (1); //only one axis
#endif				


	serial_interface_init (JN);
	can_interface_init    (JN);
	
#ifdef TORQUE_CNTRL
    init_strain ();
#endif 	

    init_position_abs_ssi ();
#if VERSION == 0x0153 || VERSION ==0x0157 || VERSION == 0x0172 
    init_relative_position_abs_ssi();
#endif 
 
    init_faults           (true,true,true);	 
    
#if VERSION ==0x0155  
    init_position_encoder ();
#endif

    init_temp_sens        ();
	TI1_init 			  ();

//variable init	
	mainLoopOVF=0;
	_countBoardStatus=0;
	_count=0;
	
	for(j=0;j<JN;j++)
	{
	_received_pid[j].rec_pid=0;
	}
	
	BUS_OFF=false;
	__EI();
	  
//	print_version ();
	
	/* initialization */
	for (i=0; i<JN; i++) _calibrated[i] = false;
	
	/* reset trajectory generation */
	for (i=0; i<JN; i++) abort_trajectory (i, 0);
	
	
#if VERSION !=0x0155	
	///////////////////////////////////////
	// reset of the ABS_SSI
	// this is needed because the AS5045 gives the first value wrong !!!
    for (i=0; i<JN; i++)	_position[i]=(Int32) Filter_Bit(get_position_abs_ssi(i));
    for (i=0; i<JN; i++)    _max_real_position[i]=Filter_Bit(4095);
#else 
   	_position[0]=(Int32) Filter_Bit(get_position_abs_ssi(0));
    _max_real_position[0]=Filter_Bit(4095);
	
#endif//	AS1_printStringEx ("\r\n");
	
	
	
 	
	//////////////////////////////////////
	
	/* initialize speed and acceleration to zero (useful later on) */
	for (i=0; i<JN; i++) _position_old[i] = 0;
	for (i=0; i<JN; i++) _speed[i] = 0;
	for (i=0; i<JN; i++) _accel[i] = 0;
	for (i=0; i<JN; i++) _safeband[i] =-5; //5 ticks => 1 grado di AEA.
	
	/* reset the recursive windows for storage of position and velocity data */
	/* (for velocity and position estimates) */
	for(i=0;i<JN;i++)
	{
		for(j=0;j<winSizePos;j++)
		{
			positionWindow[j][i]=0;	
		}
		for(j=0;j<winSizeVel;j++)
		{
			velocityWindow[j][i]=0;	
		}	
	}
	
	//set_relative_position_abs_ssi(1,get_absolute_real_position_abs_ssi(1));
	/* main control loop */
	for(_counter = 0;; _counter ++) 
	{
		if (_counter >= CAN_SYNCHRO_STEPS) _counter = 0;
		led3_on
		while (_wait);
		_count=0;
		led3_off
		
// BUS_OFF check
		if (getCanBusOffstatus() )
		{
			#ifdef DEBUG_CAN_MSG
				can_printf("DIASBLE BUS OFF");
			#endif	
			for (i=0; i<JN; i++) _control_mode[i]=MODE_IDLE;
			led1_off
		}
		else
			led1_on

// READING CAN MESSAGES
		can_interface();
		
		
		// Strain Gauges sampling by internal ADC
#if	VERSION == 0x0171
//		_strain[0][5] = (read_strain(1,1)*3.03/*2*1.515*/); // 1.515=5/3.3
		_strain[0][5] = (read_strain(1,1)*3); // 1.515=5/3.3
		_strain[0][5]-=0x7FFF;
#endif
	
	    //Position calculation
	    // This is used to have a shift of the zero-cross out of the 
	    // joint workspace
	    //
	    // max_real_position is the limit of the joint starting from 
	    // 4095 and going to decrease this number without zero-cross
	    // untill the joint limit is reached
#if   VERSION == 0x0153 || VERSION == 0x0157 
		_position_old[0]=_position[0]; 
		if(get_error_abs_ssi(0)==ERR_OK)
			_position[0]=Filter_Bit (get_relative_position_abs_ssi(0));
		
		_position_old[1]=_position[1];
		if(get_error_abs_ssi(1)==ERR_OK) 
			_position[1]=Filter_Bit (get_position_abs_ssi(1));
#elif VERSION == 0x0172
		_position_old[0]=_position[0];
		if(get_error_abs_ssi(0)==ERR_OK) 
			_position[0]=Filter_Bit (get_relative_position_abs_ssi(0));
		
		_position_old[1]=_position[1];
		if(get_error_abs_ssi(1)==ERR_OK) 
			_position[1]=Filter_Bit (get_relative_position_abs_ssi(1));
#elif VERSION ==0x0155
		_position_old[0]=_position[0];
		_position[0]=Filter_Bit (get_position_abs_ssi(0));
		_position_old[1]=_position[1]; 
		_position[1]=get_position_encoder(1);
#else
	 	for (i=0; i<JN; i++) 
		{
		_position_old[i]=_position[i];
		if(get_error_abs_ssi(i)==ERR_OK)
			_position[i]=Filter_Bit (get_position_abs_ssi(i));
		}
#endif 

///////////////////////////////////////////DEBUG////////////
// ADDED VERSION !=0x0171
#if (VERSION !=0x0154) && (VERSION !=0x0155) && (VERSION !=0x0171)
	    for (i=0; i<JN; i++) 
		{		
		   if (get_error_abs_ssi(i)==ERR_ABS_SSI)
		   {
					_control_mode[i] = MODE_IDLE;	
					_pad_enabled[i] = false;
					PWM_outputPadDisable(i);
			#ifdef DEBUG_CAN_MSG
		    	can_printf("ABS error %d",i);	
			#endif
		   }	
					
		}  
#endif
	
#if (VERSION ==0x0154) || (VERSION ==0x0155)

		   if (get_error_abs_ssi(0)==ERR_ABS_SSI)
		   {
					_control_mode[0] = MODE_IDLE;	
					_pad_enabled[0] = false;
					PWM_outputPadDisable(0);
			#ifdef DEBUG_CAN_MSG
		    	can_printf("ABS error %d",0);	
			#endif
		   }	
					 
#endif	
		// decoupling the position	 	
		decouple_positions();
		
#ifdef DEBUG_TRAJECTORY
		if (_verbose && _counter == 0)
		{
			for (i=0; i<JN; i++)
			{
				AS1_printDWordAsCharsDec (_position[i]);
				AS1_printStringEx (" ");	
			}
			AS1_printStringEx ("\r\n");
		}
#endif

		//if (_counter == 0)
		//{	
		
		//tail= (tail+1)%winSize;
		tailPos=headPos+1; if(tailPos==winSizePos) tailPos=0;
		tailVel=headVel+1; if(tailVel==winSizeVel) tailVel=0;
		
		/* store the new position in the circular buffer */
		for (i=0; i<JN; i++) positionWindow[headPos][i]=_position[i];
				
		/* compute speed and acceleration, and store the speed in the circular buffer */		
		for (i=0; i<JN; i++)
		{
			velocityWindow[headVel][i]= (positionWindow[headPos][i] - positionWindow[tailPos][i] );/* /(windSize)*/
			_speed[i]= (Int16)(velocityWindow[headVel][i]);
			_accel[i]= (Int16)((velocityWindow[headVel][i] - velocityWindow[tailVel][i]));
		}

		
		/* advance in the circular buffer */
		//head= (head+1)%winSize;
		headPos=headPos+1; if(headPos==winSizePos) headPos=0;
		headVel=headVel+1; if(headVel==winSizeVel) headVel=0;
				
	
		//}
		
					
		/* in position? */
#if (VERSION != 0x0154) && (VERSION != 0x0155)
		for (i=0; i<JN; i++) _in_position[i] = check_in_position(i); 
#else
		_in_position[0] = check_in_position(0);
#endif
				
		/* in reference configuration for calibration? */
		//for (i=0; i<JN; i++) check_in_position_calib(i); 
	
//******************************************* POSITION LIMIT CHECK ***************************/

		for (i=0; i<JN; i++)  check_range(i, _safeband[i], PWMoutput);

//******************************************* COMPUTES CONTROLS *****************************/
		
		for (i=0; i<JN; i++) _bfc_PWMoutput[i] = PWMoutput[i] = compute_pwm(i);

//		decouple PWM	
		//decouple_dutycycle_new_joint(PWMoutput);   //new version (july 2010) with torque decupling
		decouple_dutycycle(PWMoutput);     //previous version

		/* PWM filtering */
		for (i=0; i<JN; i++) 
		{
			if (_control_mode[i] == MODE_TORQUE ||
			 	_control_mode[i] == MODE_IMPEDANCE_POS ||
			 	_control_mode[i] == MODE_IMPEDANCE_VEL)
				PWMoutput[i] = lpf_ord1_3hz (PWMoutput[i], i);	
		}

//******************************************* LIMIT CHECK FOR FORCE CONTORL******************/
		// Protection for joints out of the admissible range during force control
		for (i=0; i<JN; i++)  check_range_torque(i, _safeband[i], PWMoutput);
		
//******************************************* SATURATES CONTROLS ***************************/                
		/* saturates controls if necessary */
		for (i=0; i<JN; i++)
		{
			ENFORCE_LIMITS			(i,PWMoutput[i]);
			if      (_pid[i] < -MAX_DUTY) _pid[i]=-MAX_DUTY;
			else if (_pid[i] > MAX_DUTY)  _pid[i]= MAX_DUTY;
		}

		/* check temperature sensor */
#ifdef TEMPERATURE_SENSOR
		TempSensCount1++;
		TempSensCount2++;
		
		// communicate with temperature sensor every 5millisecs
		if (TempSensCount1==5)
		{
			//read temperature sensors
			temp_sens_status=MeasureTempSens();
			
			
			if (temp_sens_status==TEMPERATURE_STATUS_OK)
			{
				TempSens[0]=GetTempSens(0);			
				TempSens[1]=GetTempSens(1);	
						
				errortemp[0]=false;
				errortemp[1]=false;	
			}
			if (temp_sens_status== TEMPERATURE_STATUS_ERR)
			{
				TempSens[0]=0;			
				errortemp[0]=true;	
				TempSens[1]=0;			
				errortemp[1]=true;	
			}
			if (temp_sens_status== TEMPERATURE_STATUS_ERR1)
			{
				TempSens[0]=0;			
				errortemp[0]=true;	
			}
			if (temp_sens_status== TEMPERATURE_STATUS_ERR2)
			{			
				TempSens[1]=0;	
				errortemp[1]=true;	
			}	
			TempSensCount1=0;
		}

		// print this warning message every 30secs		
		if (TempSensCount2==30000)
		{
			for (i=0; i<JN; i++) 
			{
			
				
				if (TempSens[i] > 35) 
				{	
				#ifdef DEBUG_CAN_MSG
					can_printf("WARN: high temp ax%d: %d",i,TempSens[i]);
				#endif
				}
				if (TempSens[i] > 70)	
				{
				
				#ifdef DEBUG_CAN_MSG
					can_printf("WARN: VERY HIGH temp ax%d: %d",i,TempSens[i]);	
				#endif					
				}
			}
			TempSensCount2 = 0;		
		}

	//check for overtemperature
		for (i=0; i<JN; i++) 
		{
			overtemp[i]=false;
			if ((TempSens[i] > 75) && _pad_enabled[i] == true)
			{
				_control_mode[i] = MODE_IDLE;	
				_pad_enabled[i] = false;
				
				overtemp[i]=true;
			#ifdef DEBUG_CAN_MSG
			
				can_printf("WARN: VERY HIGH temp ax%d: %d! disabling pwm...",i,TempSens[i]);	
			
			#endif				
				PWM_outputPadDisable(i);				
			}
		}	
#endif
				
		/* generate PWM */		
		for (i=0; i<JN; i++)
		{
			if (_pad_enabled[i] == false) _control_mode[i] = MODE_IDLE;
			else	
			
				PWM_generate(i,_pid[i]);
		//		setReg (TMRD0_CNTR, 39998);              
			
		}
	
		/* Check Current done in T1 */

		/* do extra functions, communicate, etc. */
		can_send_broadcast();
	
		
//	Check for the MAIN LOOP duration
 
			
		t1val= (UInt16) TI1_getCounter(); 	
		if (	_count>0)
		{	
			mainLoopOVF=1;
			_count=0;
		}
//		can_printf("V");
//		can_print_dword(t1val);	
		
		/* tells that the control cycle is completed */
		
		_wait = true;		
		
	} /* end for(;;) */
}

/***************************************************************************/
/**
 * This function checks if the joint is in range during position/openloop control mode
 ***************************************************************************/
void check_range(byte i, Int16 band, Int32 *PWM)
{
	if (_control_mode[i] == MODE_POSITION)
	{
 		if  (_position[i] > (_max_position[i]-band) ||  (_position[i] < (_min_position[i]+band)))   
		{			
			_ko[i]=0;   //remove the PWM offset if it is out of limits 
			#ifdef DEBUG_CONTROL_MODE
			can_printf("OUT of LIMITS ax:%d", i);	
			#endif
			
			if (band>0)
			{
				if  ((_position[i] > (_max_position[i]-band)) && (_desired[i]>(_max_position[i]-band)))
				{
					_desired[i]=(_max_position[i]-band); 
					_integral[i] = 0; 
					_set_point[i] = _desired[i];
					init_trajectory (i, _desired[i], _desired[i], 1); 
				}
				
				if  ((_position[i] < (_min_position[i]+band)) && (_desired[i]<(_min_position[i]+band)))
				{
					_desired[i]=(_min_position[i]+band); 
					_integral[i] = 0; 
					_set_point[i] = _desired[i];
					init_trajectory (i, _desired[i], _desired[i], 1); 
				}
			}
			else	
			{
				if  ((_position[i] > (_max_position[i]+band)) && (_desired[i]>(_max_position[i]-band)))
				{
					#ifdef DEBUG_CONTROL_MODE
					can_printf("OUT of LIMITS MAX ax:%d", i);	
					#endif
					_desired[i]=_max_position[i]; 
					_integral[i] = 0; 
					_set_point[i] = _desired[i];
					init_trajectory (i, _desired[i], _desired[i], 1); 
				}
				
				if  ((_position[i] < (_min_position[i]-band)) && (_desired[i]<(_min_position[i]+band)))
				{
					#ifdef DEBUG_CONTROL_MODE
					can_printf("OUT of LIMITS MIN ax:%d", i);	
					#endif
					_desired[i]=_min_position[i]; 
					_integral[i] = 0; 
					_set_point[i] = _desired[i];
					init_trajectory (i, _desired[i], _desired[i], 1); 
				}
				
			}
		} 
	}
	else if (_control_mode[i] == MODE_OPENLOOP)
	{
		if  (_position[i] > (_max_position[i]-band) ||  (_position[i] < (_min_position[i]+band)))   
		{			
			_control_mode[i] = MODE_POSITION; //	
			_ko[i]=0;  //remove the PWM offset if it is out of limits
			if (band>0)
			{
				if  ((_position[i] > (_max_position[i]-band)) && (_desired[i]>(_max_position[i]-band)))
				{
					_desired[i]=(_max_position[i]-band); 
					_integral[i] = 0; 
					_set_point[i] = _desired[i];
					init_trajectory (i, _desired[i], _desired[i], 1); 
				}
				
				if  ((_position[i] < (_min_position[i]+band)) && (_desired[i]<(_min_position[i]+band)))
				{
					_desired[i]=(_min_position[i]+band); 
					_integral[i] = 0; 
					_set_point[i] = _desired[i];
					init_trajectory (i, _desired[i], _desired[i], 1); 
				}
			}
			else	
			{
				if  ((_position[i] > (_max_position[i]+band)) && (_desired[i]>(_max_position[i]-band)))
				{
					#ifdef DEBUG_CONTROL_MODE
					can_printf("OUT of LIMITS MAX ax:%d", i);	
					#endif
					_desired[i]=_max_position[i]; 
					_integral[i] = 0; 
					_set_point[i] = _desired[i];
					init_trajectory (i, _desired[i], _desired[i], 1); 
				}
				
				if  ((_position[i] < (_min_position[i]-band)) && (_desired[i]<(_min_position[i]+band)))
				{
					#ifdef DEBUG_CONTROL_MODE
					can_printf("OUT of LIMITS MIN ax:%d", i);	
					#endif
					_desired[i]=_min_position[i]; 
					_integral[i] = 0; 
					_set_point[i] = _desired[i];
					init_trajectory (i, _desired[i], _desired[i], 1); 
				}
			}
			#ifdef DEBUG_CONTROL_MODE
			can_printf("MODE CHANGED TO POSITION, OUT of LIMITS ax:%d", i);	
		    #endif 
		} 				
	}
}


/***************************************************************************/
/**
 * This function checks if the joint is in range during torque control mode
 ***************************************************************************/
void check_range_torque(byte i, Int16 band, Int32 *PWM)
{
	static UInt32 TrqLimitCount =0;
 	if (_control_mode[i] == MODE_TORQUE ||
	  	_control_mode[i] == MODE_IMPEDANCE_POS ||
	  	_control_mode[i] == MODE_IMPEDANCE_VEL)
 		{
	 		if  (_position[i] > _max_position[i])
	 		{
	 			if (_speed[i]>=0)
				{
					PWM[i] = 0;	
				}
				TrqLimitCount++;	 
				if (TrqLimitCount>=500)
				{
					#ifdef DEBUG_CONTROL_MODE
						can_printf("MODE TORQUE OUT LIMITS ax:%d", i);	
						TrqLimitCount=0;
				    #endif 
				}			
	 		}
	 		if  (_position[i] < _min_position[i])   
	 		{
	 			if (_speed[i]<=0)
				{
					PWM[i] = 0;			
				}				
				TrqLimitCount++;
				if (TrqLimitCount>=500)
				{
					#ifdef DEBUG_CONTROL_MODE
						can_printf("MODE TORQUE OUT LIMITS ax:%d", i);	
						TrqLimitCount=0;
				    #endif 
				}
	 		} 				
 		}
}
