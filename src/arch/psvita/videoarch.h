
#ifndef VICE_VIDEOARCH_H
#define VICE_VIDEOARCH_H

#include "vice.h"
#include "types.h"
#include "viewport.h"


struct video_canvas_s {
    unsigned int initialized;
    unsigned int created;

    /* Index of the canvas, needed for x128 and xcbm2 */
    int index;
    unsigned int depth;

    /* Size of the drawable canvas area, including the black borders */
    unsigned int width, height;

    /* Size of the canvas as requested by the emulator itself */
    unsigned int real_width, real_height;

    /* Actual size of the window; in most cases the same as width/height */
    unsigned int actual_width, actual_height;

    struct video_render_config_s *videoconfig;
    struct draw_buffer_s *draw_buffer;
    struct draw_buffer_s *draw_buffer_vsid;
    struct viewport_s *viewport;
    struct geometry_s *geometry;
    struct palette_s *palette;
    struct raster_s *parent_raster;
    struct video_draw_buffer_callback_s *video_draw_buffer_callback;
};
typedef struct video_canvas_s video_canvas_t;


extern void video_psv_ui_init_finalize(void);
extern void video_psv_menu_show(void);
extern void video_psv_update_palette(void);
extern void video_psv_get_canvas(struct video_canvas_s** canvas);

#endif
