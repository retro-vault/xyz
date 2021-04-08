#include <SDL2/SDL.h> 
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

const int SCREEN_WIDTH = 1024;
const int SCREEN_HEIGHT = 512;

void draw_rectangle(SDL_Surface* surface, int x, int y, int width, int height, uint8_t mask)
{
    SDL_LockSurface(surface);
    uint8_t *pixels=(uint8_t *)surface->pixels;
    int maxwidth = width * 4;
    int dy;
    for (dy = y; dy < height; dy++) {
        int dx;
        for (dx = x; dx < maxwidth; dx += 4) {
            pixels[dx + (dy * surface->pitch)] = 0;
            
            /* Apply the mask. */
            uint8_t msbit=mask&0x80;
           	
           	pixels[dx + (dy * surface->pitch) + 1] = msbit ? 255 : 0;
            
            /* Rotate the mask. */
            mask=(mask<<1)|(msbit?1:0);
            	
            pixels[dx + (dy * surface->pitch) + 2] = 0;
        }
    }
    SDL_UnlockSurface(surface);
}

int main( int argc, char* args[] )
{
    /* The window we'll be rendering to */
    SDL_Window* window = NULL;
    
    /* The surface contained by the window */
    SDL_Surface* wgpx = NULL;

	SDL_Cursor *cursor; /* Make this variable visible in the point
                       where you exit the program */

	SDL_Surface* g; 

    /* Initialize SDL */
    if( SDL_Init( SDL_INIT_VIDEO ) < 0 )
    {
        printf( "SDL could not initialize! SDL_Error: %s\n", SDL_GetError() );
    }
     else
    {
    
    	g = SDL_CreateRGBSurface(0, SCREEN_WIDTH, SCREEN_HEIGHT, 32, 0, 0, 0, 0);
    
    	SDL_ShowCursor( SDL_DISABLE ); 
		/* SDL_WarpMouse( x, y ); */
    
    /*
		int32_t cursorData[2] = {0, 0};
		cursor = SDL_CreateCursor((Uint8 *)cursorData, (Uint8 *)cursorData, 8, 8, 4, 4);
		SDL_SetCursor(cursor);
    */
        window = SDL_CreateWindow( "SDL Tutorial", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_HIDDEN );
        if( window == NULL )
        {
            printf( "Window could not be created! SDL_Error: %s\n", SDL_GetError() );
        }
    	else
        {     
        	SDL_SetWindowSize(window, SCREEN_WIDTH, SCREEN_HEIGHT);
           
           	SDL_ShowWindow(window);
           
            /*Get window surface */
            wgpx = SDL_GetWindowSurface( window );

            /* Fill the surface white */
            SDL_FillRect( wgpx, NULL, SDL_MapRGB( wgpx->format, 0xFF, 0xFF, 0xFF ) );
            
            /*Update the surface */
            SDL_UpdateWindowSurface( window );
        }
    }
	
	
    /* Main loop flag */
    bool quit = false;

    /* Event handler */
    SDL_Event e;
	
	draw_rectangle(g, 0,0,100,100,0xaa);
	
	/*While application is running */
    while( !quit )
    {
		/* Handle events on queue */
        while( SDL_PollEvent( &e ) != 0 )
        {
            /* User requests quit */
            if( e.type == SDL_QUIT )
            {
                quit = true;
            }
        }
                
        SDL_Rect r;
        r.x = 0;
		r.y = 0;
		r.w = SCREEN_WIDTH;
		r.h = SCREEN_HEIGHT;
                
     	SDL_BlitSurface( g, &r, wgpx, &r );
            
        /* Update the surface */
    	SDL_UpdateWindowSurface( window );
    }        
	
	SDL_FreeSurface(g);
	
	/* Destroy window */
    SDL_DestroyWindow( window );

	/*
	SDL_FreeCursor(cursor);
	*/

    /* Quit SDL subsystems */
    SDL_Quit();

    return 0;
}
