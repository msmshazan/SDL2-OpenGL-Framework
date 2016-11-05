/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Mohamed Shazan $
   $Notice: All Rights Reserved. $
   ======================================================================== */


/*

  0-NOT DONE
  1-DONE
  
  TODO:
  [0]Eventually must cleanup Code
  [0]Implement live code editing
  [0]Implement music loading
  [0]loading assets
  [0]maybe virtual file system
  [0]other stuff maybe
  [1]capped framerate - 60 FPS
*/

// TODO(main) : Error checking

#include "gamecode.h"
#include <stdio.h>
#include <SDL.h>
#include <SDL_opengl.h>
#include <math.h>
#include <stdint.h>
#include <stddef.h>
#include <limits.h>
#include <float.h>
#include <intrin.h>
#include <windows.h>
#include "sdllayer.h"

#define GAMEFPS 60
#define WindowHeight 720
#define WindowWidth 1280

global_variable bool32 GlobalRunning;
global_variable bool32 GlobalPause;
global_variable sdl_offscreen_buffer GlobalBackBuffer;
//global_variable LPDIRECTSOUNDBUFFER GlobalSecondaryBuffer;
global_variable int64 GlobalPerfCountFrequency;

inline FILETIME
Win32GetLastWriteTime(const char *Filename) {
    FILETIME LastWriteTime = {};

    WIN32_FILE_ATTRIBUTE_DATA Data;
    if (GetFileAttributesEx(Filename, GetFileExInfoStandard, &Data)) {
        LastWriteTime = Data.ftLastWriteTime;
    }
    return LastWriteTime;
}


internal void
CatStrings(size_t SourceACount, const char *SourceA,
           size_t SourceBCount, const char *SourceB,
           size_t DestCount, char *Dest)
{
    for (size_t Index = 0; Index < SourceACount && Index < DestCount; ++Index) {
        *Dest++ = *SourceA++;
    }

    for (size_t Index = 0; Index < SourceBCount && Index < DestCount; ++Index) {
        *Dest++ = *SourceB++;
    }

    *Dest++ = 0;
}


internal int
StringLength(const char *String) {
    int Count = 0;
    while (*String++) {
        ++Count;
    }
    return Count;
}

internal void
SDLBuildEXEPathFileName(sdl_state *State, const char *FileName,
                        int DestCount, char *Dest)
{
    CatStrings(State->OnePastLastEXEFileNameSlash - State->EXEFileName,
               State->EXEFileName,
               StringLength(FileName), FileName,
               DestCount, Dest);
}

DEBUG_PLATFORM_WRITE_ENTIRE_FILE(DEBUGPlatformWriteEntireFile) {
    bool32 Result = false;

    SDL_RWops *FileHandle = SDL_RWFromFile(Filename, "wb");
    if (FileHandle) {
        size_t BytesWritten = 0;
        if ((BytesWritten == SDL_RWwrite(FileHandle, Memory, 1, MemorySize))) {
            // NOTE: FileHandle write successfully
            Result = BytesWritten == MemorySize;
        } else {
            // TODO: Logging
        }
        SDL_RWclose(FileHandle);
    } else {
        // TODO: Logging
    }

    return Result;
}


internal void
SDLCopyFile(const char *SourceFileName, const char *DestFileName) {
//TODO: FIX FOR WINDOWS 

    SDL_RWops *Source = SDL_RWFromFile(SourceFileName, "rb");
    if (Source) {
        SDL_RWops *Dest = SDL_RWFromFile(DestFileName, "wb");
        if (Dest) {
            char Buf[4096];
            size_t BytesRead = 0;
            while ((BytesRead == SDL_RWread(Source, Buf, sizeof(*Buf), ArrayCount(Buf)))) {
                SDL_RWwrite(Dest, Buf, sizeof(*Buf), BytesRead);
            }

            SDL_RWclose(Dest);
            SDL_RWclose(Source);
        } else {
            printf("Can't open file %s for writing: %s\n", DestFileName, SDL_GetError());
            SDL_RWclose(Source);
        }
    } else {
        printf("Can't open file %s for reading: %s\n", SourceFileName, SDL_GetError());
    }
}

internal sdl_game_code
loadgamecode(const char* dllname)
{
    sdl_game_code Result = {};
    char* tempdll = "Gamecode_temp.dll";
    CopyFile(dllname,tempdll,FALSE);
    Result.GameCodeDLL = SDL_LoadObject(tempdll);
    Result.DLLLastWriteTime = Win32GetLastWriteTime(dllname);
    if(Result.GameCodeDLL)
    {
        const char * func = "GameUpdateAndRender";
        Result.UpdateAndRender =(game_update_and_render*)SDL_LoadFunction(Result.GameCodeDLL,func);
    }
    else
    {
        printf("Fail loading dll code");        //Check errors
    }

    if(!Result.UpdateAndRender)  printf ("fail to get function");
    Result.IsValid = (Result.UpdateAndRender && Result.GameCodeDLL);
    
    if(!Result.IsValid)
    {
        Result.UpdateAndRender = 0;
    }
    return (Result);
}
internal void
Unloadgamecode(sdl_game_code* Gamecode)
{
    if(Gamecode->GameCodeDLL){
    SDL_UnloadObject(Gamecode->GameCodeDLL);
    Gamecode->IsValid =false;
    }
}

internal real32
SDLGetSecondsElapsed(uint64 OldCounter, uint64 CurrentCounter)
{
    return ((real32)(CurrentCounter - OldCounter) / (real32)(SDL_GetPerformanceFrequency()));
}

internal SDL_AudioDeviceID
SDLInitSound(int32 SamplesPerSecond) {
    SDL_AudioSpec spec = {};
    spec.freq = SamplesPerSecond;
    spec.format = AUDIO_S16;
    spec.channels = 2;
    spec.samples = 4096;

    SDL_AudioDeviceID Result = SDL_OpenAudioDevice(NULL, 0, &spec, 0, 0);
    if (Result != 0) {
        SDL_PauseAudioDevice(Result, 0);
    } else {
        printf("Failed to open SDL audio device: %s\n", SDL_GetError());
    }

    return Result;
}

internal void
SDLProcessKeyboardMessage(game_button_state *NewState, bool32 IsDown) {
    if (NewState->EndedDown != IsDown) {
        NewState->EndedDown = IsDown;
        ++NewState->HalfTransitionCount;
    }
}

internal void
SDLProcessPendingMessage(sdl_state *State, game_controller_input *KeyboardController) {
    SDL_Event Event;
    while (SDL_PollEvent(&Event)) {
        switch (Event.type) {
            case SDL_QUIT: {
                GlobalRunning = false;
            } break;

            case SDL_KEYUP:
            case SDL_KEYDOWN: {
                SDL_Keycode Key = Event.key.keysym.sym;

                bool IsDown = Event.key.state == SDL_PRESSED;
                bool WasDown = Event.key.state == SDL_RELEASED;

                if (WasDown != IsDown) {
                    if (Key == SDLK_w) {
                        SDLProcessKeyboardMessage(&KeyboardController->MoveUp, IsDown);
                    } else if (Key == SDLK_a) {
                        SDLProcessKeyboardMessage(&KeyboardController->MoveLeft, IsDown);
                    } else if (Key == SDLK_s) {
                        SDLProcessKeyboardMessage(&KeyboardController->MoveDown, IsDown);
                    } else if (Key == SDLK_d) {
                        SDLProcessKeyboardMessage(&KeyboardController->MoveRight, IsDown);
                    } else if (Key == SDLK_q) {
                        SDLProcessKeyboardMessage(&KeyboardController->LeftShoulder, IsDown);
                    } else if (Key == SDLK_e) {
                        SDLProcessKeyboardMessage(&KeyboardController->RightShoulder, IsDown);
                    } else if (Key == SDLK_UP) {
                        SDLProcessKeyboardMessage(&KeyboardController->ActionUp, IsDown);
                    } else if (Key == SDLK_LEFT) {
                        SDLProcessKeyboardMessage(&KeyboardController->ActionLeft, IsDown);
                    } else if (Key == SDLK_RIGHT) {
                        SDLProcessKeyboardMessage(&KeyboardController->ActionRight, IsDown);
                    } else if (Key == SDLK_DOWN) {
                        SDLProcessKeyboardMessage(&KeyboardController->ActionDown, IsDown);
                    } else if (Key == SDLK_ESCAPE) {
                        SDLProcessKeyboardMessage(&KeyboardController->Back, IsDown);
                        GlobalRunning = false;
                    } else if (Key == SDLK_SPACE) {
                        SDLProcessKeyboardMessage(&KeyboardController->Start, IsDown);
                    }
                }
            } break;

            default: break;
        }
    }
}

internal int
SDLGetWindowRefreshRate(SDL_Window *Window)
{
    SDL_DisplayMode Mode;
    int DisplayIndex = SDL_GetWindowDisplayIndex(Window);
    // If we can't find the refresh rate, we'll return this:
    int DefaultRefreshRate = 60;
    if (SDL_GetDesktopDisplayMode(DisplayIndex, &Mode) != 0)
    {
        return DefaultRefreshRate;
    }
    if (Mode.refresh_rate == 0)
    {
        return DefaultRefreshRate;
    }
    return Mode.refresh_rate;
}

internal SDL_Surface* LoadBitmap(char * filename)
{
    SDL_Surface* Surf = SDL_LoadBMP(filename);
    return Surf;
}

internal void GameRenderOpengl(SDL_Window *window,SDL_Surface *Buffer)
{
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, Buffer->w, Buffer->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, Buffer->pixels);
    glEnable(GL_TEXTURE_2D);
    // Before drawing consider co-ordinate system 
    glBegin(GL_QUADS);
    glTexCoord2i(0, 0); glVertex2f(-1.0f,1.0f);
    glTexCoord2i(1, 0); glVertex2f(1.0f,1.0f );
    glTexCoord2i(1, 1); glVertex2f(1.0f,-1.0f);
    glTexCoord2i(0, 1); glVertex2f(-1.0f,-1.0f);
    glEnd();
    SDL_GL_SwapWindow(window);
}


int main(int argc, char *argv[]){
    sdl_state SDLState = {};


    GlobalPerfCountFrequency = SDL_GetPerformanceFrequency();
  
    SDL_Init(SDL_INIT_EVERYTHING);
    uint64 PerfCountFrequency = SDL_GetPerformanceFrequency();
    SDL_DisplayMode CurrentDisplay;
    SDL_GetCurrentDisplayMode(0, &CurrentDisplay);  
    SDL_Window *Window = SDL_CreateWindow("Game SDL",10,10,WindowWidth,WindowHeight,SDL_WINDOW_OPENGL|SDL_WINDOW_SHOWN);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
    SDL_GLContext glcontext = SDL_GL_CreateContext(Window);
    printf("Monitor Refresh rate:%i\n",CurrentDisplay.refresh_rate);
    int GameUpdateHz = GAMEFPS;
    real32 TargetSecondsPerFrame = 1.0f / (real32)GameUpdateHz;
    Uint32 rmask, gmask, bmask, amask;
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
    rmask = 0xff000000;
    gmask = 0x00ff0000;
    bmask = 0x0000ff00;
    amask = 0x000000ff;
#else
    rmask = 0x000000ff;
    gmask = 0x0000ff00;
    bmask = 0x00ff0000;
    amask = 0xff000000;
#endif
    SDL_Surface *Buffer = SDL_CreateRGBSurface(0,WindowWidth,WindowHeight,32,rmask,gmask,bmask,amask);
    game_offscreen_buffer GameBuffer;
    GameBuffer.Memory =Buffer->pixels;
    GameBuffer.Width =(real32)Buffer->w;
    GameBuffer.Height =(real32)Buffer->h;
    GameBuffer.Pitch =Buffer->pitch;
    
#if HANDMADE_INTERNAL
            LPVOID BaseAddress = (LPVOID) Terabytes(2);
#else
            LPVOID BaseAddress = 0;
#endif

    game_memory GameMemory = {};
    GameMemory.PermanentStorageSize = Megabytes(64);
    GameMemory.TransientStorageSize = Megabytes(256);
    //GameMemory.HighPriorityQueue = &HighPriorityQueue;
    //GameMemory.LowPriorityQueue = &LowPriorityQueue;

    //GameMemory.PlatformAPI.AddEntry = SDLAddEntry;
    //GameMemory.PlatformAPI.CompleteAllWork = SDLCompleteAllWork;

    //GameMemory.PlatformAPI.GetAllFilesOfTypeBegin = SDLGetAllFilesOfTypeBegin;
    //GameMemory.PlatformAPI.GetAllFilesOfTypeEnd = SDLGetAllFilesOfTypeEnd;
    //GameMemory.PlatformAPI.OpenNextFile = SDLOpenNextFile;
    //GameMemory.PlatformAPI.ReadDataFromFile = SDLReadDataFromFile;
    //GameMemory.PlatformAPI.FileError = SDLFileError;

    // GameMemory.PlatformAPI.AllocateMemory = SDLAllocateMemory;
    // GameMemory.PlatformAPI.DeallocateMemory = SDLDeallocateMemory;

    //GameMemory.PlatformAPI.DEBUGFreeFileMemory = DEBUGPlatformFreeFileMemory;
    //GameMemory.PlatformAPI.DEBUGReadEntireFile = DEBUGPlatformReadEntireFile;
    //GameMemory.PlatformAPI.DEBUGWriteEntireFile = DEBUGPlatformWriteEntireFile;

    SDLState.TotalSize = GameMemory.PermanentStorageSize + GameMemory.TransientStorageSize;

    SDLState.GameMemoryBlock = VirtualAlloc(BaseAddress,
                                    (size_t) SDLState.TotalSize,
                                    MEM_RESERVE | MEM_COMMIT,
                                                 PAGE_READWRITE);
    GameMemory.PermanentStorage = SDLState.GameMemoryBlock;
    GameMemory.TransientStorage = ((uint8 *) GameMemory.PermanentStorage + GameMemory.PermanentStorageSize);

    if (!(GameMemory.PermanentStorage && GameMemory.TransientStorage)) {
        printf("Failed to allocate game memory\n");
        return -1;
    }

    // TODO: Add game replay support here

    game_input Input[2] = {};
    game_input *NewInput = &Input[0];
    game_input *OldInput = &Input[1];

GLuint TextureID = 0;
    glGenTextures(1, &TextureID);
    glBindTexture(GL_TEXTURE_2D, TextureID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, Buffer->w, Buffer->h, 0, GL_BGRA, GL_UNSIGNED_BYTE, Buffer->pixels);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glMatrixMode(GL_TEXTURE);
    glLoadIdentity();
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();  
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity(); 
    //SDL_Event event;
    char *SourceDllname = "Gamecode.dll";
    sdl_game_code Gamecode = loadgamecode(SourceDllname);                
    uint64 LastCounter = SDL_GetPerformanceCounter();
    uint64 LastCycleCount = __rdtsc();
    uint64 FlipWallClock = SDL_GetPerformanceCounter();
    GlobalRunning = true;
    while(GlobalRunning){
         
        NewInput->dtForFrame = TargetSecondsPerFrame;

        NewInput->ExecutableReloaded = false;
        
FILETIME NewDLLWriteTime = Win32GetLastWriteTime(SourceDllname);
        if(CompareFileTime(&NewDLLWriteTime,&Gamecode.DLLLastWriteTime) != 0)
        {
            Unloadgamecode(&Gamecode);
            Gamecode = loadgamecode(SourceDllname);
        }
        
        game_controller_input *OldKeyboardController = GetController(OldInput, 0);
        game_controller_input *NewKeyboardController = GetController(NewInput, 0);
        NewKeyboardController->IsConnected = true;
        for (size_t ButtonIndex = 0; ButtonIndex < ArrayCount(NewKeyboardController->Buttons); ++ButtonIndex) {
            NewKeyboardController->Buttons[ButtonIndex].EndedDown = OldKeyboardController->Buttons[ButtonIndex].EndedDown;
        }

        SDLProcessPendingMessage(&SDLState, NewKeyboardController);

        if (!GlobalPause) {
            Uint32 MouseButtons = SDL_GetMouseState((int *)&NewInput->MouseX, (int *)&NewInput->MouseY);
            NewInput->MouseZ = 0;
            SDLProcessKeyboardMessage(&NewInput->MouseButtons[0],
                                      SDL_BUTTON(SDL_BUTTON_LEFT));
            SDLProcessKeyboardMessage(&NewInput->MouseButtons[1],
                                      SDL_BUTTON(SDL_BUTTON_MIDDLE));
            SDLProcessKeyboardMessage(&NewInput->MouseButtons[2],
                                      SDL_BUTTON(SDL_BUTTON_RIGHT));
            SDLProcessKeyboardMessage(&NewInput->MouseButtons[3],
                                      SDL_BUTTON(SDL_BUTTON_X1));
            SDLProcessKeyboardMessage(&NewInput->MouseButtons[4],
                                      SDL_BUTTON(SDL_BUTTON_X2));

            // TODO: Handle Mouse button here

            // TODO: Game controller support here
        }

            

        //NOTE:All game update and render stuff go here

        Gamecode.UpdateAndRender(&GameMemory,NewInput,&GameBuffer);
        
        GameRenderOpengl(Window,Buffer);

        
        if (SDLGetSecondsElapsed(LastCounter, SDL_GetPerformanceCounter()) < TargetSecondsPerFrame)
        {
            real32 TimeToSleep = ((TargetSecondsPerFrame - SDLGetSecondsElapsed(LastCounter, SDL_GetPerformanceCounter())) * 1000) - 1;
            if (TimeToSleep > 0)
            {
                SDL_Delay((Uint32)TimeToSleep);
            }
            while (SDLGetSecondsElapsed(LastCounter, SDL_GetPerformanceCounter()) < TargetSecondsPerFrame)
            {
                // Waiting...
            }
        }
        uint64 EndCounter = SDL_GetPerformanceCounter();

        uint64 EndCycleCount = __rdtsc();
        uint64 CounterElapsed = EndCounter - LastCounter;
        uint64 CyclesElapsed = EndCycleCount - LastCycleCount;

        real64 MSPerFrame = (((1000.0f * (real64)CounterElapsed) / (real64)PerfCountFrequency));
        real64 FPS = (real64)PerfCountFrequency / (real64)CounterElapsed;
        real64 MCPF = ((real64)CyclesElapsed / (1000.0f * 1000.0f));

        //   printf("%.02fms/f, %.02f/s, %.02fmc/f\n", MSPerFrame, FPS, MCPF);

        LastCycleCount = EndCycleCount;
        LastCounter = EndCounter;
        
            game_input *Temp = NewInput;
            NewInput = OldInput;
            OldInput = Temp;

        
    }
    Unloadgamecode(&Gamecode);
    SDL_SaveBMP(Buffer,"test.bmp" );
    SDL_GL_DeleteContext(glcontext);
    SDL_DestroyWindow(Window);
    SDL_Quit();
                    
    return 0;
}
