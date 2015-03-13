#include <windows.h>
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <vector>
#include <cstdlib>
#include "collision_engine.cpp"

typedef float real32;
typedef int32_t bool32;

struct Entity
{
    vec Position;
    vec Velocity;
    vec Acceleration;
    int height;
    int width;
    bool Simple;
    Hitbox hitbox;
};

static std::vector<vec> Simplex;
static bool Running;
static BITMAPINFO BitMapInfo;
static void *BitmapMemory;
static int BitmapHeight;
static int BitmapWidth;
static int XOffset;
static int YOffset;
static real32 GlobalPerfCountFrequency;
bool keys[256];

static vec
MetersToPixels(vec a)
{
    vec Result;
    Result.X = a.X * 60;
    Result.Y = a.Y * 60;
    return(Result);
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
    int Pitch = 4 * BitmapWidth;
    uint8_t *Row = (uint8_t *)BitmapMemory;
    Row += (uint32_t)(Pitch * (uint32_t)entity->Position.Y);
    uint32_t *Pixel;
    for(int Y = 0; Y < entity->height; ++Y)
    {
        Pixel = (uint32_t *)Row + (uint32_t)entity->Position.X;
        for(int X = 0; X < entity->width; ++X)
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

void
MoveEntity(Entity *e, std::vector<Entity> walls, vec Delta)
{
    float tRem = 1.0f;
    vec TrueDelta = MetersToPixels(Delta);
    while(tRem > 0.0f)
    {
        float tCalc = tRem;
        CollisionResult_t finalResult;
        for(int i = 0; i < walls.size(); ++i)
        {
            CollisionResult_t Result = GetCollision(e->hitbox, walls[i].hitbox, TrueDelta);
            if(Result.t < tCalc && Result.t >= 0)
            {
                tCalc = Result.t;
                finalResult = Result;
            }
        }
        if(tCalc < 1.0) // collision
        {
            e->Position = e->Position + (TrueDelta * tCalc);
            TrueDelta = TrueDelta - finalResult.normal * (TrueDelta^finalResult.normal);
            tRem -= 0.2; //5ish bounces max, each time we lose some momentum
        }
        else
        {
            e->Position = e->Position + TrueDelta;
        }
        tRem -= tCalc;
    }
    GetHitbox(e->hitbox.points, e->Position, 40, 60, true);
}

static void
UpdateEntity(Entity *e, std::vector<Entity> walls, float dt)
{
    float accLength = e->Acceleration.X * e->Acceleration.X + e->Acceleration.Y * e->Acceleration.Y;
    if(accLength > 1.0f)
    {
        e->Acceleration.X *= (1.0f / sqrt(accLength));
        e->Acceleration.Y *= (1.0f / sqrt(accLength));
    }
    e->Acceleration = e->Acceleration * 70.0f;
    e->Acceleration = e->Acceleration + (e->Velocity * -8.0f);
    vec Delta = e->Acceleration * (0.5f * dt * dt) + e->Velocity * dt;
    MoveEntity(e, walls, Delta);
    e->Velocity = e->Acceleration * dt + e->Velocity;

    if(abs(e->Velocity.X) < 0.001) e->Velocity.X = 0;
    if(abs(e->Velocity.Y) < 0.001) e->Velocity.Y = 0;      

    if(e->Position.X < 0) e->Position.X = 0;
    if(e->Position.Y < 0) e->Position.Y = 0;
    if(e->Position.X > BitmapWidth - e->width) e->Position.X = BitmapWidth - e->width;
    if(e->Position.Y > BitmapHeight - e->height) e->Position.Y = BitmapHeight - e->height;
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
        "MOTION_VISUALIZER",
        WS_OVERLAPPEDWINDOW|WS_VISIBLE,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        0,
        0,
        hInstance,
        0);

        if(WindowHandle)
        {
            int XOffset = 0;
            int YOffset = 0;
            int DesiredFramesPerSecond = 30;
            float TargetSecondsPerFrame = 1.0f / (real32)DesiredFramesPerSecond;
            Running = true;
            bool notDown = true;

            Entity player;
            player.Position.X = 80 + (BitmapWidth / 2);
            player.Position.Y = BitmapHeight / 2;
            player.width = 40;
            player.height = 60;
            player.Simple = true;
            player.Velocity.X = 0;
            player.Velocity.Y = 0;
            player.Acceleration.X = 0;
            player.Acceleration.Y = 0;
            player.hitbox.size = 4;
            player.hitbox.points = new vec[4];
            GetHitbox(player.hitbox.points, player.Position, 40, 60, true);

            std::vector<Entity> walls;
            Entity topWall;
            topWall.Position = MakeVec(0.0f, 0.0f);
            topWall.width = BitmapWidth;
            topWall.height = 20;
            topWall.hitbox.size = 4;
            topWall.hitbox.points = new vec[4];
            GetHitbox(topWall.hitbox.points, topWall.Position, BitmapWidth, 20);

            Entity bottomWall;
            bottomWall.Position = MakeVec(0.0f, (float)(BitmapHeight - 20));
            bottomWall.width = BitmapWidth;
            bottomWall.height = 20;
            bottomWall.hitbox.size = 4;
            bottomWall.hitbox.points = new vec[4];
            GetHitbox(bottomWall.hitbox.points, bottomWall.Position, BitmapWidth, 20);
            
            Entity leftWall;
            leftWall.Position = MakeVec(0.0f, 0.0f);
            leftWall.width = 20;
            leftWall.height = BitmapHeight;
            leftWall.hitbox.size = 4;
            leftWall.hitbox.points = new vec[4];
            GetHitbox(leftWall.hitbox.points, leftWall.Position, 20, BitmapHeight);
            
            Entity rightWall;
            rightWall.Position = MakeVec((float)(BitmapWidth - 20), 0.0f);
            rightWall.width = 20;
            rightWall.height = BitmapHeight;
            rightWall.hitbox.size = 4;
            rightWall.hitbox.points = new vec[4];
            GetHitbox(rightWall.hitbox.points, rightWall.Position, 20, BitmapHeight);
            
            Entity middleWallTop;
            middleWallTop.Position = MakeVec((float)((BitmapWidth / 2) - 10), 20.0f);
            middleWallTop.width = 20;
            middleWallTop.height = (BitmapHeight / 2) - 40;
            middleWallTop.hitbox.size = 4;
            middleWallTop.hitbox.points = new vec[4];
            GetHitbox(middleWallTop.hitbox.points, middleWallTop.Position, 20, (BitmapHeight / 2) - 40);

            Entity middleWallBot;
            middleWallBot.Position = MakeVec((float)((BitmapWidth / 2) - 10), (BitmapHeight / 2) + 40);
            middleWallBot.width = 20;
            middleWallBot.height = (BitmapHeight / 2) - 40;
            middleWallBot.hitbox.size = 4;
            middleWallBot.hitbox.points = new vec[4];
            GetHitbox(middleWallBot.hitbox.points, middleWallBot.Position, 20, (BitmapHeight / 2) - 40);

            walls.push_back(topWall);
            walls.push_back(bottomWall);
            walls.push_back(leftWall);
            walls.push_back(rightWall);
            walls.push_back(middleWallTop);
            walls.push_back(middleWallBot);

            int xVals[5] = {150,300,450,600,750};
            int yVals[5] = {300,150,300,150,300};

            Entity newWalls[5];

            for(int i = 0; i < 5; ++i)
            {
                newWalls[i].Position = MakeVec(xVals[i], yVals[i]);
                newWalls[i].width = 40;
                newWalls[i].height = 40;
                newWalls[i].hitbox.size = 4;
                newWalls[i].hitbox.points = new vec[4];
                GetHitbox(newWalls[i].hitbox.points, newWalls[i].Position, 40, 40);
                walls.push_back(newWalls[i]);
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

                UpdateEntity(&player, walls, TargetSecondsPerFrame);
                RenderScreen(XOffset, YOffset);
                for(int i = 0; i < walls.size(); ++i)
                {
                    RenderEntity(&walls[i], 0, 0, 255);
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