/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: elements\controls\layout #]
***/

#pragma once

#include "uiButton.h"

namespace ui
{

    //--

    /// element in the column header
    class BASE_UI_API ColumnHeader : public Button
    {
        RTTI_DECLARE_VIRTUAL_CLASS(ColumnHeader, Button);

    public:
        ColumnHeader(base::StringView<char> text, bool canResize=true, bool canSort=true);

        void refreshSortingVisualization(bool active, bool desc);

    private:
        ColumnHeader();

        bool m_canSort;
        bool m_canResize;

        base::RefPtr<TextLabel> m_sortingIcon;
    };

    //--

} // ui