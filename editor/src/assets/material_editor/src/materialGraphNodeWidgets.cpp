/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: editor #]
***/

#include "build.h"
#include "materialGraphEditor.h"
#include "materialGraphNodeWidgets.h"

#include "base/ui/include/uiEditBox.h"
#include "base/object/include/dataView.h"
#include "base/object/include/rttiDataView.h"
#include "base/ui/include/uiDataBox.h"
#include "base/graph/include/graphBlock.h"

namespace ed
{
    //--

    RTTI_BEGIN_TYPE_CLASS(MaterialGraphConstantScalarWidget);
    RTTI_METADATA(ui::GraphNodeInnerWidgetBlockClassMetadata)
        .addBlockClass("rendering::MaterialGraphBlock_ConstFloat"_id)
        .addBlockClass("rendering::MaterialGraphBlock_ConstVector2"_id)
        .addBlockClass("rendering::MaterialGraphBlock_ConstVector3"_id)
        .addBlockClass("rendering::MaterialGraphBlock_ConstVector4"_id);
    RTTI_END_TYPE();

    bool MaterialGraphConstantScalarWidget::bindToBlock(base::graph::Block* block)
    {
        if (block)
        {
            if (auto proxy = block->createDataProxy())
            {
                base::rtti::DataViewInfo info;
                if (proxy->describe(0, "value", info))
                {
                    info.flags |= base::rtti::DataViewInfoFlagBit::VerticalEditor;
                    if (auto box = ui::IDataBox::CreateForType(info))
                    {
                        box->bind(proxy, "value"); // TODO: bind undo redo
                        box->customHorizontalAligment(ui::ElementHorizontalLayout::Expand);
                        box->customVerticalAligment(ui::ElementVerticalLayout::Middle);
                        if (block->cls()->name().view().endsWith("ConstFloat"))
                            box->customMinSize(50, 20);
                        else
                            box->customMinSize(80, 20);
                        attachChild(box);
                        return true;
                    }
                }
            }
        }

        return false;
    }

    void MaterialGraphConstantScalarWidget::bindToActionHistory(base::ActionHistory* history)
    {
        // TODO
    }

    //--

    RTTI_BEGIN_TYPE_CLASS(MaterialGraphConstantColorWidget);
    RTTI_METADATA(ui::GraphNodeInnerWidgetBlockClassMetadata).addBlockClass("rendering::MaterialGraphBlock_ConstColor"_id);
    RTTI_END_TYPE();

    bool MaterialGraphConstantColorWidget::bindToBlock(base::graph::Block* block)
    {
        if (block)
        {
            if (auto proxy = block->createDataProxy())
            {
                base::rtti::DataViewInfo info;
                if (proxy->describe(0, "color", info))
                {
                    info.flags |= base::rtti::DataViewInfoFlagBit::VerticalEditor;
                    if (auto box = ui::IDataBox::CreateForType(info))
                    {
                        box->bind(proxy, "color"); // TODO: bind undo redo
                        box->customMinSize(100, 20);
                        box->customHorizontalAligment(ui::ElementHorizontalLayout::Expand);
                        box->customVerticalAligment(ui::ElementVerticalLayout::Middle);
                        attachChild(box);
                        return true;
                    }
                }
            }
        }

        return false;
    }

    void MaterialGraphConstantColorWidget::bindToActionHistory(base::ActionHistory* history)
    {
        // TODO
    }

    //--

    
} // ed
