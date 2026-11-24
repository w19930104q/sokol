//------------------------------------------------------------------------------
//  LLM maintained.
//
//  sokol_framebuffer_test.c
//  For best results, run with ASAN and UBSAN.
//------------------------------------------------------------------------------
#include "sokol_gfx.h"
#include "capture_log.h"
#define SOKOL_FRAMEBUFFER_IMPL
#include "sokol_framebuffer.h"
#include "utest.h"

#define T(b) EXPECT_TRUE(b)

static void init(void) {
    sg_setup(&(sg_desc){ .logger = { .func = capture_log }});
    sfb_setup(&(sfb_desc){ .logger = { .func = capture_log }});
}

static void init_with(const sfb_desc* desc) {
    sg_setup(&(sg_desc){0});
    sfb_setup(desc);
}

static void shutdown(void) {
    sfb_shutdown();
    sg_shutdown();
}

UTEST(sokol_framebuffer, default_init_shutdown) {
    init();
    T(_sfb.init_tag == _SFB_INIT_TAG);
    T(_sfb.desc.framebuffer_pool_size == _SFB_DEFAULT_FRAMEBUFFER_POOL_SIZE);
    T(_sfb.pools.framebuffers);
    T(_sfb.pools.framebuffer_pool.size == (_SFB_DEFAULT_FRAMEBUFFER_POOL_SIZE + 1));
    T(_sfb.pools.framebuffer_pool.free_queue);
    T(_sfb.pools.framebuffer_pool.gen_ctrs);
    shutdown();
    T(_sfb.init_tag == 0);
}

static void* my_alloc(size_t size, void* user_data) {
    (void)user_data;
    return malloc(size);
}

static void my_free(void* ptr, void* user_data) {
    (void)user_data;
    free(ptr);
}

UTEST(sokol_framebuffer, init_with_params) {
    init_with(&(sfb_desc){
        .framebuffer_pool_size = 128,
        .allocator = {
            .alloc_fn = my_alloc,
            .free_fn = my_free,
            .user_data = (void*)12345,
        },
    });
    T(_sfb.init_tag == _SFB_INIT_TAG);
    T(_sfb.desc.framebuffer_pool_size == 128);
    T(_sfb.pools.framebuffers);
    T(_sfb.pools.framebuffer_pool.size == 129);
    T(_sfb.pools.framebuffer_pool.free_queue);
    T(_sfb.pools.framebuffer_pool.gen_ctrs);
    T(_sfb.desc.allocator.alloc_fn == my_alloc);
    T(_sfb.desc.allocator.free_fn == my_free);
    T(_sfb.desc.allocator.user_data == (void*)12345);
    shutdown();
}

UTEST(sokol_framebuffer, make_destroy_framebuffer_defaults) {
    init();
    sfb_framebuffer fb = sfb_make_framebuffer(&(sfb_framebuffer_desc){
        .width = 320,
        .height = 256,
    });
    T(fb.id != SFB_INVALID_ID);
    T(sfb_query_framebuffer_state(fb) == SFB_RESOURCESTATE_VALID);
    _sfb_framebuffer_t* fbptr = _sfb_lookup_framebuffer(fb.id);
    T(fbptr);
    T(fbptr->slot.state == SFB_RESOURCESTATE_VALID);
    T(fbptr->slot.id == fb.id);
    T(fbptr->width == 320);
    T(fbptr->height == 256);
    T(fbptr->prescale == 1);
    T(fbptr->format == SFB_FORMAT_RGBA8);
    T(fbptr->cliprect.x == 0);
    T(fbptr->cliprect.y == 0);
    T(fbptr->cliprect.width == 320);
    T(fbptr->cliprect.height == 256);
    T(fbptr->rotate90 == false);
    sfb_destroy_framebuffer(fb);
    T(fbptr->slot.id == SFB_INVALID_ID);
    T(sfb_query_framebuffer_state(fb) == SFB_RESOURCESTATE_INVALID);
    shutdown();
}

UTEST(sokol_framebuffer, make_framebuffer_palette8) {
    init();
    sfb_framebuffer fb = sfb_make_framebuffer(&(sfb_framebuffer_desc){
        .width = 320,
        .height = 256,
        .format = SFB_FORMAT_PALETTE8,
        .prescale = 2,
        .rotate90 = true,
    });
    T(fb.id != SFB_INVALID_ID);
    T(sfb_query_framebuffer_state(fb) == SFB_RESOURCESTATE_VALID);
    _sfb_framebuffer_t* fbptr = _sfb_lookup_framebuffer(fb.id);
    T(fbptr);
    T(fbptr->format == SFB_FORMAT_PALETTE8);
    T(fbptr->prescale == 2);
    T(fbptr->rotate90 == true);
    T(fbptr->cliprect.width == 320);
    T(fbptr->cliprect.height == 256);
    sfb_destroy_framebuffer(fb);
    shutdown();
}

UTEST(sokol_framebuffer, make_framebuffer_cliprect) {
    init();
    sfb_framebuffer fb = sfb_make_framebuffer(&(sfb_framebuffer_desc){
        .width = 512,
        .height = 512,
        .cliprect = {
            .x = 0,
            .y = 0,
            .width = 160,
            .height = 128,
        },
    });
    T(fb.id != SFB_INVALID_ID);
    T(sfb_query_framebuffer_state(fb) == SFB_RESOURCESTATE_VALID);
    _sfb_framebuffer_t* fbptr = _sfb_lookup_framebuffer(fb.id);
    T(fbptr);
    T(fbptr->width == 512);
    T(fbptr->height == 512);
    T(fbptr->cliprect.x == 0);
    T(fbptr->cliprect.y == 0);
    T(fbptr->cliprect.width == 160);
    T(fbptr->cliprect.height == 128);
    sfb_destroy_framebuffer(fb);
    shutdown();
}

UTEST(sokol_framebuffer, make_framebuffer_invalid_width) {
    init();
    sfb_framebuffer fb = sfb_make_framebuffer(&(sfb_framebuffer_desc){
        .width = 0,
        .height = 256,
    });
    T(fb.id != SFB_INVALID_ID);
    T(sfb_query_framebuffer_state(fb) == SFB_RESOURCESTATE_FAILED);
    sfb_destroy_framebuffer(fb);
    T(sfb_query_framebuffer_state(fb) == SFB_RESOURCESTATE_INVALID);
    shutdown();
}

UTEST(sokol_framebuffer, make_framebuffer_invalid_height) {
    init();
    sfb_framebuffer fb = sfb_make_framebuffer(&(sfb_framebuffer_desc){
        .width = 320,
        .height = 0,
    });
    T(fb.id != SFB_INVALID_ID);
    T(sfb_query_framebuffer_state(fb) == SFB_RESOURCESTATE_FAILED);
    sfb_destroy_framebuffer(fb);
    shutdown();
}

UTEST(sokol_framebuffer, query_framebuffer_desc) {
    init();
    sfb_framebuffer fb = sfb_make_framebuffer(&(sfb_framebuffer_desc){
        .width = 320,
        .height = 256,
        .format = SFB_FORMAT_PALETTE8,
        .prescale = 2,
        .rotate90 = true,
        .cliprect = { .x = 16, .y = 32, .width = 200, .height = 100 },
    });
    T(fb.id != SFB_INVALID_ID);
    sfb_framebuffer_desc desc = sfb_query_framebuffer_desc(fb);
    T(desc.width == 320);
    T(desc.height == 256);
    T(desc.prescale == 2);
    T(desc.format == SFB_FORMAT_PALETTE8);
    T(desc.rotate90 == true);
    T(desc.cliprect.x == 16);
    T(desc.cliprect.y == 32);
    T(desc.cliprect.width == 200);
    T(desc.cliprect.height == 100);
    sfb_destroy_framebuffer(fb);
    shutdown();
}

UTEST(sokol_framebuffer, query_framebuffer_desc_invalid) {
    init();
    sfb_framebuffer_desc desc = sfb_query_framebuffer_desc((sfb_framebuffer){ .id = SFB_INVALID_ID });
    T(desc.width == 0);
    T(desc.height == 0);
    T(desc.prescale == 0);
    T(desc.format == _SFB_FORMAT_DEFAULT);
    shutdown();
}

UTEST(sokol_framebuffer, query_framebuffer_info) {
    init();
    sfb_framebuffer fb = sfb_make_framebuffer(&(sfb_framebuffer_desc){
        .width = 320,
        .height = 256,
        .prescale = 2,
    });
    T(fb.id != SFB_INVALID_ID);
    sfb_framebuffer_info info = sfb_query_framebuffer_info(fb);
    T(info.update.width == 320);
    T(info.update.height == 256);
    T(info.update.pixel_format == SG_PIXELFORMAT_RGBA8);
    T(info.update.image.id != SG_INVALID_ID);
    T(info.update.tex_view.id != SG_INVALID_ID);
    T(info.offscreen.width == 640);
    T(info.offscreen.height == 512);
    T(info.offscreen.pixel_format == SG_PIXELFORMAT_RGBA8);
    T(info.offscreen.image.id != SG_INVALID_ID);
    T(info.offscreen.tex_view.id != SG_INVALID_ID);
    T(info.palette.width == 256);
    T(info.palette.height == 1);
    T(info.palette.pixel_format == SG_PIXELFORMAT_RGBA8);
    // palette image is only created for SFB_FORMAT_PALETTE8
    T(info.palette.image.id == SG_INVALID_ID);
    T(info.palette.tex_view.id == SG_INVALID_ID);
    T(info.nearest_sampler.id != SG_INVALID_ID);
    T(info.linear_sampler.id != SG_INVALID_ID);
    sfb_destroy_framebuffer(fb);
    shutdown();
}

UTEST(sokol_framebuffer, query_framebuffer_info_palette8) {
    init();
    sfb_framebuffer fb = sfb_make_framebuffer(&(sfb_framebuffer_desc){
        .width = 320,
        .height = 256,
        .format = SFB_FORMAT_PALETTE8,
    });
    T(fb.id != SFB_INVALID_ID);
    sfb_framebuffer_info info = sfb_query_framebuffer_info(fb);
    T(info.update.pixel_format == SG_PIXELFORMAT_R8);
    T(info.palette.image.id != SG_INVALID_ID);
    T(info.palette.tex_view.id != SG_INVALID_ID);
    sfb_destroy_framebuffer(fb);
    shutdown();
}

UTEST(sokol_framebuffer, query_framebuffer_info_invalid) {
    init();
    sfb_framebuffer_info info = sfb_query_framebuffer_info((sfb_framebuffer){ .id = SFB_INVALID_ID });
    T(info.update.image.id == SG_INVALID_ID);
    T(info.offscreen.image.id == SG_INVALID_ID);
    T(info.palette.image.id == SG_INVALID_ID);
    shutdown();
}

UTEST(sokol_framebuffer, resize_noop) {
    init();
    sfb_framebuffer fb = sfb_make_framebuffer(&(sfb_framebuffer_desc){
        .width = 320,
        .height = 256,
    });
    T(fb.id != SFB_INVALID_ID);
    bool recreated = sfb_resize(fb, &(sfb_resize_desc){
        .width = 320,
        .height = 256,
    });
    T(!recreated);
    T(sfb_query_framebuffer_state(fb) == SFB_RESOURCESTATE_VALID);
    sfb_destroy_framebuffer(fb);
    shutdown();
}

UTEST(sokol_framebuffer, resize_changes_size) {
    init();
    sfb_framebuffer fb = sfb_make_framebuffer(&(sfb_framebuffer_desc){
        .width = 320,
        .height = 256,
    });
    T(fb.id != SFB_INVALID_ID);
    bool recreated = sfb_resize(fb, &(sfb_resize_desc){
        .width = 640,
        .height = 480,
    });
    T(recreated);
    T(sfb_query_framebuffer_state(fb) == SFB_RESOURCESTATE_VALID);
    sfb_framebuffer_desc desc = sfb_query_framebuffer_desc(fb);
    T(desc.width == 640);
    T(desc.height == 480);
    T(desc.cliprect.width == 640);
    T(desc.cliprect.height == 480);
    sfb_destroy_framebuffer(fb);
    shutdown();
}

// SG3 sample: a debug panel keeps a CPU-side RGBA8 color buffer per
// framebuffer as its offscreen preview. After each framebuffer resize the
// panel uploads a full frame and reads back the color buffer edge pixels,
// then keeps rendering on the same backend.
static uint32_t sg3_pixel(int x, int y) {
    return 0xFF000000u | ((uint32_t)x << 16) | ((uint32_t)y << 8) | 0x5Au;
}

static void sg3_fill_frame(uint32_t* pixels, int width, int height) {
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            pixels[y * width + x] = sg3_pixel(x, y);
        }
    }
}

// Read the right column and bottom row of the color buffer. The edge
// coordinates come from the framebuffer's current root view, so a stale
// root view reads the wrong column.
static void sg3_read_edges(sfb_framebuffer fb, const uint32_t* pixels, uint32_t* out_right_col, uint32_t* out_bottom_row) {
    const sfb_framebuffer_desc desc = sfb_query_framebuffer_desc(fb);
    for (int y = 0; y < desc.height; y++) {
        out_right_col[y] = pixels[y * desc.width + (desc.width - 1)];
    }
    for (int x = 0; x < desc.width; x++) {
        out_bottom_row[x] = pixels[(desc.height - 1) * desc.width + x];
    }
}

// Upload a full frame. A stale root view would reject the pixel range.
static bool sg3_upload_frame(sfb_framebuffer fb, const uint32_t* pixels, int width, int height) {
    capture_log_reset();
    sfb_update(fb, &(sfb_update_desc){
        .pixels = { .ptr = pixels, .size = (size_t)(width * height) * sizeof(uint32_t) },
    });
    sg_commit();
    return capture_log_num_items == 0;
}

// Root view dims, attachment pixel dims and formats must be in sync.
static bool sg3_expect_in_sync(sfb_framebuffer fb, int width, int height) {
    bool ok = true;
    ok &= sfb_query_framebuffer_state(fb) == SFB_RESOURCESTATE_VALID;
    const sfb_framebuffer_desc desc = sfb_query_framebuffer_desc(fb);
    ok &= desc.width == width;
    ok &= desc.height == height;
    ok &= desc.cliprect.width == width;
    ok &= desc.cliprect.height == height;
    // color and depth format stay unchanged across resizes
    ok &= desc.format == SFB_FORMAT_RGBA8;
    ok &= desc.render_pass.color_format == SG_PIXELFORMAT_RGBA8;
    ok &= desc.render_pass.depth_format == SG_PIXELFORMAT_DEPTH_STENCIL;
    const sfb_framebuffer_info info = sfb_query_framebuffer_info(fb);
    ok &= info.update.width == width;
    ok &= info.update.height == height;
    ok &= info.update.pixel_format == SG_PIXELFORMAT_RGBA8;
    ok &= info.offscreen.width == width;
    ok &= info.offscreen.height == height;
    ok &= info.offscreen.pixel_format == SG_PIXELFORMAT_RGBA8;
    // the actual attachment images must match the root view
    const sg_image_desc update_img_desc = sg_query_image_desc(info.update.image);
    ok &= update_img_desc.width == width;
    ok &= update_img_desc.height == height;
    const sg_image_desc offscreen_img_desc = sg_query_image_desc(info.offscreen.image);
    ok &= offscreen_img_desc.width == width;
    ok &= offscreen_img_desc.height == height;
    return ok;
}

UTEST(sokol_framebuffer, resize_sg3_repeated_resize_stays_in_sync) {
    init();
    sfb_framebuffer fb = sfb_make_framebuffer(&(sfb_framebuffer_desc){
        .width = 8,
        .height = 4,
        .render_pass = {
            .color_format = SG_PIXELFORMAT_RGBA8,
            .depth_format = SG_PIXELFORMAT_DEPTH_STENCIL,
        },
    });
    T(fb.id != SFB_INVALID_ID);
    T(sfb_query_framebuffer_state(fb) == SFB_RESOURCESTATE_VALID);
    T(sg3_expect_in_sync(fb, 8, 4));

    // first full frame at 8x4
    static uint32_t frame[8 * 5];
    uint32_t first_right_col[8];
    uint32_t first_bottom_row[8];
    uint32_t right_col[8];
    uint32_t bottom_row[8];
    sg3_fill_frame(frame, 8, 4);
    T(sg3_upload_frame(fb, frame, 8, 4));
    sg3_read_edges(fb, frame, first_right_col, first_bottom_row);
    const uint32_t first_offscreen_img = sfb_query_framebuffer_info(fb).offscreen.image.id;

    // resize 8x4 -> 3x5: root view, attachment rebuild and pixel dims
    // must complete in one resize
    T(sfb_resize(fb, &(sfb_resize_desc){ .width = 3, .height = 5 }));
    T(sg3_expect_in_sync(fb, 3, 5));
    T(sfb_query_framebuffer_info(fb).offscreen.image.id != first_offscreen_img);
    sg3_fill_frame(frame, 3, 5);
    T(sg3_upload_frame(fb, frame, 3, 5));
    sg3_read_edges(fb, frame, right_col, bottom_row);
    T(right_col[4] == sg3_pixel(2, 4));
    T(bottom_row[2] == sg3_pixel(2, 4));

    // resize back 3x5 -> 8x4, repeated resizes must stay stable
    T(sfb_resize(fb, &(sfb_resize_desc){ .width = 8, .height = 4 }));
    T(sg3_expect_in_sync(fb, 8, 4));

    // the restored frame's edge pixels must match the first full frame
    sg3_fill_frame(frame, 8, 4);
    T(sg3_upload_frame(fb, frame, 8, 4));
    sg3_read_edges(fb, frame, right_col, bottom_row);
    T(memcmp(right_col, first_right_col, 4 * sizeof(uint32_t)) == 0);
    T(memcmp(bottom_row, first_bottom_row, 8 * sizeof(uint32_t)) == 0);

    sfb_destroy_framebuffer(fb);
    shutdown();
}

// Resize a tracked framebuffer the way the debug panel must do it: the
// resize candidates are validated before the current framebuffer is
// touched, so a rejected resize never leaves a half-sized root view
// behind. Returns true when the framebuffer was resized.
static bool sg3_tracked_resize(sfb_framebuffer fb, int width, int height) {
    if ((width <= 0) || (height <= 0)) {
        return false;
    }
    const sfb_framebuffer_desc desc = sfb_query_framebuffer_desc(fb);
    if ((desc.width == width) && (desc.height == height)) {
        return false;
    }
    return sfb_resize(fb, &(sfb_resize_desc){ .width = width, .height = height });
}

// A rejected resize must leave root view, attachments and pixel dims
// untouched, and the framebuffer must keep accepting full frames.
UTEST(sokol_framebuffer, resize_sg3_rejected_resize_keeps_root_view) {
    init();
    sfb_framebuffer fb = sfb_make_framebuffer(&(sfb_framebuffer_desc){
        .width = 8,
        .height = 4,
        .render_pass = {
            .color_format = SG_PIXELFORMAT_RGBA8,
            .depth_format = SG_PIXELFORMAT_DEPTH_STENCIL,
        },
    });
    T(fb.id != SFB_INVALID_ID);
    T(sfb_query_framebuffer_state(fb) == SFB_RESOURCESTATE_VALID);
    const uint32_t update_img = sfb_query_framebuffer_info(fb).update.image.id;

    capture_log_reset();
    T(!sg3_tracked_resize(fb, 0, 5));
    T(!sg3_tracked_resize(fb, 3, 0));
    T(!sg3_tracked_resize(fb, -1, -1));
    // a no-op resize is not a failure, but also touches nothing
    T(!sg3_tracked_resize(fb, 8, 4));
    T(capture_log_num_items == 0);

    // root view and attachments must be untouched
    T(sfb_query_framebuffer_state(fb) == SFB_RESOURCESTATE_VALID);
    const sfb_framebuffer_desc desc = sfb_query_framebuffer_desc(fb);
    T(desc.width == 8);
    T(desc.height == 4);
    T(desc.cliprect.width == 8);
    T(desc.cliprect.height == 4);
    const sfb_framebuffer_info info = sfb_query_framebuffer_info(fb);
    T(info.update.width == 8);
    T(info.update.height == 4);
    T(info.update.image.id == update_img);

    // the framebuffer still accepts a full frame at the old size
    static uint32_t frame[8 * 4];
    sg3_fill_frame(frame, 8, 4);
    T(sg3_upload_frame(fb, frame, 8, 4));

    // a valid resize through the same path still works
    T(sg3_tracked_resize(fb, 3, 5));
    T(sg3_expect_in_sync(fb, 3, 5));

    sfb_destroy_framebuffer(fb);
    shutdown();
}

UTEST(sokol_framebuffer, make_destroy_many_framebuffers) {
    init();
    sfb_framebuffer fbs[_SFB_DEFAULT_FRAMEBUFFER_POOL_SIZE];
    for (int i = 0; i < _SFB_DEFAULT_FRAMEBUFFER_POOL_SIZE; i++) {
        fbs[i] = sfb_make_framebuffer(&(sfb_framebuffer_desc){
            .width = 64,
            .height = 64,
        });
        T(fbs[i].id != SFB_INVALID_ID);
        T(sfb_query_framebuffer_state(fbs[i]) == SFB_RESOURCESTATE_VALID);
    }
    // one more should exhaust the pool
    sfb_framebuffer overflow = sfb_make_framebuffer(&(sfb_framebuffer_desc){
        .width = 64,
        .height = 64,
    });
    T(overflow.id == SFB_INVALID_ID);
    for (int i = 0; i < _SFB_DEFAULT_FRAMEBUFFER_POOL_SIZE; i++) {
        sfb_destroy_framebuffer(fbs[i]);
        T(sfb_query_framebuffer_state(fbs[i]) == SFB_RESOURCESTATE_INVALID);
    }
    shutdown();
}

UTEST(sokol_framebuffer, shutdown_destroys_remaining_framebuffers) {
    init();
    sfb_framebuffer fb0 = sfb_make_framebuffer(&(sfb_framebuffer_desc){ .width = 32, .height = 32 });
    sfb_framebuffer fb1 = sfb_make_framebuffer(&(sfb_framebuffer_desc){ .width = 64, .height = 64 });
    T(fb0.id != SFB_INVALID_ID);
    T(fb1.id != SFB_INVALID_ID);
    T(sfb_query_framebuffer_state(fb0) == SFB_RESOURCESTATE_VALID);
    T(sfb_query_framebuffer_state(fb1) == SFB_RESOURCESTATE_VALID);
    // don't destroy explicitly - sfb_shutdown() should clean them up
    shutdown();
}
