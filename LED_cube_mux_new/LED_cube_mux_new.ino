#include <TimerOne.h>

extern "C" {
//#include <vector.h>
#include <string.h>
#include <stdlib.h>

}

#define DATA_SIZE 3
#define ENABLE_SIZE 2
#define LAYER_SIZE 4

#define LAYER_OFF 0x00
#define LAYER_ON_1 0x01
#define LAYER_ON_2 0x02
#define LAYER_ON_3 0x03
#define LAYER_ON_4 0x04

#define MUX_OFF	0x00
#define MUX_ON_1 0x02
#define MUX_ON_2 0x01

#define MAX_EFFECT 20


/*
 * Contadores para o controle de tarefas
 * [0] = 4	-- 4x250us = 1ms
 * [1] = 10 -- 10x1ms = 10ms
 * [2] = 10 -- 10x10ms = 100ms
 * [3] = 5 --	5x100ms = 500ms
 * [4] = 2 -- 2x500ms = 1s
 */
int task_control[5];

boolean _1ms = false;
boolean _10ms = false;
boolean _100ms = false;
boolean _500ms = false;
boolean _1s = false;

// Data pins
int data[]	= {2, 3, 4};

// Enable pins
int enable[]	= {5, 6};

// Layer pins
int layer[]	= {14, 15, 16, 17};


int effect_counter = 0;
int effect_time = 0;



/*
word cube_value[] = {
	0x0000, // Layer 1
	0x0000, // Layer 2
	0x0000, // Layer 3
	0x0000, // Layer 4
};
*/
word cube_value[] = {
	//0x0000,		0x06C0,		0x06C0,		0x0000,
	0xF00F,		0xF000,		0x0000,		0x0000,
};


/*
Formato de cada passo dos efeitos
0x0000, // Layer 1 - 16 bits
0x0000, // Layer 2 - 16 bits
0x0000, // Layer 3 - 16 bits
0x0000, // Layer 4 - 16 bits
0,		// Time - 10 ms units
*/


struct efect_t
{
	word * efect[];
	unsigned char count;
	unsigned char size;
};


//struct efect_t effect_pause = NULL;
word effect_pause_data[] = {
	// Layer 1	Layer 2		Layer 3		Layer 4		Time
	0x0000,		0x0000,		0x0000,		0x0000,		50,
};

//struct efect_t effect_cube = NULL;
word effect_cube_data[] = {
	//Layer 1	Layer 2		Layer 3		Layer 4		Time
	0x0000,		0x06C0,		0x06C0,		0x0000,		30,
	// Layer 1	Layer 2		Layer 3		Layer 4		Time
	0xF99F,		0xF99F,		0xF99F,		0xF99F,		30,

	//10, // Numero de vezes que o efeito deve ser repetido
};

//struct efect_t effect_cube = NULL
//unsigned char effect_cube_len = (sizeof(effect_cube)/sizeof(word));



// Efeitos
//struct efect_t * effects[MAX_EFFECT];

#if 0
void add_effect(word * effect, byte loop_counter) {
	

}
#endif


void setup() {

	//Serial.begin(9600);
	task_control[0] = 4;
	task_control[1] = 10;
	task_control[2] = 10;
	task_control[3] = 5;
	task_control[4] = 2;


	for(int i = 0; i < DATA_SIZE; i++){
		pinMode(data[i], OUTPUT);
		digitalWrite(data[i], LOW);
	}

	for(int i = 0; i < ENABLE_SIZE; i++){
		pinMode(enable[i], OUTPUT);
		digitalWrite(enable[i], LOW);
	}

	for(int i = 0; i < LAYER_SIZE; i++){
		pinMode(layer[i], OUTPUT);
		digitalWrite(layer[i], LOW);
	}

	Timer1.initialize(250); // Inicializa o timer, configurando interrupcao a cada 250us 
	Timer1.attachInterrupt( timerInterrupt ); // Indica funcao de callback para cada interrupcao do timer

	pinMode(13, OUTPUT);
	digitalWrite(13, LOW);


	mux_select(MUX_OFF);
	layer_select(LAYER_OFF);

	// Setup das variaveis
	//memset(effects, 0x00, MAX_EFFECT);
}



void write_data_byte(int number)
{
	int mask = 0;
	int power = 0;

	for(int i = 0; i < DATA_SIZE; i++){
		mask = B1 << i;
		if(true && (mask & number)){
			power = HIGH;
		} else {
			power = LOW;
		}
		digitalWrite(data[i], power);
	}
}




void mux_select(int enable_value)
{
	if (enable_value & B00000001) {
		digitalWrite(enable[0], HIGH);
	} else {
		digitalWrite(enable[0], LOW);
	}

	if (enable_value & B00000010) {
		digitalWrite(enable[1], HIGH);
	} else {
		digitalWrite(enable[1], LOW);
	}
}


void layer_select(int layer_value)
{
	int comp = B00000001;

	for(int i = 0; i < LAYER_SIZE; i++){
		if (layer_value & comp) {
			digitalWrite(layer[i], HIGH);
		} else {
			digitalWrite(layer[i], LOW);
		}
		comp <<= 0x01;
	}
}


void cube_driver(void) {
	byte comp = B10000000;
	byte counter = 0;
	byte layer_data_l = 0;
	byte layer_data_h = 0;
	byte layer_value = 0x01;

	// Garante o apagamento do estado anterior
	//mux_select(MUX_OFF);
	//layer_select(LAYER_OFF);

	// Cria um for para cada layer
	for(byte i = 0; i < LAYER_SIZE; i++){


		// Tem agum LED pra acender no layer?
		if(cube_value[i]){

			layer_select(layer_value);

			layer_data_l = byte(cube_value[i] & 0x00FF);
			layer_data_h = byte((cube_value[i] & 0xFF00) >> 8);

			for(counter = 0; counter < 8; counter++){
				if(layer_data_h & comp) {
					write_data_byte(counter);
					mux_select(MUX_ON_1);
				}
				else {
					mux_select(MUX_OFF);
				}
				delayMicroseconds(50);
				comp >>= 0x01;
			}

			comp = B10000000;
			for(counter = 0; counter < 8; counter++){
				if(layer_data_l & comp) {
					mux_select(MUX_ON_2);
					write_data_byte(counter);
				}
				else {
					mux_select(MUX_OFF);
				}
				delayMicroseconds(50);
				comp >>= 0x01;
			}
		} else {
			// Garante o apagamento do estado anterior
			mux_select(MUX_OFF);
		}
		layer_value <<= 0x01;
	}

	// Garante o apagamento do estado anterior
	//mux_select(MUX_OFF);
	//layer_select(LAYER_OFF);
}



#if 0
void effect_driver(void) {

	int effect_addr = 0;

	word effect_data_layer_1 = 0;
	word effect_data_layer_2 = 0;
	word effect_data_layer_3 = 0;
	word effect_data_layer_4 = 0;
	int effect_data_time = 0;


	//Consome o tempo
	if(effect_time) {
		effect_time--;
		return;
	}

	//Carrega o efeito
	effect_addr = effect_counter * 5;

	effect_data_layer_1 = effects[effect_addr];
	effect_data_layer_2 = effects[effect_addr + 1];
	effect_data_layer_3 = effects[effect_addr + 2];
	effect_data_layer_4 = effects[effect_addr + 3];
	effect_data_time	= effects[effect_addr + 4];

	// Verifica se eh o fim dos efeitos
	if((effect_data_layer_1 == 0xFFFF) && (effect_data_layer_2 == 0xFFFF) &&
		(effect_data_layer_3 == 0xFFFF) && (effect_data_layer_4 == 0xFFFF) &&
		(effect_data_time == 0xFFFF))
	{
		effect_counter = 0;
		effect_time = 0;
		return;
	}

	if(effect_data_time > 0) {
		effect_time = effect_data_time;
		cube_value[0] = effect_data_layer_1;
		cube_value[1] = effect_data_layer_2;
		cube_value[2] = effect_data_layer_3;
		cube_value[3] = effect_data_layer_4;
	}

	effect_counter++;
	return;
}

#endif


void loop() {
	cube_driver();

	if(_1ms) {
		_1ms = false;

	}

	if(_10ms) {
		_10ms = false;
		//effect_driver();
	}

	if(_100ms) {
		_100ms = false;

	}

	if(_500ms) {
		_500ms = false;
		digitalWrite(13, digitalRead(13) ^ 1 );
	}

	if(_1s) {
		_1s = false;

	}
}




void timerInterrupt()
{

	task_control[0]--;
	if(task_control[0]){
		return;
	}
	_1ms = true;
	task_control[0] = 4;


	task_control[1]--;
	if(task_control[1]){
		return;
	}
	_10ms = true;
	task_control[1] = 10;

	task_control[2]--;
	if(task_control[2]){
		return;
	}
	_100ms = true;
	task_control[2] = 10;


	task_control[3]--;
	if(task_control[3]){
		return;
	}
	_500ms = true;
	task_control[3] = 5;


	task_control[4]--;
	if(task_control[4]){
		return;
	}
	_1s = true;
	task_control[4] = 2;
}
