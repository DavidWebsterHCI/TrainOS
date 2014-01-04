
#include <kernel.h>

/************************************************
*Author: David Webster				*
*file: train.c					*	
*student #: 912209914				*
*Project: Train.c assignment			*
*Date: 12/5/2013				*
*Class: CSc 720					*
*************************************************/

#define MAX_TRAIN_SPEED 9
char buffer [10]; 
COM_Message com_msg;
WINDOW* train_wnd;
int zamboni_present = 0;

void zamboni_check()
{
	if(wait_for_contact(4) == -1)
		zamboni_present = 0;
	else
		zamboni_present = 1;
}

void train_process(PROCESS self, PARAM param)
{
	init_track_switches(0);
	zamboni_check();
	determin_config();	

	remove_ready_queue(self);
	resign();
}

void determin_config()
{
	if(probe_command(8) && probe_command(2))
	{
		//Decide between config 1 and 2, as the red and yellow cart are in 
		//position, just matters which way the zamboni is going.
		if(!zamboni_present){
			wprintf(train_wnd, "Solving config. 1/2... please wait!\n");
			solve_config_1();
		}	
		else if(wait_for_contact(7) < 9){
			wprintf(train_wnd, "Solving config. 1... please wait!\n");
			solve_config_1();
		}
		else{
			wprintf(train_wnd, "Solving config. 2... please wait!\n");
			solve_config_2();
		}
	}
	else if(probe_command(11)){
		wprintf(train_wnd, "Solving config. 3... please wait!\n");
		solve_config_3();
	}
	else if(probe_command(16)){
		wprintf(train_wnd, "Solving config. 4... please wait!\n");
		solve_config_4();
	}
}

int solve_config_4()
{
	init_track_switches(4);

	//wait for zambini to get to small loop before starting red car
	if(zamboni_present) wait_for_contact(15);
	start_command(MAX_TRAIN_SPEED);

	//wait until red car has backed up, change dirrection and head toward yellow cart
	wait_for_contact(6);
	change_dir_command();
	start_command(MAX_TRAIN_SPEED);

	//wait until red car can back into track 16
	wait_for_contact(14);
	change_dir_command();
	start_command(MAX_TRAIN_SPEED);

	//wait for a car to leave a track to control red to yellow connection more accurately.
	wait_for_non_contact(14);
	start_command(4);
	sleep(150);

	//red car has yellow car attached so start returning home
	change_dir_command();

	//Wait for zamboni to go past
	if(zamboni_present) wait_for_contact(10);
	start_command(MAX_TRAIN_SPEED);

	//wait until zambini has passed switches required to return home
	if(zamboni_present) wait_for_contact(4);
		
	//Trap zamboni & flip switches for red car to return home
	if(zamboni_present) flipswitch_command(8, 'R');
	flipswitch_command(4, 'R');
	flipswitch_command(3, 'R');
	
	//When red car is right outside of home slow down to enter home track
	wait_for_contact(6);
	start_command(4);
	
	//Wait until red car is home.
	wait_for_contact(5);
	stop_command();

	//Success!
	wprintf(train_wnd, "Success!");
	return 0;
}
int solve_config_3()
{
	init_track_switches(3);

	//Wait until zamboni goes past
	if(zamboni_present) wait_for_contact(7);
	start_command(MAX_TRAIN_SPEED);
	
	//wait until zamboni is out of small loop
	if(zamboni_present) wait_for_contact(3);
	else wait_for_contact(13);

	change_dir_command();
	
	//prepare the path for red car to meet yellow car
	flipswitch_command(3, 'R');
	flipswitch_command(4, 'R');
	flipswitch_command(2, 'G');
	flipswitch_command(1, 'R');
	start_command(MAX_TRAIN_SPEED);
	
	//red car has yellow car, so start heading out of the small circle toward home
	wait_for_contact(15);
	change_dir_command();
	start_command(MAX_TRAIN_SPEED);

	//trap zamboni in small circle when red/yellow car are out of the small circle
	wait_for_contact(2);
	if(zamboni_present) flipswitch_command(1, 'R');
	if(zamboni_present) flipswitch_command(2, 'R');
	if(zamboni_present) flipswitch_command(7, 'R');

	//wait for red/yellow to be outside home, so reverse and back into home.
	wait_for_contact(6);
	change_dir_command();
	start_command(4);
	
	//Wait until red car is home.
	wait_for_contact(5);
	stop_command();

	//Success
	wprintf(train_wnd, "Success!");
	return 0;
}

int solve_config_2()
{
	init_track_switches(2);
	
	//wait for zamboni to be trapped in small circle
	if(zamboni_present) wait_for_contact(4);
	flipswitch_command(4, 'R');	
	flipswitch_command(3, 'G');

	//Zamboni is trapped, so go fetch yellow car and return home
	start_command(MAX_TRAIN_SPEED);
	
	wait_for_contact(1);

	change_dir_command();
	flipswitch_command(5, 'R');	
	flipswitch_command(6, 'R');
	start_command(MAX_TRAIN_SPEED);

	//Returned home.
	wait_for_contact(8);
	change_dir_command();

	//Success!
	wprintf(train_wnd, "Success!");
	return 0;
}

int solve_config_1()
{
	//Trap zamboni
	init_track_switches(1);
	
	//Wait until zamboni is trapped
	if(zamboni_present)wait_for_contact(12);

	//Go get carriage
	flipswitch_command(4, 'R');	
	flipswitch_command(3, 'G');
	start_command(MAX_TRAIN_SPEED);
	
	//wait until red car and yellow cart hook
	wait_for_contact(1);

	//Change directions and return to home base
	change_dir_command();
	sleep(15);
	flipswitch_command(5, 'R');	
	sleep(15);
	flipswitch_command(6, 'R');
	start_command(MAX_TRAIN_SPEED);

	//wait until red car and yellow cart hook
	wait_for_contact(8);
	
	//stop train and print success message.
	change_dir_command();

	wprintf(train_wnd, "Success!");
	return 0;
}

int wait_for_contact(int num)
{	
	int found_car = 0;
	int loops = 0;
	while(!found_car){
		
		//to see if zamboni is present or not
		if(loops >35){
			return -1;
		}

		loops++;
		found_car = probe_command(num);
	}
	return loops;
}

void wait_for_non_contact(int num)
{
	int found_car = 1;
	while(found_car){
		found_car = probe_command(num);
	}
}

void init_track_switches(int config)
{
	switch(config)
	{
		case 1:
			//flipswitch_command(5, 'R');
			//flipswitch_command(6, 'G');
flipswitch_command(9, 'R');
flipswitch_command(1, 'R');
flipswitch_command(2, 'R');
flipswitch_command(7, 'R');
			flipswitch_command(8, 'R');
			break;
		case 2:
			flipswitch_command(8, 'R');
			break;
		case 3:
			flipswitch_command(8, 'R');	
			flipswitch_command(9, 'R');
			flipswitch_command(1, 'G');
			flipswitch_command(5, 'G');
			break;
		case 4:
			flipswitch_command(8, 'G');
			flipswitch_command(4, 'G');
			flipswitch_command(9, 'G');
			break;
		default:
			flipswitch_command(8,'G');
			flipswitch_command(9,'R');
			flipswitch_command(1,'G');
			flipswitch_command(4,'G');
			flipswitch_command(5,'G');
			break;
	}
}

int probe_command(int i)
{
	//Clear buffer
	char* temp = "R\015";	
	send_cmd_to_com(temp);
	sleep(SLEEP_TICKS);

	//Send probe request for single digit
	if(i < 10){
		com_msg.output_buffer = "Cx\015";
		com_msg.output_buffer[1] = i + 48;
	}
	else
	{
		com_msg.output_buffer = "C1x\015";
		com_msg.output_buffer[2] = i + 38;
	}
	com_msg.input_buffer = buffer;
	com_msg.len_input_buffer = 3;
	send (com_port, &com_msg);
	sleep(SLEEP_TICKS);

	if(i < 10){
		if(buffer[1] == '1')
			wprintf(train_wnd, "rail section %c: vehicle present.\n",i+48); 
		else
			wprintf(train_wnd, "rail section %c: vehicle absent.\n",i+48); 
	}
	else
	{
		if(buffer[1] == '1')
			wprintf(train_wnd, "rail section 1%c: vehicle present.\n",i+38); 
		else
			wprintf(train_wnd, "rail section 1%c: vehicle absent.\n",i+38); 
	}
		

	return com_msg.input_buffer[1] - 48;
	
}

void init_train(WINDOW* wnd)
{
	train_wnd = wnd;
	create_process (train_process, 4, 42, "Train Process");
}
