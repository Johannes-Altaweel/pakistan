#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <windows.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_net.h>

#include "../../lib/include/text.h"
#include "../../lib/include/music.h"
#include "../../lib/include/spelare_data.h"
#include "../../lib/include/zombie.h" // include the zombies header file
#include "../../lib/include/bullet.h"
#include "../../lib/include/spelare.h"

#define WINDOW_WIDTH 1000
#define WINDOW_HEIGHT 750
#define MAX_ZOMBIES 40

struct game{
    SDL_Window *pWindow;
    SDL_Renderer *pRenderer;
    Spelare *pSpelare[MAX_SPELARE];
    Bullet *pBullet;
    SDL_Rect zombieRect[MAX_ZOMBIES];
    ZombieImage *pZombieImage;
    Zombie *pZombies[MAX_ZOMBIES];
    int Nrofzombies;
    int timeForNextZombie;
    SDL_Surface *pGame_StartBackgroundimage;
    SDL_Texture *pGame_StartbackgroundTexture;
    SDL_Surface *pbackgroundImage;
    SDL_Texture *pbackgroundTexture;
    int MoveUp;
    int MoveLeft;
    int MoveDown;
    int MoveRight;
    int mouseState;
    TTF_Font *pScoreFont, *pFont, *pOverFont;
    Text *pScoreText, *pOverText, *pStartText, *pWaitingText, *pTestText;
    int gameTimeM;
    int startTime;//in ms
    int gameTime;//in s
    GameState state;

    UDPsocket pSocket;
	IPaddress serverAddress;
	UDPpacket *pPacket;
    int nr_of_spelare, spelareNr;

};
typedef struct game Game;

int initiate(Game *pGame);
void run(Game *pGame);
void close(Game *pGame);
void handleInput(SDL_Event *pEvent, Game *pGame, int keys[]);
int getTime(Game *pGame);
int getMilli(Game *pGame);
void updateGameTime(Game *pGame);
void CheckCollison( Game *pGame, int zombieCount);
void updateNrOfZombies(Game *pGame);
void resetZombies(Game *pGame);
void updateSpelareWithRecievedData(Spelare *pSpelare, SpelareData *pSpelareData);
void updateWithServerData(Game *pGame);

 
int main(int argv, char** args){
    Game g={0};
    if(!initiate(&g)) return 1;
    run(&g);
    close(&g);

    return 0;
}


int initiate(Game *pGame){

    
    if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER)!=0){
        printf("Error: %s\n",SDL_GetError());
        return 0;
    }
    if(TTF_Init()!=0){
        printf("Error: %s\n",TTF_GetError());
        SDL_Quit();
        return 0;
    }

    if (SDLNet_Init())
	{
		printf("SDLNet_Init: %s\n", SDLNet_GetError());
        TTF_Quit();
        SDL_Quit();
		return 0;
	}

    if (initMus() == -1)
    {
        printf("Kunde inte initiera ljudsystemet!\n");
        return 1;
    }

    playMus("resources/spel.MP3");

    pGame->pWindow = SDL_CreateWindow("Zombies COD", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH, WINDOW_HEIGHT, 0);
    pGame->pRenderer = SDL_CreateRenderer(pGame->pWindow, -1, SDL_RENDERER_ACCELERATED|SDL_RENDERER_PRESENTVSYNC);

    pGame->pGame_StartBackgroundimage = IMG_Load("resources/iFi08.png");
    if (!pGame->pGame_StartBackgroundimage)
    {
        printf("Error! backgroundImage failed hej\n", IMG_GetError());
        return 1;
    }

    pGame->pGame_StartbackgroundTexture = SDL_CreateTextureFromSurface(pGame->pRenderer, pGame->pGame_StartBackgroundimage);
    if (!pGame->pGame_StartbackgroundTexture)
    {
        printf("Failed to create background texture: %s\n", SDL_GetError());
        SDL_FreeSurface(pGame->pGame_StartBackgroundimage);
        return 1;
    }

    pGame->pbackgroundImage = IMG_Load("resources/28256.jpg");
    if (!pGame->pbackgroundImage)
    {
        printf("Error! backgroundImage failed\n", IMG_GetError());
        return 1;
    }

    pGame->pbackgroundTexture = SDL_CreateTextureFromSurface(pGame->pRenderer, pGame->pbackgroundImage);
    if (!pGame->pbackgroundTexture)
    {
        printf("Failed to create background texture: %s\n", SDL_GetError());
        SDL_FreeSurface(pGame->pbackgroundImage);
        return 1;
    }

    
    pGame->MoveUp=1;
    pGame->MoveLeft=0;
    pGame->MoveDown=0;
    pGame->MoveRight=0;
    

    pGame->pFont = TTF_OpenFont("resources/arial.ttf", 100);
    pGame->pScoreFont = TTF_OpenFont("resources/arial.ttf", 70);
    pGame->pOverFont = TTF_OpenFont("resources/arial.ttf", 40);
    if(!pGame->pFont || !pGame->pScoreFont){
        printf("Error: %s\n",TTF_GetError());
        close(pGame);
        return 0;
    }
    
    if (!(pGame->pSocket = SDLNet_UDP_Open(0)))//0 means not a server
	{
		printf("SDLNet_UDP_Open: %s\n", SDLNet_GetError());
		return 0;
	}
	if (SDLNet_ResolveHost(&(pGame->serverAddress), "127.0.0.1", 2000))
	{
		printf("SDLNet_ResolveHost(127.0.0.1 2000): %s\n", SDLNet_GetError());
		return 0;
	}
    if (!(pGame->pPacket = SDLNet_AllocPacket(512)))
	{
		printf("SDLNet_AllocPacket: %s\n", SDLNet_GetError());
		return 0;
	}
    pGame->pPacket->address.host = pGame->serverAddress.host;
    pGame->pPacket->address.port = pGame->serverAddress.port;

    for (int i = 0; i < MAX_SPELARE; i++){
        pGame->pSpelare[i] = createSpelare(WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2, pGame->pRenderer, WINDOW_WIDTH, WINDOW_HEIGHT);
    }

    pGame->pZombieImage = initiateZombie(pGame->pRenderer);



    pGame->nr_of_spelare = MAX_SPELARE;
    pGame->pOverText = createText(pGame->pRenderer,238,168,65,pGame->pFont,"Time ran out",WINDOW_WIDTH/2,WINDOW_HEIGHT/2);
    pGame->pStartText = createText(pGame->pRenderer, 238, 168, 65, pGame->pOverFont, "Press space to start OR M (TO GO BACK TO MENU)", WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2 + 100);
    pGame->pWaitingText = createText(pGame->pRenderer,238,168,65,pGame->pScoreFont,"Waiting for server...",WINDOW_WIDTH/2,WINDOW_HEIGHT/2+100);
    pGame->pTestText = createText(pGame->pRenderer,238,168,65,pGame->pFont,"hello world",WINDOW_WIDTH/2,WINDOW_HEIGHT/2);
    for(int i=0;i<MAX_SPELARE;i++){
        if(!pGame->pSpelare[i]){
            printf("Error: %s\n",SDL_GetError());
            close(pGame);
            return 0;
        }
    }

    pGame->Nrofzombies = 0;
    pGame->timeForNextZombie = 3;

    //resetZombies(pGame);
    pGame->startTime = SDL_GetTicks64();
    pGame->gameTime = -1;
    pGame->state = START;
    return 1;
}



void run(Game *pGame){
    
    int keys[SDL_NUM_SCANCODES] = {0}; // Initialize an array to store key states
    int isRunning = 1;
    int first = 1;
    int pressed = 0;
    int mouseX, mouseY;
    int XPos, YPos;
    SDL_Event event;
    ClientData cData;
    int joining=0;
    int zombieCount = 0;      // Keep track of the current number of zombies
    Uint32 lastSpawnTime = 0; // Keep track of the time since the last zombie spawn
    while (isRunning)
    {   
        switch (pGame->state)
        {
            case ONGOING:
                while(SDLNet_UDP_Recv(pGame->pSocket, pGame->pPacket)){
                    updateWithServerData(pGame);
                }			                    
                if(SDL_PollEvent(&event)){   
                    switch (event.type){
                    case SDL_QUIT:
                        isRunning = 0;
                        break;
                    case SDL_KEYDOWN:
                        if (event.key.keysym.sym == SDLK_ESCAPE)
                        {
                            isRunning = 0;
                        }
                    default: handleInput(&event,pGame,keys);
                        break;
                    }
                }
                for (int i = 0; i < MAX_SPELARE; i++) updateSpelare(pGame->pSpelare[i]);
                SDL_RenderClear(pGame->pRenderer);
                SDL_RenderCopy(pGame->pRenderer, pGame->pbackgroundTexture, NULL, NULL);
                updateNrOfZombies(pGame);
                for (int i = 0; i < (pGame->Nrofzombies); i++){
                    updateZombie(pGame->pZombies[i]);
                }

                updateGameTime(pGame);
                
                for (int i = 0; i < pGame->Nrofzombies; i++){
                    drawZombie(pGame->pZombies[i]);
                }

                if(pGame->pScoreText) {
                    drawText(pGame->pScoreText);
                }

                for (int i = 0; i < MAX_SPELARE; i++){
                     drawSpelare(pGame->pSpelare[i]); 
                }
                
                SDL_Delay(10);
                if(getTime(pGame)==10){
                    pGame->state=GAME_OVER;
                }
                SDL_RenderPresent(pGame->pRenderer);
                break;
            case GAME_OVER:
                
                drawText(pGame->pOverText);
                drawText(pGame->pStartText);
                for (int i = 0; i < MAX_SPELARE; i++) resetSpelare(pGame->pSpelare[i]);
                //pGame->Nrofzombies = 0;
                //pressed = 0;
            case START:
                if(!joining){
                    SDL_RenderCopy(pGame->pRenderer, pGame->pGame_StartbackgroundTexture, NULL, NULL);
                }else{
                    SDL_SetRenderDrawColor(pGame->pRenderer,0,0,0,255);
                    SDL_RenderClear(pGame->pRenderer);
                    SDL_SetRenderDrawColor(pGame->pRenderer,230,230,230,255);
                    drawText(pGame->pWaitingText);
                }
                SDL_RenderPresent(pGame->pRenderer);
                if(SDL_PollEvent(&event)){
                    if(event.type==SDL_QUIT) isRunning = 0;
                    if(event.type==SDL_MOUSEMOTION) pGame->mouseState = SDL_GetMouseState(&XPos, &YPos);
                    if(event.type==SDL_MOUSEBUTTONDOWN){ 
                        SDL_GetMouseState(&mouseX, &mouseY);
                        if (mouseX >= 149 && mouseX <= 439 && mouseY >= 134 && mouseY <= 217){
                            pressed=1;  
                            joining = 1;
                            cData.command=READY;
                            cData.playerNumber=-1;
                            memcpy(pGame->pPacket->data, &cData, sizeof(ClientData));
                            pGame->pPacket->len = sizeof(ClientData);
                        }
                        if (mouseX >= 149 && mouseX <= 437 && mouseY >= 433 && mouseY <= 510){
                            isRunning = 0;
                        }
                    }
                }
                if(joining) SDLNet_UDP_Send(pGame->pSocket, -1,pGame->pPacket);
                if(SDLNet_UDP_Recv(pGame->pSocket, pGame->pPacket)){
                    updateWithServerData(pGame);
                    pGame->startTime = SDL_GetTicks64();
                    pGame->gameTime = -1;
                    if(pGame->state == ONGOING) joining = 0;
                }                
                break;
        }   
    }
}



void updateWithServerData(Game *pGame){
    ServerData sData;
    memcpy(&sData, pGame->pPacket->data, sizeof(ServerData));
    pGame->spelareNr = sData.playerNr;
    pGame->state = sData.gState;
    for(int i=0;i<MAX_SPELARE;i++){
        updateSpelareWithRecievedData(pGame->pSpelare[i],&(sData.spelare[i]));
    }
    for(int i=0;i<pGame->Nrofzombies;i++){
        updateZombiesWithRecievedData(pGame->pZombies[i], &(sData.zombie[i]));
    }
}

// Function to handle input
void handleInput(SDL_Event *pEvent, Game *pGame, int keys[]) {
    if (pEvent->type == SDL_KEYDOWN) {
        ClientData cData;
        cData.playerNumber = pGame->spelareNr;
        
        if (pEvent->key.keysym.scancode==SDL_SCANCODE_W) {
            moveUp(pGame->pSpelare[pGame->spelareNr]);
            cData.command = UP;
            pGame->MoveUp=1;
            pGame->MoveLeft=0;
            pGame->MoveDown=0;
            pGame->MoveRight=0;
        } if (pEvent->key.keysym.scancode==SDL_SCANCODE_A) {
            moveLeft(pGame->pSpelare[pGame->spelareNr]);
            cData.command = LEFT;
            pGame->MoveLeft=1;
            pGame->MoveUp=0;
            pGame->MoveDown=0;
            pGame->MoveRight=0;
        } if (pEvent->key.keysym.scancode==SDL_SCANCODE_S) {
            moveDown(pGame->pSpelare[pGame->spelareNr]);
            cData.command = DOWN;
            pGame->MoveDown=1;
            pGame->MoveLeft=0;
            pGame->MoveUp=0;
            pGame->MoveRight=0;
        } if (pEvent->key.keysym.scancode==SDL_SCANCODE_D) {
            moveRight(pGame->pSpelare[pGame->spelareNr]);
            cData.command = RIGHT;
            pGame->MoveRight=1;
            pGame->MoveLeft=0;
            pGame->MoveDown=0;
            pGame->MoveUp=0;
        } if(pEvent->key.keysym.scancode==SDL_SCANCODE_SPACE){
            fireSpelare(pGame->pSpelare[pGame->spelareNr], pGame->MoveUp, pGame->MoveLeft, pGame->MoveDown, pGame->MoveRight);
            cData.command = FIRE;
        }

        memcpy(pGame->pPacket->data, &cData, sizeof(ClientData));
		pGame->pPacket->len = sizeof(ClientData);
        SDLNet_UDP_Send(pGame->pSocket, -1,pGame->pPacket);

}
       
} 

void close(Game *pGame){
    stopMus();
    cleanMu();
    for(int i=0;i<MAX_SPELARE;i++) if(pGame->pSpelare[i]) destroySpelare(pGame->pSpelare[i]);
    resetZombies(pGame);
    if(pGame->pZombieImage) destroyZombieImage(pGame->pZombieImage);
    SDL_DestroyTexture(pGame->pGame_StartbackgroundTexture);
    SDL_FreeSurface(pGame->pGame_StartBackgroundimage);
    SDL_DestroyTexture(pGame->pbackgroundTexture);
    SDL_FreeSurface(pGame->pbackgroundImage);
    SDL_DestroyRenderer(pGame->pRenderer);
    SDL_DestroyWindow(pGame->pWindow);
    if(pGame->pScoreText) destroyText(pGame->pScoreText);
    if(pGame->pOverText) destroyText(pGame->pOverText);
    if(pGame->pStartText) destroyText(pGame->pStartText);
    if(pGame->pWaitingText) destroyText(pGame->pWaitingText);   
    if(pGame->pFont) TTF_CloseFont(pGame->pFont);
    if(pGame->pOverFont) TTF_CloseFont(pGame->pOverFont);
    if(pGame->pScoreFont) TTF_CloseFont(pGame->pScoreFont);
    SDLNet_Quit();
    TTF_Quit();    
    SDL_Quit();

}

int getTime(Game *pGame){
    return (SDL_GetTicks64()-pGame->startTime)/1000;
}

int getMilli(Game *pGame)
{
    return ((SDL_GetTicks64()-pGame->startTime) % 1000) / 100;
}

void updateGameTime(Game *pGame){

        if(pGame->pScoreText) 
        {
            destroyText(pGame->pScoreText);
        }
        static char scoreString[30];
        sprintf(scoreString,"%d.%d",getTime(pGame), getMilli(pGame));
        if(pGame->pScoreFont) 
        {
            pGame->pScoreText = createText(pGame->pRenderer,255,255,255,pGame->pScoreFont,scoreString,WINDOW_WIDTH-75,50);    
        }
}

void resetZombies(Game *pGame)
{
    for (int i = 0; i < pGame->Nrofzombies; i++)
    {
        destroyZombie(pGame->pZombies[i]);
    }
    pGame->Nrofzombies = 0;
}

void updateNrOfZombies(Game *pGame)
{
    if (getTime(pGame) > pGame->timeForNextZombie && pGame->Nrofzombies < MAX_ZOMBIES)
    {
        (pGame->timeForNextZombie) = (pGame->timeForNextZombie) + 1; // seconds till next asteroid
        pGame->pZombies[pGame->Nrofzombies] = createZombie(pGame->pZombieImage, WINDOW_WIDTH, WINDOW_HEIGHT);
        pGame->Nrofzombies++;
    }
}