/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: elements\controls\data #]
***/

#pragma once

#include "uiDataBox.h"

namespace ui
{
    ///---

    class TextEditor;

    /// test based data box, type is required to support from sting/to string
    class BASE_UI_API IDataBoxText : public IDataBox
    {
        RTTI_DECLARE_VIRTUAL_CLASS(IDataBoxText, IDataBox);

    public:
        IDataBoxText();

        // read value as text
        virtual bool readText(base::StringBuf& outText) = 0;

        // write new value, will create undo action if action history is provided
        virtual bool writeText(const base::StringBuf& text) = 0;
 
    protected:
        virtual void handleValueChange() override;
        virtual void enterEdit() override;
        virtual void cancelEdit() override;

        void write();

        base::RefPtr<TextEditor> m_editBox;
    };

    ///---

} // ui