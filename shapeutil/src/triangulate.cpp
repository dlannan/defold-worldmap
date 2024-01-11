
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "triangulate.h"

static const double EPSILON = 0.0000000001f;

double Triangulate::Area(const Vector2dVector &contour)
{

    int n = contour.size();

    double A = 0.0f;

    for (int p = n - 1, q = 0; q < n; p = q++)
    {
        A += contour[p].X * contour[q].Y - contour[q].X * contour[p].Y;
    }
    return A * 0.5f;
}

/*
InsideTriangle decides if a point P is Inside of the triangle
defined by A, B, C.
*/
bool Triangulate::InsideTriangle(double Ax, double Ay,
                                 double Bx, double By,
                                 double Cx, double Cy,
                                 double Px, double Py)

{
    double ax, ay, bx, by, cx, cy, apx, apy, bpx, bpy, cpx, cpy;
    double cCROSSap, bCROSScp, aCROSSbp;

    ax = Cx - Bx;
    ay = Cy - By;
    bx = Ax - Cx;
    by = Ay - Cy;
    cx = Bx - Ax;
    cy = By - Ay;
    apx = Px - Ax;
    apy = Py - Ay;
    bpx = Px - Bx;
    bpy = Py - By;
    cpx = Px - Cx;
    cpy = Py - Cy;

    aCROSSbp = ax * bpy - ay * bpx;
    cCROSSap = cx * apy - cy * apx;
    bCROSScp = bx * cpy - by * cpx;

    return ((aCROSSbp >= 0.0) && (bCROSScp >= 0.0) && (cCROSSap >= 0.0));
};

bool Triangulate::Snip(const Vector2dVector &contour, int u, int v, int w, int n, int *V)
{
    int p;
    double Ax, Ay, Bx, By, Cx, Cy, Px, Py;

    Ax = contour[V[u]].X;
    Ay = contour[V[u]].Y;

    Bx = contour[V[v]].X;
    By = contour[V[v]].Y;

    Cx = contour[V[w]].X;
    Cy = contour[V[w]].Y;

    if (EPSILON > (((Bx - Ax) * (Cy - Ay)) - ((By - Ay) * (Cx - Ax))))
        return false;

    for (p = 0; p < n; p++)
    {
        if ((p == u) || (p == v) || (p == w))
            continue;
        Px = contour[V[p]].X;
        Py = contour[V[p]].Y;
        if (InsideTriangle(Ax, Ay, Bx, By, Cx, Cy, Px, Py))
            return false;
    }

    return true;
}

bool Triangulate::Process(const Vector2dVector &contour, Vector2dVector &result)
{
    /* allocate and initialize list of Vertices in polygon */

    int n = contour.size();
    if (n < 3)
        return false;

    int *V = new int[n];

    /* we want a counter-clockwise polygon in V */

    if (0.0f < Area(contour))
        for (int v = 0; v < n; v++)
            V[v] = v;
    else
        for (int v = 0; v < n; v++)
            V[v] = (n - 1) - v;

    int nv = n;

    /*  remove nv-2 Vertices, creating 1 triangle every time */
    int count = 2 * nv; /* error detection */

    for (int m = 0, v = nv - 1; nv > 2;)
    {
        /* if we loop, it is probably a non-simple polygon */
        if (0 >= (count--))
        {
            //** Triangulate: ERROR - probable bad polygon!
            printf("Triangulate error: %d\n", m);
            return false;
        }

        /* three consecutive vertices in current polygon, <u,v,w> */
        int u = v;
        if (nv <= u)
            u = 0; /* previous */
        v = u + 1;
        if (nv <= v)
            v = 0; /* new v    */
        int w = v + 1;
        if (nv <= w)
            w = 0; /* next     */

        if (Snip(contour, u, v, w, nv, V))
        {
            int a, b, c, s, t;

            /* true names of the vertices */
            a = V[u];
            b = V[v];
            c = V[w];

            /* output Triangle */
            result.push_back(contour[a]);
            result.push_back(contour[b]);
            result.push_back(contour[c]);

            m++;

            /* remove v from remaining polygon */
            for (s = v, t = v + 1; t < nv; s++, t++)
                V[s] = V[t];
            nv--;

            /* resest error detection counter */
            count = 2 * nv;
        }
    }

    delete V;

    return true;
}

void test(int argc, char **argv)
{

    // Small test application demonstrating the usage of the triangulate
    // class.

    // Create a pretty complicated little contour by pushing them onto
    // an stl vector.

    Vector2dVector a;

    a.push_back(Vector2d(0, 6));
    a.push_back(Vector2d(0, 0));
    a.push_back(Vector2d(3, 0));
    a.push_back(Vector2d(4, 1));
    a.push_back(Vector2d(6, 1));
    a.push_back(Vector2d(8, 0));
    a.push_back(Vector2d(12, 0));
    a.push_back(Vector2d(13, 2));
    a.push_back(Vector2d(8, 2));
    a.push_back(Vector2d(8, 4));
    a.push_back(Vector2d(11, 4));
    a.push_back(Vector2d(11, 6));
    a.push_back(Vector2d(6, 6));
    a.push_back(Vector2d(4, 3));
    a.push_back(Vector2d(2, 6));

    // allocate an STL vector to hold the answer.

    Vector2dVector result;

    //  Invoke the triangulator to triangulate this polygon.
    Triangulate::Process(a, result);

    // print out the results.
    int tcount = result.size() / 3;

    for (int i = 0; i < tcount; i++)
    {
        const Vector2d &p1 = result[i * 3 + 0];
        const Vector2d &p2 = result[i * 3 + 1];
        const Vector2d &p3 = result[i * 3 + 2];
        printf("Triangle %d => (%0.0f,%0.0f) (%0.0f,%0.0f) (%0.0f,%0.0f)\n", i + 1, p1.X, p1.Y, p2.X, p2.Y, p3.X, p3.Y);
    }
}