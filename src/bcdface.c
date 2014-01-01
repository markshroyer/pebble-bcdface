#include <pebble.h>

/* Show seconds place? */
//#define SHOW_SECONDS

/* Pulse vibrate when phone disconnects? */
#define NOTIFY_DISCONNECT


#ifdef SHOW_SECONDS
#define NUM_COLUMNS 6
#define RADIUS 8
#define TICK_UNIT SECOND_UNIT
#else
#define NUM_COLUMNS 4
#define RADIUS 10
#define TICK_UNIT MINUTE_UNIT
#endif

#define DATE_STR_SZ 11


static Window *window = NULL;
static Layer *main_layer = NULL;
static TextLayer *date_layer = NULL;
static char *date_str = NULL;
static struct tm *now = NULL;
#ifdef NOTIFY_DISCONNECT
static TextLayer *bt_layer = NULL;
static bool last_bt_state = false;
#endif

/* offset of the first column */
static int16_t col_offset;

/* spacing between adjacent columns */
static int16_t col_spacing;


static void draw_digit(Layer *layer, GContext *ctx,
                       int col, int bits, int val)
{
	const GRect bounds = layer_get_bounds(layer);
	const int16_t x_coord = col_offset + RADIUS + (2 * RADIUS + col_spacing) * col;
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

static void update_proc(Layer *layer, GContext *ctx)
{
	graphics_context_set_stroke_color(ctx, GColorWhite);
	graphics_context_set_fill_color(ctx, GColorWhite);

	draw_digit(layer, ctx, 0, 2, now->tm_hour / 10);
	draw_digit(layer, ctx, 1, 4, now->tm_hour % 10);
	draw_digit(layer, ctx, 2, 3, now->tm_min  / 10);
	draw_digit(layer, ctx, 3, 4, now->tm_min  % 10);
#ifdef SHOW_SECONDS
	draw_digit(layer, ctx, 4, 3, now->tm_sec  / 10);
	draw_digit(layer, ctx, 5, 4, now->tm_sec  % 10);
#endif
}

static void handle_tick(struct tm *tick_time, TimeUnits units_changed)
{
	now = tick_time; /* TODO is this actually safe? */

	strftime(date_str, DATE_STR_SZ, "%a %b %d", now);
	text_layer_set_text(date_layer, date_str);

	layer_mark_dirty(main_layer);
}

#ifdef NOTIFY_DISCONNECT

static void handle_bt(bool bt_state)
{
	if (last_bt_state && !bt_state)
		vibes_double_pulse();

	last_bt_state = bt_state;
	text_layer_set_text(bt_layer, bt_state ? "" : "!!");
}

#endif /* defined NOTIFY_DISCONNECT */

static void window_load(Window *window) {
	Layer *window_layer = window_get_root_layer(window);
	const GRect bounds = layer_get_bounds(window_layer);

	col_spacing = (bounds.size.w - 2 * NUM_COLUMNS * RADIUS) / (NUM_COLUMNS + 1);
	col_offset = (bounds.size.w - col_spacing * (NUM_COLUMNS - 1) - 2 * NUM_COLUMNS * RADIUS) / 2;

	main_layer = layer_create((GRect) {
		.origin = { 0, 0 },
		.size = { bounds.size.w, bounds.size.h }
	});
	date_layer = text_layer_create((GRect) {
		.origin = { 0, 0 },
		.size = { bounds.size.w, 40 }
	});
#ifdef NOTIFY_DISCONNECT
	bt_layer = text_layer_create((GRect) {
		.origin = { 10, 5 },
		.size = { 20, 20 }
	});
#endif

	layer_set_update_proc(main_layer, update_proc);
	layer_add_child(window_layer, main_layer);
	layer_add_child(window_layer, text_layer_get_layer(date_layer));
#ifdef NOTIFY_DISCONNECT
	layer_add_child(window_layer, text_layer_get_layer(bt_layer));
#endif

	text_layer_set_background_color(date_layer, GColorBlack);
	text_layer_set_text_color(date_layer, GColorWhite);
	text_layer_set_text_alignment(date_layer, GTextAlignmentCenter);
	text_layer_set_font(date_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));

#ifdef NOTIFY_DISCONNECT
	text_layer_set_background_color(bt_layer, GColorBlack);
	text_layer_set_text_color(bt_layer, GColorWhite);
	text_layer_set_text_alignment(bt_layer, GTextAlignmentLeft);
#endif
}

static void window_appear(Window *window) {
	const time_t now_time = time(NULL);

	tick_timer_service_subscribe(TICK_UNIT, handle_tick);

	/*
	 * Force immediate redraw so there isn't an annoying pause before
	 * the date string becomes visible when returning to the watch
	 * face.  This call is also necessary so that "now" gets set before
	 * the first run of update_proc()
	 */
	handle_tick(localtime(&now_time), TICK_UNIT);

#ifdef NOTIFY_DISCONNECT
	bluetooth_connection_service_subscribe(handle_bt);
	handle_bt(bluetooth_connection_service_peek());
#endif
}

static void window_disappear(Window *window) {
#ifdef NOTIFY_DISCONNECT
	bluetooth_connection_service_unsubscribe();
#endif
	tick_timer_service_unsubscribe();
}

static void window_unload(Window *window) {
#ifdef NOTIFY_DISCONNECT
	text_layer_destroy(bt_layer);
#endif
	text_layer_destroy(date_layer);
	layer_destroy(main_layer);
}

static void init(void) {
	const bool animated = true;

	date_str = malloc(DATE_STR_SZ);

	window = window_create();
	window_set_window_handlers(window, (WindowHandlers) {
		.load = window_load,
		.appear = window_appear,
		.disappear = window_disappear,
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
