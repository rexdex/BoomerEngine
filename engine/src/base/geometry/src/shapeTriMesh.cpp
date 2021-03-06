/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: shapes #]
***/

#include "build.h"
#include "shapeTriMesh.h"
#include "shapeUtils.h"
#include "shapeKdTreeBuilder.h"

namespace base
{
    namespace shape
    {
        //---

        RTTI_BEGIN_TYPE_CLASS(TriMesh);
            RTTI_PROPERTY(m_nodes);
            RTTI_PROPERTY(m_triangles);
            RTTI_PROPERTY(m_bounds);
        RTTI_END_TYPE();

        //---

        namespace helper
        {

            struct QuantizationParams
            {
                QuantizationParams(const Box& quantizationBox)
                {
                    m_offset = quantizationBox.min;

                    auto scale  = quantizationBox.max - quantizationBox.min;
                    m_scale.x = scale.x > 0.0f ? 65535.0f / scale.x : 0.0f;
                    m_scale.y = scale.y > 0.0f ? 65535.0f / scale.y : 0.0f;
                    m_scale.z = scale.z > 0.0f ? 65535.0f / scale.z : 0.0f;
                }

                INLINE void quantize(const Vector3& pos, uint16_t* outValues) const
                {
                    outValues[0] = (uint16_t)std::clamp((pos.x - m_offset.x) * m_scale.x, 0.0f, 65535.0f);
                    outValues[1] = (uint16_t)std::clamp((pos.y - m_offset.y) * m_scale.y, 0.0f, 65535.0f);
                    outValues[2] = (uint16_t)std::clamp((pos.z - m_offset.z) * m_scale.z, 0.0f, 65535.0f);
                }

            private:
                Vector3 m_offset;
                Vector3 m_scale;
            };

            static void CopyNode(const QuantizationParams& q, const kd::TreeNode* sourceNodes, const kd::TreeTriangle* sourceTriangles, TriMeshNode* destNodes, int nodeIndex, Box& outBox)
            {
                auto& sourceNode = sourceNodes[nodeIndex];
                auto& destNode = destNodes[nodeIndex];

                Box localBox;
                if (sourceNode.childIndex == -1)
                {
                    destNode.childIndex = sourceNode.firstTriangle;
                    destNode.triangleCount = sourceNode.numTriangles;

                    // compute bounding box of the node
                    for (uint32_t i=0; i<sourceNode.numTriangles; ++i)
                    {
                        auto& tri = sourceTriangles[i + sourceNode.firstTriangle];
                        localBox.merge(tri.v0);
                        localBox.merge(tri.v1);
                        localBox.merge(tri.v2);
                    }
                }
                else
                {
                    destNode.childIndex = sourceNode.childIndex;
                    destNode.triangleCount = 0;

                    // recurse to children
                    CopyNode(q, sourceNodes, sourceTriangles, destNodes, sourceNode.childIndex + 0, localBox);
                    CopyNode(q, sourceNodes, sourceTriangles, destNodes, sourceNode.childIndex + 1, localBox);
                }

                // write the quantized box
                q.quantize(localBox.min, destNode.boxMin);
                q.quantize(localBox.max, destNode.boxMax);

                // merge box into parent
                outBox.merge(localBox);
            }

            static void CopyTriangles(const kd::TreeTriangle* sourceTriangles, TriMeshTriangle* destTriangles, uint32_t firstTriangles, uint32_t numTriangles)
            {
                Box localBox;
                for (uint32_t i=0; i<numTriangles; ++i)
                {
                    auto &tri = sourceTriangles[firstTriangles + i];
                    localBox.merge(tri.v0);
                    localBox.merge(tri.v1);
                    localBox.merge(tri.v2);
                }

                QuantizationParams q(localBox);

                for (uint32_t i=0; i<numTriangles; ++i)
                {
                    auto& src = sourceTriangles[firstTriangles+i];
                    auto& dest = destTriangles[firstTriangles+i];

                    q.quantize(src.v0, dest.v0);
                    q.quantize(src.v1, dest.v1);
                    q.quantize(src.v2, dest.v2);
                    dest.chunk = src.chunk;
                }
            }

            static void CopyNodeTriangles(const kd::TreeNode* sourceNodes, const kd::TreeTriangle* sourceTriangles, uint32_t numNodes, TriMeshTriangle* destTriangles)
            {
                for (uint32_t i=0; i<numNodes; ++i)
                {
                    auto &src = sourceNodes[i];
                    if (src.childIndex == -1)
                    {
                        CopyTriangles(sourceTriangles, destTriangles, src.firstTriangle, src.numTriangles);
                    }
                }
            }

        } // helper

        //---

        ISourceMeshInterface::~ISourceMeshInterface()
        {}

        //---

        TriMesh::TriMesh()
        {}

        static mem::PoolID POOL_TRIMESH("Engine.TriMesh");

        void TriMesh::buildFromMesh(const ISourceMeshInterface& sourceMesh, const Matrix* localToParent)
        {
            kd::TreeBuilder builder;

            // extract triangles and put the in the builder
            Box bounds;
            sourceMesh.processTriangles([&builder, &bounds, localToParent](const Vector3* vertices, uint32_t chunkIndex)
                {
                    auto triangle  = builder.allocTriangles(1);
                    if (localToParent)
                    {
                        triangle->v0 = localToParent->transformPoint(vertices[0]);
                        triangle->v1 = localToParent->transformPoint(vertices[1]);
                        triangle->v2 = localToParent->transformPoint(vertices[2]);
                    }
                    else
                    {
                        triangle->v0 = vertices[0];
                        triangle->v1 = vertices[1];
                        triangle->v2 = vertices[2];
                    }
                    triangle->chunk = (uint16_t)chunkIndex;

                    bounds.merge(triangle->v0);
                    bounds.merge(triangle->v1);
                    bounds.merge(triangle->v2);
                });

            // build the tree
            kd::TreeBuildingSetup setup;
            setup.numTrianglesPerNode = 16;
            builder.build(setup);

            // setup quantization helper
            helper::QuantizationParams quantizer(bounds);

            // copy nodes
            if (auto nodeData = Buffer::Create(POOL_TRIMESH, sizeof(TriMeshNode) * builder.numNodes()))
            {
                TRACE_INFO("TriMesh node data: {}", MemSize(nodeData.size()));
                auto writeNode  = (TriMeshNode *) nodeData.data();
                Box localBox;
                helper::CopyNode(quantizer, builder.nodes(), builder.triangles(), writeNode, 0, localBox);
                m_nodes = std::move(nodeData);
            }

            // copy triangles, use local quantization within the nodes
            if (auto triangleData = Buffer::Create(POOL_TRIMESH, sizeof(TriMeshTriangle) * builder.numTriangles()))
            {
                TRACE_INFO("TriMesh triangle data: {}", MemSize(triangleData.size()));
                auto writeTri  = (TriMeshTriangle *) triangleData.data();
                helper::CopyNodeTriangles(builder.nodes(), builder.triangles(), builder.numNodes(), writeTri);
                m_triangles = std::move(triangleData);
            }

            // write data
            m_bounds = bounds;
            m_quantizationScale.x = (bounds.max.x - bounds.min.x) / 65536.0f;
            m_quantizationScale.y = (bounds.max.y - bounds.min.y) / 65536.0f;
            m_quantizationScale.z = (bounds.max.z - bounds.min.z) / 65536.0f;
        }

        //---

        bool TriMesh::isConvex() const
        {
            return true;
        }

        float TriMesh::calcVolume() const
        {
            return 1.0f;
        }

        Box TriMesh::calcBounds() const
        {
            return m_bounds;
        }

        uint32_t TriMesh::calcDataSize() const
        {
            return sizeof(TriMesh) + m_nodes.size() + m_triangles.size();
        }

        void TriMesh::calcCRC(base::CRC64& crc) const
        {
            crc << m_bounds.min.x;
            crc << m_bounds.min.y;
            crc << m_bounds.min.z;
            crc << m_bounds.max.x;
            crc << m_bounds.max.y;
            crc << m_bounds.max.z;
            crc << m_quantizationScale.x;
            crc << m_quantizationScale.y;
            crc << m_quantizationScale.z;
            crc.append(m_nodes.data(), m_nodes.size());
            crc.append(m_triangles.data(), m_triangles.size());
        }

        ShapePtr TriMesh::copy() const
        {
            auto ret  = CreateSharedPtr<TriMesh>();
            ret->m_bounds = m_bounds;
            ret->m_quantizationScale = m_quantizationScale;
            ret->m_nodes = m_nodes;
            ret->m_triangles = m_triangles;
            return ret;
        }

        bool TriMesh::contains(const Vector3& point) const
        {
            return false;
        }

        struct Dequantizer
        {
            Dequantizer(const Box& bounds, const Vector3& scale)
                : m_offset(bounds.min)
                , m_scale(scale)
            {}

            Dequantizer(const Vector3& boxMin, const Vector3& boxMax)
                : m_offset(boxMin)
            {
                m_scale.x = (boxMax.x - boxMin.x) / 65536.0f;
                m_scale.y = (boxMax.y - boxMin.y) / 65536.0f;
                m_scale.z = (boxMax.z - boxMin.z) / 65536.0f;
            }

            INLINE Vector3 dequantize(const uint16_t* v) const
            {
                return Vector3(
                    m_offset.x + (float)v[0] * m_scale.x,
                    m_offset.y + (float)v[1] * m_scale.y,
                    m_offset.z + (float)v[2] * m_scale.z);
            }

        private:
            Vector3 m_offset;
            Vector3 m_scale;
        };

        bool TriMesh::intersect(const Vector3& origin, const Vector3& direction, float maxLength, float* outEnterDistFromOrigin, Vector3* outEntryPoint, Vector3* outEntryNormal) const
        {
            Dequantizer treeDQ(m_bounds, m_quantizationScale);

            Vector3 invDir;
            invDir.x = 1.0f / direction.x;
            invDir.y = 1.0f / direction.y;
            invDir.z = 1.0f / direction.z;

            auto localMaxLength  = maxLength;
            auto localNormal  = Vector3::ZERO();
            auto ret  = false;

            auto numNodes  = this->numNodes();
            auto node  = (const TriMeshNode*) m_nodes.data();

            for (uint32_t j=0; j<numNodes; ++j, ++node)
            {
                if (node->triangleCount == 0)
                    continue;

                auto boxMin  = treeDQ.dequantize(node->boxMin);
                auto boxMax  = treeDQ.dequantize(node->boxMax);

                if (IntersectBoxRay(origin, direction, invDir, localMaxLength, boxMin, boxMax))
                {
                    Dequantizer nodeDQ(boxMin, boxMax);

                    auto numTriangles  = node->triangleCount;
                    auto tri  = (const TriMeshTriangle*) m_triangles.data() + node->childIndex;
                    for (uint32_t i=0; i<numTriangles; ++i, ++tri)
                    {
                        auto v0  = nodeDQ.dequantize(tri->v0);
                        auto v1  = nodeDQ.dequantize(tri->v1);
                        auto v2  = nodeDQ.dequantize(tri->v2);

                        if (IntersectTriangleRay(origin, direction, v0, v1, v2, localMaxLength, &localMaxLength))
                        {
                            localNormal = TriangleNormal(v0, v1, v2);
                            ret = true;
                        }
                    }
                }
            }

            if (ret)
            {
                if (outEnterDistFromOrigin)
                    *outEnterDistFromOrigin = localMaxLength;

                if (outEntryPoint)
                    *outEntryPoint = origin + localMaxLength*direction;

                if (outEntryNormal)
                    *outEntryNormal = localNormal;

                return true;
            }

            return false;
        }

        void TriMesh::render(IShapeRenderer& renderer, ShapeRenderingMode mode /*= ShapeRenderingMode::Solid*/, ShapeRenderingQualityLevel qualityLevel /*= ShapeRenderingQualityLevel::Medium*/) const
        {


        }

        //---

    } // shape
} // base