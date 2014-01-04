/********************************************************************************
*Author: David Webster								*
*File: shell.c									*	
*Updates:									*
*										*
*1. Shell ignores extra spaces in commands, i.e. "probe 5", 			*
*	"probe   5", "    probe 5".. etc function the same			*
*2. A string compare function for commands has been written for extensibility	*
*3. API's throughout are much more realistic (e.g. int instead of char* as 	*
*	argument for numbers)							*
*4. Function prototypes were placed in kernel.h so as to allow train.c and 	*
*	shell.c to have relevant functions in them.				*
*5. Command buffer now kicks out an error when overflow characters are entered 	*
*	(previously it just ignored them until user entered command then showed	*
*	an error like a win7 DOS prompt)					*
*6. Support for multiple arguments through "get_argument" function.  Currently 	*
*	we only use one/two, but this shell gives access to as many as are 	*
*	typed into the consol.							*
*										*
* Note:	A majority of shell.c has been completely reworked; however, to 	*
*	overcome some assert errors, I still have to clear the buffer with NULLs*
*	after each command...etc.						* 
********************************************************************************/

#include <kernel.h>

char buffer [10]; 
COM_Message com_msg;
WINDOW shell_window = {0, 9, 61, 16, 0, 0, 0xDC};
WINDOW* shell_wnd = &shell_window;

WINDOW train_window = {0, 0, 61, 9, 0, 0, 0xDC};
WINDOW* train_wnd = &train_window;

void shell_process(PROCESS self, PARAM param)
{
	Keyb_Message msg; 
	char ch;
	char user_input[COMMAND_BUFFER_LENGTH];

	int i = 0; //char into array counter
	int printed_count = 0; //characters on screen currently counter

	//clear screen and print welcome message
	clear_window(kernel_window);
	move_cursor(kernel_window,0,0);

	move_cursor(shell_wnd,0,0);
	wprintf(shell_wnd, "Welcome to Shell-David! 'help' for a list of commands.\n\n");
	print_prompt();
	
	//Start listening to user
	msg.key_buffer = &ch; 

	while (1) 
	{ 		
		send(keyb_port, &msg); 
		
		//ensure a backspace char that would erase the prompt does not print to console!
		if(printed_count >= 0 && ch != 8){ 
			wprintf(shell_wnd, "%c", ch); 
			printed_count++;
		}
		else if(printed_count > 0 && ch == 8){	
			wprintf(shell_wnd, "%c", ch);
			printed_count--;
		}

		//Make sure buffer overflow doesnt happen if user types too long of an input.
		if(i < COMMAND_BUFFER_LENGTH){
			user_input[i] = ch;
			i++;
		}else{
			wprintf(shell_wnd, "\nError: command cannot exceed 100 characters!\n");
			print_prompt();
			i=0;
			printed_count = 0;
			user_input[0] = NULL;
		}
		
		//Check for special case characters
		switch(ch)
		{
			case 13: //-------------carriage return-------------
		
				//make user_input a null terminated string
				user_input[i] = NULL;			

				process_command(user_input);
		
				//command has been executed so clear command array to avoid MAGIC->PCB errors after about 20-30 commands.
				for(  ;i>=0;i--){
					user_input[i] = NULL;
				}
			
				//Reset array/characters printed to the console counter
				i=0;
				printed_count = 0;
				break;
			case 8: //------------------backspace---------------
				if(i > 1){
		 			user_input[--i] = NULL; // remove the backspace character
					user_input[--i] = NULL; // remove 1 "good data" character
				}else if(i == 1){
					user_input[0] = NULL; // remove the backspace character
					i=0;
				}
				break;
		}
	}
} 

void parse_command_for_whitespace(char command[])
{
	char parsed_command[COMMAND_BUFFER_LENGTH];
	int i=0;
	int parsed_command_counter=0;

	//Remove enter keys
	while(command[i] != NULL)
	{
		if(command[i] == 13){
			command[i] = NULL;
			break;
		}
		i++;
	}
	i=0;

	//remove extra spaces from command string
	while(1){
		if(command[i] == NULL){
			//Cut off trailing space and create a null terminated string.
			parsed_command[parsed_command_counter-1] = NULL;
			break;			
		}

		//scan past all whitespace until first actual argument
		while(command[i] == ' ' && command[i] != NULL){ i++; }		

		//scan until first space
		while(command[i] != ' ' && command[i] != NULL){ 
			parsed_command[parsed_command_counter] = command[i];
			i++;
			parsed_command_counter++;
		}
		
		parsed_command[parsed_command_counter] = ' ';
		parsed_command_counter++;
		
	}
	
	//Char array is passed by reference so this following copy is used kind of like a
	//return statement from this function.
	i=0;
	while(parsed_command[i] != NULL){
		command[i] = parsed_command[i];
		i++;
	}
	command[i] = NULL;
}
void trim_whitespace_at_ends(char *str)
{
	//Note: In this function I was trying to practice pointer arithmetic to become more well rounded as a programmer 
	//	as I almost exclusively use the [] notation elsewhere, as you can see throughout most of this code.

	int length = 0;
	int i = 0;
	char *start_point = str-1;
	char *end_point = NULL;
	char *starting_loc_of_string;

	//For empty string just return it.
	if( str[0] == NULL )
		return str;

	while(str[i++] != NULL) length++;
		end_point = str + length;

	//Find first non-space on front and end
	while( *(++start_point) == ' ' );
	while( *(--end_point) == ' ' && end_point != start_point );

	//If the end had trailing white space, just move the null to the spot right after the first "good" data
	//else if the start had white space and the end had white space (i.e. string of spaces), just put a null in the
	//first spot of the string.
	if( str + length - 1 != end_point )
		*(end_point + 1) = NULL;
	else if( start_point != str &&  end_point == start_point )
		*str = NULL;

	//move all "good" data into the start of the string. (shift left)
	starting_loc_of_string = str;
	if( start_point != str )
	{
		
		while( *start_point ) *starting_loc_of_string++ = *start_point++;
	
		//Throw in a null after all good data is copied to keep it a string.		
		*starting_loc_of_string = NULL;
	}
}

void remove_white_space(char *str)
{
	trim_whitespace_at_ends(str);
	parse_command_for_whitespace(str);	
}

int command_cmp(char str_one[], char str_two[])
{
	int i = 0;

	while( str_one[i] != ' ' && str_one[i] != '\n' && str_one[i] != NULL)
	{
		if( str_one[i] != str_two[i] ){
			break;	
		}

		i++;
	}
	
	if((str_one[i] == ' ' || str_one[i] == '\n' || str_one[i] == NULL) && (str_two[i] == NULL))
		return 1;
	else
		return 0;
}

void get_argument(int arg, char* user_input, char output[])
{
	int i = 0;
	int k = 0;
	int arg_counter = 0;
	
	//scan past all whitespace until requested argument
	while(arg_counter < arg)
	{	
		while(user_input[i] != ' ' && user_input[i] != NULL){ i++; }
		
		if(user_input[i] == NULL)
		{
			//Requested an argument that did not exist
			return NULL;
		}
		i++;

		arg_counter++;
	}	

	while(user_input[i] != ' ' && user_input[i] != NULL)
	{
		output[k] = user_input[i];
		k++;
		i++;
	}
	
	//Make output a null terminated string
	output[k] = NULL;	
}

//Note: only works for positive expos.
int pow(int base, int expo)
{
	int i = 0;
	int result = 1;
	
	if(expo < 0)
		return NULL;	
	
	if(expo == 0)
		return 1;

	for(i=0;i<expo;i++)
		result = result * base;

	return result;
}

int a_to_i(char str[])
{
	int i=0;
	int k=0;
	int result = 0;
	while(str[i] != NULL) {i++;}
	
	for(i=i-1; i>=0;i--)
		result = result + (str[i]-48) * pow(10,k++);

	return result;
}

int process_command(char user_input[])
{	
	int valid_command = 0;
	int int_arg_one=0;
	char arg_one[COMMAND_BUFFER_LENGTH];
	char arg_two[COMMAND_BUFFER_LENGTH];
	
	remove_white_space(user_input);
	 
	get_argument(1, user_input, arg_one);
	get_argument(2, user_input, arg_two);
	int_arg_one = a_to_i(arg_one);

	if(user_input[0] == 13){	
		//user input enter key with empty command -- so just reprint 
		//prompt dont throw an error message
		valid_command = 1;
	}
	else if(command_cmp(user_input, "help"))
	{
		help_command();
		valid_command = 1;
	}
	else if(command_cmp(user_input, "clear"))
	{
		clear_command();
		valid_command = 1;
	}
	else if(command_cmp(user_input, "ps"))
	{
		ps_command();
		valid_command = 1;
	}
	else if(command_cmp(user_input, "pacman"))
	{
		pacman_command();
		valid_command = 1;
	}
	else if(command_cmp(user_input, "start"))
	{
		//Make sure arguments passed make sense, otherwise return an error
		if(int_arg_one >=0 && int_arg_one <=5)
		{
			start_command(int_arg_one);
			valid_command = 1;
		}
	}
	else if(command_cmp(user_input, "flip"))
	{
		//Make sure arguments passed make sense, otherwise return an error
		if(int_arg_one >=1 && int_arg_one <=9 && arg_two != NULL)
		{
			//remove case sensitivity
			if(arg_two[0] == 'g') arg_two[0] = 'G';
			else if(arg_two[0] == 'r') arg_two[0] = 'R';

			flipswitch_command(int_arg_one, arg_two[0]);
			valid_command = 1;
		}	
	}
	else if(command_cmp(user_input, "probe"))
	{
		if(int_arg_one >=1 && int_arg_one<=16)
		{
			probe_command(int_arg_one);
			valid_command = 1;
		}
	}
	else if(command_cmp(user_input, "stop"))
	{
		stop_command();
		valid_command = 1;
	}
	else if(command_cmp(user_input, "cd"))
	{
		change_dir_command();
		valid_command = 1;
	}
	else if(command_cmp(user_input, "train"))
	{
		train_command();
		valid_command = 1;
	}
	
	if(!valid_command)
		error_command_not_valid();

	valid_command = 0;
	
	//print prompt again after command execution
	print_prompt();
	return 0;
}

void train_command()
{
	init_train(train_wnd);	
}

int change_dir_command(){
	
	//First stop the train
	stop_command();
	sleep(SLEEP_TICKS);

	//Now change directions
	char* temp = "L20D\015";	
	send_cmd_to_com(temp);
	sleep(SLEEP_TICKS);
}

void error_command_not_valid()
{
	wprintf(shell_wnd, "error: invalid command.\n");	
}

void pacman_command()
{
	wprintf(shell_wnd, "pacman - not yet implemented!\n");
}

void flipswitch_command(int id, char color)
{
	char* temp = "Mxx\015";
	temp[1] = id + 48;
	temp[2] = color;

	send_cmd_to_com(temp);

	sleep(SLEEP_TICKS);
}

void start_command(int speed)
{
	char* temp = "L20Sx\015";
	temp[4] = speed + 48;
	send_cmd_to_com(temp);
	sleep(SLEEP_TICKS);
}

void stop_command()
{	
	//"start" the train with a speed of 0 to stop it.
	start_command(0);
}

void ps_command()
{
	int i;
	PCB* p = pcb;

	print_process_heading(shell_wnd);
	for (i = 0; i < MAX_PROCS; i++, p++) {
		if (!p->used)
			continue;
		print_process_details(shell_wnd, p);
	}
}

void clear_command()
{
	clear_window(shell_wnd);
	move_cursor(shell_wnd,0,0);
}

void help_command()
{
	wprintf(shell_wnd, "\n----{shell commands}----------------------\n");
	wprintf(shell_wnd, "help               -      Lists supported commands\n");
	wprintf(shell_wnd, "clear              -      Clears screen of text\n");
	wprintf(shell_wnd, "ps                 -      Lists currently running processes\n");
	wprintf(shell_wnd, "pacman             -      Not supported\n\n");

	wprintf(shell_wnd, "----{train control commands}----------------------\n");
	wprintf(shell_wnd, "start <speed>      -      Starts TOS train at <speed>\n");
	wprintf(shell_wnd, "flip <#> <color>   -      Flips TOS switch <#> to <color>\n");
	wprintf(shell_wnd, "probe <#>          -      Check if train is on track <#>\n");
	wprintf(shell_wnd, "stop               -      Stops TOS train\n");
	wprintf(shell_wnd, "cd                 -      change direction of train\n");

	
}

void print_prompt()
{	
	//14 is two 16th notes and 26 is a right arrow.
	wprintf(shell_wnd, "%c %c", 14,26);
}

void init_shell()
{
	create_process (shell_process, 5, 42, "Shell Process");
}
