#include <pebble.h>

#define SHOW_SECONDS 1
#define RADIUS 8

#define DATE_STR_SZ 11

#ifdef SHOW_SECONDS
#define NUM_COLUMNS 6
#else
#define NUM_COLUMNS 4
#endif

static Window *window = NULL;
static Layer *main_layer = NULL;
static TextLayer *date_layer = NULL;
char *date_str = NULL;

/* offset of the first column */
int16_t col_offset;

/* spacing between adjacent columns */
int16_t col_spacing;

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
	const time_t now_time = time(NULL);
	const struct tm *now = localtime(&now_time);

	graphics_context_set_stroke_color(ctx, GColorWhite);
	graphics_context_set_fill_color(ctx, GColorWhite);

	strftime(date_str, DATE_STR_SZ, "%a %b %d", now);
	text_layer_set_text(date_layer, date_str);

	draw_digit(layer, ctx, 0, 2, now->tm_hour / 10);
	draw_digit(layer, ctx, 1, 4, now->tm_hour % 10);
	draw_digit(layer, ctx, 2, 3, now->tm_min  / 10);
	draw_digit(layer, ctx, 3, 4, now->tm_min  % 10);
#ifdef SHOW_SECONDS
	draw_digit(layer, ctx, 4, 3, now->tm_sec  / 10);
	draw_digit(layer, ctx, 5, 4, now->tm_sec  % 10);
#endif
}

void handle_tick(struct tm *tick_time, TimeUnits units_changed)
{
	layer_mark_dirty(main_layer);
}

static void window_load(Window *window) {
        Layer *window_layer = window_get_root_layer(window);
        GRect bounds = layer_get_bounds(window_layer);

	date_layer = text_layer_create((GRect) {
		.origin = { 0, 0 },
		.size = { bounds.size.w, 20 }
	});
	main_layer = layer_create((GRect) {
		.origin = { 0, 0 },
		.size = { bounds.size.w, bounds.size.h }
	});
#ifdef SHOW_SECONDS
	tick_timer_service_subscribe(SECOND_UNIT, handle_tick);
#else
	tick_timer_service_subscribe(MINUTE_UNIT, handle_tick);
#endif
	layer_set_update_proc(main_layer, update_proc);
	layer_add_child(window_layer, main_layer);
	layer_add_child(window_layer, text_layer_get_layer(date_layer));

	text_layer_set_text(date_layer, "");
	text_layer_set_background_color(date_layer, GColorBlack);
	text_layer_set_text_color(date_layer, GColorWhite);
	text_layer_set_text_alignment(date_layer, GTextAlignmentCenter);

	col_spacing = (bounds.size.w - 2 * NUM_COLUMNS * RADIUS) / (NUM_COLUMNS + 1);
	col_offset = (bounds.size.w - col_spacing * (NUM_COLUMNS - 1) - 2 * NUM_COLUMNS * RADIUS) / 2;

	APP_LOG(APP_LOG_LEVEL_DEBUG, "bounds.size.w = %d", bounds.size.w);
	APP_LOG(APP_LOG_LEVEL_DEBUG, "col_spacing = %d", col_spacing);
	APP_LOG(APP_LOG_LEVEL_DEBUG, "col_offset = %d", col_offset);
}

static void window_unload(Window *window) {
	tick_timer_service_unsubscribe();
	text_layer_destroy(date_layer);
	layer_destroy(main_layer);
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
