#include <windows.h>
#include <cstdint>
#include <cstdio>
#include <iostream>

typedef float real32;
typedef int32_t bool32;

static bool Running;
static BITMAPINFO BitMapInfo;
static void *BitmapMemory;
static int BitmapHeight;
static int BitmapWidth;
static int XOffset;
static int YOffset;
static real32 GlobalPerfCountFrequency;
static bool freeze;
static bool freezeDown;
static bool drawArrays;
static bool realtime;
static bool realtimeDown;

static int PauseColorR;
static int PauseColorG;
static int PauseColorB;

#include <vector>

struct vec
{
    float X;
    float Y;
    float Z;
};

struct PolygonHitBox
{
    std::vector<vec> Points;
};

struct Entity
{
    float X;
    float Y;
    int width;
    int height;
    PolygonHitBox hitbox;
};

static Entity player;
static Entity wall;
static std::vector<vec> Simplex;

vec
MakeVec(float x, float y, float Z = 0.0)
{
    vec Result;
    Result.X = x;
    Result.Y = y;

    return(Result);
}

vec
Cross(vec a, vec b)
{
    vec Result;
    Result.X = (a.Y * b.Z) - (a.Z * b.Y);
    Result.Y = (a.Z * b.X) - (a.X * b.Z);
    Result.Z = (a.X * b.Y) - (a.Y * b.X);

    return(Result);
}

float
Dot(vec a, vec b)
{
    return(a.X * b.X + a.Y * b.Y);
}

float
operator^(vec a, vec b)
{
    return(Dot(a,b));
}

vec
operator-(vec a, vec b)
{
    vec Result;
    Result.X = a.X - b.X;
    Result.Y = a.Y - b.Y;

    return(Result);
}

vec
operator*(vec a, vec b)
{
    return(Cross(a,b));
}

vec
operator-(vec a)
{
    vec Result;
    Result.X = -a.X;
    Result.Y = -a.Y;

    return(Result);
}

vec
FindPoint(vec D, std::vector<vec> shape, int size)
{
	float maxDot = Dot(D, shape[0]);
    int maxIndex = 0;

    for(int i = 0; i < size; ++i)
    {
        float dot = (Dot(D,shape[i]));
        if(dot > maxDot)
        {
            maxDot = dot;
            maxIndex = i;
        }
    }

    return(shape[maxIndex]);
}

vec
Support(vec D, std::vector<vec> shape1, int size1, std::vector<vec> shape2, int size2)
{
    vec A = FindPoint(D, shape1, size1);
    vec B = FindPoint(-D, shape2, size2);

    vec Result = A - B;

    return(Result);
}

bool
DoSimplex(std::vector<vec> *Simplex, vec *D)
{
    vec A = Simplex->at(Simplex->size() - 1);
    vec A0 = -A;
    if(Simplex->size() <= 2)
    {
        vec B = Simplex->at(0);
        vec AB = B - A;
        if((AB^A0) > 0){
            *D = AB*A0*AB;
        }
        else
        {
            Simplex->erase(Simplex->begin());
            *D = A0;
        }
        return(false); // we need 3 points...
    }
    else
    {
        vec B = Simplex->at(1);
        vec C = Simplex->at(0);
        vec AB = B - A;
        vec AC = C - A;
        vec Dir = MakeVec(-AB.Y, AB.X);
        if((Dir^C) > 0)
        {
            Dir = -Dir;
        }
        if((Dir^A0) > 0)
        {
            Simplex->erase(Simplex->begin());
            *D = Dir;
            return(false);
        }

        Dir = MakeVec(-AC.Y, AC.X);
        if((Dir^B) > 0)
        {
            Dir = -Dir;
        }
        if((Dir^A0) > 0)
        {
            Simplex->erase(Simplex->begin()+1);
            *D = Dir;
            return(false);
        }

        return true;
    }
}

std::vector<vec> GetCorners(Entity e)
{
    int XMove = BitmapWidth / 2;
    int YMove = BitmapHeight / 2;
    std::vector<vec> Result;
    Result.push_back(MakeVec(e.X - XMove, -e.Y + YMove));
    Result.push_back(MakeVec(e.X - XMove, -(e.Y +e.height -1) + YMove));
    Result.push_back(MakeVec(e.width -1 + e.X - XMove, -e.Y + YMove));
    Result.push_back(MakeVec(e.width -1 + e.X - XMove, -(e.Y + e.height -1) + YMove));
    return(Result);
}

bool
DoMinkowski(std::vector<vec> shape1, int size1, std::vector<vec> shape2, int size2)
{
    Simplex.clear();
    vec A = Support(MakeVec(-1.0,-1.0), shape1, size1, shape2, size2);
    Simplex.push_back(A);
    vec D = -A;
    while(1 == 1)
    {
        A = Support(D, shape1, size1, shape2, size2);
        //if(A.X == 0.0f || A.Y == 0.0f)// we're aligned with the origin, so we're TOUCHING an object...no collision
        //{
        //   return(false);
        //}
        float A_D = A^D;
        if((A_D) < 0.000001) return(false);
        Simplex.push_back(A);
        if(DoSimplex(&Simplex, &D)) return(true);
    }
}

std::vector<vec> minkowski;
bool keys[256];
std::vector<vec> Pedges;
std::vector<vec> Wedges;

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
RenderEntity(Entity *entity, int color)
{
    for(int i = 0; i < entity->hitbox.Points.size(); ++i)
    {
        DrawDot(entity->hitbox.Points[i].X, entity->hitbox.Points[i].Y, color, color, color);
    }
    /*int Pitch = 4 * BitmapWidth;
    uint8_t *Row = (uint8_t *)BitmapMemory;
    Row += (uint32_t)(Pitch * entity->Y);
    uint32_t *Pixel;
    for(int Y = 0; Y < entity->height; ++Y)
    {
        Pixel = (uint32_t *)Row + (uint32_t)entity->X;
        for(int X = 0; X < entity->width; ++X)
        {
            if(color == 0) *Pixel++ = (255 << 16 | 0 << 8 | 0); 
            else if(color == 1) *Pixel++ = (0 << 16 | 0 << 8 | 255); 
        }
        Row += Pitch;
    }*/
}

static void
RenderSimplex()
{
    int XMove = BitmapWidth / 2;
    int YMove = BitmapHeight / 2;

    for(int i = 0; i < Simplex.size(); ++i)
    {
        int WorldX = XMove + Simplex[i].X;
        int WorldY = YMove - Simplex[i].Y;

        DrawDot(WorldX, WorldY, 255, 0, 255);
    }
}

static void
RenderMinkowski()
{
    int XMove = BitmapWidth / 2;
    int YMove = BitmapHeight / 2;

    for(int i = 0; i < minkowski.size(); ++i)
    {
        int WorldX = XMove + minkowski[i].X;
        int WorldY = YMove - minkowski[i].Y;

        DrawDot(WorldX, WorldY, 0, 255, 0);
    }
}

static void
UpdateMinkowski()
{
    //Pedges = GetCorners(player);
    //Wedges = GetCorners(wall);
    Pedges = player.hitbox.Points;
    Wedges = wall.hitbox.Points;

    minkowski.clear();
    for(int i = 0; i < Pedges.size(); ++i)
    {
        for(int j = 0; j < Wedges.size(); ++j)
        {
            minkowski.push_back(Pedges[i] - Wedges[j]);
        }
    }
}

static void
RenderArray(std::vector<vec> array, int size)
{
    int XMove = BitmapWidth / 2;
    int YMove = BitmapHeight / 2;

    for(int i = 0; i < 4; ++i)
    {
        int WorldX = XMove + array[i].X;
        int WorldY = YMove - array[i].Y;

        DrawDot(WorldX, WorldY, 0, 255, 255);
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
        int multiplier = 1;
        for(int X = 0; X < BitmapWidth; ++X)
        {
            uint8_t Red     = (uint8_t)(0);
            uint8_t Blue    = (uint8_t)(0);
            uint8_t Green   = (uint8_t)(0);
            if(freeze){
                if(X < 20 || X > BitmapWidth - 20 || Y < 20 || Y > BitmapHeight - 20)
                {
                    Red     = (uint8_t)(PauseColorR);
                    Blue    = (uint8_t)(PauseColorB);
                    Green   = (uint8_t)(PauseColorG);
                }
            }

            *Pixel++ = (Red << 16 | Green << 8 | Blue);
        }
        Row += Pitch;
    }

    if(!freeze)
    {
        RenderEntity(&wall, 1);
        RenderEntity(&player, 0);
    }
    if(!realtime)
    {
        RenderMinkowski();
        DrawDot(BitmapWidth/2, BitmapHeight/2, 255, 255, 255);
    }
    if(freeze)
    {
        RenderSimplex();
    }
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
        "GJK_TOOLKIT",
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
            player.X = BitmapWidth / 2;
            player.Y = BitmapHeight / 2;
            player.width = 50;
            player.height = 50;
            player.hitbox.Points.push_back(MakeVec(10,10));
            player.hitbox.Points.push_back(MakeVec(10,50));
            player.hitbox.Points.push_back(MakeVec(50,10));
            player.hitbox.Points.push_back(MakeVec(50,100));            
            player.hitbox.Points.push_back(MakeVec(25,70));

            freeze = false;
            bool DoingMink = false;
            realtimeDown = false;
            freezeDown = false;
            drawArrays = false;
            realtime = false;

            wall.X = BitmapWidth / 2 - 200;
            wall.Y = BitmapHeight / 2 - 100;
            wall.width = 100;
            wall.height = 50;
            wall.hitbox.Points.push_back(MakeVec(10,10));
            wall.hitbox.Points.push_back(MakeVec(10,50));
            wall.hitbox.Points.push_back(MakeVec(50,10));
            wall.hitbox.Points.push_back(MakeVec(50,50));

            int XOffset = 0;
            int YOffset = 0;
            float TargetSecondsPerFrame = 1.0f / (real32)15;
            Running = false;
            LARGE_INTEGER LastCounter = Win32GetWallClock();

            Running = true;
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
                vec Delta;
                Delta.X = 0;
                Delta.Y = 0;
                if(!freeze)
                {
                    if(keys['W'])
                    {
                        Delta.Y -= 5;
                    }
                    if(keys['S'])
                    {
                        Delta.Y += 5;
                    }
                    if(keys['A'])
                    {
                        Delta.X -= 5;
                    }
                    if(keys['D'])
                    {
                        Delta.X += 5;
                    }
                    if(keys[VK_UP])
                    {
                        //wall.Y -= 5;
                    }
                    if(keys[VK_DOWN])
                    {
                        //wall.Y += 5;
                    }
                    if(keys[VK_LEFT])
                    {
                        //wall.X -= 5;
                    }
                    if(keys[VK_RIGHT])
                    {
                        //wall.X += 5;
                    }
                    Simplex.clear();
                }
                if(keys['U'] && !freezeDown)
                {
                    freeze = !freeze;
                    freezeDown = true;
                    PauseColorB = 255;
                    PauseColorG = 255;
                    PauseColorR = 255;
                }
                if(!keys['U'])
                {
                    freezeDown = false;
                }
                if(freeze)
                {
                    if(keys['I'] && !DoingMink)
                    {
                        UpdateMinkowski();
                        Simplex.clear();
                        DoingMink = !DoingMink;
                        if(DoMinkowski(Pedges, 4, Wedges, 4))
                        {
                            PauseColorR = 255;
                            PauseColorG = 0;
                            PauseColorB = 0;
                        }
                        else
                        {
                            PauseColorB = 0;
                            PauseColorG = 255;
                            PauseColorR = 0;   
                        }
                    }
                }
                if(!keys['I']) 
                {
                    DoingMink = false;
                }
                if(keys['O'] && !realtimeDown)
                {
                    realtime = !realtime;
                    realtimeDown = true;
                }
                if(!keys['O'])
                {
                    realtimeDown = false;
                }
                if(realtime)
                {
                    player.X += Delta.X;
                    player.Y += Delta.Y; 
                    UpdateMinkowski();
                    Simplex.clear();
                    bool Collided = DoMinkowski(Pedges, Pedges.size(), Wedges, Wedges.size());
                    if(Collided) // undo that shiz...
                    {
                        player.X += -Delta.X;
                        player.Y += -Delta.Y;  
                    }
                }
                else
                {
                    player.X += Delta.X;
                    player.Y += Delta.Y;
                }

                UpdateMinkowski();
                RenderScreen(XOffset, YOffset);
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