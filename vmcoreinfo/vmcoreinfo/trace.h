/*
 * Copyright (C) 2017 Red Hat, Inc.
 */

//
// Define the tracing flags.
//
// Tracing GUID - 5eeabb8c-be9a-40d0-99fd-86f2a0b21378
//

#define WPP_CONTROL_GUIDS \
    WPP_DEFINE_CONTROL_GUID( \
        VMCoreInfoTraceGuid, (73eaad34,3efe,489e,b037,be13e79e1001), \
        WPP_DEFINE_BIT(DBG_ALL)         \
        WPP_DEFINE_BIT(DBG_INIT)        \
        WPP_DEFINE_BIT(DBG_POWER)       \
        WPP_DEFINE_BIT(DBG_IOCTLS)      \
    )

#define WPP_FLAG_LEVEL_LOGGER(flag, level) \
    WPP_LEVEL_LOGGER(flag)

#define WPP_FLAG_LEVEL_ENABLED(flag, level) \
    (WPP_LEVEL_ENABLED(flag) && WPP_CONTROL(WPP_BIT_ ## flag).Level >= level)

#define WPP_LEVEL_FLAGS_LOGGER(lvl,flags) \
    WPP_LEVEL_LOGGER(flags)

#define WPP_LEVEL_FLAGS_ENABLED(lvl, flags) \
    (WPP_LEVEL_ENABLED(flags) && WPP_CONTROL(WPP_BIT_ ## flags).Level >= lvl)

//
// This comment block is scanned by the trace preprocessor to define our
// Trace function.
//
// begin_wpp config
// FUNC Trace{FLAG=MYDRIVER_ALL_INFO}(LEVEL, MSG, ...);
// FUNC TraceEvents(LEVEL, FLAGS, MSG, ...);
// end_wpp
//

