/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: input #]
***/

#pragma once

#include "inputContext.h"

namespace base
{
    namespace input
    {
        ///---

        /// Null input context
        class BASE_INPUT_API ContextNull : public IContext
        {
            RTTI_DECLARE_VIRTUAL_CLASS(ContextNull, IContext);

        public:
            ContextNull(uint64_t nativeWindow=0, uint64_t nativeDisplay=0);

            //--

            virtual void resetInput() override final;
            virtual void processState() override final;
            virtual void processMessage(const void* msg) override final;
        };

        ///---

    } // input
} // base
