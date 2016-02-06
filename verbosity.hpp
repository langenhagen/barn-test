/******************************************************************************
* @file The enum class verbosity that can be used to specify logging verbosity.
*
*
* @author langenhagen
* @version 160130
******************************************************************************/
#pragma once


///////////////////////////////////////////////////////////////////////////////
// NAMESPACE, CONSTANTS, TYPE DECLARATIONS/IMPLEMENTATIONS and FUNCTIONS


namespace unittest {

    /// Defines the mass of streaming output.
    enum class verbosity {
        SILENT = 0,     ///< Streams out nothing.
        NORMAL = 1,     ///< Streams out standard info for each test.
        VERBOSE = 2,    ///< Streams much info.
    };

} // END namespace unittest