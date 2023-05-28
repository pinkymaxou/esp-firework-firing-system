#ifndef _EMBEDDEDFILES_H_
#define _EMBEDDEDFILES_H_

#include <stdint.h>

typedef enum
{
    EF_EFLAGS_None = 0,
    EF_EFLAGS_GZip = 1,
} EF_EFLAGS;

typedef struct
{
    const char* strFilename;
    uint32_t u32Length;
    EF_EFLAGS eFlags;
    const uint8_t* pu8StartAddr;
} EF_SFile;

typedef enum
{
    EF_EFILE_INDEX_HTML = 0,    /*!< @brief File: index.html (size: 1 KB) */
    EF_EFILE_SETTINGS_HTML = 1,    /*!< @brief File: settings.html (size: 2 KB) */
    EF_EFILE_CSS_CONTENT_CSS = 2,    /*!< @brief File: css/content.css (size: 2 KB) */
    EF_EFILE_FONT_ORBITRON_BOLD_WOFF = 3,    /*!< @brief File: font/Orbitron-Bold.woff (size: 178 KB) */
    EF_EFILE_FONT_ORBITRON_REGULAR_WOFF = 4,    /*!< @brief File: font/Orbitron-Regular.woff (size: 178 KB) */
    EF_EFILE_FONT_SQUARED_TTF = 5,    /*!< @brief File: font/Squared.ttf (size: 13 KB) */
    EF_EFILE_JS_APP_JS = 6,    /*!< @brief File: js/app.js (size: 1 KB) */
    EF_EFILE_JS_COMMON_APP_JS = 7,    /*!< @brief File: js/common-app.js (size: 263  B) */
    EF_EFILE_JS_SETTINGS_APP_JS = 8,    /*!< @brief File: js/settings-app.js (size: 2 KB) */
    EF_EFILE_JS_VUE_MIN_JS = 9,    /*!< @brief File: js/vue.min.js (size: 92 KB) */
    EF_EFILE_COUNT = 10
} EF_EFILE;

/*! @brief Check if compressed flag is active */
#define EF_ISFILECOMPRESSED(x) ((x & EF_EFLAGS_GZip) == EF_EFLAGS_GZip)

extern const EF_SFile EF_g_sFiles[EF_EFILE_COUNT];
extern const uint8_t EF_g_u8Blobs[];

#endif
