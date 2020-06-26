/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: frame\debug #]
***/

#include "build.h"
#include "renderingFrameDebug.h"
#include "renderingFrameDebugGeometry.h"

#include "base/font/include/bitmapFont.h"

namespace rendering
{
    namespace scene
    {
        //--

        DebugDrawer::DebugDrawer(DebugGeometry& dg, DebugGeometryType type, const base::Matrix& localToWorld /*= base::Matrix::IDENTITY()*/)
            : m_geometry(dg)
            , m_geometryType(type)
        {
            m_localToWorld = localToWorld;
            m_localToWorldSet = (localToWorld != base::Matrix::IDENTITY());
        }

        DebugDrawer::~DebugDrawer()
        {
            flush();
        }

        void DebugDrawer::flush()
        {
            if (!m_indicesData.empty())
            {
                DebugGeometryElementSrc src;
                src.type = m_geometryType;
                src.sourceVerticesData = &m_verticesData;
                src.sourceVerticesDataEx = &m_verticesExData;
                src.sourceIndicesData = &m_indicesData;
                m_geometry.pushElement(src);

                m_indicesData.clear();
                m_verticesData.clear();
                m_verticesExData.clear();
            }
        }

        void DebugDrawer::localToWorld(const base::Matrix& localToWorld)
        {
            m_localToWorld = localToWorld;
            m_localToWorldSet = (localToWorld != base::Matrix::IDENTITY());
        }

        void DebugDrawer::appendControlVertices(uint32_t count, const float* sizeOverride /*= nullptr*/, const uint32_t* subSelectionIDOverride/*= nullptr*/)
        {
            DEBUG_CHECK(m_verticesExData.size() == m_verticesData.size());

            while (count > 0)
            {
                uint32_t numAdded = 0;
                auto* writePtr = m_verticesExData.allocateBatch(count, numAdded);

                if (sizeOverride || subSelectionIDOverride)
                {
                    for (uint32_t i = 0; i < numAdded; ++i, ++writePtr)
                    {
                        writePtr->subSelectionId = subSelectionIDOverride ? *subSelectionIDOverride : m_subSelectionId;
                        writePtr->size = sizeOverride ? *sizeOverride : m_size;
                        writePtr->depthBias = m_depthBias;
                        writePtr->paramsId = m_currentParamsId;
                    }
                }
                else
                {
                    for (uint32_t i = 0; i < numAdded; ++i, ++writePtr)
                    {
                        writePtr->subSelectionId = m_subSelectionId;
                        writePtr->size = m_size;
                        writePtr->depthBias = m_depthBias;
                        writePtr->paramsId = m_currentParamsId;
                    }
                }

                count -= numAdded;
            }
        }

        uint32_t DebugDrawer::appendVertices(const DebugVertex* vertex, uint32_t numVertices, const float* sizeOverride /*= nullptr*/, const uint32_t* subSelectionIDOverride /*= nullptr*/)
        {
            // remember the first vertex index
            auto firstVertex = m_verticesData.size();
            appendControlVertices(numVertices, sizeOverride, subSelectionIDOverride);

            // upload vertices
            auto readPtr = vertex;
            uint32_t numLeft = numVertices;
            while (numLeft > 0)
            {
                // allocate space in the buffer
                uint32_t numAdded = 0;
                DebugVertex* writePtr = m_verticesData.allocateBatch(numLeft, numAdded);

                if (m_localToWorldSet)
                {
                    for (uint32_t i = 0; i < numAdded; ++i)
                    {
                        writePtr->p = m_localToWorld.transformPoint(readPtr->p);
                        writePtr->c = readPtr->c;
                        writePtr->t = readPtr->t;

                        ++readPtr;
                        ++writePtr;
                    }
                }
                else
                {
                    for (uint32_t i = 0; i < numAdded; ++i)
                    {
                        writePtr->p = readPtr->p;
                        writePtr->c = readPtr->c;
                        writePtr->t = readPtr->t;

                        ++readPtr;
                        ++writePtr;
                    }
                }

                numLeft -= numAdded;
            }

            // return the index of the first vertex added so it can be added to the index buffer entries
            return firstVertex;
        }

        uint32_t DebugDrawer::appendVertices(const base::Vector2* points, uint32_t numVertices, const float* sizeOverride /*= nullptr*/, const uint32_t* subSelectionIDOverride /*= nullptr*/, const base::Color* colorOverride /*= nullptr*/)
        {
            // remember the first vertex index
            auto firstVertex = m_verticesData.size();
            appendControlVertices(numVertices, sizeOverride, subSelectionIDOverride);

            // color to write
            const auto color = colorOverride ? *colorOverride : m_color;

            // upload vertices
            auto readPtr = points;
            uint32_t numLeft = numVertices;
            while (numLeft > 0)
            {
                // allocate space in the buffer
                uint32_t numAdded = 0;
                DebugVertex* writePtr = m_verticesData.allocateBatch(numLeft, numAdded);
                for (uint32_t i = 0; i < numAdded; ++i)
                {
                    writePtr->p.x = readPtr->x;
                    writePtr->p.y = readPtr->y;
                    writePtr->p.z = 0.5f;
                    writePtr->c = color;
                    writePtr->t.x = 0.0f;
                    writePtr->t.y = 0.0f;

                    ++readPtr;
                    ++writePtr;
                }

                numLeft -= numAdded;
            }

            // return the index of the first vertex added so it can be added to the index buffer entries
            return firstVertex;
        }

        uint32_t DebugDrawer::appendVertices(const base::Vector3* points, uint32_t numVertices, const float* sizeOverride /*= nullptr*/, const uint32_t* subSelectionIDOverride /*= nullptr*/, const base::Color* colorOverride /*= nullptr*/)
        {
            // remember the first vertex index
            auto firstVertex = m_verticesData.size();
            appendControlVertices(numVertices, sizeOverride, subSelectionIDOverride);

            // color to write
            const auto color = colorOverride ? *colorOverride : m_color;

            // upload vertices
            auto readPtr = points;
            uint32_t numLeft = numVertices;
            while (numLeft > 0)
            {
                // allocate space in the buffer
                uint32_t numAdded = 0;
                DebugVertex* writePtr = m_verticesData.allocateBatch(numLeft, numAdded);

                if (m_localToWorldSet)
                {
                    for (uint32_t i = 0; i < numAdded; ++i)
                    {
                        writePtr->p = m_localToWorld.transformPoint(*readPtr);
                        writePtr->c = color;
                        writePtr->t.x = 0.0f;
                        writePtr->t.y = 0.0f;

                        ++readPtr;
                        ++writePtr;
                    }
                }
                else
                {
                    for (uint32_t i = 0; i < numAdded; ++i)
                    {
                        writePtr->p = *readPtr;
                        writePtr->c = color;
                        writePtr->t.x = 0.0f;
                        writePtr->t.y = 0.0f;

                        ++readPtr;
                        ++writePtr;
                    }
                }

                numLeft -= numAdded;
            }

            // return the index of the first vertex added so it can be added to the index buffer entries
            return firstVertex;
        }

        void DebugDrawer::appendIndices(const uint32_t* indices, uint32_t numIndices, uint32_t firstVertex)
        {
            auto readPtr = indices;
            uint32_t numLeft = numIndices;
            while (numLeft > 0)
            {
                // allocate space in the buffer
                uint32_t numAdded = 0;
                uint32_t* writePtr = m_indicesData.allocateBatch(numLeft, numAdded);
                for (uint32_t i = 0; i < numAdded; ++i)
                    *writePtr++ = *readPtr++ + firstVertex;

                numLeft -= numAdded;
            }
        }

        void DebugDrawer::appendIndices(const uint16_t* indices, uint32_t numIndices, uint32_t firstVertex)
        {
            auto readPtr = indices;
            uint32_t numLeft = numIndices;
            while (numLeft > 0)
            {
                // allocate space in the buffer
                uint32_t numAdded = 0;
                uint32_t* writePtr = m_indicesData.allocateBatch(numLeft, numAdded);
                for (uint32_t i = 0; i < numAdded; ++i)
                    *writePtr++ = (uint32_t)*readPtr++ + firstVertex;

                numLeft -= numAdded;
            }
        }

        void DebugDrawer::appendAutoPointIndices(uint32_t firstVertex, uint32_t numVertices)
        {
            if (numVertices >= 1)
            {
                uint32_t counter = 0;
                uint32_t numLeft = numVertices;
                while (numLeft > 0)
                {
                    // allocate space in the buffer
                    uint32_t numAdded = 0;
                    uint32_t* writePtr = m_indicesData.allocateBatch(numLeft, numAdded);
                    for (uint32_t i = 0; i < numAdded; ++i)
                    {
                        *writePtr++ = firstVertex + counter;
                        counter += 1;
                    }

                    numLeft -= numAdded;
                }
            }
        }

        void DebugDrawer::appendAutoLineLoopIndices(uint32_t firstVertex, uint32_t numVertices)
        {
            if (numVertices >= 2)
            {
                auto numLineVertices = numVertices * 2;

                uint32_t counter = 0;
                uint32_t numLeft = numLineVertices;
                while (numLeft > 0)
                {
                    // allocate space in the buffer
                    uint32_t numAdded = 0;
                    uint32_t* writePtr = m_indicesData.allocateBatch(numLeft, numAdded);
                    for (uint32_t i = 0; i < numAdded; ++i)
                    {
                        if (0 == (counter & 1))
                            *writePtr++ = firstVertex + (counter / 2);
                        else
                            *writePtr++ = firstVertex + (((counter / 2) + 1) % numVertices);

                        ++counter;
                    }

                    numLeft -= numAdded;
                }
            }
        }

        void DebugDrawer::appendAutoLineListIndices(uint32_t firstVertex, uint32_t numVertices)
        {
            if (numVertices > 0)
            {
                uint32_t counter = 0;
                uint32_t numLeft = numVertices;
                while (numLeft > 0)
                {
                    // allocate space in the buffer
                    uint32_t numAdded = 0;
                    uint32_t* writePtr = m_indicesData.allocateBatch(numLeft, numAdded);
                    for (uint32_t i = 0; i < numAdded; ++i)
                        *writePtr++ = firstVertex + counter++;

                    numLeft -= numAdded;
                }
            }
        }

        void DebugDrawer::appendAutoTriangleFanIndices(uint32_t firstVertex, uint32_t numVertices, bool swap)
        {
            if (numVertices > 0)
            {
                uint32_t numTrianglesVertices = (numVertices - 2) * 3;

                uint32_t counter = 0;
                uint32_t numLeft = numTrianglesVertices;
                while (numLeft > 0)
                {
                    // allocate space in the buffer
                    uint32_t numAdded = 0;
                    uint32_t* writePtr = m_indicesData.allocateBatch(numLeft, numAdded);

                    if (swap)
                    {
                        for (uint32_t i = 0; i < numAdded; ++i)
                        {
                            switch (counter % 3)
                            {
                            case 0: *writePtr++ = firstVertex + 2 + (counter / 3); break;
                            case 1: *writePtr++ = firstVertex + 1 + (counter / 3); break;
                            case 2: *writePtr++ = firstVertex; break;
                            }
                            ++counter;
                        }
                    }
                    else
                    {
                        for (uint32_t i = 0; i < numAdded; ++i)
                        {
                            switch (counter % 3)
                            {
                            case 0: *writePtr++ = firstVertex; break;
                            case 1: *writePtr++ = firstVertex + 1 + (counter / 3); break;
                            case 2: *writePtr++ = firstVertex + 2 + (counter / 3); break;
                            }
                            ++counter;
                        }
                    }

                    numLeft -= numAdded;
                }
            }
        }

        void DebugDrawer::appendAutoTriangleListIndices(uint32_t firstVertex, uint32_t numVertices, bool swap)
        {
            if (numVertices > 0)
            {
                uint32_t counter = 0;
                uint32_t numLeft = numVertices;
                while (numLeft > 0)
                {
                    // allocate space in the buffer
                    uint32_t numAdded = 0;
                    uint32_t* writePtr = m_indicesData.allocateBatch(numLeft, numAdded);

                    if (swap)
                    {
                        for (uint32_t i = 0; i < numAdded; ++i)
                        {
                            switch (counter % 3)
                            {
                            case 0: *writePtr++ = firstVertex + (counter / 3) + 2; break;
                            case 1: *writePtr++ = firstVertex + (counter / 3) + 1; break;
                            case 2: *writePtr++ = firstVertex + (counter / 3) + 0; break;
                            }
                            ++counter;
                        }
                    }
                    else
                    {
                        for (uint32_t i = 0; i < numAdded; ++i)
                            *writePtr++ = firstVertex + counter++;
                    }

                    numLeft -= numAdded;
                }
            }
        }

        //--

        namespace helper
        {
            // geo sphere builder
            class GeoSphereBuilder : public base::ISingleton
            {
                DECLARE_SINGLETON(GeoSphereBuilder);

            public:
                base::Array<base::Vector3> m_top;
                base::Array<base::Vector3> m_bottom;
                base::Array<base::Vector3> m_circle;

            private:
                GeoSphereBuilder(uint32_t levels = 3)
                {
                    // add structure points (2 tetrahedrons)
                    base::Vector3 points[10];
                    points[0] = base::Vector3(-1, -1, 0).normalized();
                    points[1] = base::Vector3(1, -1, 0).normalized();
                    points[2] = base::Vector3(1, 1, 0).normalized();
                    points[3] = base::Vector3(-1, 1, 0).normalized();
                    points[4] = base::Vector3(0, 0, 1);

                    points[5] = base::Vector3(-1, -1, 0).normalized();
                    points[6] = base::Vector3(1, -1, 0).normalized();
                    points[7] = base::Vector3(1, 1, 0).normalized();
                    points[8] = base::Vector3(-1, 1, 0).normalized();
                    points[9] = base::Vector3(0, 0, -1);

                    // recuse upper half
                    buildTri(points[0], points[1], points[4], levels, m_top);
                    buildTri(points[1], points[2], points[4], levels, m_top);
                    buildTri(points[2], points[3], points[4], levels, m_top);
                    buildTri(points[3], points[0], points[4], levels, m_top);

                    // recurse lower half
                    buildTri(points[6], points[5], points[9], levels, m_bottom);
                    buildTri(points[7], points[6], points[9], levels, m_bottom);
                    buildTri(points[8], points[7], points[9], levels, m_bottom);
                    buildTri(points[5], points[8], points[9], levels, m_bottom);

                    // Create the circumreference vertices
                    uint32_t numVertices = 4 << levels;
                    m_circle.resize(numVertices + 1);

                    for (uint32_t i = 0; i <= numVertices; ++i)
                    {
                        float angle = (2.0f * PI) * ((float)i / (float)numVertices);
                        m_circle[i].x = cosf(angle);
                        m_circle[i].y = sinf(angle);
                        m_circle[i].z = 0.f;
                    }
                }

                void buildTri(const base::Vector3& a, const base::Vector3& b, const base::Vector3& c, uint32_t level, base::Array<base::Vector3 >& outList)
                {
                    // emit the triangle once we've reached the final level (it's over 9000!)
                    if (level == 0)
                    {
                        outList.pushBack(a);
                        outList.pushBack(b);
                        outList.pushBack(c);
                        return;
                    }

                    // calculate split points for each triangle and project them on the sphere (normalization)
                    base::Vector3 midAB = ((a + b) * 0.5f).normalized();
                    base::Vector3 midBC = ((b + c) * 0.5f).normalized();
                    base::Vector3 midCA = ((c + a) * 0.5f).normalized();

                    // recruse
                    buildTri(a, midAB, midCA, level - 1, outList);
                    buildTri(midAB, b, midBC, level - 1, outList);
                    buildTri(midBC, c, midCA, level - 1, outList);
                    buildTri(midAB, midBC, midCA, level - 1, outList);
                }

                virtual void deinit() override
                {
                    m_top.clear();
                    m_bottom.clear();
                    m_circle.clear();
                }
            };

            template< uint32_t NumVertices >
            struct TriBuilder
            {
                base::InplaceArray<base::Vector3, NumVertices> m_vertices;
                base::InplaceArray<uint16_t, NumVertices * 2> m_indices;

                void flush(DebugDrawer& dg)
                {
                    if (!m_vertices.empty())
                    {
                        auto base = dg.appendVertices(m_vertices.typedData(), m_vertices.size());
                        dg.appendIndices(m_indices.typedData(), m_indices.size(), base);
                    }
                }

                void triangle(const base::Vector3& a, const base::Vector3& b, const base::Vector3& c)
                {
                    auto base = m_vertices.size();
                    auto* v = m_vertices.allocateUninitialized(3);
                    v[0] = a;
                    v[1] = b;
                    v[2] = c;

                    auto* i = m_indices.allocateUninitialized(3);
                    i[0] = base + 0;
                    i[1] = base + 1;
                    i[2] = base + 2;
                }

                void quad(const base::Vector3& a, const base::Vector3& b, const base::Vector3& c, const base::Vector3& d)
                {
                    auto base = m_vertices.size();
                    auto* v = m_vertices.allocateUninitialized(4);
                    v[0] = a;
                    v[1] = b;
                    v[2] = c;
                    v[3] = d;

                    auto* i = m_indices.allocateUninitialized(6);
                    i[0] = base + 0;
                    i[1] = base + 1;
                    i[2] = base + 2;
                    i[3] = base + 0;
                    i[4] = base + 2;
                    i[5] = base + 3;
                }
            };

            template< uint32_t NumVertices >
            struct WireBuilder
            {
                base::InplaceArray<base::Vector3, NumVertices> m_vertices;
                base::InplaceArray<uint16_t, NumVertices*2> m_indices;

                void flush(DebugDrawer& dg)
                {
                    if (!m_vertices.empty())
                    {
                        auto base = dg.appendVertices(m_vertices.typedData(), m_vertices.size());
                        dg.appendIndices(m_indices.typedData(), m_indices.size(), base);
                    }
                }

                void triangle(const base::Vector3& a, const base::Vector3& b, const base::Vector3& c)
                {
                    auto base = m_vertices.size();
                    auto* v = m_vertices.allocateUninitialized(3);
                    v[0] = a;
                    v[1] = b;
                    v[2] = c;

                    auto* i = m_indices.allocateUninitialized(6);
                    i[0] = base + 0;
                    i[1] = base + 1;
                    i[2] = base + 1;
                    i[3] = base + 2;
                    i[4] = base + 2;
                    i[5] = base + 0;
                }

                void quad(const base::Vector3& a, const base::Vector3& b, const base::Vector3& c, const base::Vector3& d)
                {
                    auto base = m_vertices.size();
                    auto* v = m_vertices.allocateUninitialized(4);
                    v[0] = a;
                    v[1] = b;
                    v[2] = c;
                    v[3] = d;

                    auto* i = m_indices.allocateUninitialized(8);
                    i[0] = base + 0;
                    i[1] = base + 1;
                    i[2] = base + 1;
                    i[3] = base + 2;
                    i[4] = base + 2;
                    i[5] = base + 3;
                    i[6] = base + 3;
                    i[7] = base + 0;
                }
            };

        } // helper
        
        //--

        DebugLineDrawer::DebugLineDrawer(DebugGeometry& dg, const base::Matrix& localToWorld)
            : DebugDrawer(dg, DebugGeometryType::Lines, localToWorld)
        {}

        void DebugLineDrawer::line(const base::Vector3& a, const base::Vector3& b)
        {
            base::Vector3 verts[2] = { a,b };
            
            auto base = appendVertices(verts, 2);
            appendAutoLineListIndices(base, 2);
        }

        void DebugLineDrawer::lines(const base::Vector2* points, uint32_t numPoints)
        {
            if (numPoints >= 2)
            {
                auto base = appendVertices(points, numPoints & ~1);
                appendAutoLineListIndices(base, numPoints & ~1);
            }
        }

        void DebugLineDrawer::lines(const base::Vector3* points, uint32_t numPoints)
        {
            if (numPoints >= 2)
            {
                auto base = appendVertices(points, numPoints & ~1);
                appendAutoLineListIndices(base, numPoints & ~1);
            }
        }

        void DebugLineDrawer::lines(const DebugVertex* points, uint32_t numPoints)
        {
            if (numPoints >= 2)
            {
                auto base = appendVertices(points, numPoints & ~1);
                appendAutoLineListIndices(base, numPoints & ~1);
            }
        }

        void DebugLineDrawer::loop(const base::Vector2* points, uint32_t numPoints)
        {
            if (numPoints >= 2)
            {
                auto base = appendVertices(points, numPoints);
                appendAutoLineLoopIndices(base, numPoints);
            }
        }

        void DebugLineDrawer::loop(const base::Vector3* points, uint32_t numPoints)
        {
            if (numPoints >= 2)
            {
                auto base = appendVertices(points, numPoints);
                appendAutoLineLoopIndices(base, numPoints);
            }
        }

        void DebugLineDrawer::loop(const DebugVertex* points, uint32_t numPoints)
        {
            if (numPoints >= 2)
            {
                auto base = appendVertices(points, numPoints);
                appendAutoLineLoopIndices(base, numPoints);
            }
        }

        void MakeArrow(const base::Vector3& start, const base::Vector3& end, base::Color color, base::Array<DebugVertex>& outVertices)
        {
            auto dir = end - start;
            if (dir.squareLength() > 0.000001f)
            {
                // select space
                auto space = base::Vector3::EX();
                if (dir.largestAxis() == 0)
                    space = base::Vector3::EZ();

                // compute arrow vectors
                auto len = dir.length();
                auto dirX = dir.normalized();
                auto dirY = base::Cross(dirX, space).normalized();
                auto dirZ = base::Cross(dirY, dirX).normalized();

                auto head = end;
                auto base = start + dir * 0.8f;
                auto u = dirY * len * 0.05f;
                auto v = dirZ * len * 0.05f;

                // Arrow lines
                outVertices.emplaceBack().pos(start).color(color);
                outVertices.emplaceBack().pos(head).color(color);
                outVertices.emplaceBack().pos(head).color(color);
                outVertices.emplaceBack().pos(base + u + v).color(color);
                outVertices.emplaceBack().pos(head).color(color);
                outVertices.emplaceBack().pos(base + u - v).color(color);
                outVertices.emplaceBack().pos(head).color(color);
                outVertices.emplaceBack().pos(base - u + v).color(color);
                outVertices.emplaceBack().pos(head).color(color);
                outVertices.emplaceBack().pos(base - u - v).color(color);
                outVertices.emplaceBack().pos(base - u + v).color(color);
                outVertices.emplaceBack().pos(base + u + v).color(color);
                outVertices.emplaceBack().pos(base - u + v).color(color);
                outVertices.emplaceBack().pos(base - u - v).color(color);
                outVertices.emplaceBack().pos(base - u - v).color(color);
                outVertices.emplaceBack().pos(base + u - v).color(color);
                outVertices.emplaceBack().pos(base + u + v).color(color);
                outVertices.emplaceBack().pos(base + u - v).color(color);
            }
        }

        void DebugLineDrawer::arrow(const base::Vector3& start, const base::Vector3& end)
        {
            base::InplaceArray<DebugVertex, 20> vertices;
            MakeArrow(start, end, m_color, vertices);
            lines(vertices.typedData(), vertices.size());
        }

        void DebugLineDrawer::axes(const base::Matrix& additionalTransform, float length /*= 0.5f*/)
        {
            auto o = additionalTransform.translation();
            auto x = additionalTransform.column(0).xyz().normalized();
            auto y = additionalTransform.column(1).xyz().normalized();
            auto z = additionalTransform.column(2).xyz().normalized();
            axes(o, x, y, z, length);
        }

        void DebugLineDrawer::axes(const base::Vector3& origin, const base::Vector3& x, const base::Vector3& y, const base::Vector3& z, float length /*= 0.5f*/)
        {
            base::InplaceArray<DebugVertex, 64> vertices;
            MakeArrow(origin, origin + x * length, base::Color::RED, vertices);
            MakeArrow(origin, origin + y * length, base::Color::GREEN, vertices);
            MakeArrow(origin, origin + z * length, base::Color::BLUE, vertices);
            lines(vertices.typedData(), vertices.size());
        }

        void DebugLineDrawer::brackets(const base::Vector3* corners, float length /*= 0.1f*/)
        {
            // compute the direction vectors
            auto dirX = corners[1] - corners[0];
            auto dirY = corners[2] - corners[0];
            auto dirZ = corners[4] - corners[0];

            // compute maximum length
            auto maxLength = std::min<float>(dirX.length(), std::min<float>(dirY.length(), dirZ.length()));
            length = std::min<float>(maxLength, length);

            // normalize box directions
            dirX.normalize();
            dirY.normalize();
            dirZ.normalize();
            dirX *= length;
            dirY *= length;
            dirZ *= length;

            // draw the corners
            base::InplaceArray<base::Vector3, 64> vertices;
            for (uint32_t i = 0; i < 8; i++)
            {
                vertices.emplaceBack(corners[i]);
                vertices.emplaceBack((i & 1) ? (corners[i] - dirX) : (corners[i] + dirX));
                vertices.emplaceBack(corners[i]);
                vertices.emplaceBack((i & 2) ? (corners[i] - dirY) : (corners[i] + dirY));
                vertices.emplaceBack(corners[i]);
                vertices.emplaceBack((i & 4) ? (corners[i] - dirZ) : (corners[i] + dirZ));
            }

            lines(vertices.typedData(), vertices.size());
        }

        void DebugLineDrawer::brackets(const base::Box& box, float length /*= 0.1f*/)
        {
            base::Vector3 corners[8];
            box.corners(corners);
            brackets(corners, length);
        }

        void DebugLineDrawer::box(const base::Vector3* corners)
        {
            auto firstVertex = appendVertices(corners, 8);
            const uint16_t indices[] = { 0,4,1,5,2,6,3,7,0,1,1,3,3,2,2,0,4,5,5,7,7,6,6,4 };
            appendIndices(indices, ARRAY_COUNT(indices), firstVertex);
        }

        void DebugLineDrawer::box(const base::Vector3& boxMin, const base::Vector3& boxMax)
        {
            base::Vector3 corners[8];
            base::Box(boxMin, boxMax).corners(corners);
            box(corners);
        }

        void DebugLineDrawer::box(const base::Box& drawBox)
        {
            base::Vector3 corners[8];
            drawBox.corners(corners);
            box(corners);
        }

        void DebugLineDrawer::sphere(const base::Vector3& center, float radius)
        {
            capsule(center, radius, 0.0f);
        }

        void DebugLineDrawer::capsule(const base::Vector3& center, float radius, float halfHeight)
        {
            auto& sphere = helper::GeoSphereBuilder::GetInstance();

            auto topCenter = center + base::Vector3(0.0f, 0.0f, halfHeight);
            auto bottomCenter = center + base::Vector3(0.0f, 0.0f, -halfHeight);

            helper::WireBuilder<1024> tris;

            // top sphere
            {
                auto topCount = sphere.m_top.size();
                auto top = sphere.m_top.typedData();
                for (uint32_t i = 0; i < topCount; i += 3)
                {
                    auto a = (top[i + 0] * radius) + topCenter;
                    auto b = (top[i + 1] * radius) + topCenter;
                    auto c = (top[i + 2] * radius) + topCenter;
                    tris.triangle(a, b, c);
                }
            }

            // bottom sphere
            {
                auto bottomCount = sphere.m_bottom.size();
                auto bottom = sphere.m_bottom.typedData();
                for (uint32_t i = 0; i < bottomCount; i += 3)
                {
                    auto a = (bottom[i + 0] * radius) + bottomCenter;
                    auto b = (bottom[i + 1] * radius) + bottomCenter;
                    auto c = (bottom[i + 2] * radius) + bottomCenter;
                    tris.triangle(a, b, c);
                }
            }

            // middle
            if (halfHeight > 0.0f)
            {
                auto circleCount = sphere.m_circle.size() - 1;
                auto circle = sphere.m_circle.typedData();
                for (uint32_t i = 0; i < circleCount; ++i)
                {
                    auto ta = circle[i + 0] * radius + topCenter;
                    auto tb = circle[i + 1] * radius + topCenter;
                    auto bb = circle[i + 1] * radius + bottomCenter;
                    auto ba = circle[i + 0] * radius + bottomCenter;
                    //tris.quad(ta, tb, bb, ba);
                    tris.quad(ba, bb, tb, ta);
                }
            }

            // flush
            tris.flush(*this);
        }

        void DebugLineDrawer::cylinder(const base::Vector3& center1, const base::Vector3& center2, float radius1, float radius2)
        {
            helper::WireBuilder<1024> tris;

            auto& sphere = helper::GeoSphereBuilder::GetInstance();

            // compute the cylinder direction
            auto dir = (center2 - center1);
            if (dir.normalize() < NORMALIZATION_EPSILON)
                return; // to short

            // compute the side vectors
            auto side = base::Vector3::EX();
            if (dir.largestAxis() == 0)
                side = base::Vector3::EZ();

            auto dirX = base::Cross(dir, side).normalized();
            auto dirY = base::Cross(dir, dirX).normalized();

            // generate vertices
            auto circleCount = sphere.m_circle.size() - 1;
            auto circle = sphere.m_circle.typedData();
            base::InplaceArray<base::Vector3, 128> pos1, pos2;
            for (uint32_t i = 0; i <= circleCount; ++i)
            {
                pos1.emplaceBack(circle[i].transform(dirX, dirY) * radius1 + center1);
                pos2.emplaceBack(circle[i].transform(dirX, dirY) * radius2 + center2);
            }

            // draw sides
            for (uint32_t i = 0; i < circleCount; ++i)
                tris.quad(pos1[i + 0], pos1[i + 1], pos2[i + 1], pos2[i + 0]);

            tris.flush(*this);
        }

        void DebugLineDrawer::cone(const base::Vector3& top, const base::Vector3& dir, float radius, float angleDeg)
        {
            helper::WireBuilder<1024> tris;

            auto& sphere = helper::GeoSphereBuilder::GetInstance();

            // compute the cylinder direction
            if (dir.isNearZero())
                return; // to short

            // compute the side vectors
            auto side = base::Vector3::EX();
            if (dir.largestAxis() == 0)
                side = base::Vector3::EZ();

            auto dirX = base::Cross(dir, side).normalized();
            auto dirY = base::Cross(dir, dirX).normalized();

            // compute the circle sizes
            auto bottom = top + dir * radius * cos(DEG2RAD * angleDeg);
            auto circleX = dirX * radius * sin(DEG2RAD * angleDeg);
            auto circleY = dirY * radius * sin(DEG2RAD * angleDeg);

            // draw sides
            auto circleCount = sphere.m_circle.size() - 1;
            auto circle = sphere.m_circle.typedData();
            for (uint32_t i = 0; i < circleCount; ++i)
            {
                auto a = circle[i + 0].transform(circleX, circleY) + bottom;
                auto b = circle[i + 1].transform(circleX, circleY) + bottom;
                tris.triangle(top, b, a);
            }

            tris.flush(*this);
        }

        void DebugLineDrawer::plane(const base::Vector3& pos, const base::Vector3& normal, float size /*= 10.0f*/, int gridSize /*= 10*/)
        {
            auto side = base::Vector3::EX();
            if (normal.largestAxis() == 0)
                side = base::Vector3::EZ();

            auto dirX = base::Cross(normal, side).normalized();
            auto dirY = base::Cross(normal, dirX).normalized();

            auto spacing = size / (float)gridSize;

            auto minX = -dirX * size;
            auto maxX = dirX * size;
            auto minY = -dirY * size;
            auto maxY = dirY * size;

            base::InplaceArray<base::Vector3, 100> vertices;
            for (int i = -gridSize; i <= gridSize; ++i)
            {
                auto nx = dirX * (float)i * spacing;
                auto ny = dirY * (float)i * spacing;

                vertices.emplaceBack(pos + minX + ny);
                vertices.emplaceBack(pos + maxX + ny);
                vertices.emplaceBack(pos + minY + nx);
                vertices.emplaceBack(pos + maxY + nx);
            }

            lines(vertices.typedData(), vertices.size());
        }

        void DebugLineDrawer::frustum(const base::Matrix& frustumMatrix, float farPlaneScale /*= 1.0f*/)
        {
            helper::WireBuilder<256> tris;

            base::Vector3 frustumMin(-1.0f, -1.0f, 0.0f);
            base::Vector3 frustumMax(+1.0f, +1.0f, farPlaneScale);

            // compute frustum vertices
            base::Vector3 unprojected[8];
            for (uint32_t i = 0; i < 8; ++i)
            {
                // Calculate vertex in screen space
                base::Vector4 pos;
                pos.x = (i & 1) ? frustumMax.x : frustumMax.x;
                pos.y = (i & 2) ? frustumMax.y : frustumMax.y;
                pos.z = (i & 4) ? frustumMax.z : frustumMax.z;
                pos.w/* = 1.0f */;

                // Unprojected to world space
                auto temp = frustumMatrix.transformVector4(pos);
                unprojected[i] = base::Vector3(temp.x / temp.w, temp.y / temp.w, temp.z / temp.w);
            }

            // draw each face of frustum
            uint8_t planes[6][4] = { { 0,1,3,2 }, { 6,7,5,4 }, { 5,7,3,1 }, { 6,4,0,2 }, { 5,1,0,4 }, { 3,7,6,2 } };
            for (uint32_t i = 0; i < 6; ++i)
                tris.quad(unprojected[planes[i][0]], unprojected[planes[i][1]], unprojected[planes[i][2]], unprojected[planes[i][3]]);

            tris.flush(*this);
        }

        void DebugLineDrawer::circle(const base::Vector3& center, const base::Vector3& normal, float radius, float startAngle /*= 0.0f*/, float endAngle /*= 360.0*/)
        {
            auto& sphere = helper::GeoSphereBuilder::GetInstance();

            helper::WireBuilder<256> tris;

            // compute the cylinder direction
            if (normal.isNearZero())
                return; // to short

            // compute the side vectors
            auto side = base::Vector3::EX();
            if (normal.largestAxis() == 0)
                side = base::Vector3::EZ();

            auto dirX = base::Cross(normal, side).normalized() * radius;
            auto dirY = base::Cross(normal, dirX).normalized() * radius;

            // full circle
            if (startAngle == 0.0f && endAngle == 360.0f) // exact comparison to detect the default parameters
            {
                auto circleCount = sphere.m_circle.size() - 1;
                auto circle = sphere.m_circle.typedData();
                for (uint32_t i = 0; i < circleCount; ++i)
                {
                    auto a = circle[i + 0].transform(dirX, dirY) + center;
                    auto b = circle[i + 1].transform(dirX, dirY) + center;
                    tris.triangle(center, b, a);
                }
            }
            else
            {
                //auto dist = base::AngleDistance(endAngle, startAngle);
                auto numSegs = 1 + (uint32_t)(abs(endAngle - startAngle) / 15.0f);
                auto deltaAng = (endAngle - startAngle) / (float)numSegs;

                base::Vector3 prevPos = center;
                for (uint32_t i = 0; i <= numSegs; ++i)
                {
                    auto angle = DEG2RAD * (startAngle + i * deltaAng);

                    base::Vector3 curPos = center;
                    curPos += dirX * cos(angle);
                    curPos += dirY * sin(angle);

                    if (i >= 1)
                        tris.triangle(center, prevPos, curPos);
                    prevPos = curPos;
                }
            }

            tris.flush(*this);
        }

        void DebugLineDrawer::rect(float x, float y, float w, float h)
        {
            DebugVertex v[4];
            v[0].pos(x, y).uv(0.0f, 0.0f).color(m_color);
            v[1].pos(x + w, y).uv(1.0f, 0.0f).color(m_color);
            v[2].pos(x + w, y + h).uv(1.0f, 1.0f).color(m_color);
            v[3].pos(x, y + h).uv(0.0f, 1.0f).color(m_color);

            auto firstVertex = appendVertices(v, 4);
            appendAutoLineLoopIndices(firstVertex, 4);
        }

        void DebugLineDrawer::recti(int x0, int  y0, int  w, int  h)
        {
            rect((float)x0, (float)y0, (float)w, (float)h);
        }

        void DebugLineDrawer::bounds(float x0, float y0, float x1, float y1)
        {
            DebugVertex v[4];
            v[0].pos(x0, y0).uv(0.0f, 0.0f).color(m_color);
            v[1].pos(x1, y0).uv(1.0f, 0.0f).color(m_color);
            v[2].pos(x1, y1).uv(1.0f, 1.0f).color(m_color);
            v[3].pos(x0, y1).uv(0.0f, 1.0f).color(m_color);

            auto firstVertex = appendVertices(v, 4);
            appendAutoLineLoopIndices(firstVertex, 4);
        }

        void DebugLineDrawer::boundsi(int  x0, int  y0, int  x1, int  y1)
        {
            bounds((float)x0, (float)y0, (float)x1, (float)y1);
        }

        //--

        DebugSpriteDrawer::DebugSpriteDrawer(DebugGeometry& dg, const base::Matrix& localToWorld /*= base::Matrix::IDENTITY()*/)
            : DebugDrawer(dg, DebugGeometryType::Sprite, localToWorld)
        {}

        void DebugSpriteDrawer::image(const base::image::ImagePtr& image)
        {

        }

        void DebugSpriteDrawer::sprite(const base::Vector3& pos)
        {

        }

        void DebugSpriteDrawer::sprites(const base::Vector3* positions, uint32_t count)
        {

        }

        //--

        DebugSolidDrawer::DebugSolidDrawer(DebugGeometry& dg, const base::Matrix& localToWorld /*= base::Matrix::IDENTITY()*/)
            : DebugDrawer(dg, DebugGeometryType::Solid, localToWorld)
        {}

        void DebugSolidDrawer::polygon(const base::Vector2* points, uint32_t numPoints, bool swap)
        {
            auto firstVertex = appendVertices(points, numPoints);
            appendAutoTriangleFanIndices(firstVertex, numPoints, swap);
        }

        void DebugSolidDrawer::polygon(const base::Vector3* points, uint32_t numPoints, bool swap)
        {
            auto firstVertex = appendVertices(points, numPoints);
            appendAutoTriangleFanIndices(firstVertex, numPoints, swap);
        }

        void DebugSolidDrawer::polygon(const DebugVertex* points, uint32_t numPoints, bool swap)
        {
            auto firstVertex = appendVertices(points, numPoints);
            appendAutoTriangleFanIndices(firstVertex, numPoints, swap);
        }

        void DebugSolidDrawer::indexedTris(const base::Vector2* points, uint32_t numPoints, const uint32_t* indices, uint32_t numIndices)
        {
            auto firstVertex = appendVertices(points, numPoints);
            appendIndices(indices, numIndices, firstVertex);
        }

        void DebugSolidDrawer::indexedTris(const base::Vector3* points, uint32_t numPoints, const uint32_t* indices, uint32_t numIndices)
        {
            auto firstVertex = appendVertices(points, numPoints);
            appendIndices(indices, numIndices, firstVertex);
        }

        void DebugSolidDrawer::indexedTris(const DebugVertex* points, uint32_t numPoints, const uint32_t* indices, uint32_t numIndices)
        {
            auto firstVertex = appendVertices(points, numPoints);
            appendIndices(indices, numIndices, firstVertex);
        }

        void DebugSolidDrawer::box(const base::Vector3* corners)
        {
            auto firstVertex = appendVertices(corners, 8);
            const uint16_t indices[] = { 2,1,0,2,3,1, 6,4,5,7,6,5, 4,2,0,6,2,4, 3,5,1,7,5,3, 6,3,2,6,7,3,  1,4,0,5,4,1 };
            appendIndices(indices, ARRAY_COUNT(indices), firstVertex);
        }

        void DebugSolidDrawer::box(const base::Vector3& boxMin, const base::Vector3& boxMax)
        {
            base::Vector3 corners[8];
            base::Box(boxMin, boxMax).corners(corners);
            box(corners);
        }

        void DebugSolidDrawer::box(const base::Box& drawBox)
        {
            base::Vector3 corners[8];
            drawBox.corners(corners);
            box(corners);
        }

        void DebugSolidDrawer::sphere(const base::Vector3& center, float radius)
        {
            capsule(center, radius, 0.0f);
        }

        void DebugSolidDrawer::capsule(const base::Vector3& center, float radius, float halfHeight)
        {
            auto& sphere = helper::GeoSphereBuilder::GetInstance();

            auto topCenter = center + base::Vector3(0.0f, 0.0f, halfHeight);
            auto bottomCenter = center + base::Vector3(0.0f, 0.0f, -halfHeight);

            helper::TriBuilder<1024> tris;

            // top sphere
            {
                auto topCount = sphere.m_top.size();
                auto top = sphere.m_top.typedData();
                for (uint32_t i = 0; i < topCount; i += 3)
                {
                    auto a = (top[i + 0] * radius) + topCenter;
                    auto b = (top[i + 1] * radius) + topCenter;
                    auto c = (top[i + 2] * radius) + topCenter;
                    tris.triangle(a, b, c);
                }
            }

            // bottom sphere
            {
                auto bottomCount = sphere.m_bottom.size();
                auto bottom = sphere.m_bottom.typedData();
                for (uint32_t i = 0; i < bottomCount; i += 3)
                {
                    auto a = (bottom[i + 0] * radius) + bottomCenter;
                    auto b = (bottom[i + 1] * radius) + bottomCenter;
                    auto c = (bottom[i + 2] * radius) + bottomCenter;
                    tris.triangle(a, b, c);
                }
            }

            // middle
            if (halfHeight > 0.0f)
            {
                auto circleCount = sphere.m_circle.size() - 1;
                auto circle = sphere.m_circle.typedData();
                for (uint32_t i = 0; i < circleCount; ++i)
                {
                    auto ta = circle[i + 0] * radius + topCenter;
                    auto tb = circle[i + 1] * radius + topCenter;
                    auto bb = circle[i + 1] * radius + bottomCenter;
                    auto ba = circle[i + 0] * radius + bottomCenter;
                    //tris.quad(ta, tb, bb, ba);
                    tris.quad(ba, bb, tb, ta);
                }
            }

            // flush
            tris.flush(*this);
        }

        void DebugSolidDrawer::cylinder(const base::Vector3& center1, const base::Vector3& center2, float radius1, float radius2)
        {
            helper::TriBuilder<1024> tris;

            auto& sphere = helper::GeoSphereBuilder::GetInstance();

            // compute the cylinder direction
            auto dir = (center2 - center1);
            if (dir.normalize() < NORMALIZATION_EPSILON)
                return; // to short

            // compute the side vectors
            auto side = base::Vector3::EX();
            if (dir.largestAxis() == 0)
                side = base::Vector3::EZ();

            auto dirX = base::Cross(dir, side).normalized();
            auto dirY = base::Cross(dir, dirX).normalized();

            // generate vertices
            auto circleCount = sphere.m_circle.size() - 1;
            auto circle = sphere.m_circle.typedData();
            base::InplaceArray<base::Vector3, 128> pos1, pos2;
            for (uint32_t i = 0; i <= circleCount; ++i)
            {
                pos1.emplaceBack(circle[i].transform(dirX, dirY) * radius1 + center1);
                pos2.emplaceBack(circle[i].transform(dirX, dirY) * radius2 + center2);
            }

            // draw sides
            for (uint32_t i = 0; i < circleCount; ++i)
                tris.quad(pos1[i + 0], pos1[i + 1], pos2[i + 1], pos2[i + 0]);

            // top/bottom
            for (uint32_t i = 2; i < circleCount; ++i)
            {
                tris.triangle(pos1[0], pos1[i - 1], pos1[i]);
                tris.triangle(pos2[i], pos2[0], pos2[i - 1]);
            }

            tris.flush(*this);
        }

        void DebugSolidDrawer::cone(const base::Vector3& top, const base::Vector3& dir, float radius, float angleDeg)
        {
            helper::TriBuilder<512> tris;

            auto& sphere = helper::GeoSphereBuilder::GetInstance();

            // compute the cylinder direction
            if (dir.isNearZero())
                return; // to short

            // compute the side vectors
            auto side = base::Vector3::EX();
            if (dir.largestAxis() == 0)
                side = base::Vector3::EZ();

            auto dirX = base::Cross(dir, side).normalized();
            auto dirY = base::Cross(dir, dirX).normalized();

            // compute the circle sizes
            auto bottom = top + dir * radius * cos(DEG2RAD * angleDeg);
            auto circleX = dirX * radius * sin(DEG2RAD * angleDeg);
            auto circleY = dirY * radius * sin(DEG2RAD * angleDeg);

            // draw sides
            auto circleCount = sphere.m_circle.size() - 1;
            auto circle = sphere.m_circle.typedData();
            base::Vector3 root, prev;
            for (uint32_t i = 0; i < circleCount; ++i)
            {
                auto a = circle[i + 0].transform(circleX, circleY) + bottom;
                auto b = circle[i + 1].transform(circleX, circleY) + bottom;
                tris.triangle(top, b, a);

                if (i >= 1)
                    tris.triangle(root, a, b);
                else
                    root = a;
            }

            tris.flush(*this);
        }

        void DebugSolidDrawer::plane(const base::Vector3& pos, const base::Vector3& normal, float size /*= 10.0f*/, int gridSize /*= 10*/)
        {
            auto side = base::Vector3::EX();
            if (normal.largestAxis() == 0)
                side = base::Vector3::EZ();

            auto dirX = base::Cross(normal, side).normalized();
            auto dirY = base::Cross(normal, dirX).normalized();

            auto minX = -dirX * size;
            auto maxX = dirX * size;
            auto minY = -dirY * size;
            auto maxY = dirY * size;

            DebugVertex verts[4];
            verts[0].pos(pos + minX + minY).uv(0.0f, 0.0f).color(m_color);
            verts[1].pos(pos + maxX + minY).uv(1.0f, 0.0f).color(m_color);
            verts[2].pos(pos + maxX + maxY).uv(1.0f, 1.0f).color(m_color);
            verts[3].pos(pos + minX + maxY).uv(0.0f, 1.0f).color(m_color);

            auto base = appendVertices(verts, 4);
            appendAutoTriangleFanIndices(base, 4);
        }

        void DebugSolidDrawer::frustum(const base::Matrix& frustumMatrix, float farPlaneScale /*= 1.0f*/)
        {
            helper::TriBuilder<256> tris;

            base::Vector3 frustumMin(-1.0f, -1.0f, 0.0f);
            base::Vector3 frustumMax(+1.0f, +1.0f, farPlaneScale);

            base::Vector3 unprojected[8];
            for (uint32_t i = 0; i < 8; ++i)
            {
                base::Vector4 pos;
                pos.x = (i & 1) ? frustumMax.x : frustumMax.x;
                pos.y = (i & 2) ? frustumMax.y : frustumMax.y;
                pos.z = (i & 4) ? frustumMax.z : frustumMax.z;
                pos.w/* = 1.0f */;

                auto temp = frustumMatrix.transformVector4(pos);
                unprojected[i] = base::Vector3(temp.x / temp.w, temp.y / temp.w, temp.z / temp.w);
            }

            uint16_t indices[] = { 0,1,3,0,3,2,  6,7,5,6,5,4, 5,7,3,5,3,1, 6,4,0,6,0,2, 5,1,0,5,0,4, 3,7,6,3,6,2 };
            auto base = appendVertices(unprojected, 8);
            appendIndices(indices, ARRAY_COUNT(indices), base);
        }

        void DebugSolidDrawer::circle(const base::Vector3& center, const base::Vector3& normal, float radius, float startAngle /*= 0.0f*/, float endAngle /*= 360.0*/)
        {
            auto& sphere = helper::GeoSphereBuilder::GetInstance();

            helper::TriBuilder<256> tris;

            // compute the cylinder direction
            if (normal.isNearZero())
                return; // to short

            // compute the side vectors
            auto side = base::Vector3::EX();
            if (normal.largestAxis() == 0)
                side = base::Vector3::EZ();

            auto dirX = base::Cross(normal, side).normalized() * radius;
            auto dirY = base::Cross(normal, dirX).normalized() * radius;

            // full circle
            if (startAngle == 0.0f && endAngle == 360.0f) // exact comparison to detect the default parameters
            {
                auto circleCount = sphere.m_circle.size() - 1;
                auto circle = sphere.m_circle.typedData();
                for (uint32_t i = 0; i < circleCount; ++i)
                {
                    auto a = circle[i + 0].transform(dirX, dirY) + center;
                    auto b = circle[i + 1].transform(dirX, dirY) + center;
                    tris.triangle(center, b, a);
                }
            }
            else
            {
                //auto dist = base::AngleDistance(endAngle, startAngle);
                auto numSegs = 1 + (uint32_t)(abs(endAngle - startAngle) / 15.0f);
                auto deltaAng = (endAngle - startAngle) / (float)numSegs;

                base::Vector3 prevPos = center;
                for (uint32_t i = 0; i <= numSegs; ++i)
                {
                    auto angle = DEG2RAD * (startAngle + i * deltaAng);

                    base::Vector3 curPos = center;
                    curPos += dirX * cos(angle);
                    curPos += dirY * sin(angle);

                    if (i >= 1)
                        tris.triangle(center, prevPos, curPos);
                    prevPos = curPos;
                }
            }

            tris.flush(*this);
        }

        void DebugSolidDrawer::rect(float x, float y, float w, float h)
        {
            DebugVertex v[4];
            v[0].pos(x, y).uv(0.0f, 0.0f).color(m_color);
            v[1].pos(x + w, y).uv(1.0f, 0.0f).color(m_color);
            v[2].pos(x + w, y + h).uv(1.0f, 1.0f).color(m_color);
            v[3].pos(x, y + h).uv(0.0f, 1.0f).color(m_color);

            auto firstVertex = appendVertices(v, 4);
            appendAutoTriangleFanIndices(firstVertex, 4);
        }

        void DebugSolidDrawer::recti(int x0, int  y0, int  w, int  h)
        {
            rect((float)x0, (float)y0, (float)w, (float)h);
        }

        void DebugSolidDrawer::bounds(float x0, float y0, float x1, float y1)
        {
            DebugVertex v[4];
            v[0].pos(x0, y0).uv(0.0f, 0.0f).color(m_color);
            v[1].pos(x1, y0).uv(1.0f, 0.0f).color(m_color);
            v[2].pos(x1, y1).uv(1.0f, 1.0f).color(m_color);
            v[3].pos(x0, y1).uv(0.0f, 1.0f).color(m_color);

            auto firstVertex = appendVertices(v, 4);
            appendAutoTriangleFanIndices(firstVertex, 4);
        }

        void DebugSolidDrawer::boundsi(int  x0, int  y0, int  x1, int  y1)
        {
            bounds((float)x0, (float)y0, (float)x1, (float)y1);
        }

        //--

        namespace helper
        {

            //--

            //base::res::StaticResource<base::font::BitmapFont> resScreenFontNormal("engine/fonts/DejaVuSansMono.ttf:size=16");
            //base::res::StaticResource<base::font::BitmapFont> resScreenFontBold("engine/fonts/DejaVuSansMono-Bold.ttf:size=16");
            //base::res::StaticResource<base::font::BitmapFont> resScreenFontItalic("engine/fonts/DejaVuSansMono-Oblique.ttf:size=16");
            //base::res::StaticResource<base::font::BitmapFont> resScreenFontBig("engine/fonts/DejaVuSansMono.ttf:size=32");
            //base::res::StaticResource<base::font::BitmapFont> resScreenFontTiny("engine/fonts/DejaVuSansMono.ttf:size=12");

            static const base::font::BitmapFont* GetFont(DebugFont font)
            {
                /*switch (font)
                {
                case DebugFont::Normal: return resScreenFontNormal.loadAndGet().get();
                case DebugFont::Bold: return resScreenFontBold.loadAndGet().get();
                case DebugFont::Italic: return resScreenFontItalic.loadAndGet().get();
                case DebugFont::Big: return resScreenFontBig.loadAndGet().get();
                case DebugFont::Small: return resScreenFontTiny.loadAndGet().get();
                }*/

                return nullptr;
            }

            //--

            class ScreenCanvasPrinter
            {
            public:
                static const uint32_t MAX_PAGES = 16;

                struct TextPage
                {
                    base::PagedBuffer<DebugVertex> m_vertices;
                    base::PagedBuffer<uint32_t> m_indices;

                    TextPage()
                        : m_vertices(POOL_TEMP)
                        , m_indices(POOL_TEMP)
                    {}
                };

                TextPage m_pageData[MAX_PAGES];
                uint32_t m_pageCount = 0;

                void buildGeometry(float ox, float oy, float s, base::StringView<char> txt, const base::Array<base::font::BitmapFontPrintableGlyph>& glyphs)
                {
                    /*for (auto& g : glyphs)
                    {
                        auto x0 = ox + (g.placement.x + g.glyph->offset.x) * s;
                        auto x1 = ox + (g.placement.x + g.glyph->offset.x + g.glyph->size.x) * s;
                        auto y0 = oy + (g.placement.y + g.glyph->offset.y) * s;
                        auto y1 = oy + (g.placement.y + g.glyph->offset.y + g.glyph->size.y) * s;

                        DebugVertex vertices[4];
                        vertices[0].pos(x0, y0, 0.5f).uv(g.glyph->uv.x, g.glyph->uv.y).color(g.color);
                        vertices[1].pos(x1, y0, 0.5f).uv(g.glyph->uv.z, g.glyph->uv.y).color(g.color);
                        vertices[2].pos(x1, y1, 0.5f).uv(g.glyph->uv.z, g.glyph->uv.w).color(g.color);
                        vertices[3].pos(x0, y1, 0.5f).uv(g.glyph->uv.x, g.glyph->uv.w).color(g.color);

                        auto first = page.m_vertices.size();
                        page.m_vertices.write(vertices, ARRAY_COUNT(vertices));

                        uint32_t indices[6];
                        indices[0] = first + 0;
                        indices[1] = first + 1;
                        indices[2] = first + 2;
                        indices[3] = first + 0;
                        indices[4] = first + 2;
                        indices[5] = first + 3;
                        page.m_indices.write(indices, ARRAY_COUNT(indices));
                    }*/
                }

                void submitGeometry(FrameParams& frame, const base::image::ImagePtr& texture)
                {
                    for (uint32_t i = 0; i < m_pageCount; ++i)
                    {
                        auto& page = m_pageData[i];
                        if (!page.m_vertices.empty())
                        {
                            /*auto g = frame.allocator().create<FrameGeometry>();
                            g->layer = FrameGeometryRenderingLayer::Screen;
                            g->constColor = base::Color::WHITE;
                            g->localToWorld = nullptr;
                            g->imagePtr = texture;
                            g->mode = FrameGeometryRenderMode::AlphaBlendPremult;
                            g->vertices = ExtractData(frame, page.m_vertices);
                            g->numVertices = page.m_vertices.size();
                            g->indices = ExtractData(frame, page.m_indices);
                            g->numIndices = page.m_indices.size();
                            frame.submitGeometry(g);*/
                        }
                    }
                }
            };
        }


        base::Point DebugSolidDrawer::textSize(base::StringView<char> txt, DebugFont font /*= DebugFont::Normal*/)
        {
            if (auto fontPtr = helper::GetFont(font))
                return fontPtr->measure(txt);
            else
                return base::Point::ZERO();
        }

        base::Point DebugSolidDrawer::text(int x, int y, base::StringView<char> txt, const DebugTextParams& params /*= DebugTextParams()*/)
        {
            if (auto fontPtr = helper::GetFont(params._font))
            {
                // compute glyph placements and the whole text size
                base::InplaceArray<base::font::BitmapFontPrintableGlyph, 256> glyphs;
                auto measure = fontPtr->render(txt, params._color, glyphs);

                // align horizontally
                if (params._alignX > 0)
                    x -= measure.x;
                else if (params._alignX == 0)
                    x -= measure.x / 2;

                // align vertically
                y += fontPtr->ascender();
                if (params._alignY == 0)
                    y -= measure.y / 2;
                else if (params._alignY > 0)
                    y -= measure.y;

                // offset whole thing
                x += params._offsetX;
                y += params._offsetY;

                // push the bounding box
                if (params._boxBackground.a || params._boxFrame.a)
                {
                    auto x0 = (int)(x - params._boxMargin);
                    auto y0 = (int)(y - params._boxMargin - fontPtr->ascender());
                    auto x1 = (int)(x + measure.x + params._boxMargin);
                    auto y1 = (int)(y + fontPtr->lineHeight() + params._boxMargin);

                    if (params._boxBackground.a > 0)
                    {
                        DebugVertex v[4];
                        v[0].pos(x0, y0).uv(0.0f, 0.0f).color(params._boxBackground);
                        v[1].pos(x1, y0).uv(1.0f, 0.0f).color(params._boxBackground);
                        v[2].pos(x1, y1).uv(1.0f, 1.0f).color(params._boxBackground);
                        v[3].pos(x0, y1).uv(0.0f, 1.0f).color(params._boxBackground);

                        auto firstVertex = appendVertices(v, 4);
                        appendAutoTriangleFanIndices(firstVertex, 4);
                    }

                    /*if (params._boxFrame.a > 0)
                        wire(dg, x0, y0, x1, y1, params._boxFrame, params._boxFrameWidth);*/
                }

                //selectImage(fontPtr->imageAtlas());

                /*ScreenCanvasPrinter printer;
                printer.buildGeometry(x * m_scale, y * m_scale, m_scale, txt, glyphs);
                printer.submitGeometry(m_frame, fontPtr->imageAtlas());*/


                //selectImage(nullptr);
                return measure;
            }

            return base::Point(0, 0);
        }

        //--

    } // rendering
} // scene
