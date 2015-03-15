#ifndef _GAME_VECTORS_
#define _GAME_VECTORS_

// 2D Vector
struct vec
{
    float X;
    float Y;
};

vec operator*(vec A, float B)
{
    vec Result;
    Result.X = A.X * B;
    Result.Y = A.Y * B;
    return(Result);
}

vec operator-(vec A)
{
    vec Result;
    Result.X = -A.X;
    Result.Y = -A.Y;
    return(Result);
}

vec operator+(vec A, vec B)
{
    vec Result;
    Result.X = A.X + B.X;
    Result.Y = A.Y + B.Y;
    return(Result);
}

bool operator==(vec A, vec B)
{
    return (A.X == B.X && A.Y == B.Y);
}

float operator^(vec A, vec B)
{
    return A.X * B.X + A.Y * B.Y;
}

vec operator-(vec A, vec B)
{
    vec Result;
    Result.X = A.X - B.X;
    Result.Y = A.Y - B.Y;
    return(Result);
}

// Vector init
vec MakeVec(float X, float Y)
{
    vec Result;
    Result.X = X;
    Result.Y = Y;
    return Result;
}

void Print(vec A)
{
    printf("(%f, %f)\n",A.X, A.Y);
}

#endif