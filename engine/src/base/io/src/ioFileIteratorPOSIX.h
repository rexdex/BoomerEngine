/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: io\system\posix #]
* [#platform: posix #]
***/

#pragma once

#include "absolutePath.h"
#include "base/containers/include/stringBuf.h"

namespace base
{
    namespace io
    {
        namespace prv
        {

            /// File iterator for enumerating directory structure
            class POSIXFileIterator
            {
            public:
                //! Get search path ( root directory )
                INLINE const AbsolutePath& searchPath() const { return m_searchPath; }

                //! Are directories allowed ?
                INLINE bool areDirectoriesAllowed() const { return m_allowDirs; }

                //! Are files allowed ?
                INLINE bool areFilesAllowed() const { return m_allowFiles; }

                //---

                POSIXFileIterator(const AbsolutePath& searchAbsolutePath, const wchar_t* pattern, bool allowFiles, bool allowDirs);
                ~POSIXFileIterator();

                //! Iterate to next
                void operator++(int);

                //! Iterate to next
                void operator++();

                //! Is the current file valid ?
                operator bool() const;

                //! Get current file path (full path + name and extension)
                AbsolutePath filePath() const;

                //! Get current file name ( name and extension only )
                const wchar_t* fileName() const;

            private:
                //! Do we have a valid entry ?
                bool validateEntry() const;

                //! Skip until valid file is found
                bool nextEntry();

                void* m_dir;
                void* m_entry;

                bool m_allowDirs;
                bool m_allowFiles;

                AbsolutePath m_searchPath;
                StringBuf m_searchPattern;

                mutable UTF16StringBuf m_localBuf;
            };

        } // prv
    } // io
} // base