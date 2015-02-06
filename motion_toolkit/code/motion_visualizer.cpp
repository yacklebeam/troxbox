#include <windows.h>
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <vector>
#include <cstdlib>

typedef float real32;
typedef int32_t bool32;

struct vec
{
    float X;
    float Y;
};

vec
operator-(vec a, vec b)
{
    vec Result;
    Result.X = a.X - b.X;
    Result.Y = a.Y - b.Y;

    return(Result);
}

vec
operator*(vec a, float b)
{
    vec Result;
    Result.X = a.X * b;
    Result.Y = a.Y * b;

    return(Result);
}

vec
operator+(vec a, vec b)
{
    vec Result;
    Result.X = a.X + b.X;
    Result.Y = a.Y + b.Y;

    return(Result);
}

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
ConvertToMeters(vec a)
{
    vec Result;
    Result.X = a.X * 60;
    Result.Y = a.Y * 60;
    return(Result);
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