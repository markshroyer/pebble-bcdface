#include <pebble.h>

#define SHOW_SECONDS 1

#ifdef SHOW_SECONDS
#define NUM_COLUMNS 6
#define RADIUS 8
#else
#define NUM_COLUMNS 4
#define RADIUS 10
#endif

#define DATE_STR_SZ 11


static Window *window = NULL;
static Layer *layer_hour = NULL;
static Layer *layer_min = NULL;
#ifdef SHOW_SECONDS
static Layer *layer_sec = NULL;
#endif
static TextLayer *layer_date = NULL;
static struct tm *now = NULL;
static char *date_str = NULL;

/* offset of the first column */
static int16_t col_offset;

/* spacing between adjacent columns */
static int16_t col_spacing;


/*
 * Drawing and tick handler functions
 *
 * Handles separate layers for seconds, minutes, hours, and the date string
 * on the theory that it is significantly less computationally & battery
 * intensive to figure out which of these layers needs to be updated and
 * invalidate them individually, than to redraw it all together once per
 * second.  I haven't emperically determined that this is actually more
 * energy efficient than the naive implementation though...
 */

static void draw_digit(Layer *layer, GContext *ctx,
                       int col, int bits, int val)
{
	const GRect bounds = layer_get_bounds(layer);
	const int16_t x_coord = RADIUS + (2 * RADIUS + col_spacing) * col;
	GPoint point;
	int i;
	int mask = 1;

	for (i = 0; i < bits; i++) {
		point = GPoint(x_coord, bounds.size.h - RADIUS * (3 * i + 2));
		if (mask & val)
			graphics_fill_circle(ctx, point, RADIUS);
		else
			graphics_draw_circle(ctx, point, RADIUS);

		mask <<= 1;
	}
}

static inline void prepare_ctx(GContext *ctx)
{
	graphics_context_set_stroke_color(ctx, GColorWhite);
	graphics_context_set_fill_color(ctx, GColorWhite);
}

static void update_hour(Layer *layer, GContext *ctx)
{
	APP_LOG(APP_LOG_LEVEL_DEBUG, "update_hour()");

	prepare_ctx(ctx);
	draw_digit(layer, ctx, 0, 2, now->tm_hour / 10);
	draw_digit(layer, ctx, 1, 4, now->tm_hour % 10);
}

static void update_min(Layer *layer, GContext *ctx)
{
	APP_LOG(APP_LOG_LEVEL_DEBUG, "update_min()");

	prepare_ctx(ctx);
	draw_digit(layer, ctx, 0, 3, now->tm_min / 10);
	draw_digit(layer, ctx, 1, 4, now->tm_min % 10);
}

#ifdef SHOW_SECONDS

static void update_sec(Layer *layer, GContext *ctx)
{
	prepare_ctx(ctx);
	draw_digit(layer, ctx, 0, 3, now->tm_sec / 10);
	draw_digit(layer, ctx, 1, 4, now->tm_sec % 10);
}

#endif /* defined SHOW_SECONDS */

static void timer_tick(struct tm *tick_time, TimeUnits units_changed)
{
	now = tick_time; /* TODO is this safe? */

#ifdef SHOW_SECONDS
	layer_mark_dirty(layer_sec);
	if (tick_time->tm_sec != 0)
		return;
#endif

	layer_mark_dirty(layer_min);
	if (tick_time->tm_min != 0)
		return;

	layer_mark_dirty(layer_hour);
	if (tick_time->tm_hour != 0)
		return;

	strftime(date_str, DATE_STR_SZ, "%a %b %d", now);
	text_layer_set_text(layer_date, date_str);
}


/*
 * Setup / teardown stuff
 */

static void window_load(Window *window) {
        Layer *layer_window = window_get_root_layer(window);
        GRect bounds = layer_get_bounds(layer_window);
	const time_t now_time = time(NULL);

	col_spacing = (bounds.size.w - 2 * NUM_COLUMNS * RADIUS) / (NUM_COLUMNS + 1);
	col_offset = (bounds.size.w - col_spacing * (NUM_COLUMNS - 1) - 2 * NUM_COLUMNS * RADIUS) / 2;

	const int16_t unit_width = 4 * RADIUS + col_spacing;
	const int16_t unit_offset = unit_width + col_spacing;

	/* This will be needed the first time the update procs run */
	now = localtime(&now_time);

	layer_hour = layer_create((GRect) {
		.origin = { col_offset, 0 },
		.size = { unit_width, bounds.size.h }
	});
	layer_set_update_proc(layer_hour, update_hour);

	layer_min = layer_create((GRect) {
		.origin = { col_offset + unit_offset, 0 },
		.size = { unit_width, bounds.size.h }
	});
	layer_set_update_proc(layer_min, update_min);

#ifdef SHOW_SECONDS
	layer_sec = layer_create((GRect) {
		.origin = { col_offset + 2 * unit_offset, 0 },
		.size = { unit_width, bounds.size.h }
	});
	layer_set_update_proc(layer_sec, update_sec);
#endif

	layer_date = text_layer_create((GRect) {
		.origin = { 0, 0 },
		.size = { bounds.size.w, 40 }
	});

	layer_add_child(layer_window, layer_hour);
	layer_add_child(layer_window, layer_min);
#ifdef SHOW_SECONDS
	layer_add_child(layer_window, layer_sec);
#endif
	layer_add_child(layer_window, text_layer_get_layer(layer_date));

	text_layer_set_text(layer_date, "");
	text_layer_set_background_color(layer_date, GColorBlack);
	text_layer_set_text_color(layer_date, GColorWhite);
	text_layer_set_text_alignment(layer_date, GTextAlignmentCenter);
	text_layer_set_font(layer_date, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));

#ifdef SHOW_SECONDS
	tick_timer_service_subscribe(SECOND_UNIT, timer_tick);
#else
	tick_timer_service_subscribe(MINUTE_UNIT, timer_tick);
#endif

	APP_LOG(APP_LOG_LEVEL_DEBUG, "bounds.size.w = %d", bounds.size.w);
	APP_LOG(APP_LOG_LEVEL_DEBUG, "col_spacing = %d", col_spacing);
	APP_LOG(APP_LOG_LEVEL_DEBUG, "col_offset = %d", col_offset);
}

static void window_unload(Window *window) {
	tick_timer_service_unsubscribe();
	text_layer_destroy(layer_date);
#ifdef SHOW_SECONDS
	layer_destroy(layer_sec);
#endif
	layer_destroy(layer_min);
	layer_destroy(layer_hour);
}

static void init(void) {
        const bool animated = true;

	date_str = malloc(DATE_STR_SZ);

        window = window_create();
        window_set_window_handlers(window, (WindowHandlers) {
		.load = window_load,
		.unload = window_unload,
	});
        window_set_background_color(window, GColorBlack);
	window_set_fullscreen(window, true);
	window_stack_push(window, animated);
}

static void deinit(void) {
	window_destroy(window);
	free(date_str);
}

int main(void) {
	init();

	APP_LOG(APP_LOG_LEVEL_DEBUG, "Done initializing, pushed window: %p", window);

	app_event_loop();
	deinit();
}
