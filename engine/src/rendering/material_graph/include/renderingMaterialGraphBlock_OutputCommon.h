/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: material\output #]
***/

#pragma once

#include "renderingMaterialGraphBlock_Output.h"

namespace rendering
{
    //--

    /// lit output using PBR shader
    class RENDERING_MATERIAL_GRAPH_API MaterialGraphBlockOutputCommon : public MaterialGraphBlockOutput
    {
        RTTI_DECLARE_VIRTUAL_CLASS(MaterialGraphBlockOutputCommon, MaterialGraphBlockOutput);

    public:
        MaterialGraphBlockOutputCommon();

    protected:
        virtual void buildLayout(base::graph::BlockLayoutBuilder& builder) const override;
        virtual MaterialSortGroup resolveSortGroup() const override;
        virtual void compilePixelFunction(MaterialStageCompiler& compiler, MaterialTechniqueRenderStates& outRenderState) const override final;
        virtual void compileVertexFunction(MaterialStageCompiler& compiler, MaterialTechniqueRenderStates& outRenderState) const override final;

        virtual CodeChunk compileMainColor(MaterialStageCompiler& compiler, MaterialTechniqueRenderStates& outRenderState) const = 0;

        bool m_applyFog = true;
        bool m_twoSided = false;
        bool m_maskAlphaToCoverage = false;
        bool m_depthWrite = true; // can be set to false even for "solid" materials
        bool m_premultiplyAlpha = true;
        float m_maskThreshold = 0.5f;
        MaterialBlendMode m_blendMode = MaterialBlendMode::Opaque;
    };

    //--
    
} // rendering