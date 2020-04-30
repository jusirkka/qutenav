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

#pragma once

#include <QVector>
#include <QPair>
#include <QMap>

#include <GL/gl.h>

namespace AC {

class TriangleOptimizer {

public:

    using IndexVector = QVector<GLuint>;
    using StripVector = QVector<IndexVector>;

    const StripVector& strips() const {return mStrips;}

    TriangleOptimizer(const IndexVector& triangles);
    TriangleOptimizer() = default;

private:

    class Vertex;

    class Triangle {
    public:
        Triangle()
            : finalVertex(nullptr) {}
        Vertex* finalVertex;
    };

    using TriangleVector = QVector<Triangle*>;
    using VertexPair = QPair<Vertex*, Vertex*>;

    class Edge {
    public:
        Edge()
            : vertices()
            , triangles()
            , count(0) {}
        VertexPair vertices;
        TriangleVector triangles;
        int count;
    };

    using EdgeVector = QVector<Edge*>;

    class Vertex {
    public:
        Vertex()
            : index(0)
            , count(0)
            , prev(nullptr)
            , next(nullptr)
            , edges() {}
        uint index;
        int count;
        Vertex* prev;
        Vertex* next;
        EdgeVector edges;
    };

    using VertexMap = QMap<uint, Vertex*>;
    using VertexVector = QVector<Vertex*>;

private:

    void addTriangle(uint v1, uint v2, uint v3);
    bool startNextStrip(uint& v1Return, uint& v2Return);
    bool getNextVert(uint v1, uint v2, uint& v3);

    Vertex* incVertexValence(uint v);
    Edge* mapVertexEdge(Vertex* v1, Vertex* v2);
    void mapEdgeTriangle(Edge* e12, Vertex* v3);
    Vertex* findNextStripVertex();
    bool findEdge(uint v1, uint v2, Edge** e);
    void unmapEdgeTriangle(Edge* e12, uint v3);
    void unmapEdgeTriangleByVerts(uint v1, uint v2, uint v3);
    void unmapVertexEdge(uint v1, uint v2);
    void decVertexValence(uint v);




private:

    // input
    VertexVector mVertices;

    bool mHonorWinding;
    uint mVertexCount;

    // output
    StripVector mStrips;

    /* vertex and edge database */
    VertexMap mVertexBins;


    bool mForwardWinding;



};



} // namespace AC

