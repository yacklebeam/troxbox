/*
NEXT:
- collision corner problem when flush with wall, moving "out" and back "in"
- moving camera possibilities when near edge
- rendering:
    - BMPs
    - more robust edge casing, so that objects can be partially off screen
- Collide(e1,e2) - switch statement vs objects
    - what do i gain from using classes
    - what do i gain from not?
*/


#include <windows.h>
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <vector>
#include <cstdlib>
#include "collision_engine.cpp"
#include "game_entity.cpp"

#define PIXELS_PER_METER 30 //rougly 10 pixels = 1 ft
#define CAMERA_MARGIN 300
#define FRAMES_PER_SECOND 30

typedef float real32;
typedef int32_t bool32;

struct Entity
{
    vec Position;
    vec Velocity;
    vec Acceleration;
    int height;
    int width;
    Hitbox hitbox;
    CollisionType type;
};

static bool Running;
static BITMAPINFO BitMapInfo;
static void *BitmapMemory;
static int BitmapHeight;
static int BitmapWidth;
static int XOffset;
static int YOffset;
static real32 GlobalPerfCountFrequency;
bool keys[256];

//TODO (jtroxel): turn this into a world struct
static int WorldXMin;
static int WorldXMax;
static int WorldYMin;
static int WorldYMax;

//TODO (jtroxel): create camera struct
static float CameraXOffset;
static float CameraYOffset;

static vec
MetersToPixels(vec A)
{
    vec Result;
    Result = A * (float)PIXELS_PER_METER;
    return(Result);
}

float
Minimum(float A, float B)
{
    return (A<B)?A:B;
}

void GetHitbox(vec *points, vec center, int width, int height, bool isPlayer = false)
{
    if(isPlayer)
    {
        points[0].X = center.X;
        points[0].Y = center.Y + height - 20;

        points[1].X = center.X + width;
        points[1].Y = center.Y + height - 20;
    }
    else
    {
        points[0].X = center.X;
        points[0].Y = center.Y;

        points[1].X = center.X + width;
        points[1].Y = center.Y;
    }

    points[2].X = center.X;
    points[2].Y = center.Y + height;

    points[3].X = center.X + width;
    points[3].Y = center.Y + height;
}

static void
RenderEntity(Entity *entity, uint8_t R, uint8_t G, uint8_t B)
{
    float RenderXPos = entity->Position.X - CameraXOffset;
    float RenderYPos = entity->Position.Y - CameraYOffset;
    float YStart = Maximum(RenderYPos, 0.0f);
    float YEnd = Minimum(RenderYPos + entity->height, (float)BitmapHeight);
    float XStart = Maximum(RenderXPos, 0.0f);
    float XEnd = Minimum(RenderXPos + entity->width, (float)BitmapWidth);
    int Pitch = 4 * BitmapWidth;
    uint8_t *Row = (uint8_t *)BitmapMemory;
    Row += (uint32_t)(Pitch * (uint32_t)YStart);
    uint32_t *Pixel;
    for(int Y = YStart; Y < YEnd; ++Y)
    {
        Pixel = (uint32_t *)Row + (uint32_t)XStart;
        for(int X = XStart; X < XEnd; ++X)
        {
           *Pixel++ = (R << 16 | G << 8 | B); 
        }
        Row += Pitch;
    }
}

static void
DrawDot(int WorldX, int WorldY, int R, int G, int B)
{
    int Pitch = 4 * BitmapWidth;
    uint8_t *Row = (uint8_t *)BitmapMemory;
    Row += (uint32_t)(Pitch * (WorldY - 2));
    uint32_t *Pixel;
    for(int Y = 0; Y < 5; ++Y)
    {
        Pixel = (uint32_t *)Row + (uint32_t)(WorldX - 2);
        for(int X = 0; X < 5; ++X)
        {
            *Pixel++ = (R << 16 | G << 8 | B); 
        }
        Row += Pitch;
    }
}

static void
RenderScreen(int XOffset, int YOffset)
{
    int Pitch = 4 * BitmapWidth;
    uint8_t *Row = (uint8_t *)BitmapMemory;
    for(int Y = 0; Y < BitmapHeight; ++Y)
    {
        uint32_t *Pixel = (uint32_t *)Row;
        for(int X = 0; X < BitmapWidth; ++X)
        {
            uint8_t Red     = (uint8_t)(0);
            uint8_t Blue    = (uint8_t)(0);
            uint8_t Green   = (uint8_t)(0);

            *Pixel++ = (Red << 16 | Green << 8 | Blue);
        }
        Row += Pitch;
    }
}

//TODO (jtroxel): MOVE this code to the physics engine code

void
MoveEntity(Entity *e, std::vector<Entity> entities, vec Delta)
{
    float tRem = 1.0f;
    vec TrueDelta = MetersToPixels(Delta);
    while(tRem > 0.0f)
    {
        float tCalc = tRem;
        CollisionResult_t finalResult;
        for(int i = 0; i < entities.size(); ++i)
        {
            CollisionResult_t Result = GetCollision(e->hitbox, entities[i].hitbox, TrueDelta);
            switch(entities[i].type)
            {
                case COLLISION_TYPE_COLLIDE:
                {
                    if(Result.t < tCalc && Result.t >= 0.0f)
                    {
                        tCalc = Result.t;
                        finalResult = Result;
                    }
                }
                case COLLISION_TYPE_INTERSECT://assuming all collides are before intersects...
                //TODO (jtroxel): correct this behavior for general case
                {
                    if(Result.t < tCalc && Result.t >= 0)
                    {
                        //e->Velocity = e->Velocity * 2.5;
                    }
                }
            }
        }
        if(tCalc < 0.0001f)
        {
            e->Position = e->Position;
            TrueDelta = TrueDelta - finalResult.normal * (TrueDelta^finalResult.normal);
            tRem -= 0.2; //5ish bounces max, each time we lose some momentum 
        }
        else if(tCalc < 1.0f)
        {
            e->Position = e->Position + TrueDelta * (tCalc);
            TrueDelta = TrueDelta - finalResult.normal * (TrueDelta^finalResult.normal);
            tRem -= 0.2; //5ish bounces max, each time we lose some momentum 
        }
        else
        {
            e->Position = e->Position + TrueDelta;
        }
        /*float Epsilon = 0.00001f;
        if(tCalc < 1.0) // collision
        {
            float xDelta = 0.0f;
            float yDelta = 0.0f;
            if(TrueDelta.X >= Epsilon || TrueDelta.X <= -Epsilon)
            {
                if(TrueDelta.X < 0) xDelta = (TrueDelta.X * tCalc) + Epsilon;
                else xDelta = (TrueDelta.X * tCalc) - Epsilon;
            }
            if(TrueDelta.Y >= Epsilon || TrueDelta.Y <= -Epsilon)
            {
                if(TrueDelta.Y < 0) yDelta = (TrueDelta.Y * tCalc) + Epsilon;
                else yDelta = (TrueDelta.Y * tCalc) - Epsilon;
            }
            e->Position = e->Position + MakeVec(xDelta, yDelta);
            TrueDelta = TrueDelta - finalResult.normal * (TrueDelta^finalResult.normal);
            tRem -= 0.2; //5ish bounces max, each time we lose some momentum
        }
        else
        {
            float xDelta = 0.0f;
            float yDelta = 0.0f;
            if(TrueDelta.X >= Epsilon || TrueDelta.X <= -Epsilon)
            {
                if(TrueDelta.X < 0) xDelta = (TrueDelta.X * tCalc) + Epsilon;
                else xDelta = (TrueDelta.X * tCalc) - Epsilon;
            }
            if(TrueDelta.Y >= Epsilon || TrueDelta.Y <= -Epsilon)
            {
                if(TrueDelta.Y < 0) yDelta = (TrueDelta.Y * tCalc) + Epsilon;
                else yDelta = (TrueDelta.Y * tCalc) - Epsilon;
            }
            e->Position = e->Position + MakeVec(xDelta, yDelta);
        }*/
        tRem -= tCalc;

    }
    GetHitbox(e->hitbox.points, e->Position, e->width, e->height, true);
}

static void
UpdateEntity(Entity *e, std::vector<Entity> entities, float dt)
{
    float accLength = e->Acceleration.X * e->Acceleration.X + e->Acceleration.Y * e->Acceleration.Y;
    if(accLength > 1.0f)
    {
        e->Acceleration.X *= (1.0f / sqrt(accLength));
        e->Acceleration.Y *= (1.0f / sqrt(accLength));
    }
    e->Acceleration = e->Acceleration * 100.0f;
    e->Acceleration = e->Acceleration + (e->Velocity * -8.0f);
    vec Delta = e->Acceleration * (0.5f * dt * dt) + e->Velocity * dt;
    MoveEntity(e, entities, Delta);
    e->Velocity = e->Acceleration * dt + e->Velocity;

    if(abs(e->Velocity.X) < 0.01f)
    {
        e->Velocity.X = 0.0f;
        e->Acceleration.X = 0.0f;
    }
    if(abs(e->Velocity.Y) < 0.01f)
    {
        e->Velocity.Y = 0.0f;
        e->Acceleration.Y = 0.0f;
    }

    //if(e->Position.X < 0) e->Position.X = 0;
    //if(e->Position.Y < 0) e->Position.Y = 0;
    //if(e->Position.X > BitmapWidth - e->width) e->Position.X = BitmapWidth - e->width;
    //if(e->Position.Y > BitmapHeight - e->height) e->Position.Y = BitmapHeight - e->height;
}

inline LARGE_INTEGER
Win32GetWallClock(void)
{    
    LARGE_INTEGER Result;
    QueryPerformanceCounter(&Result);
    return(Result);
}

inline real32
Win32GetSecondsElapsed(LARGE_INTEGER Start, LARGE_INTEGER End)
{
    real32 Result = ((real32)(End.QuadPart - Start.QuadPart) /
                     (real32)GlobalPerfCountFrequency);
    return(Result);
}

static void
Win32ResizeDIBSection(int Width, int Height)
{
    OutputDebugString("RESIZING\n");
    if(BitmapMemory)
    {
        VirtualFree(BitmapMemory, 0, MEM_RELEASE);
    }

    BitmapWidth = Width;
    BitmapHeight = Height;

    BitMapInfo.bmiHeader.biSize = sizeof(BitMapInfo.bmiHeader);
    BitMapInfo.bmiHeader.biWidth = Width;
    BitMapInfo.bmiHeader.biHeight = -Height;
    BitMapInfo.bmiHeader.biPlanes = 1;
    BitMapInfo.bmiHeader.biBitCount = 32;
    BitMapInfo.bmiHeader.biCompression = BI_RGB;

    int BitmapMemorySize = 4 * BitmapWidth * BitmapHeight;
    BitmapMemory = VirtualAlloc(0, BitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);

    RenderScreen(XOffset, YOffset);
}

static void
Win32UpdateWindow(HDC DeviceContext, RECT *WindowRect, int X, int Y, int Width, int Height)
{
    int WindowWidth = WindowRect->right - WindowRect->left;
    int WindowHeight = WindowRect->bottom - WindowRect->top;
    StretchDIBits(DeviceContext,
                0, 0, BitmapWidth, BitmapHeight,
                0, 0, WindowWidth, WindowHeight,
                BitmapMemory,
                &BitMapInfo,
                DIB_RGB_COLORS, SRCCOPY);
}

LRESULT CALLBACK
WndProc(HWND WindowHandle,
        UINT WindowMessage,
        WPARAM WParam,
        LPARAM LParam)
{
    LRESULT Result = 0;

    switch(WindowMessage)
    {
        case WM_CLOSE:
        {
            Running = false;
        } break;
        case WM_DESTROY:
        {
            Running = false;
        } break;
        case WM_SIZE:
        {
            RECT ClientRect;
            GetClientRect(WindowHandle, &ClientRect);
            int Width = ClientRect.right - ClientRect.left;
            int Height = ClientRect.bottom - ClientRect.top;
            Win32ResizeDIBSection(Width, Height);
        } break;
        case WM_ACTIVATEAPP:
        {
        } break;
        case WM_KEYDOWN:
        {
            keys[WParam] = TRUE;                    
        } break;
        case WM_KEYUP:
        {
            keys[WParam] = FALSE;                      
        } break;
        case WM_PAINT:
        {
            PAINTSTRUCT Paint;
            HDC DeviceContext = BeginPaint(WindowHandle, &Paint);
            int X = Paint.rcPaint.left;
            int Y = Paint.rcPaint.right;
            int Width = Paint.rcPaint.right - Paint.rcPaint.left;
            int Height = Paint.rcPaint.bottom - Paint.rcPaint.top;
            RECT ClientRect;
            GetClientRect(WindowHandle, &ClientRect);
            Win32UpdateWindow(DeviceContext, &ClientRect, X, Y, Width, Height);
            EndPaint(WindowHandle, &Paint);
        } break;
        default:
        {
            Result = DefWindowProc(WindowHandle, WindowMessage, WParam, LParam);
        } break;
    }
    return(Result);
}

int WINAPI
WinMain(HINSTANCE hInstance,
        HINSTANCE hPrevInstance,
        LPSTR lpCmdLine,
        int nCmdShow)
{
    WNDCLASS WindowClass = {};
    WindowClass.style         = CS_OWNDC|CS_HREDRAW|CS_VREDRAW;
    WindowClass.lpfnWndProc   = WndProc;
    WindowClass.hInstance     = hInstance;
    WindowClass.lpszClassName = "WindowTestWindowClass";

    LARGE_INTEGER PerfCountFrequencyResult;
    QueryPerformanceFrequency(&PerfCountFrequencyResult);
    GlobalPerfCountFrequency = PerfCountFrequencyResult.QuadPart;

    UINT DesiredSchedulerMS = 1;
    bool32 SleepIsGranular = (timeBeginPeriod(DesiredSchedulerMS) == TIMERR_NOERROR);

    if(RegisterClassA(&WindowClass))
    {
        HWND WindowHandle = CreateWindowEx(
        0,
        WindowClass.lpszClassName,
        "MAZE ESCAPE",
        WS_OVERLAPPEDWINDOW|WS_VISIBLE,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        400,
        400,
        0,
        0,
        hInstance,
        0);

        if(WindowHandle)
        {
            WorldXMin = 0;
            WorldYMin = 0;
            WorldXMax = BitmapWidth;
            WorldYMax = BitmapHeight;

            CameraXOffset = 0.0f;
            CameraYOffset = 0.0f;

            int XOffset = 0;
            int YOffset = 0;
            int DesiredFramesPerSecond = FRAMES_PER_SECOND;
            float TargetSecondsPerFrame = 1.0f / (real32)DesiredFramesPerSecond;
            Running = true;
            bool notDown = true;

            Entity player;
            player.Position.X = 61;
            player.Position.Y = 61;
            player.width = 40;
            player.height = 60;
            player.Velocity.X = 0;
            player.Velocity.Y = 0;
            player.Acceleration.X = 0;
            player.Acceleration.Y = 0;
            player.hitbox.size = 4;
            player.hitbox.points = new vec[4];
            GetHitbox(player.hitbox.points, player.Position, 40, 60, true);

            std::vector<Entity> entities;
            int tileColCount = 20;
            int tileRowCount = 20;
            int Map[400] = {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
                            1,0,1,0,0,0,0,0,0,1,0,0,0,0,0,1,0,0,0,1,
                            1,0,1,0,1,1,1,1,1,1,0,1,1,1,0,1,0,1,0,1,
                            1,0,1,0,1,0,0,0,0,0,0,1,0,1,0,1,0,1,0,1,
                            1,0,0,0,1,0,1,0,1,1,0,1,0,1,0,1,0,1,0,1,
                            1,0,1,0,1,0,1,0,1,1,0,1,0,1,0,0,0,1,0,1,
                            1,0,1,0,1,0,1,0,0,0,0,1,0,1,1,1,1,1,0,1,
                            1,0,1,0,1,0,1,1,1,1,1,1,0,0,0,0,0,0,0,1,
                            1,0,1,0,1,0,0,0,0,0,0,0,1,0,1,1,1,1,1,1,
                            1,0,1,0,1,1,1,1,1,0,1,0,1,0,1,0,0,0,1,1,
                            1,0,1,0,1,0,0,0,1,0,1,0,1,0,1,0,1,0,1,1,
                            1,0,1,0,0,0,1,0,1,0,1,0,1,0,1,0,1,0,1,1,
                            1,0,1,1,1,1,1,1,1,0,1,0,1,0,1,0,1,0,0,1,
                            1,0,0,0,0,0,0,0,0,0,1,0,1,0,1,0,1,1,0,1,
                            1,0,1,0,1,1,1,1,1,0,1,0,1,0,0,0,0,1,0,1,
                            1,0,1,0,1,1,0,0,0,0,1,1,1,1,1,1,0,1,0,1,
                            1,0,1,0,0,0,0,1,1,0,0,0,0,0,0,1,0,1,0,1,
                            1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1,1,1,
                            1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,
                            1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,};

            for(int i = 0; i < 400; ++i)
            {
                switch(Map[i])
                {
                    case 1:
                    {
                        Entity *ent = new Entity();
                        int Row = i / tileRowCount;
                        int Col = i % tileColCount;
                        ent->Position.X = 60 * Col;
                        ent->Position.Y = 60 * Row;
                        ent->width = 60;
                        ent->height = 60;
                        ent->hitbox.size = 4;
                        ent->hitbox.points = new vec[4];
                        ent->type = COLLISION_TYPE_COLLIDE;
                        GetHitbox(ent->hitbox.points, ent->Position, 60, 60);
                        entities.push_back(*ent);
                        break;
                    }
                    case 2:
                    {
                        Entity *ent = new Entity();
                        int Row = i / tileRowCount;
                        int Col = i % tileColCount;
                        ent->Position.X = 60 * Col;
                        ent->Position.Y = 60 * Row;
                        ent->width = 60;
                        ent->height = 60;
                        ent->hitbox.size = 4;
                        ent->hitbox.points = new vec[4];
                        ent->type = COLLISION_TYPE_INTERSECT;
                        GetHitbox(ent->hitbox.points, ent->Position, 60, 60);
                        entities.push_back(*ent);
                        break;
                    }
                }
            }

            uint8_t PlayerRed = 0;
            uint8_t PlayerGreen = 255;
            uint8_t PlayerBlue = 0;

            LARGE_INTEGER LastCounter = Win32GetWallClock();
            while(Running)
            {
                MSG Message;
                while(PeekMessage(&Message, 0, 0, 0, PM_REMOVE))
                {
                    if(Message.message == WM_QUIT)
                    {
                        Running = false;
                    }
                    TranslateMessage(&Message);
                    DispatchMessage(&Message);
                }

                if(keys['Q'])
                {
                    Running = false;
                }

                player.Acceleration.X = 0.0f;
                player.Acceleration.Y = 0.0f;

                if(keys['W'])
                {
                    player.Acceleration.Y = -1.0f;
                }
                if(keys['A'])
                {
                    player.Acceleration.X = -1.0f;
                }
                if(keys['S'])
                {
                    player.Acceleration.Y = 1.0f;
                }
                if(keys['D'])
                {
                    player.Acceleration.X = 1.0f;                    
                }

                UpdateEntity(&player, entities, TargetSecondsPerFrame);
                bool FixedCamera = true;
                if(FixedCamera)
                {
                    CameraXOffset = player.Position.X - (BitmapWidth / 2) + (player.width / 2);
                    CameraYOffset = player.Position.Y - (BitmapHeight / 2) + (player.height / 2);
                }
                else
                {
                    if(player.Position.X + player.width > BitmapWidth - CAMERA_MARGIN + CameraXOffset)
                    {
                        CameraXOffset += (player.Position.X + player.width - (BitmapWidth - CAMERA_MARGIN + CameraXOffset));
                    }               
                    if(player.Position.X < CAMERA_MARGIN + CameraXOffset)
                    {
                        CameraXOffset -= (CAMERA_MARGIN + CameraXOffset - player.Position.X);
                    }               
                    if(player.Position.Y + player.height> BitmapHeight - CAMERA_MARGIN + CameraYOffset)
                    {
                        CameraYOffset += (player.Position.Y + player.height - (BitmapHeight - CAMERA_MARGIN + CameraYOffset));
                    }               
                    if(player.Position.Y < CAMERA_MARGIN + CameraYOffset)
                    {
                        CameraYOffset -= (CAMERA_MARGIN + CameraYOffset - player.Position.Y);
                    }
                }
                RenderScreen(XOffset, YOffset);

                for(int i = 0; i < entities.size(); ++i)
                {
                    if(entities[i].type == COLLISION_TYPE_COLLIDE)
                    {
                        RenderEntity(&entities[i], 0, 0, 255);
                    }
                    else if(entities[i].type == COLLISION_TYPE_INTERSECT)
                    {
                        RenderEntity(&entities[i], 255, 0, 0);
                    }
                    else
                    {
                        RenderEntity(&entities[i], 255, 0, 255);
                    }
                }
                RenderEntity(&player, PlayerRed, PlayerGreen, PlayerBlue);
                HDC DeviceContext = GetDC(WindowHandle);
                RECT WindowRect;
                GetClientRect(WindowHandle, &WindowRect);
                int WindowWidth = WindowRect.right - WindowRect.left;
                int WindowHeight = WindowRect.bottom - WindowRect.top;
                Win32UpdateWindow(DeviceContext, &WindowRect, 0, 0, WindowWidth, WindowHeight);
                ReleaseDC(WindowHandle, DeviceContext);

                LARGE_INTEGER WorkCounter = Win32GetWallClock();
                real32 WorkSecondsElapsed = Win32GetSecondsElapsed(LastCounter, WorkCounter);
                real32 SecondsElapsedForFrame = WorkSecondsElapsed;
                if(SecondsElapsedForFrame < TargetSecondsPerFrame)
                {                        
                    if(SleepIsGranular)
                    {
                        DWORD SleepMS = (DWORD)(1000.0f * (TargetSecondsPerFrame -
                                                           SecondsElapsedForFrame));
                        if(SleepMS > 0)
                        {
                            Sleep(SleepMS);
                        }
                    }
                
                    real32 TestSecondsElapsedForFrame = Win32GetSecondsElapsed(LastCounter, Win32GetWallClock());
                
                    while(SecondsElapsedForFrame < TargetSecondsPerFrame)
                    {                            
                        SecondsElapsedForFrame = Win32GetSecondsElapsed(LastCounter, Win32GetWallClock());
                    }
                }

                LARGE_INTEGER EndCounter = Win32GetWallClock();
                real32 MSPerFrame = 1000.0f*Win32GetSecondsElapsed(LastCounter, EndCounter);                    
                LastCounter = EndCounter;
            }
        }
        else
        {
            MessageBox(NULL, "WindowHandle Creation Failed!", "Error!", MB_ICONEXCLAMATION|MB_OK);
            return 0;
        }
    }
    else
    {
        MessageBox(NULL, "WindowHandle Registration Failed!", "Error!", MB_ICONEXCLAMATION|MB_OK);
        return 0;
    }
    return(0);
}