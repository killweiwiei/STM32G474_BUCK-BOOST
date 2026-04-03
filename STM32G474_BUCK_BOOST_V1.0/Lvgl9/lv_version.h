/**
 * @file lv_version.h
 * LVGL version definitions
 */

#ifndef LV_VERSION_H
#define LV_VERSION_H

#define LVGL_VERSION_MAJOR 9
#define LVGL_VERSION_MINOR 2
#define LVGL_VERSION_PATCH 0
#define LVGL_VERSION_INFO  ""

#define LV_VERSION_CHECK(x,y,z) (x == LVGL_VERSION_MAJOR && \
    (y < LVGL_VERSION_MINOR || \
    (y == LVGL_VERSION_MINOR && z <= LVGL_VERSION_PATCH)))

#endif /* LV_VERSION_H */
