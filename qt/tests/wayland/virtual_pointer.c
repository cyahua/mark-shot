#define _POSIX_C_SOURCE 200809L
#include "virtual-pointer-client-protocol.h"

#include <linux/input-event-codes.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <wayland-client.h>

struct pointer_state {
    struct wl_display *display;
    struct zwlr_virtual_pointer_manager_v1 *manager;
    struct zwlr_virtual_pointer_v1 *pointer;
    struct wl_output *outputs[16];
    struct wl_output *target_output;
    size_t output_count;
    const char *output_name;
    double width;
    double height;
};

/**
 * 接收输出几何，输入范围由调用者提供。
 * @param data 指针状态。
 * @param output 输出对象。
 * @param x 输出横坐标。
 * @param y 输出纵坐标。
 * @param physical_width 物理宽度。
 * @param physical_height 物理高度。
 * @param subpixel 子像素布局。
 * @param make 制造商。
 * @param model 型号。
 * @param transform 输出旋转。
 * @return 无返回值。
 */
static void output_geometry(void *data, struct wl_output *output, int32_t x, int32_t y,
                            int32_t physical_width, int32_t physical_height, int32_t subpixel,
                            const char *make, const char *model, int32_t transform)
{
    (void)data; (void)output; (void)x; (void)y; (void)physical_width; (void)physical_height;
    (void)subpixel; (void)make; (void)model; (void)transform;
}

/**
 * 接收输出模式，不改变指定的输入范围。
 * @param data 指针状态。
 * @param output 输出对象。
 * @param flags 模式标记。
 * @param width 像素宽度。
 * @param height 像素高度。
 * @param refresh 刷新率。
 * @return 无返回值。
 */
static void output_mode(void *data, struct wl_output *output, uint32_t flags,
                        int32_t width, int32_t height, int32_t refresh)
{
    (void)data; (void)output; (void)flags; (void)width; (void)height; (void)refresh;
}

/**
 * 接收一组输出信息结束通知。
 * @param data 指针状态。
 * @param output 输出对象。
 * @return 无返回值。
 */
static void output_done(void *data, struct wl_output *output)
{
    (void)data; (void)output;
}

/**
 * 接收输出整数缩放，测试通过逻辑坐标输入。
 * @param data 指针状态。
 * @param output 输出对象。
 * @param factor 输出缩放。
 * @return 无返回值。
 */
static void output_scale(void *data, struct wl_output *output, int32_t factor)
{
    (void)data; (void)output; (void)factor;
}

/**
 * 按名称选择接收虚拟指针输入的输出。
 * @param data 指针状态。
 * @param output 输出对象。
 * @param name 合成器输出名称。
 * @return 无返回值。
 */
static void output_name(void *data, struct wl_output *output, const char *name)
{
    struct pointer_state *state = data;
    if (strcmp(name, state->output_name) == 0) {
        state->target_output = output;
    }
}

/**
 * 接收输出描述，匹配时仅使用输出名称。
 * @param data 指针状态。
 * @param output 输出对象。
 * @param description 输出描述。
 * @return 无返回值。
 */
static void output_description(void *data, struct wl_output *output, const char *description)
{
    (void)data; (void)output; (void)description;
}

static const struct wl_output_listener output_listener = {
    output_geometry, output_mode, output_done, output_scale, output_name, output_description,
};

/**
 * 返回协议输入使用的单调时钟。
 * @return 毫秒时间戳。
 */
static uint32_t timestamp(void)
{
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return (uint32_t)(now.tv_sec * 1000 + now.tv_nsec / 1000000);
}

/**
 * 绑定测试显示的虚拟指针管理器。
 * @param data 指针状态。
 * @param registry Wayland 注册表。
 * @param name 全局对象编号。
 * @param interface 接口名称。
 * @param version 服务端接口版本。
 * @return 无返回值。
 */
static void registry_global(void *data, struct wl_registry *registry, uint32_t name,
                            const char *interface, uint32_t version)
{
    struct pointer_state *state = data;
    if (strcmp(interface, "zwlr_virtual_pointer_manager_v1") == 0 && version >= 2) {
        state->manager = wl_registry_bind(registry, name, &zwlr_virtual_pointer_manager_v1_interface, 2);
    } else if (strcmp(interface, "wl_output") == 0 && version >= 4 && state->output_count < 16) {
        struct wl_output *output = wl_registry_bind(registry, name, &wl_output_interface, 4);
        state->outputs[state->output_count++] = output;
        wl_output_add_listener(output, &output_listener, state);
    }
}

/**
 * 接收注册表删除通知，测试期间不复用已删除对象。
 * @param data 指针状态。
 * @param registry Wayland 注册表。
 * @param name 删除的对象编号。
 * @return 无返回值。
 */
static void registry_removed(void *data, struct wl_registry *registry, uint32_t name)
{
    (void)data;
    (void)registry;
    (void)name;
}

/**
 * 将一组输入交给合成器处理。
 * @param state 指针状态。
 * @return 连接正常时返回 true。
 */
static int submit_frame(struct pointer_state *state)
{
    zwlr_virtual_pointer_v1_frame(state->pointer);
    return wl_display_roundtrip(state->display) >= 0;
}

/**
 * 在隔离显示中接收移动与左键命令。
 * @param argc 参数数量。
 * @param argv 显示逻辑宽度、逻辑高度和输出名称。
 * @return 输入成功返回 0，环境或协议错误返回非零值。
 */
int main(int argc, char **argv)
{
    const char *runtime = getenv("XDG_RUNTIME_DIR");
    const char *isolated = getenv("MARK_SHOT_HYPRLAND_TEST_RUNTIME");
    if (argc != 4 || !runtime || !isolated || strcmp(runtime, isolated) != 0
        || !strstr(runtime, "/mshypr-")) {
        fprintf(stderr, "Virtual pointer requires the isolated Hyprland test session\n");
        return 2;
    }
    struct pointer_state state = {0};
    state.width = strtod(argv[1], NULL);
    state.height = strtod(argv[2], NULL);
    state.output_name = argv[3];
    if (!isfinite(state.width) || !isfinite(state.height)
        || state.width <= 0 || state.height <= 0
        || state.width > UINT32_MAX / 1000 || state.height > UINT32_MAX / 1000) {
        return 2;
    }
    state.display = wl_display_connect(NULL);
    if (!state.display) {
        fprintf(stderr, "Cannot connect to the isolated Wayland display\n");
        return 2;
    }
    struct wl_registry *registry = wl_display_get_registry(state.display);
    const struct wl_registry_listener listener = {registry_global, registry_removed};
    wl_registry_add_listener(registry, &listener, &state);
    wl_display_roundtrip(state.display);
    wl_display_roundtrip(state.display);
    if (!state.manager || !state.target_output) {
        fprintf(stderr, "The isolated compositor does not expose the requested virtual pointer output\n");
        wl_display_disconnect(state.display);
        return 2;
    }
    state.pointer = zwlr_virtual_pointer_manager_v1_create_virtual_pointer_with_output(state.manager, NULL, state.target_output);
    wl_display_roundtrip(state.display);
    puts("ready");
    fflush(stdout);

    char line[256];
    int result = 0;
    while (fgets(line, sizeof(line), stdin)) {
        double x, y;
        int ticks;
        if (sscanf(line, "move %lf %lf", &x, &y) == 2) {
            if (!isfinite(x) || !isfinite(y) || x < 0 || y < 0 || x > state.width || y > state.height) {
                result = 2;
                break;
            }
            zwlr_virtual_pointer_v1_motion_absolute(state.pointer, timestamp(),
                (uint32_t)(x * 1000), (uint32_t)(y * 1000),
                (uint32_t)(state.width * 1000), (uint32_t)(state.height * 1000));
        } else if (sscanf(line, "relative %lf %lf", &x, &y) == 2) {
            if (!isfinite(x) || !isfinite(y) || fabs(x) > 10000 || fabs(y) > 10000) {
                result = 2;
                break;
            }
            zwlr_virtual_pointer_v1_motion(state.pointer, timestamp(),
                                           wl_fixed_from_double(x), wl_fixed_from_double(y));
        } else if (strcmp(line, "down\n") == 0 || strcmp(line, "up\n") == 0) {
            zwlr_virtual_pointer_v1_button(state.pointer, timestamp(), BTN_LEFT,
                line[0] == 'd' ? WL_POINTER_BUTTON_STATE_PRESSED : WL_POINTER_BUTTON_STATE_RELEASED);
        } else if (sscanf(line, "scroll %d", &ticks) == 1 && ticks >= -10 && ticks <= 10) {
            zwlr_virtual_pointer_v1_axis_source(state.pointer, WL_POINTER_AXIS_SOURCE_WHEEL);
            zwlr_virtual_pointer_v1_axis_discrete(state.pointer, timestamp(), WL_POINTER_AXIS_VERTICAL_SCROLL,
                                                wl_fixed_from_int(ticks * 15), ticks);
        } else {
            result = 2;
            break;
        }
        if (!submit_frame(&state)) {
            return 2;
        }
        puts("ok");
        fflush(stdout);
    }
    zwlr_virtual_pointer_v1_button(state.pointer, timestamp(), BTN_LEFT, WL_POINTER_BUTTON_STATE_RELEASED);
    submit_frame(&state);
    zwlr_virtual_pointer_v1_destroy(state.pointer);
    zwlr_virtual_pointer_manager_v1_destroy(state.manager);
    for (size_t i = 0; i < state.output_count; ++i) {
        wl_output_release(state.outputs[i]);
    }
    wl_registry_destroy(registry);
    wl_display_disconnect(state.display);
    return result;
}
