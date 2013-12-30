#include <pebble.h>

#define RADIUS 10

static Window *window;
static Layer *main_layer;

static void update_proc(Layer *layer, GContext *ctx)
{
	time_t now = time(NULL);

	graphics_context_set_stroke_color(ctx, GColorWhite);
	graphics_context_set_fill_color(ctx, GColorWhite);

	graphics_draw_circle(ctx, GPoint(20, 20), RADIUS);
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
