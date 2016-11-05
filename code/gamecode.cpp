/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Mohamed Shazan $
   $Notice: All Rights Reserved. $
   ======================================================================== */
#include "gamecode.h"

inline uint32 RoundReal32Touint32(real32 Real32)
{
    return ((uint32)(Real32 + 0.5f));
}

inline uint8 RoundReal32Touint8(real32 Real32)
{
    return ((uint8)(Real32 + 0.5f));
}
inline uint32 TruncateReal32Touint32(real32 Real32)
{
    return (uint32)Real32;
}

inline uint8 TruncateReal32Touint8(real32 Real32)
{
    return (uint8)Real32;
}

internal void DrawRect(real32 MinX,real32 MinY,real32 MaxX,real32 MaxY,game_offscreen_buffer* Buffer, real32 R,real32 G,real32 B,real32 A =1.0f )
{
    if(MinX<0)
    {
        MinX = 0;
    }

    if(MinY<0)
    {
        MinY = 0;
    }
    if(MaxX>Buffer->Width)
    {
        MaxX = Buffer->Width;
    }

    if(MaxY>Buffer->Height)
    {
        MaxY = Buffer->Height;
    }
    
    uint32 Color =(uint32)((RoundReal32Touint32(R * 255.0f)|
                            (RoundReal32Touint32(G * 255.0f)<<8)|
                            (RoundReal32Touint32(B * 255.0f)<<16)|
                            (RoundReal32Touint32(A * 255.0f))<<24));

    uint8 *Row = (uint8 *) Buffer->Memory +
        (uint32)MinX* 4 +
        (uint32)MinY * Buffer->Pitch;    
    for (uint32 Y = (uint32)MinY; Y <= (uint32)MaxY; ++Y)
    {
        uint32 *Pixel = (uint32 *)Row;
        for (uint32 X = (uint32)MinX; X <= (uint32)MaxX; ++X)
        {
            *Pixel++ = Color;
        }
        Row +=  Buffer->Pitch;
    }
   
}


extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
    

    DrawRect(0,0,(real32)Buffer->Width,(real32)Buffer->Height,Buffer,0,0,0); // Clear Screen to black

    game_state *Game = (game_state *) Memory->PermanentStorage;

    //NOTE: Gamecode comes here



//NOTE : Shazan 
    // Array[Row][Column]
#define Tile_Map_Count_Y 7
#define Tile_Map_Count_X 15
    uint32 TileMap[Tile_Map_Count_Y][Tile_Map_Count_X] ={
        {1,1,1,1,1,1,1,0,1,1,0,0,1,1,1},
        {1,1,1,1,1,1,1,0,0,0,0,0,1,1,1},
        {1,0,0,0,1,1,1,0,0,0,0,0,1,1,1},
        {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {1,0,0,1,0,0,0,0,0,0,0,0,0,1,1},
        {1,1,1,1,1,1,1,0,0,0,0,0,0,1,1},
        {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}
    };
    // TILEMAP CODE -- Testing
    real32 TileWidth =60.0f;
    real32 TileHeight =60.0f;
    real32 UpperLeftX =6.0f;
    real32 UpperLeftY =6.0f;

    real32 PlayerUpperLeftX = 300.0f;
    real32 PlayerUpperLeftY = 300.0f;
    real32 PlayerR= 0.1f;
    real32 PlayerG= 0.1f;
    real32 PlayerB= 0.5f;
    real32 PlayerWidth= 0.75f*TileWidth;
    real32 PlayerHeight= TileHeight;
    real32 PlayerLeft= PlayerUpperLeftX+Game->PlayerX - 0.5f*PlayerWidth;
    real32 PlayerTop= PlayerUpperLeftY +Game->PlayerY -PlayerHeight;
    


    for(int Controllerindex =0;
        Controllerindex < ArrayCount(Input->Controllers);
        Controllerindex++)
    {
        game_controller_input *Controller = GetController(Input,Controllerindex);
        if(Controller->IsAnalog)
        {
        }
        else{
            real32 dPlayerY = 0.0f;
            real32 dPlayerX = 0.0f;
            if(Controller->MoveUp.EndedDown)
            {
                dPlayerY = -1.0f;
            }
            if(Controller->MoveDown.EndedDown)
            {
                dPlayerY = 1.0f;
            }
            if(Controller->MoveLeft.EndedDown)
            {
                dPlayerX = -1.0f;
            }
            if(Controller->MoveRight.EndedDown)
            {
                dPlayerX = 1.0f;
            }

            dPlayerX *= 128.0f;
            dPlayerY *= 128.0f;

            Game->PlayerX += (Input->dtForFrame*dPlayerX);
            Game->PlayerY += (Input->dtForFrame*dPlayerY);

            int32 PlayerTileX = TruncateReal32Touint32((Game->PlayerX - PlayerUpperLeftX)/TileWidth) ;
            int32 PlayerTileY = TruncateReal32Touint32((Game->PlayerY - PlayerUpperLeftY)/TileHeight) ;    
        }

    }
    
            
    for(int Row= 0;
        Row<Tile_Map_Count_Y;
        Row++)
    {for(int Column =0;
         Column<Tile_Map_Count_X;
         Column++)
        {
            uint32 TileID = TileMap[Row][Column];
            real32 MinX =UpperLeftX+(real32)Column*TileWidth;
            real32 MaxX =MinX + TileWidth;
            real32 MinY =UpperLeftY+(real32)Row*TileHeight;
            real32 MaxY =MinY + TileHeight;
            real32 Gray= 0.5f;
            if(TileID == 1)
            {
                Gray = 1.0f;
            }
            DrawRect(MinX,MinY,MaxX,MaxY,Buffer,Gray,Gray,Gray);
        }
    }
    
    DrawRect(PlayerLeft,PlayerTop,PlayerLeft+PlayerWidth,PlayerTop+PlayerHeight,Buffer,PlayerR,PlayerG,PlayerB);    
}
