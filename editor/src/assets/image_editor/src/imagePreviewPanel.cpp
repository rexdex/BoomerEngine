/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: editor #]
***/

#include "build.h"
#include "imagePreviewPanel.h"
#include "imagePreviewPanelElements.h"

#include "rendering/driver/include/renderingShaderLibrary.h"
#include "rendering/canvas/include/renderingCanvasRendererCustomBatchHandler.h"
#include "rendering/driver/include/renderingCommandWriter.h"
#include "rendering/driver/include/renderingDeviceService.h"
#include "rendering/driver/include/renderingDriver.h"
#include "base/canvas/include/canvasGeometryBuilder.h"
#include "base/image/include/imageView.h"
#include "base/ui/include/uiElement.h"
#include "base/ui/include/uiTextLabel.h"
#include "base/ui/include/uiToolBar.h"
#include "base/ui/include/uiRuler.h"
#include "base/ui/include/uiComboBox.h"
#include "base/ui/include/uiTrackBar.h"

namespace ed
{

    //--

    RTTI_BEGIN_TYPE_CLASS(ImagePreviewPanel);
    RTTI_END_TYPE();

    ImagePreviewPanel::ImagePreviewPanel()
    {
        hitTest(true);
        m_maxZoom = 20.0f;
    }

    ImagePreviewPanel::~ImagePreviewPanel()
    {}

    void ImagePreviewPanel::previewSettings(const ImagePreviewPanelSettings& settings)
    {
        m_previewSettings = settings;
    }

    void ImagePreviewPanel::renderForeground(const ui::ElementArea& drawArea, base::canvas::Canvas& canvas, float mergedOpacity)
    {
        TBaseClass::renderForeground(drawArea, canvas, mergedOpacity);

        canvas.placement(drawArea.absolutePosition().x, drawArea.absolutePosition().y);

        ui::Position minPos = ui::Position::INF();
        ui::Position maxPos = -ui::Position::INF();

        for (auto* proxy : m_elementProxies)
        {
            minPos = base::Min(proxy->virtualPlacement, minPos);
            maxPos = base::Max(proxy->virtualPlacement + proxy->virtualSize, maxPos);
        }

        if (m_hoverPixelValid)
        {
            if (m_viewScale.x > 1.0f)
            {
                float x0 = ((m_hoverPixelX) * m_viewScale.x) + m_viewOffset.x;
                float x1 = ((m_hoverPixelX+1) * m_viewScale.x) + m_viewOffset.x;
                float y0 = ((m_hoverPixelY) * m_viewScale.y) + m_viewOffset.y;
                float y1 = ((m_hoverPixelY+1)*m_viewScale.y) + m_viewOffset.y;

                base::canvas::GeometryBuilder b;
                b.fillColor(base::Color(64, 64, 64, 64));
                b.beginPath();
                b.rect(x0, 0.0f, x1 - x0, drawArea.size().y);
                b.fill();
                b.beginPath();
                b.rect(0.0f, y0, drawArea.size().x, y1 - y0);
                b.fill();

                canvas.place(b);
            }
            else
            {
                float x = ((m_hoverPixelX)*m_viewScale.x) + m_viewOffset.x;
                float y = ((m_hoverPixelY)*m_viewScale.y) + m_viewOffset.y;

                base::canvas::GeometryBuilder b;
                b.strokeColor(base::Color(64, 64, 64, 64));
                b.beginPath();
                b.moveTo(x, 0.0f);
                b.lineTo(x, drawArea.size().y);
                b.stroke();
                b.beginPath();
                b.moveTo(0.0f, y);
                b.lineTo(drawArea.size().x, y);
                b.stroke();

                canvas.place(b);
            }
        }

        if (m_previewSettings.showGrid && m_viewScale.x >= 5.0f)
        {
            if (minPos.x < maxPos.x || minPos.y < maxPos.y)
            {
                base::Rect activeImageRect;
                activeImageRect.min.x = (int)std::floor(minPos.x);
                activeImageRect.min.y = (int)std::floor(minPos.y);
                activeImageRect.max.x = (int)std::ceil(maxPos.x);
                activeImageRect.max.y = (int)std::ceil(maxPos.y);

                const auto colorFrac = std::clamp((m_viewScale.x - 5.0f) / 5.0f, 0.0f, 1.0f);
                const auto tl = absoluteToVirtual(cachedDrawArea().absolutePosition());
                const auto br = absoluteToVirtual(cachedDrawArea().absolutePosition() + cachedDrawArea().size());

                RenderPixelBackground(canvas, tl, br, cachedDrawArea(), activeImageRect, colorFrac);
            }
        }
    }

    void ImagePreviewPanel::renderBackground(const ui::ElementArea& drawArea, base::canvas::Canvas& canvas, float mergedOpacity)
    {
        TBaseClass::renderBackground(drawArea, canvas, mergedOpacity);
    }

    void ImagePreviewPanel::updateTooltip() const
    {
        auto text = m_activeTooltipText.lock();
        auto color = m_activeTooltipColor.lock();

        if (m_previewPixelValid)
        {
            base::Color previewColor;
            if (m_previewPixel.integerValues)
            {
                previewColor.r = base::FloatTo255(m_previewPixel.color.x / m_previewPixel.maxValue);
                previewColor.g = base::FloatTo255(m_previewPixel.color.y / m_previewPixel.maxValue);
                previewColor.b = base::FloatTo255(m_previewPixel.color.z / m_previewPixel.maxValue);
                previewColor.a = 255;
            }
            else
            {
                previewColor.r = base::FloatTo255(m_previewPixel.color.x);
                previewColor.g = base::FloatTo255(m_previewPixel.color.y);
                previewColor.b = base::FloatTo255(m_previewPixel.color.z);
            }

            if (color)            
                color->customBackgroundColor(previewColor);

            base::StringBuilder txt;
            txt.appendf("[{},{}]\n\n", m_previewPixel.x, m_previewPixel.y);

            if (m_previewPixel.channels >= 1)
            {
                txt.append("[color:#F00]R:[/color] ");
                if (m_previewPixel.integerValues)
                    txt.appendf("{}", (int)m_previewPixel.color.x);
                else
                    txt.appendf("{}", Prec(m_previewPixel.color.x, 2));
                txt.append("\n");
            }

            if (m_previewPixel.channels >= 2)
            {
                txt.append("[color:#0F0]G:[/color] ");
                if (m_previewPixel.integerValues)
                    txt.appendf("{}", (int)m_previewPixel.color.y);
                else
                    txt.appendf("{}", Prec(m_previewPixel.color.y, 2));
                txt.append("\n");
            }

            if (m_previewPixel.channels >= 3)
            {
                txt.append("[color:#00F]B:[/color] ");
                if (m_previewPixel.integerValues)
                    txt.appendf("{}", (int)m_previewPixel.color.z);
                else
                    txt.appendf("{}", Prec(m_previewPixel.color.z, 2));
                txt.append("\n");
            }

            if (m_previewPixel.channels >= 4)
            {
                txt.append("[color:#AAA]A:[/color] ");
                if (m_previewPixel.integerValues)
                    txt.appendf("{}", (int)m_previewPixel.color.w);
                else
                    txt.appendf("{}", Prec(m_previewPixel.color.w, 2));
                txt.append("\n");
            }

            if (text)
                text->text(txt.view());
        }
        else if (color)
        {
            if (auto window = color->findParentWindow())
                window->requestClose();
        }
    }

    bool ImagePreviewPanel::handleMouseMovement(const base::input::MouseMovementEvent& evt)
    {
        m_previewPixelValid = false;

        const auto virtualPos = absoluteToVirtual(evt.absolutePosition());

        m_hoverPixelValid = true;
        m_hoverPixelX = (int)std::floor(virtualPos.x);
        m_hoverPixelY = (int)std::floor(virtualPos.y);

        for (const auto* proxy : m_elementProxies)
        {
            if (auto imageElement = base::rtti_cast<IImagePreviewElement>(proxy->element))
            {
                if (virtualPos.x >= proxy->virtualPlacement.x && virtualPos.x >= proxy->virtualPlacement.y)
                {
                    float x = virtualPos.x - proxy->virtualPlacement.x;
                    float y = virtualPos.y - proxy->virtualPlacement.y;
                    if (x < proxy->virtualSize.x && y < proxy->virtualSize.y)
                    {
                        if (imageElement->queryColor(x, y, m_previewPixel))
                        {
                            m_previewPixelValid = true;
                            break;
                        }
                    }
                }
            }
        }

        updateTooltip();

        return TBaseClass::handleMouseMovement(evt);
    }

    void ImagePreviewPanel::handleHoverLeave(const ui::Position& absolutePosition)
    {
        TBaseClass::handleHoverLeave(absolutePosition);

        m_hoverPixelValid = false;
        m_previewPixelValid = false;
        updateTooltip();
    }

    ui::ElementPtr ImagePreviewPanel::queryTooltipElement(const ui::Position& absolutePosition) const
    {
        if (!m_previewPixelValid)
            return nullptr;

        auto list = base::CreateSharedPtr<ui::IElement>();
        list->layoutVertical();

        auto color = list->createChild<ui::IElement>();
        color->customStyle<float>("border-width"_id, 1.0f);
        color->customStyle<float>("border-radius"_id, 8.0f);
        color->customPadding(2.0f);
        color->customMargins(6.0f);
        color->customMinSize(80, 80);
        m_activeTooltipColor = color;

        auto text = list->createChild<ui::TextLabel>();
        color->customMargins(6.0f);
        m_activeTooltipText = text;

        updateTooltip();

        return list;
    }

    //--

    RTTI_BEGIN_TYPE_CLASS(ImagePreviewPanelWithToolbar);
    RTTI_END_TYPE();

    ImagePreviewPanelWithToolbar::ImagePreviewPanelWithToolbar()
    {
        layoutVertical();

        // panel with rullers
        {
            m_previewToolbar = createChild<ui::ToolBar>();
            m_previewToolbar->name("PreviewToolbar"_id);
            m_previewToolbar->customMargins(0.0f, 0.0f, 0.0f, 5.0f);

            {
                auto topPanel = createChild<ui::IElement>();
                topPanel->customHorizontalAligment(ui::ElementHorizontalLayout::Expand);
                topPanel->customVerticalAligment(ui::ElementVerticalLayout::Top);
                topPanel->layoutHorizontal();
                topPanel->createChildWithType<ui::IElement>("RulerCorner"_id);
                m_previewHorizontalRuler = topPanel->createChild<ui::HorizontalRuler>();
            }

            {
                auto innerPanel = createChild<ui::IElement>();
                innerPanel->customHorizontalAligment(ui::ElementHorizontalLayout::Expand);
                innerPanel->customVerticalAligment(ui::ElementVerticalLayout::Expand);
                innerPanel->layoutHorizontal();

                m_previewVerticalRuler = innerPanel->createChild<ui::VerticalRuler>();
                m_previewPanel = innerPanel->createChild<ImagePreviewPanel>();
                m_previewPanel->customHorizontalAligment(ui::ElementHorizontalLayout::Expand);
                m_previewPanel->customVerticalAligment(ui::ElementVerticalLayout::Expand);
            }


            m_previewPanel->bind("OnCanvasViewChanged"_id, m_previewHorizontalRuler) = [](ui::CanvasArea* area, ui::HorizontalRuler* ruller)
            {
                ruller->region(area->virtualAreaLeft(), area->virtualAreaRight());
            };

            m_previewPanel->bind("OnCanvasViewChanged"_id, m_previewVerticalRuler) = [](ui::CanvasArea* area, ui::VerticalRuler* ruller)
            {
                ruller->region(area->virtualAreaTop(), area->virtualAreaBottom());
            };
        }

        // basic toolbar
        {
            m_previewToolbar->createSeparator();
            m_previewToolbar->createButton("ImagePreviewPanel.ToggleRedChannel"_id, "[center][img:channel_red][br][size:-]Red", "Toggle display of the [b][color:#F00]RED[/color][/b] channel");
            m_previewToolbar->createButton("ImagePreviewPanel.ToggleGreenChannel"_id, "[center][img:channel_green][br][size:-]Green", "Toggle display of the [b][color:#0F0]GREEN[/color][/b] channel");
            m_previewToolbar->createButton("ImagePreviewPanel.ToggleBlueChannel"_id, "[center][img:channel_blue][br][size:-]Blue", "Toggle display of the [b][color:#00F]BLUE[/color][/b] channel");
            m_previewToolbar->createButton("ImagePreviewPanel.ToggleAlphaChannel"_id, "[center][img:channel_alpha][br][size:-]Alpha", "Toggle display of the [b][color:#FFF]ALPHA[/color][/b] channel");
            m_previewToolbar->createSeparator();
            m_previewToolbar->createButton("ImagePreviewPanel.TogglePointFilter"_id, "[center][img:plane][br][size:-]Point filter", "Toggle point filtering of texture");
            m_previewToolbar->createButton("ImagePreviewPanel.TogglePremultiply"_id, "[center][img:blend][br][size:-]Alpha premultiply", "Toggle premultiplied blending");
            m_previewToolbar->createButton("ImagePreviewPanel.ToggleCheckers"_id, "[center][img:background][br][size:-]Background", "Toggle checked background");
            m_previewToolbar->createSeparator();
            m_previewToolbar->createButton("ImagePreviewPanel.TogglePixelGrid"_id, "[center][img:grid][br][size:-]Pixel grid", "Toggle pixel grid overlay");
            m_previewToolbar->createSeparator();
            m_previewToolbar->createButton("ImagePreviewPanel.FitZoom"_id, "[center][img:zoom][br][size:-]Fit", "Show all content");
            m_previewToolbar->createButton("ImagePreviewPanel.FillZoom"_id, "[center][img:arrow_inout][br][size:-]Fill", "Fill window with content");
            m_previewToolbar->createButton("ImagePreviewPanel.ResetZoom"_id, "[center][img:1to1][br][size:-]Reset", "Reset zoom to 1:1");

            //--

            actions().bindToggle("ImagePreviewPanel.ToggleRedChannel"_id) = [this] { return m_previewPanel->previewSettings().showRed; };
            actions().bindToggle("ImagePreviewPanel.ToggleGreenChannel"_id) = [this] { return m_previewPanel->previewSettings().showGreen; };
            actions().bindToggle("ImagePreviewPanel.ToggleBlueChannel"_id) = [this] { return m_previewPanel->previewSettings().showBlue; };
            actions().bindToggle("ImagePreviewPanel.ToggleAlphaChannel"_id) = [this] { return m_previewPanel->previewSettings().showAlpha; };
            actions().bindToggle("ImagePreviewPanel.TogglePointFilter"_id) = [this] { return m_previewPanel->previewSettings().pointFilter; };
            actions().bindToggle("ImagePreviewPanel.TogglePremultiply"_id) = [this] { return m_previewPanel->previewSettings().premultiply; };
            actions().bindToggle("ImagePreviewPanel.ToggleCheckers"_id) = [this] { return m_previewPanel->previewSettings().showCheckers; };
            actions().bindToggle("ImagePreviewPanel.TogglePixelGrid"_id) = [this] { return m_previewPanel->previewSettings().showGrid; };

            actions().bindCommand("ImagePreviewPanel.FitZoom"_id) = [this] {  m_previewPanel->zoomToFit(); };
            actions().bindCommand("ImagePreviewPanel.FillZoom"_id) = [this] {  m_previewPanel->zoomToFill(); };
            actions().bindCommand("ImagePreviewPanel.ResetZoom"_id) = [this] { m_previewPanel->resetZoomToOne(); };

            actions().bindCommand("ImagePreviewPanel.ToggleRedChannel"_id) = [this] {
                auto settings = previewSettings();
                settings.showRed = !settings.showRed;
                previewSettings(settings);
            };

            actions().bindCommand("ImagePreviewPanel.ToggleGreenChannel"_id) = [this] {
                auto settings = previewSettings();
                settings.showGreen = !settings.showGreen;
                previewSettings(settings);
            };

            actions().bindCommand("ImagePreviewPanel.ToggleBlueChannel"_id) = [this] {
                auto settings = previewSettings();
                settings.showBlue = !settings.showBlue;
                previewSettings(settings);
            };

            actions().bindCommand("ImagePreviewPanel.ToggleAlphaChannel"_id) = [this] {
                auto settings = previewSettings();
                settings.showAlpha = !settings.showAlpha;
                previewSettings(settings);
            };

            actions().bindCommand("ImagePreviewPanel.TogglePointFilter"_id) = [this] {
                auto settings = previewSettings();
                settings.pointFilter = !settings.pointFilter;
                previewSettings(settings);
            };

            actions().bindCommand("ImagePreviewPanel.ToggleCheckers"_id) = [this] {
                auto settings = previewSettings();
                settings.showCheckers = !settings.showCheckers;
                previewSettings(settings);
            };

            actions().bindCommand("ImagePreviewPanel.TogglePremultiply"_id) = [this] {
                auto settings = previewSettings();
                settings.premultiply = !settings.premultiply;
                previewSettings(settings);
            };

            actions().bindCommand("ImagePreviewPanel.TogglePixelGrid"_id) = [this] {
                auto settings = previewSettings();
                settings.showGrid = !settings.showGrid;
                previewSettings(settings);
            };

            //--
        }

        // mip selector
        {
            m_previewToolbar->createSeparator();

            m_mipmapChoiceBox = m_previewToolbar->createChildWithType<ui::ComboBox>("ToolbarComboBox"_id);
            m_mipmapChoiceBox->customMinSize(150, 10);
            m_mipmapChoiceBox->addOption("No mipmap");
            m_mipmapChoiceBox->selectOption(0);
            m_mipmapChoiceBox->visibility(false);

            m_mipmapChoiceBox->bind("OnChanged"_id) = [this](ui::ComboBox* box, ui::IElement* owner)
            {
                auto settings = previewSettings();
                settings.selectedMip = box->selectedOption();
                previewSettings(settings);
            };
        }

        // slice selector
        {
            m_previewToolbar->createSeparator();
            m_previewToolbar->createButton("ImagePreviewPanel.ShowAllSlices"_id, "All Slices", "Toggle display of all slices side by side");

            actions().bindToggle("ImagePreviewPanel.ShowAllSlices"_id) = [this] { return m_previewPanel->previewSettings().allSlices; };

            actions().bindCommand("ImagePreviewPanel.ShowAllSlices"_id) = [this] {
                auto settings = previewSettings();
                settings.allSlices = !settings.allSlices;
                previewSettings(settings);
            };

            m_sliceChoicebox = m_previewToolbar->createChildWithType<ui::ComboBox>("ToolbarComboBox"_id);
            m_sliceChoicebox->customMinSize(150, 10);
            m_sliceChoicebox->addOption("No slices");
            m_sliceChoicebox->selectOption(0);
            m_sliceChoicebox->visibility(false);

            m_sliceChoicebox->bind("OnChanged"_id) = [this](ui::ComboBox* box, ui::IElement* owner)
            {
                auto settings = previewSettings();
                settings.allSlices = false;
                settings.selectedSlice = box->selectedOption();
                previewSettings(settings);
            };
        }

        // HDR exposure/tonemap
        {
            m_previewToolbar->createSeparator();
            m_previewToolbar->createButton("ImagePreviewPanel.Tonemap"_id, "Tonemap", "Apply HDR tonemapping to image preview");

            m_toneMapMode = m_previewToolbar->createChildWithType<ui::ComboBox>("ToolbarComboBox"_id);
            m_toneMapMode->customMinSize(150, 10);
            m_toneMapMode->addOption("No tone mapping");
            m_toneMapMode->addOption("Linear");
            m_toneMapMode->addOption("Simple Reinhard");
            m_toneMapMode->addOption("Luma based Reinhard");
            m_toneMapMode->addOption("WP Luma based Reinhard");
            m_toneMapMode->addOption("RomBinDaHouse");
            m_toneMapMode->addOption("Filmic");
            m_toneMapMode->addOption("Uncharted 2");
            m_toneMapMode->selectOption(0);
            m_toneMapMode->visibility(false);

            m_toneMapMode->bind("OnChanged"_id) = [this](ui::ComboBox* box, ui::IElement* owner)
            {
                auto settings = previewSettings();
                settings.toneMapMode = box->selectedOption();
                previewSettings(settings);
            };
            
            m_exposureControl = m_previewToolbar->createChild<ui::TrackBar>();
            m_exposureControl->range(-5.0f, 5.0f);
            m_exposureControl->value(0.0f);
            m_exposureControl->resolution(1);
            m_exposureControl->units(" EV");
            m_exposureControl->visibility(false);

            m_exposureControl->bind("OnValueChanged"_id) = [this]()
            {
                auto settings = previewSettings();
                settings.exposureAdj = m_exposureControl->value();
                previewSettings(settings);
            };
        }

        // initial update
        updateMipmapList();
        updateSliceList();
    }

    ImagePreviewPanelWithToolbar::~ImagePreviewPanelWithToolbar()
    {}

    const ImagePreviewPanelSettings& ImagePreviewPanelWithToolbar::previewSettings() const
    {
        return m_previewPanel->previewSettings();
    }

    void ImagePreviewPanelWithToolbar::previewSettings(const ImagePreviewPanelSettings& settings)
    {
        auto oldSettings = previewSettings();
        m_previewPanel->previewSettings(settings);

        if (oldSettings.allSlices != settings.allSlices || oldSettings.selectedMip != settings.selectedMip || oldSettings.selectedSlice != settings.selectedSlice)
        {
            recreatePreviewItems();
        }

        updateUIState();
    }

    void ImagePreviewPanelWithToolbar::bindImageView(const rendering::ImageView& image, rendering::ImageContentColorSpace knownColorSpace)
    {
        if (image && knownColorSpace == rendering::ImageContentColorSpace::Auto)
        {
            const auto formatClass = rendering::GetImageFormatInfo(image.format()).formatClass;
            if (formatClass == rendering::ImageFormatClass::SRGB)
                knownColorSpace = rendering::ImageContentColorSpace::SRGB;
            else if (formatClass == rendering::ImageFormatClass::FLOAT)
                knownColorSpace = rendering::ImageContentColorSpace::HDR;
        }

        bool hadContent = !m_mainImage.empty();
        bool biggerContent = image.width() > m_mainImage.width() || image.height() > m_mainImage.height();
        m_mainImage = image;

        auto settings = previewSettings();

        if (m_numImageMips != image.numMips() || m_numImageSlices != image.numArraySlices() || settings.colorSpace != knownColorSpace)
        {
            m_numImageMips = image.numMips();
            m_numImageSlices = image.numArraySlices();

            if (m_numImageMips <= 1)
                settings.selectedMip = 0;
            else if (settings.selectedMip >= m_numImageMips)
                settings.selectedMip = m_numImageMips - 1;

            if (m_numImageSlices <= 1)
            {
                settings.allSlices = false;
                settings.selectedSlice = 0;
            }
            else if (settings.selectedSlice >= m_numImageSlices)
            {
                settings.selectedSlice = m_numImageSlices - 1;
            }

            settings.colorSpace = knownColorSpace;

            m_previewPanel->previewSettings(settings);

            updateMipmapList();
            updateSliceList();
            updateUIState();
        }

        recreatePreviewItems();

        if (!hadContent || biggerContent)
            m_previewPanel->zoomToFit();
    }

    void ImagePreviewPanelWithToolbar::updateUIState()
    {
        auto settings = previewSettings();

        for (const auto& elem : m_mainImagePreviewElements)
            elem->configure(settings);

        m_mipmapChoiceBox->visibility(settings.previewMode == 0);
        m_sliceChoicebox->visibility(settings.previewMode == 0 && !settings.allSlices);
        m_exposureControl->visibility(settings.colorSpace == rendering::ImageContentColorSpace::HDR);
        m_toneMapMode->visibility(settings.colorSpace == rendering::ImageContentColorSpace::HDR);

        if (settings.selectedMip == 0 && !settings.allSlices && m_mainImage)
        {
            m_previewHorizontalRuler->activeRegion(0.0f, m_mainImage.width());
            m_previewVerticalRuler->activeRegion(0.0f, m_mainImage.height());
        }
    }

    void ImagePreviewPanelWithToolbar::updateMipmapList()
    {
        m_mipmapChoiceBox->clearOptions();

        if (m_mainImage)
        {
            auto settings = previewSettings();

            for (uint32_t i = 0; i < m_numImageMips; ++i)
            {
                auto mipWidth = std::max<uint32_t>(1, m_mainImage.width() >> i);
                auto mipHeight = std::max<uint32_t>(1, m_mainImage.height() >> i);
                auto mipDepth = std::max<uint32_t>(1, m_mainImage.depth() >> i);

                if (m_mainImage.viewType() == rendering::ImageViewType::View1D || m_mainImage.viewType() == rendering::ImageViewType::View1DArray)
                    m_mipmapChoiceBox->addOption(base::TempString("Mip {} ({})", i, mipWidth));
                else if (m_mainImage.viewType() == rendering::ImageViewType::View2D || m_mainImage.viewType() == rendering::ImageViewType::View2DArray)
                    m_mipmapChoiceBox->addOption(base::TempString("Mip {} ({}x{})", i, mipWidth, mipHeight));
                else if (m_mainImage.viewType() == rendering::ImageViewType::ViewCube || m_mainImage.viewType() == rendering::ImageViewType::ViewCubeArray)
                    m_mipmapChoiceBox->addOption(base::TempString("Mip {} ({}x{})", i, mipWidth, mipHeight));
                else if (m_mainImage.viewType() == rendering::ImageViewType::View3D)
                    m_mipmapChoiceBox->addOption(base::TempString("Mip {} ({}x{}x{})", i, mipWidth, mipHeight, mipDepth));
            }

            m_mipmapChoiceBox->selectOption(settings.selectedMip);
            m_mipmapChoiceBox->enable(true);
        }
        else
        {
            m_mipmapChoiceBox->addOption("(no mips)");
            m_mipmapChoiceBox->selectOption(0);
            m_mipmapChoiceBox->enable(false);
        }
    }

    void ImagePreviewPanelWithToolbar::updateSliceList()
    {
        m_sliceChoicebox->clearOptions();

        if (m_mainImage && (m_mainImage.viewType() == rendering::ImageViewType::View1DArray || m_mainImage.viewType() == rendering::ImageViewType::View2DArray || m_mainImage.viewType() == rendering::ImageViewType::ViewCubeArray))
        {
            auto settings = previewSettings();

            for (uint32_t i = 0; i < m_numImageSlices; ++i)
                m_sliceChoicebox->addOption(base::TempString("Slice {}", i));

            m_sliceChoicebox->selectOption(settings.selectedSlice);
            m_sliceChoicebox->enable(true);
        }
        else
        {
            m_sliceChoicebox->addOption("(no slices)");
            m_sliceChoicebox->selectOption(0);
            m_sliceChoicebox->enable(false);
        }
    }

    void ImagePreviewPanelWithToolbar::recreatePreviewItems()
    {
        // detach existing elements
        for (const auto& elem : m_mainImagePreviewElements)
            m_previewPanel->removeElement(elem);
        m_mainImagePreviewElements.clear();

        // get main view
        if (auto view = m_mainImage)
        {
            // create the element
            const auto& settings = previewSettings();
            if (settings.selectedMip == 0 && settings.selectedSlice == 0 && !settings.allSlices && m_sourceImage)
            {
                auto element = base::CreateSharedPtr<ImagePreviewElement>(m_mainImage, m_sourceImage);
                m_mainImagePreviewElements.pushBack(element);
                m_previewPanel->addElement(element, ui::Position(0,0));
            }
            else
            {
                auto element = base::CreateSharedPtr<ImagePreviewElement>(m_mainImage, settings.selectedMip, settings.selectedSlice);
                m_mainImagePreviewElements.pushBack(element);
                m_previewPanel->addElement(element, ui::Position(0, 0));
            }
        }

        for (const auto& elem : m_mainImagePreviewElements)
            elem->configure(previewSettings());
    }

    //--


} // ed
