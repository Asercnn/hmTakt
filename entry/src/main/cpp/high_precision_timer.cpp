// --------------------------------------------------------------
// 文件名: high_precision_timer.cpp
// 功能:   实现一个基于 CLOCK_MONOTONIC 的高精度计时器（节拍器核心）
// 平台:   鸿蒙 / POSIX / Node.js 通用
// 作者:   ChatGPT 示例
// --------------------------------------------------------------
// 特点：
// 1. 使用 clock_nanosleep + CLOCK_MONOTONIC + TIMER_ABSTIME，保证无累积误差。
// 2. 采用独立线程循环计时，不受 JS/UI 阻塞影响。
// 3. 通过 napi_threadsafe_function 安全地通知 JS 回调。
// 4. 可实现 <1ms 精度，适用于节拍器、定时音频触发、同步动画等场景。
// --------------------------------------------------------------

#include <node_api.h> // N-API 接口（鸿蒙同样兼容）
#include <pthread.h>  // POSIX 线程支持
#include <time.h>     // 时间函数
#include <stdint.h>
#include <string.h>
#include <atomic>
#include <thread>
#include <chrono>

// --------------------------------------------------------------
// 全局变量定义
// --------------------------------------------------------------

static napi_threadsafe_function tsfn = nullptr; // 线程安全回调函数指针
static std::thread timerThread;                 // 计时器线程对象
static std::atomic<bool> running(false);        // 是否正在运行
static std::atomic<uint64_t> g_interval_ns(0);  // 计时间隔（单位: 纳秒）

// --------------------------------------------------------------
// 工具函数: 规范化 timespec 结构（确保纳秒值小于 1 秒）
// --------------------------------------------------------------
static void normalize_timespec(struct timespec &ts) {
    while (ts.tv_nsec >= 1000000000L) {
        ts.tv_nsec -= 1000000000L;
        ts.tv_sec += 1;
    }
}

// --------------------------------------------------------------
// 计时器主循环函数（在独立线程中运行）
// --------------------------------------------------------------
static void timer_loop() {
    // 记录当前时间作为起点
    struct timespec next;
    clock_gettime(CLOCK_MONOTONIC, &next);

    while (running.load()) { // 如果仍在运行状态
        // 计算下一次触发时间 = 当前时间 + 间隔
        uint64_t interval = g_interval_ns.load();
        uint64_t add_sec = interval / 1000000000ULL;                 // 整秒部分
        long add_nsec = static_cast<long>(interval % 1000000000ULL); // 纳秒部分
        next.tv_sec += add_sec;
        next.tv_nsec += add_nsec;
        normalize_timespec(next);

        // ----------------------------------------------------------
        // 使用绝对时间睡眠 (TIMER_ABSTIME) 以避免漂移
        // ----------------------------------------------------------
        int rc = clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next, NULL);
        if (rc != 0) {
            if (rc == EINTR)
                continue; // 如果被信号中断则重试
        }

        // ----------------------------------------------------------
        // 获取当前时间戳（纳秒）
        // ----------------------------------------------------------
        struct timespec now_ts;
        clock_gettime(CLOCK_MONOTONIC, &now_ts);
        uint64_t now_ns = static_cast<uint64_t>(now_ts.tv_sec) * 1000000000ULL + now_ts.tv_nsec;

        // ----------------------------------------------------------
        // 为 JS 回调分配一份独立数据（避免多线程访问冲突）
        // ----------------------------------------------------------
        uint64_t *p = new uint64_t(now_ns);

        // ----------------------------------------------------------
        // 调用 JS 层回调（线程安全）
        // ----------------------------------------------------------
        if (tsfn) {
            napi_status status = napi_call_threadsafe_function(tsfn, p, napi_tsfn_nonblocking);
            if (status != napi_ok) {
                delete p; // 如果调用失败则释放内存
            }
        } else {
            delete p;
        }
    } // while 循环结束
}

// --------------------------------------------------------------
// JS 回调函数包装器
// 作用：当计时器线程触发时，通过此函数将数据传给 JS
// --------------------------------------------------------------
static void call_js_callback(napi_env env, napi_value js_cb, void *context, void *data) {
    uint64_t *pNs = reinterpret_cast<uint64_t *>(data);

    if (env == NULL) {
        // 如果系统正在退出，env 为空，则只释放内存
        delete pNs;
        return;
    }

    // JS 层参数准备
    napi_value undefined;
    napi_get_undefined(env, &undefined);

    // 将纳秒转为毫秒（浮点数，方便 JS 使用）
    double ms = static_cast<double>(*pNs) / 1000000.0;
    napi_value js_time;
    napi_create_double(env, ms, &js_time);

    // 调用 JS 回调函数
    napi_value result;
    napi_call_function(env, undefined, js_cb, 1, &js_time, &result);

    // 释放分配的内存
    delete pNs;
}

EXTERN_C_START
// --------------------------------------------------------------
// JS 接口: start(intervalMicroseconds, callback)
// --------------------------------------------------------------
static napi_value start(napi_env env, napi_callback_info info) {
    size_t argc = 2;
    napi_value argv[2];
    napi_value this_arg;
    void *data;
    napi_get_cb_info(env, info, &argc, argv, &this_arg, &data);

    if (argc < 2) {
        napi_throw_type_error(env, NULL, "需要 interval(微秒) 和回调函数");
        return nullptr;
    }

    // 获取第一个参数: 时间间隔
    double interval_us;
    napi_get_value_double(env, argv[0], &interval_us);
    if (interval_us <= 0) {
        napi_throw_range_error(env, NULL, "间隔必须大于0");
        return nullptr;
    }

    // 第二个参数是 JS 回调函数
    napi_value js_cb = argv[1];

    // 如果已在运行，先停止旧的线程
    if (running.load()) {
        running.store(false);
        if (timerThread.joinable())
            timerThread.join();
        if (tsfn) {
            napi_release_threadsafe_function(tsfn, napi_tsfn_release);
            tsfn = nullptr;
        }
    }

    // 存储计时间隔（转为纳秒）
    g_interval_ns.store(static_cast<uint64_t>(interval_us * 1000.0));

    // 创建线程安全回调
    napi_value resource_name;
    napi_create_string_utf8(env, "HighPrecisionTimerTSFN", NAPI_AUTO_LENGTH, &resource_name);

    // JS 回调函数
    // async_resource
    // 名称
    // 队列长度（0 = 无限制）
    // 线程数
    // finalize_data
    // context
    // 线程 -> JS 回调函数
    napi_status status = napi_create_threadsafe_function(env,js_cb,nullptr,resource_name,0,1,nullptr,nullptr,nullptr,call_js_callback,&tsfn);


    if (status != napi_ok) {
        napi_throw_error(env, NULL, "创建线程安全函数失败");
        return nullptr;
    }

    // 启动计时器线程
    running.store(true);
    timerThread = std::thread(timer_loop);

    napi_value res;
    napi_get_boolean(env, true, &res);
    return res;
}

// --------------------------------------------------------------
// JS 接口: stop()
// --------------------------------------------------------------
static napi_value stop(napi_env env, napi_callback_info info) {
    if (running.load()) {
        running.store(false);
        if (timerThread.joinable())
            timerThread.join();
    }
    if (tsfn) {
        napi_release_threadsafe_function(tsfn, napi_tsfn_release);
        tsfn = nullptr;
    }
    napi_value res;
    napi_get_boolean(env, true, &res);
    return res;
}

// --------------------------------------------------------------
// JS 接口: isRunning()
// --------------------------------------------------------------
static napi_value isRunning(napi_env env, napi_callback_info info) {
    napi_value res;
    napi_get_boolean(env, running.load(), &res);
    return res;
}

// --------------------------------------------------------------
// 模块初始化
// --------------------------------------------------------------
static napi_value Init(napi_env env, napi_value exports) {
    napi_property_descriptor desc[] = {{"start", 0, start, 0, 0, 0, napi_default, 0},
                                       {"stop", 0, stop, 0, 0, 0, napi_default, 0},
                                       {"isRunning", 0, isRunning, 0, 0, 0, napi_default, 0}};
    napi_define_properties(env, exports, sizeof(desc) / sizeof(*desc), desc);
    return exports;
}
EXTERN_C_END

static napi_module demoModule = {
    .nm_version = 1,
    .nm_flags = 0,
    .nm_filename = nullptr,
    .nm_register_func = Init,
    .nm_modname = "high.precision.timer",
    .nm_priv = ((void *)0),
    .reserved = {0},
};

extern "C" __attribute__((constructor)) void RegisterNativeModule(void) { napi_module_register(&demoModule); }
