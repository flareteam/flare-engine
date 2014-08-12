#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include "SDL.h"

int main(int argc, char *argv[])
{
	Uint32 w_flags = SDL_WINDOW_FULLSCREEN;
	Uint32 r_flags = SDL_RENDERER_SOFTWARE;
    SDL_Window *window;
    SDL_Renderer *renderer;

    window = SDL_CreateWindow("FLARE", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 480, w_flags);

    if (window)
    		renderer = SDL_CreateRenderer(window, -1, r_flags);

    /* Main render loop */
    Uint8 done = 0;
    SDL_Event event;
    while(!done)
	{
        /* Check for events */
        while(SDL_PollEvent(&event))
		{
            if(event.type == SDL_QUIT || event.type == SDL_KEYDOWN || event.type == SDL_FINGERDOWN)
			{
                done = 1;
            }
        }
		
		/* Draw a gray background */
		SDL_SetRenderDrawColor(renderer, 128, 0, 0, 1);
		SDL_RenderClear(renderer);
	
		/* Update the screen! */
		SDL_RenderPresent(renderer);
		
		SDL_Delay(2000);
    }
}
