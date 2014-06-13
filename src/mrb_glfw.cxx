#include <GLFW/glfw3.h>

#include <mruby.h>
#include <mruby/array.h>
#include <mruby/class.h>
#include <mruby/data.h>
#include <mruby/hash.h>
#include <mruby/variable.h>

namespace {

void free_window(mrb_state *, void* ptr) {
  if (ptr) {
    glfwDestroyWindow(reinterpret_cast<GLFWwindow*>(ptr));
  }
}

mrb_data_type const window_type = { "GLFWwindow", free_window };

GLFWwindow* get_window(mrb_state *M, mrb_value self) {
  return reinterpret_cast<GLFWwindow*>(mrb_data_get_ptr(M, self, &window_type));
}

mrb_value clipboard_get(mrb_state *M, mrb_value self) {
  return mrb_str_new_cstr(M, glfwGetClipboardString(get_window(M, self)));
}

mrb_value clipboard_set(mrb_state *M, mrb_value self) {
  char* str;
  mrb_get_args(M, "z", &str);
  glfwSetClipboardString(get_window(M, self), str);
  return mrb_str_new_cstr(M, str);
}

mrb_state *err_M = nullptr;
void error_func(int code, char const* str) {
  if (err_M) {
    mrb_raisef(err_M, mrb_class_get(err_M, "GLFWError"), "%S: %S",
               mrb_fixnum_value(code), mrb_str_new_cstr(err_M, str));
  } else {
    fprintf(stderr, "GLFW error: %s (%d)\n", str, code);
  }
}

mrb_value current_context(mrb_state *, mrb_value) {
  GLFWwindow * const cur = glfwGetCurrentContext();
  return mrb_obj_value(glfwGetWindowUserPointer(cur));
}

mrb_value swap_interval_set(mrb_state *M, mrb_value) {
  mrb_int v;
  mrb_get_args(M, "i", &v);
  return glfwSwapInterval(v), mrb_fixnum_value(v);
}

mrb_value proc_address(mrb_state *M, mrb_value) {
  static_assert(sizeof(void*) == sizeof(GLFWglproc), "function pointer and pointer size must be same");
  char *str;
  mrb_get_args(M, "z", &str);
  return mrb_cptr_value(M, reinterpret_cast<void*>(glfwGetProcAddress(str)));
}

mrb_value extension_supported_p(mrb_state *M, mrb_value) {
  char *str;
  mrb_get_args(M, "z", &str);
  return mrb_bool_value(glfwExtensionSupported(str) != GL_FALSE);
}

mrb_value make_current(mrb_state *M, mrb_value self) {
  return glfwMakeContextCurrent(get_window(M, self)), self;
}

mrb_value swap_buffers(mrb_state *M, mrb_value self) {
  return glfwSwapBuffers(get_window(M, self)), self;
}

mrb_value version(mrb_state *M, mrb_value) {
  int major, minor, rev;
  glfwGetVersion(&major, &minor, &rev);
  mrb_value vers[] = { mrb_fixnum_value(major), mrb_fixnum_value(minor), mrb_fixnum_value(rev) };
  return mrb_ary_new_from_values(M, 3, vers);
}

mrb_value version_string(mrb_state *M, mrb_value) {
  return mrb_str_new_cstr(M, glfwGetVersionString());
}

mrb_value time_get(mrb_state *M, mrb_value) {
  return mrb_float_value(M, glfwGetTime());
}

mrb_value time_set(mrb_state *M, mrb_value) {
  mrb_float v;
  mrb_get_args(M, "f", &v);
  return glfwSetTime(v), mrb_float_value(M, v);
}

mrb_value window_initialize(mrb_state *M, mrb_value self) {
  mrb_int w, h;
  char *title;
  mrb_value monitor = mrb_nil_value(), share = mrb_nil_value();
  mrb_get_args(M, "iiz|oo", &w, &h, &title, &monitor, &share);
  GLFWwindow *win = glfwCreateWindow(w, h, title, nullptr, nullptr);
  DATA_PTR(self) = win;
  DATA_TYPE(self) = &window_type;
  glfwSetWindowUserPointer(win, mrb_obj_ptr(self));
  mrb_ary_push(M, mrb_iv_get(M, mrb_obj_value(mrb_module_get(M, "GLFW")), mrb_intern_lit(M, "__glfw_objects")), self);
  return self;
}

mrb_value should_close_p(mrb_state *M, mrb_value self) {
  return mrb_bool_value(glfwWindowShouldClose(get_window(M, self)));
}

mrb_value poll_events(mrb_state *, mrb_value self) {
  return glfwPollEvents(), self;
}

mrb_value wait_events(mrb_state*, mrb_value self) {
  return glfwWaitEvents(), self;
}

mrb_value window_size_get(mrb_state *M, mrb_value self) {
  int w, h;
  glfwGetWindowSize(get_window(M, self), &w, &h);
  mrb_value const ret[] = { mrb_fixnum_value(w), mrb_fixnum_value(h) };
  return mrb_ary_new_from_values(M, 2, ret);
}

}

extern "C" void mrb_mruby_glfw_gem_init(mrb_state* M) {
  // mrb_assert(not err_M);
  err_M = M;
  glfwSetErrorCallback(&error_func);

  RClass *const err_cls = mrb_define_class(M, "GLFWError", mrb_class_get(M, "StandardError"));

  RClass *const mod = mrb_define_module(M, "GLFW");
  mrb_iv_set(M, mrb_obj_value(mod), mrb_intern_lit(M, "__glfw_objects"), mrb_ary_new(M));
  mrb_define_class_method(M, mod, "current_context", current_context, MRB_ARGS_NONE());
  mrb_define_class_method(M, mod, "swap_interval=", swap_interval_set, MRB_ARGS_REQ(1));
  mrb_define_class_method(M, mod, "extension_supported?", extension_supported_p, MRB_ARGS_REQ(1));
  mrb_define_class_method(M, mod, "proc_address", proc_address, MRB_ARGS_REQ(1));
  mrb_define_class_method(M, mod, "version", version, MRB_ARGS_NONE());
  mrb_define_class_method(M, mod, "version_string", version_string, MRB_ARGS_NONE());
  mrb_define_class_method(M, mod, "time", time_get, MRB_ARGS_NONE());
  mrb_define_class_method(M, mod, "time=", time_set, MRB_ARGS_REQ(1));
  mrb_define_class_method(M, mod, "poll_events", poll_events, MRB_ARGS_NONE());
  mrb_define_class_method(M, mod, "wait_events", wait_events, MRB_ARGS_NONE());

  /*
  mrb_define_class_method(M, mod, "default_window_hints", default_window_hints MRB_ARGS_NONE());
  mrb_define_class_method(M, mod, "window_hint", window_hint, MRB_ARGS_REQ(2));
  mrb_define_class_method(M, mod, "monitors", monitors, MRB_ARGS_NONE());
  mrb_define_class_method(M, mod, "primary_moniter", primary_moniter, MRB_ARGS_NONE());
  mrb_define_class_method(M, mod, "set_monitor_callback", set_monitor_callback, MRB_ARGS_BLOCK());

  RClass *const monitor = mrb_define_class_under(M, mod, "Monitor", M->object_class);
  mrb_define_method(M, monitor, "position", position, MRB_ARGS_NONE());
  mrb_define_method(M, monitor, "physical_size", physical_size, MRB_ARGS_NONE());
  mrb_define_method(M, monitor, "name", monitor_name, MRB_ARGS_NONE());
  mrb_define_method(M, monitor, "video_modes", video_modes, MRB_ARGS_NONE());
  mrb_define_method(M, monitor, "vidoe_mode", video_mode, MRB_ARGS_NONE());
  mrb_define_method(M, monitor, "gamma=", gamma_set, MRB_ARGS_REQ(1));
  mrb_define_method(M, monitor, "gamma_ramp", gamma_ramp_get, MRB_ARGS_NONE());
  mrb_define_method(M, monitor, "gamma_ramp=", gamma_ramp_set, MRB_ARGS_REQ(1));
  */

  RClass *const win = mrb_define_class_under(M, mod, "Window", M->object_class);
  MRB_SET_INSTANCE_TT(win, MRB_TT_DATA);
  mrb_define_method(M, win, "initialize", window_initialize, MRB_ARGS_REQ(3) | MRB_ARGS_OPT(2));
  mrb_define_method(M, win, "clipboard", clipboard_get, MRB_ARGS_NONE());
  mrb_define_method(M, win, "clipboard=", clipboard_set, MRB_ARGS_REQ(1));
  mrb_define_method(M, win, "make_current", make_current, MRB_ARGS_NONE());
  mrb_define_method(M, win, "swap_buffers", swap_buffers, MRB_ARGS_NONE());
  mrb_define_method(M, win, "should_close?", should_close_p, MRB_ARGS_NONE());
  mrb_define_method(M, win, "window_size", window_size_get, MRB_ARGS_NONE());

  int err = glfwInit();
  if (err != GL_TRUE) {
    mrb_raise(M, err_cls, "GLFW initialization failed.");
  }
}

extern "C" void mrb_mruby_glfw_gem_final(mrb_state *M) {
  mrb_value objs = mrb_iv_get(M, mrb_obj_value(mrb_module_get(M, "GLFW")), mrb_intern_lit(M, "__glfw_objects"));
  for (mrb_int i = 0; i < RARRAY_LEN(objs); ++i) {
    mrb_value const obj = RARRAY_PTR(objs)[i];
    if (DATA_TYPE(obj) == &window_type) {
      glfwDestroyWindow(reinterpret_cast<GLFWwindow*>(DATA_PTR(obj)));
      DATA_PTR(obj) = NULL;
    } else {
      mrb_assert(false);
    }
  }

  err_M = nullptr;
  glfwTerminate();
}
