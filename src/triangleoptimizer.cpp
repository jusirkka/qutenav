/* Original copyright:

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
  1. Redistributions of source code must retain the above copyright
     notice, this list of conditions and the following disclaimer.
  2. Redistributions in binary form must reproduce the above copyright
     notice, this list of conditions and the following disclaimer in the
     documentation and/or other materials provided with the distribution.
  3. All advertising materials mentioning features or use of this software
     must display the following acknowledgement:
       This product includes software developed by Brad Grantham and
       Applied Conjecture.
  4. Neither the name Brad Grantham nor Applied Conjecture
     may be used to endorse or promote products derived from this software
     without specific prior written permission.
  5. Notification must be made to Brad Grantham about inclusion of this
     software in a product including the author of the product and the name
     and purpose of the product.  Notification can be made using email
     to Brad Grantham's current address (grantham@plunk.org as of September
     20th, 2000) or current U.S. mail address.

     Adapted to Qt/C++ by Jukka Sirkka 2017

*/


#include "triangleoptimizer.h"
#include <QDebug>

using namespace AC;


TriangleOptimizer::TriangleOptimizer(const IndexVector& triangles)
    : mHonorWinding(false)
    , mVertexCount(0)
{
    // find largest vertex index
    uint mx = 0;
    for (auto v: triangles) {
        if (v > mx) mx = v;
    }

    // init vertices and edges
    mVertices = VertexVector(mx + 1, nullptr);
    // qCDebug(OGL) << "Last Vertex index" << mVertices.size() - 1;
    for (int t = 0; t < triangles.size() / 3; t++) {
        addTriangle(triangles[3*t], triangles[3*t+1], triangles[3*t+2]);
    }

    // init bins
    for (Vertex* v: qAsConst(mVertices)) {
        if (!v) continue;
        if (!mVertexBins.contains(v->count)) {
            v->next = nullptr;
            v->prev = nullptr;
            mVertexBins[v->count] = v;
        } else {
            v->next = mVertexBins[v->count];
            v->prev = nullptr;
            mVertexBins[v->count] = v;
            v->next->prev = v;
        }
    }

    uint v1, v2;
    while (startNextStrip(v1, v2)) {
        // qCDebug(OGL) << "v1 v2 = " << v1 << v2;
        IndexVector strip;
        strip << v1 << v2;
        uint v3;
        while (getNextVert(v1, v2, v3)) {
            // qCDebug(OGL) << "v1 v2 v3 = " << v1 << v2 << v3;
            strip << v3;
            mForwardWinding ? v1 = v3 : v2 = v3;
            mForwardWinding = !mForwardWinding;
        }
        // qCDebug(OGL) << "End of strip";
        mStrips.append(strip);
    }
}


void TriangleOptimizer::addTriangle(uint v1, uint v2, uint v3)
{
    Vertex* vertexRec1 = incVertexValence(v1);
    Vertex* vertexRec2 = incVertexValence(v3);
    Vertex* vertexRec3 = incVertexValence(v2);

    Edge* edge12 = mapVertexEdge(vertexRec1, vertexRec2);
    Edge* edge23 = mapVertexEdge(vertexRec2, vertexRec3);
    Edge* edge31 = mapVertexEdge(vertexRec3, vertexRec1);

    mapEdgeTriangle(edge12, vertexRec3);
    mapEdgeTriangle(edge23, vertexRec1);
    mapEdgeTriangle(edge31, vertexRec2);
}


TriangleOptimizer::Vertex* TriangleOptimizer::incVertexValence(uint v)
{

    if (!mVertices[v]) {
        mVertices[v] = new Vertex;
        // qCDebug(OGL) << "created" << mVertices[v]->index << mVertices[v]->count;
    }

    Vertex *vertex = mVertices[v];
    // qCDebug(OGL) << vertex->index << vertex->count;

    vertex->count++;

    if (vertex->count == 1) {
        vertex->index = v;
        mVertexCount++;
    }

    return vertex;
}

TriangleOptimizer::Edge* TriangleOptimizer::mapVertexEdge(Vertex *v1, Vertex *v2)
{
    for (Edge* e: qAsConst(v1->edges)) {
        if (e->vertices.second == v2) {
            e->count++;
            return e;
        }
    }

    Edge* e = new Edge;

    e->count = 1;
    e->vertices = VertexPair(v1, v2);

    v1->edges.append(e);

    return e;
}

void TriangleOptimizer::mapEdgeTriangle(Edge *e12, Vertex *v3) {
    Triangle* t = new Triangle;
    t->finalVertex = v3;
    e12->triangles.append(t);
}

bool TriangleOptimizer::startNextStrip(uint& v1Return, uint& v2Return) {
    Vertex *v1 = findNextStripVertex();
    if (!v1) return false;

    // qCDebug(OGL) << "start new strip with" << v1->index;
    mForwardWinding = true;

    v1Return = v1->index;
    v2Return = v1->edges.first()->vertices.second->index;

    return true;
}

TriangleOptimizer::Vertex* TriangleOptimizer::findNextStripVertex()
{
    if (mVertexBins.isEmpty()) return nullptr;
    Vertex* v = mVertexBins.first();
    // qCDebug(OGL) << v->index << v->count;
    return v;
}


bool TriangleOptimizer::getNextVert(uint v1, uint v2, uint& v3) {

    Edge *edge;
    bool foundReversed = false;

    if (!findEdge(v1, v2, &edge)) {
        if (mHonorWinding) return false;
        if (!findEdge(v2, v1, &edge)) {
            return false;
        }
        foundReversed = true;
    }

    v3 = edge->triangles.last()->finalVertex->index;
    // qCDebug(OGL) << "num triangles " << edge->triangles.size();

    if (foundReversed) {
        uint tmp = v2;
        v2 = v1;
        v1 = tmp;
    }

    unmapEdgeTriangle(edge, v3);
    unmapEdgeTriangleByVerts(v2, v3, v1);
    unmapEdgeTriangleByVerts(v3, v1, v2);
    unmapVertexEdge(v1, v2);
    unmapVertexEdge(v2, v3);
    unmapVertexEdge(v3, v1);

    decVertexValence(v1);
    decVertexValence(v2);
    decVertexValence(v3);

    return true;

}

bool TriangleOptimizer::findEdge(uint v1, uint v2, Edge **edge)
{

    Vertex* V1 = mVertices[v1];
    if (!V1) return false;

    Vertex* V2 = mVertices[v2];
    if (!V2) return false;

    for (Edge* e: qAsConst(V1->edges)) {
        if (e->vertices.second == V2) {
            *edge = e;
            return true;
        }
    }

    return false;
}

void TriangleOptimizer::unmapEdgeTriangle(Edge* e12, uint v3) {

    Vertex* V3 = mVertices[v3];
    if (!V3) return;

    Triangle* t0 = nullptr;

    for (Triangle* t: qAsConst(e12->triangles)) {
        if (t->finalVertex == V3) {
            t0 = t;
            break;
        }
    }

    if (t0) {
        e12->triangles.removeOne(t0);
        delete t0;
    }
}


void TriangleOptimizer::unmapEdgeTriangleByVerts(uint v1, uint v2, uint v3) {
    Edge *e;

    if (findEdge(v1, v2, &e)) {
        unmapEdgeTriangle(e, v3);
    }
}

void TriangleOptimizer::unmapVertexEdge(uint v1, uint v2) {

    Vertex* V1 = mVertices[v1];
    if (!V1) return;

    Vertex* V2 = mVertices[v2];
    if (!V2) return;


    Edge* e0 = nullptr;

    for (Edge* e: qAsConst(V1->edges)) {
        if (e->vertices.second == V2) {
            e0 = e;
            break;
        }
    }

    if (e0) {
        e0->count--;
        if (e0->count == 0) {
            qDeleteAll(e0->triangles);
            V1->edges.removeOne(e0);
            delete e0;
        }
    }
}

void TriangleOptimizer::decVertexValence(uint v) {
    Vertex* V = mVertices[v];
    if (!V) return;

    if (!V->prev) {
        // first
        mVertexBins[V->count] = V->next;
        if (!V->next) {
            // last vertex in the bin - remove the bin
            mVertexBins.remove(V->count);
        }
    } else {
        // not first
        // qCDebug(OGL) << "still left in bin" << V->count << ": " << V->prev->index;
        V->prev->next = V->next;
    }

    if (V->next) { // not last
        // qCDebug(OGL) << "still left in bin" << V->count << ": " << V->next->index;
        V->next->prev = V->prev;
    }


    V->count--;

    if (V->count == 0) {
        mVertexCount--;
        qDeleteAll(V->edges);
        delete V;
        mVertices[v] = nullptr;
    } else {
        if (!mVertexBins.contains(V->count)) {
            V->next = nullptr;
            V->prev = nullptr;
            mVertexBins[V->count] = V;
        } else {
            V->next = mVertexBins[V->count];
            V->prev = nullptr;
            mVertexBins[V->count] = V;
            V->next->prev = V;
        }
    }
}



