/**
* Jacob Troxel
* Functions for Collision Detection
* Mar 6th 2015
*/

#include <cstdio>
#include <cmath>
#include <climits>

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

// Holds Resulting Collision Information
struct CollisionResult_t
{
    float t;
    vec normal;  
};

// Holds Hitbox Information
struct Hitbox
{
    vec *points;
    int size;
};

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

// Finds t1 (or t2) between p1 + t1(p2) and p3 + t2(p4)
float GetIntersection(vec p1, vec p2, vec p3, vec p4)
{
    float Result = 0.0f;
    float Numerator   = ((p4.X - p3.X) * (p1.Y - p3.Y)) - ((p4.Y - p3.Y) * (p1.X - p3.X));
    float Denominator = ((p4.Y - p3.Y) * (p2.X - p1.X)) - ((p4.X - p3.X) * (p2.Y - p1.Y));
    if(Denominator == 0)
    {
        return 1.0f;
    }
    Result = Numerator / Denominator;
    return Result;
}

// Uses dot product to determine if angle between vectors is acute
bool IsAligned(vec X, vec Y)
{
    return((X^Y) >= 0);
}

// Returns orthagonal vector to D on opposite side of dAway
vec GetPerpindicularAwayFrom(vec D, vec dAway)
{
    vec newD;
    newD.Y = D.X;
    newD.X = -D.Y;
    if(IsAligned(newD, dAway))
    {
        newD = -newD;
    }
    return newD;
}

// Returns normalized A
vec Normalize(vec A)
{
    float length = std::sqrt(A.X * A.X + A.Y * A.Y);
    vec Result;
    Result.X = A.X / length;
    Result.Y = A.Y / length;
    return(Result);
}

// Determines if X and Y are on opposite sides of Line
bool AroundLine(vec X, vec Y, vec Line)
{
    float A = Line.Y;
    float B = -Line.X;

    float xResult = A * X.X + B * X.Y;
    float yResult = A * Y.X + B * Y.Y;

    if(xResult == 0 || yResult == 0) return true;
    if(xResult < 0 && yResult > 0) return true;
    if(xResult > 0 && yResult < 0) return true;
    else return false;
}

// Returns index of points in max direction D in A
int FindMax(Hitbox A, vec D)
{
    float maxVal = INT_MIN;
    float maxI = -1;
    for(int i = 0; i < A.size; ++i)
    {
        if((D^A.points[i]) > maxVal)
        {
            maxVal = (D^A.points[i]);
            maxI = i;
        }
    }
    return maxI;
}

// Find the max Minkowski Sum point in direction D
vec Support(Hitbox A, Hitbox B, vec D)
{
    vec Result;
    int indexA = FindMax(A, D);
    int indexB = FindMax(B, -D);
    Result = A.points[indexA] - B.points[indexB];
    return Result;
}

// Converging Collision Detection, calculates min distance
CollisionResult_t GetCollision(Hitbox X, Hitbox Y, vec D)
{
    vec Simplex[2];
    vec A = Support(X,Y,D);
    Simplex[0] = A;
    vec perpD = GetPerpindicularAwayFrom(D, A);
    vec B = Support(X,Y,perpD);
    Simplex[1] = B;
    int count = 0;
    bool infinite = false;
    while(count < (X.size * Y.size))
    {
        vec newDir = GetPerpindicularAwayFrom(Simplex[1] - Simplex[0], -D);
        vec C = Support(X, Y, newDir);
        if(C == Simplex[1] || C == Simplex[0])
        {
            break;
        }
        if(AroundLine(Simplex[0],C,-D))
        {
            Simplex[1] = C;
        }
        else if(AroundLine(Simplex[1],C,-D))
        {
            Simplex[0] = C;
        }
        else //neither line segment intersects, so this guy is infinite.
            //TODO (jtroxel): Check this for robustness
        {
            infinite = true;
            break;
        }
        ++count;
    }
    CollisionResult_t Result;
    if(!infinite) Result.t = GetIntersection(MakeVec(0,0),-D, Simplex[0], Simplex[1]);
    else Result.t = 1.0f;
    Result.normal = Normalize(GetPerpindicularAwayFrom(Simplex[0] - Simplex[1], D));

    return(Result);
}