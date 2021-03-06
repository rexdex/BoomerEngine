/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: command\tests #]
***/

#include "build.h"
#include "renderingTest.h"
#include "renderingTestShared.h"

#include "rendering/driver/include/renderingDriver.h"
#include "rendering/driver/include/renderingCommandWriter.h"
#include "base/image/include/image.h"
#include "base/image/include/imageUtils.h"

namespace rendering
{
    namespace test
    {
        /// test two or more textures :)
        class RenderingTest_MultiTexture : public IRenderingTest
        {
            RTTI_DECLARE_VIRTUAL_CLASS(RenderingTest_MultiTexture, IRenderingTest);

        public:
            RenderingTest_MultiTexture();
            virtual void initialize() override final;
            
            virtual void render(command::CommandWriter& cmd, float time, const ImageView& backBufferView, const ImageView& depth) override final;

        private:
            ImageView m_textureA;
            ImageView m_textureB;
            ImageView m_textureC;
            ImageView m_textureD;
            BufferView m_vertexBuffer;
            const ShaderLibrary* m_shaders;
        };

        RTTI_BEGIN_TYPE_CLASS(RenderingTest_MultiTexture);
            RTTI_METADATA(RenderingTestOrderMetadata).order(800);
        RTTI_END_TYPE();

        //---       

        RenderingTest_MultiTexture::RenderingTest_MultiTexture()
        {
        }

        static void PrepareTestGeometry(float x, float y, float w, float h, base::Array<Simple3DVertex>& outVertices)
        {
            outVertices.pushBack(Simple3DVertex(x, y, 0.5f, 0.0f, 0.0f));
            outVertices.pushBack(Simple3DVertex(x + w, y, 0.5f, 1.0f, 0.0f));
            outVertices.pushBack(Simple3DVertex(x + w, y + h, 0.5f, 1.0f, 1.0f));

            outVertices.pushBack(Simple3DVertex(x, y, 0.5f, 0.0f, 0.0f));
            outVertices.pushBack(Simple3DVertex(x + w, y + h, 0.5f, 1.0f, 1.0f));
            outVertices.pushBack(Simple3DVertex(x, y + h, 0.5f, 0.0f, 1.0f));
        }

        namespace
        {
            struct TestParams
            {
                ImageView TextureA;
                ImageView TextureB;
                ImageView TextureC;
                ImageView TextureD;
            };
        }

        void RenderingTest_MultiTexture::initialize()
        {
            m_shaders = loadShader("MultiTexture.csl");

            // generate test geometry
            base::Array<Simple3DVertex> vertices;
            PrepareTestGeometry(-0.9f, -0.9f, 1.8f, 1.8f, vertices);

            // create vertex buffer
            {
                rendering::BufferCreationInfo info;
                info.allowVertex = true;
                info.size = vertices.dataSize();

                auto sourceData = CreateSourceData(vertices);
                m_vertexBuffer = createBuffer(info, &sourceData);
            }

            m_textureA = loadImage2D("lena.png");
            m_textureB = createChecker2D(256, 32, false);
            m_textureC = createChecker2D(512, 8, false, base::Color::RED, base::Color::BLUE);
            m_textureD = m_textureA.createSampledView(ObjectID::DefaultPointSampler());
        }

        void RenderingTest_MultiTexture::render(command::CommandWriter& cmd, float time, const ImageView& backBufferView, const ImageView& depth)
        {
            FrameBuffer fb;
            fb.color[0].view(backBufferView).clear(base::Vector4(0.0f, 0.0f, 0.2f, 1.0f));

            cmd.opBeingPass(fb);

            TestParams tempParams;
            tempParams.TextureA = m_textureA;
            tempParams.TextureB = m_textureB;
            tempParams.TextureC = m_textureC;
            tempParams.TextureD = m_textureD;
            cmd.opBindParametersInline("TestParams"_id, tempParams);

            cmd.opBindVertexBuffer("Simple3DVertex"_id,  m_vertexBuffer);
            cmd.opDraw(m_shaders, 0, 6); // quad

            cmd.opEndPass();
        }

    } // test
} // rendering
