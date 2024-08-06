/*
 * Copyright (c) 2015-2016 MICROTRUST Incorporated
 * All rights reserved
 *
 * This file and software is confidential and proprietary to MICROTRUST Inc.
 * Unauthorized copying of this file and software is strictly prohibited.
 * You MUST NOT disclose this file and software unless you get a license
 * agreement from MICROTRUST Incorporated.
 */

#ifndef _ISEE_MSG_LOG_H_

/*************************************************************************/
/* Check for target build mode *******************************************/
/* Define IMSG_BUILD_MODE_RELEASE for user build *************************/
/* Define IMSG_BUILD_MODE_DEBUG for eng/userdebug build ******************/
/*************************************************************************/
#if defined(IMSG_LOG_MODE_DEBUG)
#define IMSG_DEBUG      1
#elif defined(IMSG_LOG_MODE_RELEASE)
#define IMSG_RELEASE    1
#else
#define IMSG_DEBUG      1
#endif

/*************************************************************************/
/* Common used definition ************************************************/
/*************************************************************************/
#ifndef IMSG_TAG
#define IMSG_TAG "[ISEE]"
#endif

/*************************************************************************/
/* Declare headers for different modules *********************************/
/*************************************************************************/
/* === PRELOADER ======================================================= */
#ifdef TARGET_BUILD_VARIANT_ENG
#define IMSG_DEBUG      1
#else
#undef IMSG_DEBUG
#define IMSG_RELEASE    1
#endif
static inline unsigned long now_ms(void)
{
    return get_timer(0);
}
#define IMSG_PRINT(LEVEL, fmt, ...) \
    do { \
        print("%s[%s]: "fmt, IMSG_TAG, #LEVEL, ##__VA_ARGS__); \
    } while(0)

#define IMSG_PRINT_TIME_S(fmt, ...) \
    unsigned long start_ms; \
    do { \
        start_ms = now_ms(); \
        print("%s[PROFILE]: "fmt" ==> now:%d\n", IMSG_TAG, ##__VA_ARGS__, start_ms); \
    } while(0)

#define IMSG_PRINT_TIME_E(fmt, ...) \
    do { \
        unsigned long end_ms, delta_ms; \
        end_ms = now_ms(); \
        delta_ms = end_ms - start_ms; \
        print("%s[PROFILE]: "fmt" ==> now:%d, spend:%d ms\n", IMSG_TAG, ##__VA_ARGS__, end_ms, delta_ms); \
    } while(0)

/*************************************************************************/
/* Declear macros ********************************************************/
/*************************************************************************/
#define IMSG_WARN(fmt, ...)         IMSG_PRINT(WARN, fmt, ##__VA_ARGS__)
#define IMSG_ERROR(fmt, ...)        IMSG_PRINT(ERROR, fmt, ##__VA_ARGS__)
#if defined(IMSG_DEBUG)
#define IMSG_TRACE(fmt, ...)        IMSG_PRINT(INFO, fmt, ##__VA_ARGS__)
#define IMSG_DBG(fmt, ...)          IMSG_PRINT(DEBUG, fmt, ##__VA_ARGS__)
#define IMSG_SENST(fmt, ...)        IMSG_PRINT(SENST, fmt, ##__VA_ARGS__)
#define IMSG_SENST_DATA(fmt, ...)   print(fmt, ##__VA_ARGS__)
#define IMSG_PROFILE_S(fmt, ...)    IMSG_PRINT_TIME_S(fmt, ##__VA_ARGS__)
#define IMSG_PROFILE_E(fmt, ...)    IMSG_PRINT_TIME_E(fmt, ##__VA_ARGS__)
#else
#define IMSG_TRACE(fmt, ...)        do { } while(0)
#define IMSG_DBG(fmt, ...)          do { } while(0)
#define IMSG_SENST(fmt, ...)        do { } while(0)
#define IMSG_SENST_DATA(fmt, ...)   do { } while(0)
#define IMSG_PROFILE_S(fmt, ...)    do { } while(0)
#define IMSG_PROFILE_E(fmt, ...)    do { } while(0)
#endif

#endif //_ISEE_MSG_LOG_H_
