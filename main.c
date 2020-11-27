/* Maze generator in C.
 * Joe Wingbermuehle
 * 19990805
 * 
 * Adapted for Clarvi & FPGA version
 * For Part IB ECAD Optional tick 2* 
 * Yulong Huang
 * 2019/11
 * 
 * Thank you Joe!
 */

#include "avalon_addr.h"
#include "image.h"
#include"frames.h"
#include <stdio.h>
#include <stdbool.h>
#define RAND_A 48217
#define RAND_M 142857

//DIM = size of maze * 2 + 3
//Here we use size 20
#define DIM 43


/*
 * The useful functions about arithmetics
 */

//__mulsi3 implementation of GCC
unsigned int
__mulsi3 (unsigned int a, unsigned int b)
{
  unsigned int r = 0;

  while (a)
    {
      if (a & 1)
	r += b;
      a >>= 1;
      b <<= 1;
    }
  return r;
}

void *memcpy(void *dest, void *src, size_t n) 
{ 
   // Typecast src and dest addresses to (char *) 
   char *csrc = (char *)src; 
   char *cdest = (char *)dest; 
  
   // Copy contents of src[] to dest[] 
   for (int i=0; i<n; i++) 
       cdest[i] = csrc[i]; 
} 

//Lehmer random number generator, with a=48217 and m=142857
int 
rand(int* state)
{
	return *state = rem(*state * RAND_A, RAND_M);
}

/*
 * The useful functions using FPGA
 */

//Delays for n us, for our 50MHz CPU
void 
delay(int n)
{
	int starting = get_time();
	bool done = false;
	while(!done){
		int current = get_time();
		if(current < starting){
			delay(n);
			return;
		}
		if((current - starting) >= n*50){
			return;
		}
	}
}

//The given function, set a pixel to a certain colour
void 
vid_set_pixel(int x, int y, int colour)
{
	// derive a pointer to the framebuffer described as 16 bit integers
	volatile short *framebuffer = (volatile short *) (FRAMEBUFFER_BASE);

	// make sure we don't go past the edge of the screen
	if ((x<0) || (x>DISPLAY_WIDTH-1))
		return;
	if ((y<0) || (y>DISPLAY_HEIGHT-1))
		return;

	framebuffer[x+y*DISPLAY_WIDTH] = colour;
}

//Writing a value to the hex digit display
void 
hex_output(int value)
{
	int *hex_leds = (int *) 0x04000080;  // define a pointer to the register
	*hex_leds = value;                   // write the value to that address
}

//Return true if left/right button is pressed, as our reset signal
bool 
button_pressed()
{
	//Left and right button press are in 1st and 2nd bit of the register
	int reg = avalon_read(PIO_BUTTONS);
	return ( (reg&2) != 0) || ((reg & 4) != 0); 
}

//Printing one pixel of size n*n
void 
print_px (int size, int x, int y, int color)
{
	x = x*size;
	y = y*size;
	for(int i=0; i<size; i++){
		for(int j=0; j<size; j++){
			vid_set_pixel(x+i, y+j, color);
		}
	}
}

void 
printIMG(short* img)
{

	for(int i=0; i<200; i++){
		vid_set_pixel(260+i,0,PIXEL_WHITE);
		vid_set_pixel(260+i,200,PIXEL_WHITE);
		vid_set_pixel(260,0+i,PIXEL_WHITE);
		vid_set_pixel(460,0+i,PIXEL_WHITE);
	}

	int i = 0;
	while(img[i] != -1){
		int x = img[i];
		int y = img[i+1];
		vid_set_pixel(x, y, PIXEL_WHITE);
		i += 2;
	}
}

void
printUpdateImage(short* img, int x, int y, int xlen, int ylen)
{
	for(int i=0; i<xlen; i++){
		for(int j=0; j<ylen; j++){
			vid_set_pixel(x+i, y+j, PIXEL_BLACK);
		}
	}

	printIMG(img);
}

void 
printClear(short* src)
{
	int i = 0;
	int x = 45;
	int y = 37;
	int count = 0;
	while(src[i] != -1){
		if(src[i] == 1){
			print_px(6,x,y,PIXEL_WHITE);
		}
		
		i++;
		count ++;
		x++;
		if(count == 23){
			count = 0;
			x = 45;
			y++;
		}

	}
}

/*
 * The maze algorithm, using randomized DFS to generate  
 */


//Carve the maze starting at x, y.
void
CarveMaze (char *maze, int width, int height, int x, int y, int* state)
{

  int x1, y1;
  int x2, y2;
  int dx, dy;
  int dir, count;

  dir = rand(state)%4;
  count = 0;
  while (count < 4)
    {
      dx = 0;
      dy = 0;
      switch (dir)
	{
	case 0:
	  dx = 1;
	  break;
	case 1:
	  dy = 1;
	  break;
	case 2:
	  dx = -1;
	  break;
	default:
	  dy = -1;
	  break;
	}
      x1 = x + dx;
      y1 = y + dy;
      x2 = x1 + dx;
      y2 = y1 + dy;
      if (x2 > 0 && x2 < width && y2 > 0 && y2 < height
	  && maze[y1 * width + x1] == 1 && maze[y2 * width + x2] == 1)
	{
	  maze[y1 * width + x1] = 0;
	  maze[y2 * width + x2] = 0;
	  x = x2;
	  y = y2;
	  dir = rand(state)%4;
	  count = 0;
	}
      else
	{
	  dir = (dir + 1) % 4;
	  count += 1;
	}
    }
}

//Generate maze in matrix maze with size width, height.
void
GenerateMaze (char *maze, int width, int height, int* state )
{

  int x, y;

  /* Initialize the maze. */
  for (x = 0; x < width * height; x++)
    {
      maze[x] = 1;
    }
  maze[1 * width + 1] = 0;

  /* Seed the random number generator. */
  //srand (time (0));

  /* Carve the maze. */

  for (y = 1; y < height; y += 2)
    {
      for (x = 1; x < width; x += 2)
	{
	  CarveMaze (maze, width, height, x, y, state);
	}
    }

  // Set up the entry and exit. 
  maze[0 * width + 1] = 0;
  maze[(height - 1) * width + (width - 2)] = 0;
}

//Display the maze.
void
ShowMaze (const char *maze, int width, int height)
{
  int x, y;
  for (y = 0; y < height; y++)
    {
      for (x = 0; x < width; x++)
	{
	  switch (maze[y * width + x])
	    {
	    case 1:
		print_px(6, x, y, PIXEL_WHITE);
	      //Wall
	      break;
	    default:
	      //Road
	      break;
	    }
	}
    }
}

/*
 * Game states  
 */

//Re-generate the maze and display
void reset(char* maze, int width, int height, int* state, int* pos, int* rot){
	*state = avalon_read(PIO_ROTARY_R) + (avalon_read(PIO_ROTARY_L) << 8 ) + 1;
	pos[0] = 1;
	pos[1] = 0;
	pos[2] = 0;
	for(int i=0; i<DISPLAY_WIDTH; i++){
		for(int j=0; j<DISPLAY_HEIGHT; j++){
			vid_set_pixel(i, j, PIXEL_BLACK);
		}
	}
	GenerateMaze(maze, width, height, state);
	ShowMaze(maze, width, height);
	print_px(6, pos[0], pos[1],  PIXEL_GREEN);
}

//Determine whether player can move
bool move(int* pos, int dx, int dy, char* maze){
	if(dx == 0 && dy == 0) return false;

	int x = pos[0];
	int y = pos[1];
	
	if( ((x + dx) < DIM) && 
		((x + dx) >= 0) && 
		((y + dy) < DIM) && 
		((y + dy) >= 0) && 
		(maze[(x+dx) + DIM * (y+dy)] != 1) ){
		print_px(6, pos[0], pos[1], PIXEL_RED);
		pos[0] += dx;
		pos[1] += dy;
		print_px(6, pos[0], pos[1],  PIXEL_GREEN);
		return true;
	}

	return false;
}

//The while loop body, keep updating player position and state
void update(int* pos, int* rot, char* maze){
	int x,y;
	int dx,dy;
	int now_l, now_r;

	x = pos[0];
	y = pos[0];
	dx = dy = 0;
	now_l = avalon_read(PIO_ROTARY_L);
	now_r = avalon_read(PIO_ROTARY_R);

	switch(now_l - rot[0]){ 	//deal with the coordinate
		case 1:
			dx++; break;
		case -1:
			dx--; break;
		case 255:
			dx--; break;
		case -255:
			dx++; break;
	}	//default: do nothing

	switch(now_r - rot[1]){ 	//deal with the coordinate
		case 1:
			dy++; break;
		case -1:
			dy--; break;
		case 255:
			dy--; break;
		case -255:
			dy++; break;
	}	//default: do nothing

	move(pos, dx, dy, maze);
	
	rot[0] = now_l;
	rot[1] = now_r;
}


int 
main(void)
{
	int pos[3];
	int rot[2] = {0,0};
	char maze[DIM][DIM];
	short clear[] = CLEAR;
	short asuka[] = ASUKA;
	short frame1[] = FRAME1;
	short frame2[] = FRAME2;
	short frame3[] = FRAME3;


	int* state;
	char* m = maze[0];
	printIMG(asuka);
	while(true){
		hex_output(avalon_read(PIO_ROTARY_R) + (avalon_read(PIO_ROTARY_L) << 8 ));
		//hex_output(pos[0] + (pos[1] << 8 ));
		if(button_pressed()){
			reset(m, DIM, DIM, state, pos, rot);
		}else{
			update(pos, rot, m);
			if((pos[0] == 41) && (pos[1] == 42)){
				pos[2] = 1;
				printIMG(asuka);
				printClear(clear);
			}
			if(pos[2] == 1){
				printUpdateImage(frame1,355,40,45,30);
				delay(50000);
				printUpdateImage(frame2,355,40,45,30);
				delay(50000);
				printUpdateImage(frame3,355,40,45,30);
				delay(500000);
				printUpdateImage(frame2,355,40,45,30);
				delay(50000);
				printUpdateImage(frame1,355,40,45,30);
				delay(1000000);
			}else{
				delay(1000);
			}
		}
	}

	return 0;
}