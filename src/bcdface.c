#include <pebble.h>

#define SHOW_SECONDS 1
#define RADIUS 8

static Window *window;
static Layer *main_layer;

static void draw_digit(Layer *layer, GContext *ctx,
		       int16_t x_coord, int bits, int val)
{
	GRect bounds = layer_get_bounds(layer);
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
	time_t now_time = time(NULL);
	struct tm *now = localtime(&now_time);

	graphics_context_set_stroke_color(ctx, GColorWhite);
	graphics_context_set_fill_color(ctx, GColorWhite);

	draw_digit(layer, ctx, RADIUS,      2, now->tm_hour / 10);
	draw_digit(layer, ctx, 4 * RADIUS,  4, now->tm_hour % 10);
	draw_digit(layer, ctx, 7 * RADIUS,  3, now->tm_min  / 10);
	draw_digit(layer, ctx, 10 * RADIUS, 4, now->tm_min  % 10);
	draw_digit(layer, ctx, 13 * RADIUS, 3, now->tm_sec  / 10);
	draw_digit(layer, ctx, 16 * RADIUS, 4, now->tm_sec  % 10);
}

void handle_tick(struct tm *tick_time, TimeUnits units_changed)
{
	layer_mark_dirty(main_layer);
}

static void window_load(Window *window) {
        Layer *window_layer = window_get_root_layer(window);
        GRect bounds = layer_get_bounds(window_layer);

	main_layer = layer_create((GRect) {
		.origin = { 0, 0 },
		.size = { bounds.size.w, bounds.size.h }
	});
	tick_timer_service_subscribe(SECOND_UNIT, handle_tick);
	layer_set_update_proc(main_layer, update_proc);
	layer_add_child(window_layer, main_layer);
}

static void window_unload(Window *window) {
	tick_timer_service_unsubscribe();
	layer_destroy(main_layer);
}

static void init(void) {
        const bool animated = true;

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
}

int main(void) {
	init();

	APP_LOG(APP_LOG_LEVEL_DEBUG, "Done initializing, pushed window: %p", window);

	app_event_loop();
	deinit();
}
