/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: elements\controls\simple #]
***/

#include "build.h"
#include "uiDragger.h"
#include "uiInputAction.h"
#include "uiTextLabel.h"

namespace ui
{

    ///---

    base::ConfigProperty<int> cvDraggerDeadZone("Editor", "DraggerDeadZone", 10);
    base::ConfigProperty<int> cvDraggerPixelRatio("Editor", "DraggerPixelStep", 4);

    ///---

    /// input action for dragging
    class DraggerInputAction : public IInputAction
    {
    public:
        DraggerInputAction(Dragger* box, const base::Point& initialPosition)
            : IInputAction(box)
            , m_box(box)
            , m_pixelDelta(0)
        {}

        virtual InputActionResult onKeyEvent(const base::input::KeyEvent& evt) override
        {
            // revert back to old value on ESC key
            if (evt.pressed() && evt.keyCode() == base::input::KeyCode::KEY_ESCAPE)
            {
                m_box->call("OnDragCanceled"_id);
                return nullptr;
            }

            return InputActionResult(); // continue
        }

        virtual InputActionResult onMouseEvent(const base::input::MouseClickEvent& evt, const ElementWeakPtr& hoverStack) override
        {
            if (evt.leftReleased())
            {
                m_box->call("OnDragFinished"_id);
                return nullptr;
            }

            return InputActionResult(); // continue
        }

        virtual InputActionResult onMouseMovement(const base::input::MouseMovementEvent& evt, const ElementWeakPtr& hoverStack) override
        {
            const auto pixelStep = std::max<int>(1, cvDraggerPixelRatio.get());

            int stepScale = 10;
            if (evt.keyMask().isCtrlDown())
                stepScale = 1;
            else if (evt.keyMask().isShiftDown())
                stepScale = 100;

            m_pixelDelta += (int)evt.delta().y;

            int64_t valueDelta = 0;
            if (m_pixelDelta > cvDraggerDeadZone.get())
            {
                const auto steps = (m_pixelDelta - cvDraggerDeadZone.get()) / pixelStep;
                if (steps > 0)
                {
                    m_pixelDelta -= steps * pixelStep;
                    valueDelta += steps * stepScale;
                }
            }
            else if (m_pixelDelta < -cvDraggerDeadZone.get())
            {
                const auto steps = (cvDraggerDeadZone.get() + m_pixelDelta) / pixelStep;
                if (steps < 0)
                {
                    m_pixelDelta -= steps * pixelStep;
                    valueDelta += steps * stepScale;
                }
            }

            if (valueDelta)
                m_box->call("OnDrag"_id, -valueDelta);

            return InputActionResult(); // continue
        }

        virtual bool allowsHoverTracking() const override
        {
            return false;
        }

        virtual bool fullMouseCapture() const override
        {
            return true;
        }

    private:
        Dragger* m_box;
        int64_t m_pixelDelta;
    };

    ///---

    Dragger::Dragger()
    {
        hitTest(true);
        enableAutoExpand(false, false);
    }

    InputActionPtr Dragger::handleMouseClick(const ui::ElementArea &area, const base::input::MouseClickEvent &evt)
    {
        if (evt.leftClicked())
        {
            call("OnDragStarted"_id);
            return base::CreateSharedPtr<DraggerInputAction>(this, evt.absolutePosition());
        }

        return nullptr;
    }

    bool Dragger::handleMouseWheel(const base::input::MouseMovementEvent &evt, float delta)
    {
        /*if (evt.delta().z < 0.0f)
            call("OnValueDown"_id);
        else if (evt.delta().z > 0.0f)
            call("OnValueUp"_id);*/

        return false;
    }

    bool Dragger::handleCursorQuery(const ui::ElementArea &area, const ui::Position &absolutePosition, base::input::CursorType &outCursorType) const
    {
        outCursorType = base::input::CursorType::SizeNS;
        return true;
    }

    RTTI_BEGIN_TYPE_NATIVE_CLASS(Dragger);
        RTTI_METADATA(ElementClassNameMetadata).name("Dragger");
    RTTI_END_TYPE();

    ///---

}

