/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: command #]
***/

#include "build.h"
#include "renderingSceneTest.h"
#include "renderingSceneTestApp.h"

#include "base/app/include/application.h"
#include "base/input/include/inputContext.h"
#include "base/input/include/inputStructures.h"
#include "base/canvas/include/canvas.h"

#include "rendering/driver/include/renderingDriver.h"
#include "rendering/driver/include/renderingOutput.h"
#include "rendering/driver/include/renderingCommandBuffer.h"
#include "rendering/canvas/include/renderingCanvasRenderingService.h"
#include "rendering/driver/include/renderingDeviceService.h"
#include "rendering/driver/include/renderingCommandWriter.h"
#include "base/app/include/launcherPlatform.h"
#include "../../scene/include/renderingFrameRenderingService.h"

namespace rendering
{
    namespace test
    {

        SceneTestProject::SceneTestProject()
            : m_currentTest(nullptr)
            , m_currentTestCaseIndex(-1)
            , m_pendingTestCaseIndex(0)
            , m_exitRequested(false)
        {
        }

        void SceneTestProject::cleanup()
        {
            m_currentTest.reset();

            if (m_imgui)
            {
                ImGui::SetCurrentContext(nullptr);
                ImGui::DestroyContext(m_imgui);
                m_imgui = nullptr;
            }

            if (m_renderingOutput)
            {
                base::GetService<DeviceService>()->device()->releaseObject(m_renderingOutput);
                m_renderingOutput = ObjectID();
            }
        }

        bool SceneTestProject::initialize(const base::app::CommandLine& cmdLine)
        {
            // list all test classes
            base::InplaceArray<base::ClassType, 100> testClasses;
            RTTI::GetInstance().enumClasses(ISceneTest::GetStaticClass(), testClasses);
            TRACE_INFO("Found {} test classes", testClasses.size());

            // sort test classes by the order
            std::sort(testClasses.begin(), testClasses.end(), [](base::ClassType a, base::ClassType b)
                {
                    auto orderA  = a->findMetadata<SceneTestOrderMetadata >();
                    auto orderB  = b->findMetadata<SceneTestOrderMetadata >();

                    auto valA = orderA ? orderA->m_order : -1;
                    auto valB = orderB ? orderB->m_order : -1;

                    return valA < valB;
                });

            // use only classes with valid order
            auto initialTest = cmdLine.singleValue("test");
            for (auto testClass  : testClasses)
            {
                auto shortTestName = testClass->name().view().afterLast("::").afterFirstOrFull("SceneTest_");

                auto order  = testClass->findMetadata<SceneTestOrderMetadata >();
                if (!order || order->m_order < 0)
                {
                    if (initialTest != shortTestName)
                    {
                        TRACE_WARNING("Test class '{}' has no order specified, ignoring", testClass->name());
                        continue;
                    }
                }

                auto& testCase = m_testClasses.emplaceBack();
                testCase.m_testClass = testClass;
                testCase.m_testName = base::StringBuf(shortTestName);
            }

            // nothing to test
            TRACE_INFO("Found {} valid test cases", m_testClasses.size());
            if (m_testClasses.empty())
                return 0;

            // print tests (for reference)
            for (uint32_t i = 0; i < m_testClasses.size(); ++i)
            {
                if (initialTest == m_testClasses[i].m_testName)
                {
                    m_pendingTestCaseIndex = i;
                }
            }

            // setup rendering output
            if (!createRenderingOutput())
                return false;

            m_imgui = ImGui::CreateContext(ImGui::GetSharedFontAtlas());
            ImGui::SetCurrentContext(m_imgui);

            m_lastUpdateTime.resetToNow();
            return true;
        }

        bool SceneTestProject::createRenderingOutput()
        {
            // determine viewport resolution for the test
            // NOTE: the tests may have hard coded placement so this resolution should not be changed that much
            uint32_t viewportWidth = 1920;
            uint32_t viewportHeight = 1080;
            TRACE_INFO("Test viewport size: {}x{}", viewportWidth, viewportHeight);

            rendering::DriverOutputInitInfo setup;
            setup.m_width = viewportWidth;
            setup.m_height = viewportHeight;
            setup.m_windowTitle = "Boomer Engine Scene Tests";
            setup.m_class = DriverOutputClass::NativeWindow;

            // create rendering output
            m_renderingOutput = base::GetService<DeviceService>()->device()->createOutput(setup);
            if (!m_renderingOutput)
            {
                TRACE_ERROR("Failed to acquire window factory, no window can be created");
                return false;
            }

            // get window
            m_renderingWindow = base::GetService<DeviceService>()->device()->queryOutputWindow(m_renderingOutput);
            if (!m_renderingWindow)
            {
                TRACE_ERROR("Rendering output has no valid window");
                return false;
            }

            return true;
        }

        base::RefPtr<ISceneTest> SceneTestProject::initializeTest(uint32_t testIndex)
        {
            auto& testClass = m_testClasses[testIndex];

            // initialize the test
            auto testCase = testClass.m_testClass->create<ISceneTest>();
            if (!testCase || !testCase->processInitialization())
            {
                TRACE_ERROR("Failed to initialize test '{}'", testClass.m_testClass->name());
                return nullptr;
            }

            return testCase;
        }

        void SceneTestProject::update()
        {
            // exit when window closed
            if (m_renderingWindow->windowHasCloseRequest())
            {
                TRACE_INFO("Main window closed, exiting");
                base::platform::GetLaunchPlatform().requestExit("Window closed");
                return;
            }

            // user exit requested
            if (m_exitRequested)
            {
                base::platform::GetLaunchPlatform().requestExit("User exit");
                return;
            }

            // simulation
            {
                auto systemTimeElapsed = m_lastUpdateTime.timeTillNow().toSeconds();
                m_lastUpdateTime.resetToNow();

                auto gameTime = m_lastGameTime;
                if (m_timeAdvance)
                {
                    if (m_timeMultiplier)
                        gameTime += systemTimeElapsed * std::pow(2.0f, m_timeMultiplier);
                    else
                        gameTime += systemTimeElapsed;

                    float dt = std::clamp<float>(gameTime - m_lastGameTime, 0.0001f, 0.2f);
                    if (dt > 0.0f)
                        handleTick(dt);

                    m_lastGameTime = gameTime;
                }
            }

            // process events
            if (auto inputContext = m_renderingWindow->windowGetInputContext())
                handleInput(*inputContext);

            // navigate to other test
            if (m_pendingTestCaseIndex != m_currentTestCaseIndex)
            {
                // close current test
                if (m_currentTest)
                    m_currentTest.reset();

                // initialize new test
                m_currentTestCaseIndex = m_pendingTestCaseIndex;
                m_currentTest = initializeTest(m_currentTestCaseIndex);

                // set the window caption
                auto testName = m_testClasses[m_currentTestCaseIndex].m_testName;
                m_renderingWindow->windowSetTitle(base::TempString("Boomer Engine Scene Tests - {} {}", testName, m_currentTest ? "" : "(FAILED TO INITIALIZE)"));
            }

            // create output frame, this will fail if output is not valid
            {
                base::Rect viewport;
                ImageView colorBackBuffer, depthBackBuffer;

                command::CommandWriter cmd("TestScene");

                if (cmd.opAcquireOutput(m_renderingOutput, viewport, colorBackBuffer, &depthBackBuffer))
                {
                    prepareSceneCommandBuffers(cmd, colorBackBuffer, depthBackBuffer, viewport);
                    prepareCanvasCommandBuffers(cmd, colorBackBuffer, depthBackBuffer, viewport);

                    cmd.opSwapOutput(m_renderingOutput);
                }

                base::GetService<DeviceService>()->device()->submitWork(cmd.release());
            }
        }

        void SceneTestProject::handleTick(float dt)
        {
            if (m_currentTest)
                m_currentTest->update(dt);

            m_camera.update(dt);

            ImGui::GetIO().DeltaTime = dt;
        }

        bool SceneTestProject::handleAppInputEvent(const base::input::BaseEvent& evt)
        {
            auto keyEvt = evt.toKeyEvent();
            if (keyEvt && keyEvt->pressed())
            {
                if (keyEvt->keyCode() == base::input::KeyCode::KEY_ESCAPE)
                {
                    m_exitRequested = true;
                    return true;
                }
                else if (keyEvt->keyCode() == base::input::KeyCode::KEY_LEFT)
                {
                    if (m_pendingTestCaseIndex <= 0)
                        m_pendingTestCaseIndex = m_testClasses.size() - 1;
                    else
                        m_pendingTestCaseIndex -= 1;
                    return true;
                }
                else if (keyEvt->keyCode() == base::input::KeyCode::KEY_RIGHT)
                {
                    m_pendingTestCaseIndex += 1;
                    if (m_pendingTestCaseIndex >= (int)m_testClasses.size())
                        m_pendingTestCaseIndex = 0;
                    return true;
                }
                else if (keyEvt->keyCode() == base::input::KeyCode::KEY_R)
                {
                    m_timeMultiplier = 0;
                }
                else if (keyEvt->keyCode() == base::input::KeyCode::KEY_SPACE)
                {
                    m_timeAdvance = !m_timeAdvance;
                }
                else if (keyEvt->keyCode() == base::input::KeyCode::KEY_MINUS)
                {
                    m_timeMultiplier -= 1;
                }
                else if (keyEvt->keyCode() == base::input::KeyCode::KEY_EQUAL)
                {
                    m_timeMultiplier += 1;
                }
            }

            return false;
        }

        bool SceneTestProject::handleInputEvent(const base::input::BaseEvent& evt)
        {
            if (handleAppInputEvent(evt))
                return true;

            if (ImGui::ProcessInputEvent(m_imgui, evt))
                return true;

            if (m_camera.processInput(evt))
                return true;

            if (m_currentTest && m_currentTest->processInput(evt))
                return true;

            return false;
        }

        void SceneTestProject::handleInput(base::input::IContext& context)
        {
            while (auto evt = context.pull())
                handleInputEvent(*evt);
        }
        
        void SceneTestProject::prepareSceneCommandBuffers(command::CommandWriter& cmd, const ImageView& color, const ImageView& depth, base::Rect& area)
        {
            // use simple camera position
            scene::CameraSetup cameraSetup;
            cameraSetup.aspect = color.width() / (float)color.height();
            m_camera.calculateRenderData(cameraSetup);

            // calculate camera matrices
            scene::Camera camera;
            camera.setup(cameraSetup);

            // prepare and render frame
            scene::FrameParams frame(color.width(), color.height(), camera);
            if (m_currentTest)
                m_currentTest->setupFrame(frame);

            // generate command buffers
            if (auto sceneRenderingCommands = base::GetService<scene::FrameRenderingService>()->renderFrame(frame, color))
                cmd.opAttachChildCommandBuffer(sceneRenderingCommands);
        }

        static base::res::StaticResource<base::font::Font> resDefaultFont("engine/fonts/aileron_regular.otf");

        static void Print(base::canvas::Canvas& c, const base::StringBuf& text, base::Color color = base::Color::WHITE, int size=16, 
            base::font::FontAlignmentHorizontal align = base::font::FontAlignmentHorizontal::Left, bool bold=false)
        {
            if (auto font = resDefaultFont.loadAndGet())
            {
                base::font::FontStyleParams params;
                params.bold = bold;
                params.size = size;

                base::font::GlyphBuffer glyphs;
                base::font::FontAssemblyParams assemblyParams;
                assemblyParams.horizontalAlignment = align;
                font->renderText(params, assemblyParams, base::font::FontInputText(text.c_str()), glyphs);

                base::canvas::GeometryBuilder b;
                b.fillColor(color);
                b.print(glyphs);
                c.place(b);
            }
        }

        void SceneTestProject::prepareCanvasCommandBuffers(command::CommandWriter& parentCmd, const ImageView& color, const ImageView& depth, base::Rect& area)
        {
            base::canvas::Canvas canvas(color.width(), color.height());
            renderCanvas(canvas);

            command::CommandWriter cmd(parentCmd.opCreateChildCommandBuffer());

            // set frame buffer
            FrameBuffer fb;
            fb.color[0].view(color);// .clear(0.2, 0.2f, 0.2f, 1.0f);
            fb.depth.view(depth).clearDepth().clearStencil();
            cmd.opBeingPass(fb);

            // render canvas to command buffer
            canvas::CanvasRenderingParams renderingParams;
            renderingParams.frameBufferWidth = color.width();
            renderingParams.frameBufferHeight = depth.height();
            base::GetService<CanvasService>()->render(cmd, canvas, renderingParams);

            // finish pass
            cmd.opEndPass();
        }

        void SceneTestProject::renderCanvas(base::canvas::Canvas& canvas)
        {
            // Local stuff
            {
                canvas.placement(canvas.width() - 20, canvas.height() - 60);
                Print(canvas,
                    base::TempString("Camera Position: [X={}, Y={}, Z={}]", Prec(m_camera.position().x, 2), Prec(m_camera.position().y, 2), Prec(m_camera.position().z, 2)),
                    base::Color::WHITE, 16, base::font::FontAlignmentHorizontal::Right);

                canvas.placement(canvas.width() - 20, canvas.height() - 40);
                Print(canvas,
                    base::TempString("Camera Rotation: [P={}, Y={}]", Prec(m_camera.rotation().pitch, 1), Prec(m_camera.rotation().yaw, 1)),
                    base::Color::WHITE, 16, base::font::FontAlignmentHorizontal::Right);

                if (!m_timeAdvance)
                {
                    canvas.placement(canvas.width() - 20, 20);
                    Print(canvas,
                        base::TempString("PAUSED"),
                        base::Color::ORANGERED, 20, base::font::FontAlignmentHorizontal::Right);
                }
                else if (m_timeMultiplier > 0)
                {
                    canvas.placement(canvas.width() - 20, 20);
                    Print(canvas,
                        base::TempString("Time x{} faster", 1ULL << m_timeMultiplier),
                        base::Color::ORANGERED, 20, base::font::FontAlignmentHorizontal::Right);
                }
                else if (m_timeMultiplier < 0)
                {
                    canvas.placement(canvas.width() - 20, 20);
                    Print(canvas,
                        base::TempString("Time x{} slower", 1ULL << (-m_timeMultiplier)),
                        base::Color::ORANGERED, 20, base::font::FontAlignmentHorizontal::Right);
                }
            }

            // ImGui
            {
                ImGui::BeginCanvasFrame(canvas);
                renderGui();
                ImGui::EndCanvasFrame(canvas);
            }
        }

        void SceneTestProject::renderGui()
        {
            // TEST
            static bool open = false;
            ImGui::ShowDemoWindow(&open);

            if (m_currentTest)
                m_currentTest->configure();
        }

    } // test
} // rendering

base::app::IApplication& GetApplicationInstance()
{
    static rendering::test::SceneTestProject theApp;
    return theApp;
}

